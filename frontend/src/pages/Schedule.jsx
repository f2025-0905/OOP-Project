import { useState, useEffect } from 'react';
import api from '../utils/api';
import toast from 'react-hot-toast';
import { ChevronLeft, ChevronRight, Plus, Send, X, AlertTriangle } from 'lucide-react';
import { format, startOfWeek, addDays, addWeeks, subWeeks, isSameDay, parseISO } from 'date-fns';

const DAYS = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];

function getWeekDays(weekStart) {
  return Array.from({ length: 7 }, (_, i) => addDays(weekStart, i));
}

function AddShiftModal({ date, employees, positions, onClose, onSuccess }) {
  const [form, setForm] = useState({
    employee_id: '', start_time: '09:00', end_time: '17:00',
    position_id: '', notes: ''
  });
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!form.employee_id) { toast.error('Select an employee'); return; }
    setLoading(true);
    try {
      await api.post('/schedule/shift', {
        ...form, shift_date: format(date, 'yyyy-MM-dd')
      });
      toast.success('Shift added');
      onSuccess();
      onClose();
    } catch (err) {
      const msg = err.response?.data?.error || 'Failed to add shift';
      toast.error(msg);
    } finally {
      setLoading(false);
    }
  };

  const f = (key) => ({
    value: form[key],
    onChange: e => setForm({...form, [key]: e.target.value})
  });

  return (
    <div className="modal-overlay" onClick={e => e.target === e.currentTarget && onClose()}>
      <div className="modal">
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 20 }}>
          <h2 className="modal-title" style={{ margin: 0 }}>
            Add Shift — {format(date, 'EEE, MMM d')}
          </h2>
          <button className="btn btn-icon btn-ghost" onClick={onClose}><X size={16} /></button>
        </div>
        <form onSubmit={handleSubmit}>
          <div className="form-group">
            <label className="form-label">Employee *</label>
            <select className="form-input" required {...f('employee_id')}>
              <option value="">Select employee...</option>
              {employees.map(e => (
                <option key={e.id} value={e.id}>{e.first_name} {e.last_name}</option>
              ))}
            </select>
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">Start Time *</label>
              <input className="form-input" type="time" required {...f('start_time')} />
            </div>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">End Time *</label>
              <input className="form-input" type="time" required {...f('end_time')} />
            </div>
          </div>
          <div className="form-group mt-2">
            <label className="form-label">Position</label>
            <select className="form-input" {...f('position_id')}>
              <option value="">— Default position —</option>
              {positions.map(p => (
                <option key={p.id} value={p.id}>{p.name}</option>
              ))}
            </select>
          </div>
          <div className="form-group">
            <label className="form-label">Notes</label>
            <input className="form-input" placeholder="Optional notes..." {...f('notes')} />
          </div>
          <div style={{ display: 'flex', gap: 8, justifyContent: 'flex-end' }}>
            <button type="button" className="btn btn-ghost" onClick={onClose}>Cancel</button>
            <button type="submit" className="btn btn-primary" disabled={loading}>
              {loading ? <div className="spinner" /> : 'Add Shift'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}

