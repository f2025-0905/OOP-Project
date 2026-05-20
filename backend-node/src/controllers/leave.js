const pool = require('../db');

const applyLeave = async (req, res) => {
  try {
    const { start_date, end_date, leave_type, reason } = req.body;
    if (!start_date || !end_date)
      return res.status(400).json({ error: 'start_date and end_date required' });
    if (end_date < start_date)
      return res.status(400).json({ error: 'end_date must be after start_date' });

    // Check balance for annual leave
    if ((leave_type || 'annual') === 'annual') {
      const { rows } = await pool.query(
        'SELECT leave_balance FROM employee_profiles WHERE user_id=$1', [req.user.userId]
      );
      if (rows.length) {
        const balance = rows[0].leave_balance;
        const { rows: dR } = await pool.query(
          "SELECT ($2::date - $1::date + 1) as days", [start_date, end_date]
        );
        const days = parseInt(dR[0].days);
        if (days > balance)
          return res.status(400).json({
            error: `Insufficient leave balance. You have ${balance} days remaining`
          });
      }
    }

    // Check overlap
    const overlap = await pool.query(
      `SELECT id FROM leave_requests
       WHERE employee_id=$1 AND status IN ('pending','approved')
       AND NOT (end_date < $2::date OR start_date > $3::date)`,
      [req.user.userId, start_date, end_date]
    );
    if (overlap.rows.length)
      return res.status(409).json({ error: 'You already have a leave request for this period' });

    const { rows } = await pool.query(
      `INSERT INTO leave_requests (employee_id, start_date, end_date, leave_type, reason)
       VALUES ($1,$2,$3,$4,$5) RETURNING id, created_at`,
      [req.user.userId, start_date, end_date, leave_type||'annual', reason||null]
    );
    res.status(201).json({
      id: rows[0].id, start_date, end_date, leave_type: leave_type||'annual',
      status: 'pending', message: 'Leave request submitted'
    });
  } catch (e) {
    console.error(e);
    res.status(500).json({ error: 'Internal server error' });
  }
};

const getMyLeaves = async (req, res) => {
  try {
    const { rows } = await pool.query(
      `SELECT lr.id, lr.start_date, lr.end_date, lr.leave_type, lr.reason,
              lr.status, lr.created_at, lr.reviewed_at,
              u.first_name as rv_first, u.last_name as rv_last
       FROM leave_requests lr
       LEFT JOIN users u ON lr.reviewed_by = u.id
       WHERE lr.employee_id=$1
       ORDER BY lr.created_at DESC`,
      [req.user.userId]
    );
    res.json(rows.map(r => ({
      id:          r.id,
      start_date:  r.start_date,
      end_date:    r.end_date,
      leave_type:  r.leave_type,
      reason:      r.reason || '',
      status:      r.status,
      created_at:  r.created_at,
      reviewed_by: r.rv_first ? `${r.rv_first} ${r.rv_last}` : null,
      reviewed_at: r.reviewed_at
    })));
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const getAllLeaves = async (req, res) => {
  try {
    const { status } = req.query;
    const query = status && status !== 'all'
      ? `SELECT lr.id, lr.start_date, lr.end_date, lr.leave_type, lr.reason,
                lr.status, lr.created_at,
                u.id as emp_id, u.first_name, u.last_name, u.email
         FROM leave_requests lr
         JOIN users u ON lr.employee_id = u.id
         WHERE lr.status=$1 ORDER BY lr.created_at DESC`
      : `SELECT lr.id, lr.start_date, lr.end_date, lr.leave_type, lr.reason,
                lr.status, lr.created_at,
                u.id as emp_id, u.first_name, u.last_name, u.email
         FROM leave_requests lr
         JOIN users u ON lr.employee_id = u.id
         ORDER BY CASE lr.status WHEN 'pending' THEN 0 ELSE 1 END, lr.created_at DESC`;

    const { rows } = await pool.query(query, status && status !== 'all' ? [status] : []);
    res.json(rows.map(r => ({
      id:         r.id,
      start_date: r.start_date,
      end_date:   r.end_date,
      leave_type: r.leave_type,
      reason:     r.reason || '',
      status:     r.status,
      created_at: r.created_at,
      employee:   { id: r.emp_id, first_name: r.first_name, last_name: r.last_name, email: r.email }
    })));
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const reviewLeave = async (req, res) => {
  const client = await pool.connect();
  try {
    const { action } = req.body;
    if (!['approve','reject'].includes(action))
      return res.status(400).json({ error: "Action must be 'approve' or 'reject'" });

    const { rows: lr } = await client.query(
      'SELECT employee_id, start_date, end_date, leave_type, status FROM leave_requests WHERE id=$1',
      [req.params.id]
    );
    if (!lr.length) return res.status(404).json({ error: 'Leave request not found' });
    if (lr[0].status !== 'pending') return res.status(409).json({ error: 'Leave request already reviewed' });

    const { employee_id, start_date, end_date, leave_type } = lr[0];
    const newStatus = action === 'approve' ? 'approved' : 'rejected';

    await client.query('BEGIN');
    await client.query(
      'UPDATE leave_requests SET status=$1, reviewed_by=$2, reviewed_at=NOW() WHERE id=$3',
      [newStatus, req.user.userId, req.params.id]
    );

    if (action === 'approve') {
      if (['annual','sick'].includes(leave_type)) {
        await client.query(
          `UPDATE employee_profiles SET
           leave_balance = leave_balance - ($1::date - $2::date + 1)
           WHERE user_id=$3 AND leave_balance > 0`,
          [end_date, start_date, employee_id]
        );
      }
      // Remove conflicting draft shifts
      await client.query(
        `DELETE FROM shifts WHERE employee_id=$1 AND status='draft'
         AND shift_date BETWEEN $2::date AND $3::date`,
        [employee_id, start_date, end_date]
      );
    }

    await client.query('COMMIT');
    res.json({ status: newStatus, message: `Leave request ${newStatus}` });
  } catch (e) {
    await client.query('ROLLBACK');
    console.error(e);
    res.status(500).json({ error: 'Internal server error' });
  } finally {
    client.release();
  }
};

module.exports = { applyLeave, getMyLeaves, getAllLeaves, reviewLeave };
