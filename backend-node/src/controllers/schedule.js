const pool = require('../db');

const hasLeaveConflict = async (employeeId, shiftDate) => {
  const { rows } = await pool.query(
    `SELECT id FROM leave_requests
     WHERE employee_id=$1 AND status='approved'
     AND $2::date BETWEEN start_date AND end_date`,
    [employeeId, shiftDate]
  );
  return rows.length > 0;
};

const getWeeklySchedule = async (req, res) => {
  try {
    const { week } = req.query;
    if (!week) return res.status(400).json({ error: 'week parameter required (YYYY-MM-DD)' });

    const { rows } = await pool.query(
      `SELECT s.id, s.shift_date, s.start_time, s.end_time, s.status, s.notes,
              u.id as emp_id, u.first_name, u.last_name,
              p.id as pos_id, p.name as pos_name, p.color as pos_color
       FROM shifts s
       JOIN users u ON s.employee_id = u.id
       LEFT JOIN positions p ON s.position_id = p.id
       WHERE s.shift_date BETWEEN $1::date AND ($1::date + interval '6 days')
       ORDER BY s.shift_date ASC, s.start_time ASC`,
      [week]
    );

    res.json(rows.map(r => ({
      id:         r.id,
      shift_date: r.shift_date,
      start_time: r.start_time,
      end_time:   r.end_time,
      status:     r.status,
      notes:      r.notes || '',
      employee:   { id: r.emp_id, first_name: r.first_name, last_name: r.last_name },
      position:   r.pos_id ? { id: r.pos_id, name: r.pos_name, color: r.pos_color } : null
    })));
  } catch (e) {
    console.error(e);
    res.status(500).json({ error: 'Internal server error' });
  }
};

const createShift = async (req, res) => {
  try {
    const { employee_id, shift_date, start_time, end_time, position_id, notes } = req.body;
    if (!employee_id || !shift_date || !start_time || !end_time)
      return res.status(400).json({ error: 'employee_id, shift_date, start_time, end_time required' });

    const conflict = await hasLeaveConflict(employee_id, shift_date);
    if (conflict)
      return res.status(409).json({ error: 'Employee has approved leave on this date', conflict: 'leave' });

    const { rows } = await pool.query(
      `INSERT INTO shifts (employee_id, position_id, shift_date, start_time, end_time, notes, created_by)
       VALUES ($1,$2,$3,$4,$5,$6,$7) RETURNING id`,
      [employee_id, position_id||null, shift_date, start_time, end_time, notes||null, req.user.userId]
    );
    res.status(201).json({ id: rows[0].id, message: 'Shift created' });
  } catch (e) {
    console.error(e);
    res.status(500).json({ error: 'Internal server error' });
  }
};

const updateShift = async (req, res) => {
  try {
    const { id } = req.params;
    const check = await pool.query('SELECT status FROM shifts WHERE id=$1', [id]);
    if (!check.rows.length) return res.status(404).json({ error: 'Shift not found' });
    if (check.rows[0].status === 'published')
      return res.status(403).json({ error: 'Cannot edit a published shift' });

    const { start_time, end_time, notes } = req.body;
    await pool.query(
      `UPDATE shifts SET
       start_time = COALESCE(NULLIF($1,'')::time, start_time),
       end_time   = COALESCE(NULLIF($2,'')::time, end_time),
       notes      = $3
       WHERE id=$4`,
      [start_time||'', end_time||'', notes||null, id]
    );
    res.json({ message: 'Shift updated' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const deleteShift = async (req, res) => {
  try {
    await pool.query('DELETE FROM shifts WHERE id=$1', [req.params.id]);
    res.json({ message: 'Shift deleted' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const publishShifts = async (req, res) => {
  try {
    const { week_start } = req.body;
    if (!week_start) return res.status(400).json({ error: 'week_start required' });
    const { rows } = await pool.query(
      `UPDATE shifts SET status='published'
       WHERE status='draft'
       AND shift_date BETWEEN $1::date AND ($1::date + interval '6 days')
       RETURNING id`,
      [week_start]
    );
    res.json({ published_count: rows.length, message: 'Shifts published successfully' });
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

const getMyShifts = async (req, res) => {
  try {
    const { week } = req.query;
    let query, params;

    if (week) {
      query = `SELECT s.id, s.shift_date, s.start_time, s.end_time, s.notes,
                      p.name as pos_name, p.color as pos_color
               FROM shifts s
               LEFT JOIN positions p ON s.position_id = p.id
               WHERE s.employee_id=$1 AND s.status='published'
               AND s.shift_date BETWEEN $2::date AND ($2::date + interval '6 days')
               ORDER BY s.shift_date ASC, s.start_time ASC`;
      params = [req.user.userId, week];
    } else {
      query = `SELECT s.id, s.shift_date, s.start_time, s.end_time, s.notes,
                      p.name as pos_name, p.color as pos_color
               FROM shifts s
               LEFT JOIN positions p ON s.position_id = p.id
               WHERE s.employee_id=$1 AND s.status='published'
               AND s.shift_date >= CURRENT_DATE
               ORDER BY s.shift_date ASC LIMIT 14`;
      params = [req.user.userId];
    }

    const { rows } = await pool.query(query, params);
    res.json(rows.map(r => ({
      id:         r.id,
      shift_date: r.shift_date,
      start_time: r.start_time,
      end_time:   r.end_time,
      notes:      r.notes || '',
      position:   r.pos_name || null,
      color:      r.pos_color || null
    })));
  } catch (e) {
    res.status(500).json({ error: 'Internal server error' });
  }
};

module.exports = { getWeeklySchedule, createShift, updateShift, deleteShift, publishShifts, getMyShifts };