export default function Schedule() {
  const [weekStart, setWeekStart]  = useState(startOfWeek(new Date()));
  const [shifts,    setShifts]     = useState([]);
  const [employees, setEmployees]  = useState([]);
  const [positions, setPositions]  = useState([]);
  const [loading,   setLoading]    = useState(true);
  const [addModal,  setAddModal]   = useState(null);
  const [publishing, setPublishing] = useState(false);

  const weekDays = getWeekDays(weekStart);
  const weekKey  = format(weekStart, 'yyyy-MM-dd');

  const fetchData = async () => {
    setLoading(true);
    try {
      const [s, e, p] = await Promise.all([
        api.get(`/schedule?week=${weekKey}`),
        api.get('/staff'),
        api.get('/positions')
      ]);
      setShifts(s.data);
      setEmployees(e.data.filter(e => e.is_active));
      setPositions(p.data);
    } catch (err) {
      toast.error('Failed to load schedule');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { fetchData(); }, [weekKey]);

  const getShiftsForDay = (empId, date) =>
    shifts.filter(s => s.employee.id === empId &&
      isSameDay(parseISO(s.shift_date), date));

  const deleteShift = async (id) => {
    if (!window.confirm('Delete this shift?')) return;
    await api.delete(`/schedule/shift/${id}`);
    toast.success('Shift deleted');
    fetchData();
  };

  const publishWeek = async () => {
    setPublishing(true);
    try {
      const res = await api.post('/schedule/publish', { week_start: weekKey });
      toast.success(`Published ${res.data.published_count} shifts`);
      fetchData();
    } catch (err) {
      toast.error('Publish failed');
    } finally {
      setPublishing(false);
    }
  };

  const draftCount = shifts.filter(s => s.status === 'draft').length;

  return (
    <div>
      <div className="page-header">
        <div>
          <h1 className="page-title">Schedule</h1>
          <p className="page-subtitle">Build and publish weekly shifts</p>
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
          {draftCount > 0 && (
            <button className="btn btn-primary" onClick={publishWeek} disabled={publishing}>
              {publishing ? <div className="spinner" /> : <><Send size={15} /> Publish ({draftCount})</>}
            </button>
          )}
        </div>
      </div>

      {loading ? (
        <div style={{ display: 'flex', justifyContent: 'center', paddingTop: 60 }}>
          <div className="spinner" style={{ width: 32, height: 32 }} />
        </div>
      ) : employees.length === 0 ? (
        <div className="card">
          <div className="empty-state">
            <p>No employees yet. Add employees in the Staff module first.</p>
          </div>
        </div>
      ) : (
        <div className="card" style={{ padding: 0, overflow: 'hidden' }}>
          <div className="schedule-grid">
            <div className="schedule-header-cell" style={{ borderRight: '1px solid var(--border)' }}>
              Employee
            </div>
            {weekDays.map(day => (
              <div key={day.toISOString()} className="schedule-header-cell"
                style={{ borderRight: '1px solid var(--border)', textAlign: 'center' }}>
                <div>{DAYS[day.getDay()]}</div>
                <div style={{
                  fontSize: '1rem', fontWeight: 800, color: 'var(--text-primary)',
                  fontFamily: 'var(--font-display)', marginTop: 2
                }}>
                  {format(day, 'd')}
                </div>
              </div>
            ))}

            {employees.map((emp, empIdx) => (
              <>
                <div key={`emp-${emp.id}`} className="schedule-emp-cell"
                  style={{ borderRight: '1px solid var(--border)',
                    borderTop: empIdx > 0 ? '1px solid var(--border)' : 'none' }}>
                  <div className="user-avatar" style={{ width: 28, height: 28, fontSize: '0.7rem' }}>
                    {emp.first_name[0]}{emp.last_name[0]}
                  </div>
                  <div>
                    <div style={{ fontSize: '0.78rem', fontWeight: 600 }}>
                      {emp.first_name} {emp.last_name}
                    </div>
                    {emp.position && (
                      <div style={{ fontSize: '0.65rem', color: 'var(--text-muted)' }}>
                        {emp.position.name}
                      </div>
                    )}
                  </div>
                </div>

                {weekDays.map(day => {
                  const dayShifts = getShiftsForDay(emp.id, day);
                  return (
                    <div key={`${emp.id}-${day.toISOString()}`} className="schedule-cell"
                      style={{
                        borderRight: '1px solid var(--border)',
                        borderTop: empIdx > 0 ? '1px solid var(--border)' : 'none',
                        position: 'relative'
                      }}>
                      {dayShifts.map(shift => (
                        <div
                          key={shift.id}
                          className="shift-block"
                          onClick={() => deleteShift(shift.id)}
                          style={{
                            background: shift.position?.color
                              ? shift.position.color + '33'
                              : 'var(--accent-dim)',
                            color: shift.position?.color || 'var(--accent)',
                            border: `1px solid ${shift.position?.color || 'var(--accent)'}44`
                          }}
                          title={`${shift.start_time} – ${shift.end_time} (click to delete)`}
                        >
                          {shift.start_time.slice(0,5)}–{shift.end_time.slice(0,5)}
                          {shift.status === 'draft' && (
                            <span style={{ marginLeft: 4, opacity: 0.6, fontSize: '0.65rem' }}>draft</span>
                          )}
                        </div>
                      ))}
                      <button
                        className="btn btn-icon"
                        onClick={() => setAddModal(day)}
                        style={{
                          width: '100%', height: dayShifts.length ? 20 : '100%',
                          background: 'transparent', color: 'var(--text-muted)',
                          display: 'flex', alignItems: 'center', justifyContent: 'center',
                          opacity: 0, transition: 'opacity 0.15s'
                        }}
                        onMouseEnter={e => e.currentTarget.style.opacity = 1}
                        onMouseLeave={e => e.currentTarget.style.opacity = 0}
                      >
                        <Plus size={12} />
                      </button>
                    </div>
                  );
                })}
              </>
            ))}
          </div>
        </div>
      )}

      {addModal && (
        <AddShiftModal
          date={addModal}
          employees={employees}
          positions={positions}
          onClose={() => setAddModal(null)}
          onSuccess={fetchData}
        />
      )}
    </div>
  );
}
