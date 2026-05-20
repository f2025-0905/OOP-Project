// ============================================
// MySchedule.jsx — Employee's own schedule view
// ============================================
import { useState, useEffect } from 'react';
import { useAuth } from '../hooks/useAuth';
import api from '../utils/api';
import { ChevronLeft, ChevronRight, X } from 'lucide-react';
import { format, startOfWeek, addDays, addWeeks, subWeeks, isSameDay, parseISO } from 'date-fns';

export function MySchedule() {
  const { user }   = useAuth();
  const [weekStart, setWeekStart] = useState(startOfWeek(new Date()));
  const [shifts, setShifts]       = useState([]);
  const [dashData, setDash]       = useState(null);
  const [loading, setLoading]     = useState(true);

  const weekKey = format(weekStart, 'yyyy-MM-dd');

  useEffect(() => {
    const fetchAll = async () => {
      setLoading(true);
      try {
        const [s, d] = await Promise.all([
          api.get(`/schedule/my?week=${weekKey}`),
          api.get('/dashboard/employee')
        ]);
        setShifts(s.data);
        setDash(d.data);
      } catch (err) {
        console.error(err);
      } finally {
        setLoading(false);
      }
    };
    fetchAll();
  }, [weekKey]);

  const weekDays = Array.from({length: 7}, (_, i) => addDays(weekStart, i));

  const getShift = (day) => shifts.find(s => isSameDay(parseISO(s.shift_date), day));

  return (
    <div>
      <div className="page-header">
        <div>
          <h1 className="page-title">My Schedule</h1>
          <p className="page-subtitle">
            {user?.first_name} · {dashData?.position || 'No position assigned'} ·{' '}
            <strong>{dashData?.leave_balance ?? 0}</strong> leave days remaining
          </p>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
          <button className="btn btn-icon btn-ghost" onClick={() => setWeekStart(subWeeks(weekStart, 1))}>
            <ChevronLeft size={16} />
          </button>
          <span style={{ fontWeight: 600, fontSize: '0.875rem', minWidth: 160, textAlign: 'center' }}>
            {format(weekStart, 'MMM d')} – {format(addDays(weekStart, 6), 'MMM d, yyyy')}
          </span>
          <button className="btn btn-icon btn-ghost" onClick={() => setWeekStart(addWeeks(weekStart, 1))}>
            <ChevronRight size={16} />
          </button>
        </div>
      </div>

      {loading ? (
        <div style={{ display: 'flex', justifyContent: 'center', paddingTop: 80 }}>
          <div className="spinner" style={{ width: 32, height: 32 }} />
        </div>
      ) : (
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(7, 1fr)', gap: 12 }}>
          {weekDays.map(day => {
            const shift = getShift(day);
            const isToday = isSameDay(day, new Date());
            return (
              <div key={day.toISOString()} style={{
                background: isToday ? 'var(--accent-dim)' : 'var(--bg-surface)',
                border: `1px solid ${isToday ? 'rgba(79,142,247,0.4)' : 'var(--border)'}`,
                borderRadius: 'var(--radius-md)',
                padding: 16, minHeight: 130
              }}>
                <div style={{ marginBottom: 12 }}>
                  <div style={{ fontSize: '0.72rem', fontWeight: 600, textTransform: 'uppercase',
                    letterSpacing: '0.08em', color: isToday ? 'var(--accent)' : 'var(--text-muted)' }}>
                    {format(day, 'EEE')}
                  </div>
                  <div style={{ fontFamily: 'var(--font-display)', fontSize: '1.4rem',
                    fontWeight: 800, lineHeight: 1, color: isToday ? 'var(--accent)' : 'var(--text-primary)' }}>
                    {format(day, 'd')}
                  </div>
                </div>
                {shift ? (
                  <div style={{
                    background: shift.color ? shift.color + '22' : 'var(--bg-elevated)',
                    border: `1px solid ${shift.color ? shift.color + '44' : 'var(--border)'}`,
                    borderRadius: 8, padding: '8px 10px'
                  }}>
                    <div style={{ fontWeight: 700, fontSize: '0.85rem',
                      color: shift.color || 'var(--accent)' }}>
                      {shift.start_time?.slice(0,5)} – {shift.end_time?.slice(0,5)}
                    </div>
                    {shift.position && (
                      <div style={{ fontSize: '0.72rem', color: 'var(--text-secondary)', marginTop: 2 }}>
                        {shift.position}
                      </div>
                    )}
                    {shift.notes && (
                      <div style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginTop: 4 }}>
                        {shift.notes}
                      </div>
                    )}
                  </div>
                ) : (
                  <div style={{ color: 'var(--text-muted)', fontSize: '0.78rem', paddingTop: 4 }}>
                    Day off
                  </div>
                )}
              </div>
            );
          })}
        </div>
      )}
    </div>
  );
}

