const pool = require('../db');

const getManagerDashboard = async (req, res) => {
  try {
    const [working, onLeave, stats] = await Promise.all([
      pool.query(
        `SELECT u.id, u.first_name, u.last_name,
                s.start_time, s.end_time, p.name as position
         FROM shifts s
         JOIN users u ON s.employee_id = u.id
         LEFT JOIN positions p ON s.position_id = p.id
         WHERE s.shift_date = CURRENT_DATE AND s.status='published'
         AND NOW()::time BETWEEN s.start_time AND s.end_time
         ORDER BY s.start_time ASC`
      ),
      pool.query(
        `SELECT u.id, u.first_name, u.last_name,
                lr.leave_type, lr.start_date, lr.end_date
         FROM leave_requests lr
         JOIN users u ON lr.employee_id = u.id
         WHERE lr.status='approved'
         AND CURRENT_DATE BETWEEN lr.start_date AND lr.end_date`
      ),
      pool.query(
        `SELECT
          (SELECT COUNT(*) FROM users WHERE role='employee' AND is_active=true)::int as total_employees,
          (SELECT COUNT(*) FROM leave_requests WHERE status='pending')::int as pending_leaves,
          (SELECT COUNT(*) FROM shifts WHERE shift_date=CURRENT_DATE AND status='published')::int as shifts_today`
      )
    ]);

    res.json({
      working_now:    working.rows,
      on_leave_today: onLeave.rows,
      stats: stats.rows[0]
    });
  } catch (e) {
    console.error(e);
    res.status(500).json({ error: 'Internal server error' });
  }
};

const getEmployeeDashboard = async (req, res) => {
  try {
    const [shifts, profile, pending] = await Promise.all([
      pool.query(
        `SELECT s.shift_date, s.start_time, s.end_time, p.name as position, p.color
         FROM shifts s
         LEFT JOIN positions p ON s.position_id = p.id
         WHERE s.employee_id=$1 AND s.status='published'
         AND s.shift_date >= CURRENT_DATE
         ORDER BY s.shift_date ASC LIMIT 7`,
        [req.user.userId]
      ),
      pool.query(
        `SELECT ep.leave_balance, p.name as position
         FROM employee_profiles ep
         LEFT JOIN positions p ON ep.position_id = p.id
         WHERE ep.user_id=$1`,
        [req.user.userId]
      ),
      pool.query(
        "SELECT COUNT(*)::int as cnt FROM leave_requests WHERE employee_id=$1 AND status='pending'",
        [req.user.userId]
      )
    ]);

    const prof = profile.rows[0];
    res.json({
      upcoming_shifts: shifts.rows,
      leave_balance:   prof?.leave_balance ?? 20,
      position:        prof?.position || '',
      pending_leaves:  pending.rows[0].cnt
    });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const getAnnouncements = async (req, res) => {
  try {
    const { rows } = await pool.query(
      `SELECT a.id, a.title, a.body, a.priority, a.created_at,
              u.first_name, u.last_name
       FROM announcements a
       JOIN users u ON a.posted_by = u.id
       ORDER BY a.created_at DESC LIMIT 20`
    );
    res.json(rows.map(r => ({
      ...r, posted_by: `${r.first_name} ${r.last_name}`
    })));
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const createAnnouncement = async (req, res) => {
  try {
    const { title, body, priority } = req.body;
    if (!title || !body) return res.status(400).json({ error: 'Title and body required' });
    const { rows } = await pool.query(
      'INSERT INTO announcements (title,body,priority,posted_by) VALUES ($1,$2,$3,$4) RETURNING *',
      [title, body, priority||'normal', req.user.userId]
    );
    res.status(201).json({ ...rows[0], message: 'Announcement posted' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const deleteAnnouncement = async (req, res) => {
  try {
    await pool.query('DELETE FROM announcements WHERE id=$1', [req.params.id]);
    res.json({ message: 'Announcement deleted' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

module.exports = {
  getManagerDashboard, getEmployeeDashboard,
  getAnnouncements, createAnnouncement, deleteAnnouncement
};
