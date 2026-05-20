const bcrypt = require('bcryptjs');
const pool   = require('../db');

const getAllEmployees = async (req, res) => {
  try {
    const { rows } = await pool.query(
      `SELECT u.id, u.username, u.email, u.first_name, u.last_name, u.is_active, u.created_at,
              ep.leave_balance, ep.phone, ep.hire_date,
              p.id as position_id, p.name as position_name, p.color as position_color
       FROM users u
       LEFT JOIN employee_profiles ep ON u.id = ep.user_id
       LEFT JOIN positions p ON ep.position_id = p.id
       WHERE u.role = 'employee'
       ORDER BY u.first_name ASC`
    );
    res.json(rows.map(r => ({
      id:           r.id,
      username:     r.username,
      email:        r.email,
      first_name:   r.first_name,
      last_name:    r.last_name,
      is_active:    r.is_active,
      created_at:   r.created_at,
      leave_balance: r.leave_balance ?? 20,
      phone:        r.phone ?? '',
      hire_date:    r.hire_date ?? '',
      position:     r.position_id ? {
        id: r.position_id, name: r.position_name, color: r.position_color
      } : null
    })));
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const createEmployee = async (req, res) => {
  const client = await pool.connect();
  try {
    const { username, email, password, first_name, last_name, phone, position_id } = req.body;
    for (const f of ['username','email','password','first_name','last_name']) {
      if (!req.body[f]) return res.status(400).json({ error: `${f} is required` });
    }

    const dup = await client.query(
      'SELECT id FROM users WHERE username=$1 OR email=$2', [username, email]
    );
    if (dup.rows.length) return res.status(409).json({ error: 'Username or email already exists' });

    const hash = await bcrypt.hash(password, 12);
    await client.query('BEGIN');

    const { rows } = await client.query(
      `INSERT INTO users (username,email,password_hash,first_name,last_name,role)
       VALUES ($1,$2,$3,$4,$5,'employee') RETURNING id`,
      [username, email, hash, first_name, last_name]
    );
    const userId = rows[0].id;

    await client.query(
      'INSERT INTO employee_profiles (user_id, phone) VALUES ($1,$2)',
      [userId, phone || null]
    );

    if (position_id) {
      await client.query(
        'UPDATE employee_profiles SET position_id=$1 WHERE user_id=$2',
        [position_id, userId]
      );
    }

    await client.query('COMMIT');
    res.status(201).json({ id: userId, username, email, first_name, last_name, role: 'employee', message: 'Employee created' });
  } catch (e) {
    await client.query('ROLLBACK');
    console.error(e);
    res.status(500).json({ error: 'Internal server error' });
  } finally {
    client.release();
  }
};

const updateEmployee = async (req, res) => {
  try {
    const { id } = req.params;
    const { first_name, last_name, email, phone, leave_balance } = req.body;
    if (first_name || last_name || email) {
      await pool.query(
        `UPDATE users SET
         first_name = COALESCE(NULLIF($1,''), first_name),
         last_name  = COALESCE(NULLIF($2,''), last_name),
         email      = COALESCE(NULLIF($3,''), email)
         WHERE id=$4`,
        [first_name||'', last_name||'', email||'', id]
      );
    }
    if (phone !== undefined || leave_balance !== undefined) {
      await pool.query(
        `UPDATE employee_profiles SET
         phone         = COALESCE(NULLIF($1,''), phone),
         leave_balance = COALESCE($2, leave_balance)
         WHERE user_id=$3`,
        [phone||'', leave_balance ?? null, id]
      );
    }
    res.json({ message: 'Employee updated' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const deleteEmployee = async (req, res) => {
  try {
    await pool.query(
      "UPDATE users SET is_active=false WHERE id=$1 AND role='employee'",
      [req.params.id]
    );
    res.json({ message: 'Employee deactivated' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const assignPosition = async (req, res) => {
  try {
    await pool.query(
      'UPDATE employee_profiles SET position_id=$1 WHERE user_id=$2',
      [req.body.position_id, req.params.id]
    );
    res.json({ message: 'Position assigned' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const getAllPositions = async (req, res) => {
  try {
    const { rows } = await pool.query(
      `SELECT p.id, p.name, p.description, p.color, p.created_at,
              COUNT(ep.user_id)::int as employee_count
       FROM positions p
       LEFT JOIN employee_profiles ep ON p.id = ep.position_id
       GROUP BY p.id ORDER BY p.name ASC`
    );
    res.json(rows);
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const createPosition = async (req, res) => {
  try {
    const { name, description, color } = req.body;
    if (!name) return res.status(400).json({ error: 'Name is required' });
    const { rows } = await pool.query(
      'INSERT INTO positions (name,description,color,created_by) VALUES ($1,$2,$3,$4) RETURNING *',
      [name, description||null, color||'#3B82F6', req.user.userId]
    );
    res.status(201).json(rows[0]);
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const deletePosition = async (req, res) => {
  try {
    await pool.query('DELETE FROM positions WHERE id=$1', [req.params.id]);
    res.json({ message: 'Position deleted' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

module.exports = {
  getAllEmployees, createEmployee, updateEmployee, deleteEmployee, assignPosition,
  getAllPositions, createPosition, deletePosition
};