// ============================================
// Announcements.jsx — shared announcements page
// ============================================
import { format as dateFnsFormat } from 'date-fns';

export function Announcements() {
  const { user } = useAuth();
  const isManager = user?.role === 'manager';
  const [announcements, setAnn] = useState([]);
  const [loading, setLoading]   = useState(true);
  const [form, setForm]         = useState({ title: '', body: '', priority: 'normal' });
  const [posting, setPosting]   = useState(false);

  const fetch = async () => {
    const res = await api.get('/announcements');
    setAnn(res.data);
    setLoading(false);
  };

  useEffect(() => { fetch(); }, []);

  const handlePost = async () => {
    if (!form.title || !form.body) return;
    setPosting(true);
    try {
      await api.post('/announcements', form);
      setForm({ title: '', body: '', priority: 'normal' });
      fetch();
    } finally {
      setPosting(false);
    }
  };

  const handleDelete = async (id) => {
    if (!window.confirm('Delete announcement?')) return;
    await api.delete(`/announcements/${id}`);
    fetch();
  };

  return (
    <div>
      <div className="page-header">
        <div>
          <h1 className="page-title">Announcements</h1>
          <p className="page-subtitle">Company-wide messages and updates</p>
        </div>
      </div>

      {isManager && (
        <div className="card" style={{ marginBottom: 20 }}>
          <h2 className="card-title" style={{ marginBottom: 16 }}>Post Announcement</h2>
          <div className="form-group">
            <label className="form-label">Title</label>
            <input className="form-input" placeholder="Announcement title..."
              value={form.title} onChange={e => setForm({...form, title: e.target.value})} />
          </div>
          <div className="form-group">
            <label className="form-label">Message</label>
            <textarea className="form-input" rows={4}
              placeholder="Write your announcement..."
              value={form.body} onChange={e => setForm({...form, body: e.target.value})} />
          </div>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <select className="form-input" value={form.priority}
              onChange={e => setForm({...form, priority: e.target.value})}
              style={{ width: 'auto' }}>
              <option value="normal">Normal Priority</option>
              <option value="high">High Priority</option>
              <option value="urgent">Urgent</option>
            </select>
            <button className="btn btn-primary" onClick={handlePost}
              disabled={posting || !form.title || !form.body}>
              {posting ? <div className="spinner" /> : 'Post'}
            </button>
          </div>
        </div>
      )}

      {loading ? (
        <div className="spinner" style={{ margin: '60px auto' }} />
      ) : announcements.length === 0 ? (
        <div className="card">
          <div className="empty-state">No announcements yet</div>
        </div>
      ) : (
        <div>
          {announcements.map(a => (
            <div key={a.id} className={`announcement-card ${a.priority}`}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
                <div style={{ flex: 1 }}>
                  <div style={{ display: 'flex', alignItems: 'center', gap: 10, marginBottom: 6 }}>
                    <div className="announcement-title">{a.title}</div>
                    <span className={`badge badge-${a.priority}`}>{a.priority}</span>
                  </div>
                  <div className="announcement-body">{a.body}</div>
                  <div className="announcement-meta">
                    <span>Posted by {a.posted_by}</span>
                    <span>{dateFnsFormat(new Date(a.created_at), 'MMM d, yyyy • h:mm a')}</span>
                  </div>
                </div>
                {isManager && (
                  <button className="btn btn-icon btn-ghost btn-sm" style={{ marginLeft: 12 }}
                    onClick={() => handleDelete(a.id)}>
                    <X size={14} />
                  </button>
                )}
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
