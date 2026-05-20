const bcrypt = require('bcryptjs');
const pool   = require('../db');
const { generateToken } = require('../middleware/auth');

const login = async (req, res) => {
  try {
    const { username, password } = req.body;
    if (!username || !password)
      return res.status(400).json({ error: 'Username and password required' });

    const { rows } = await pool.query(
      `SELECT id, username, email, first_name, last_name, role, password_hash, is_active
       FROM users WHERE username=$1 OR email=$1`,
      [username]
    );

    if (!rows.length) return res.status(401).json({ error: 'Invalid credentials' });
    const user = rows[0];

    if (!user.is_active) return res.status(403).json({ error: 'Account is deactivated' });

    const valid = await bcrypt.compare(password, user.password_hash);
    if (!valid) return res.status(401).json({ error: 'Invalid credentials' });

    const token = generateToken(user.id, user.role);
    res.json({
      token,
      user: {
        id:         user.id,
        username:   user.username,
        email:      user.email,
        first_name: user.first_name,
        last_name:  user.last_name,
        role:       user.role
      }
    });
  } catch (e) {
    console.error('Login error:', e);
    res.status(500).json({ error: 'Internal server error' });
  }
};

const signup = async (req, res) => {
  try {
    const { username, email, password, first_name, last_name } = req.body;
    for (const f of ['username','email','password','first_name','last_name']) {
      if (!req.body[f]) return res.status(400).json({ error: `${f} is required` });
    }
    if (password.length < 8)
      return res.status(400).json({ error: 'Password must be at least 8 characters' });

    const dup = await pool.query(
      'SELECT id FROM users WHERE username=$1 OR email=$2', [username, email]
    );
    if (dup.rows.length) return res.status(409).json({ error: 'Username or email already exists' });

    const hash = await bcrypt.hash(password, 12);
    const { rows } = await pool.query(
      `INSERT INTO users (username,email,password_hash,first_name,last_name,role)
       VALUES ($1,$2,$3,$4,$5,'manager') RETURNING id`,
      [username, email, hash, first_name, last_name]
    );

    const userId = rows[0].id;
    const token  = generateToken(userId, 'manager');
    res.status(201).json({
      token,
      user: { id: userId, username, email, first_name, last_name, role: 'manager' }
    });
  } catch (e) {
    console.error('Signup error:', e);
    res.status(500).json({ error: 'Internal server error' });
  }
};

const getMe = async (req, res) => {
  try {
    const { rows } = await pool.query(
      `SELECT u.id, u.username, u.email, u.first_name, u.last_name, u.role, u.created_at,
              ep.leave_balance, p.id as position_id, p.name as position_name
       FROM users u
       LEFT JOIN employee_profiles ep ON u.id = ep.user_id
       LEFT JOIN positions p ON ep.position_id = p.id
       WHERE u.id=$1`,
      [req.user.userId]
    );
    if (!rows.length) return res.status(404).json({ error: 'User not found' });
    const u = rows[0];
    res.json({
      id:           u.id,
      username:     u.username,
      email:        u.email,
      first_name:   u.first_name,
      last_name:    u.last_name,
      role:         u.role,
      created_at:   u.created_at,
      leave_balance: u.leave_balance,
      position_id:  u.position_id,
      position_name: u.position_name
    });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const changePassword = async (req, res) => {
  try {
    const { old_password, new_password } = req.body;
    if (!old_password || !new_password)
      return res.status(400).json({ error: 'Both passwords required' });
    if (new_password.length < 8)
      return res.status(400).json({ error: 'New password must be at least 8 characters' });

    const { rows } = await pool.query(
      'SELECT password_hash FROM users WHERE id=$1', [req.user.userId]
    );
    const valid = await bcrypt.compare(old_password, rows[0].password_hash);
    if (!valid) return res.status(401).json({ error: 'Incorrect current password' });

    const hash = await bcrypt.hash(new_password, 12);
    await pool.query('UPDATE users SET password_hash=$1 WHERE id=$2', [hash, req.user.userId]);
    res.json({ message: 'Password updated successfully' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

module.exports = { login, signup, getMe, changePassword };
