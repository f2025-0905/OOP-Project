import { useState, useEffect } from 'react';
import { useAuth } from '../hooks/useAuth';
import api from '../utils/api';
import toast from 'react-hot-toast';
import { Plus, CheckCircle, XCircle, X } from 'lucide-react';
import { format, parseISO, differenceInDays } from 'date-fns';

function ApplyLeaveModal({ onClose, onSuccess, leaveBalance }) {
  const [form, setForm] = useState({
    start_date: '', end_date: '', leave_type: 'annual', reason: ''
  });
  const [loading, setLoading] = useState(false);

  const days = form.start_date && form.end_date
    ? Math.max(0, differenceInDays(parseISO(form.end_date), parseISO(form.start_date)) + 1)
    : 0;

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!form.start_date || !form.end_date) { toast.error('Select dates'); return; }
    if (form.end_date < form.start_date) { toast.error('End date must be after start'); return; }
    setLoading(true);
    try {
      await api.post('/leave', form);
      toast.success('Leave request submitted');
      onSuccess();
      onClose();
    } catch (err) {
      toast.error(err.response?.data?.error || 'Failed to submit');
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
          <h2 className="modal-title" style={{ margin: 0 }}>Apply for Leave</h2>
          <button className="btn btn-icon btn-ghost" onClick={onClose}><X size={16} /></button>
        </div>

        <div style={{
          background: 'var(--accent-dim)', border: '1px solid rgba(79,142,247,0.2)',
          borderRadius: 'var(--radius-sm)', padding: '10px 14px', marginBottom: 16,
          fontSize: '0.85rem'
        }}>
          Annual leave balance: <strong>{leaveBalance} days</strong>
          {days > 0 && <span style={{ color: 'var(--text-secondary)' }}> · This request: {days} day{days > 1 ? 's' : ''}</span>}
        </div>

        <form onSubmit={handleSubmit}>
          <div className="form-group">
            <label className="form-label">Leave Type</label>
            <select className="form-input" {...f('leave_type')}>
              <option value="annual">Annual Leave</option>
              <option value="sick">Sick Leave</option>
              <option value="emergency">Emergency</option>
              <option value="unpaid">Unpaid</option>
            </select>
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">Start Date *</label>
              <input className="form-input" type="date" required {...f('start_date')}
                min={format(new Date(), 'yyyy-MM-dd')} />
            </div>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">End Date *</label>
              <input className="form-input" type="date" required {...f('end_date')}
                min={form.start_date || format(new Date(), 'yyyy-MM-dd')} />
            </div>
          </div>
          <div className="form-group mt-2">
            <label className="form-label">Reason</label>
            <textarea className="form-input" rows={3} placeholder="Optional reason..."
              {...f('reason')} />
          </div>
          <div style={{ display: 'flex', gap: 8, justifyContent: 'flex-end' }}>
            <button type="button" className="btn btn-ghost" onClick={onClose}>Cancel</button>
            <button type="submit" className="btn btn-primary" disabled={loading}>
              {loading ? <div className="spinner" /> : 'Submit Request'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}

function LeaveRow({ leave, isManager, onReview }) {
  const days = differenceInDays(parseISO(leave.end_date), parseISO(leave.start_date)) + 1;

  return (
    <tr>
      {isManager && (
        <td>
          <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
            <div className="user-avatar" style={{ width: 30, height: 30, fontSize: '0.7rem' }}>
              {leave.employee.first_name[0]}{leave.employee.last_name[0]}
            </div>
            <div>
              <div style={{ fontWeight: 600, fontSize: '0.85rem' }}>
                {leave.employee.first_name} {leave.employee.last_name}
              </div>
              <div style={{ fontSize: '0.72rem', color: 'var(--text-muted)' }}>
                {leave.employee.email}
              </div>
            </div>
          </div>
        </td>
      )}
      <td>
        <span className={`badge badge-${leave.leave_type === 'annual' ? 'approved'
          : leave.leave_type === 'sick' ? 'pending'
          : 'draft'}`}>
          {leave.leave_type}
        </span>
      </td>
      <td>
        <div style={{ fontSize: '0.85rem', fontWeight: 500 }}>
          {format(parseISO(leave.start_date), 'MMM d')} → {format(parseISO(leave.end_date), 'MMM d, yyyy')}
        </div>
        <div style={{ fontSize: '0.72rem', color: 'var(--text-muted)' }}>{days} day{days > 1 ? 's' : ''}</div>
      </td>
      <td style={{ color: 'var(--text-secondary)', fontSize: '0.82rem', maxWidth: 180 }}>
        {leave.reason || '—'}
      </td>
      <td><span className={`badge badge-${leave.status}`}>{leave.status}</span></td>
      <td style={{ fontSize: '0.78rem', color: 'var(--text-muted)' }}>
        {format(parseISO(leave.created_at), 'MMM d, yyyy')}
      </td>
      {isManager && leave.status === 'pending' && (
        <td>
          <div style={{ display: 'flex', gap: 6 }}>
            <button className="btn btn-success btn-sm"
              onClick={() => onReview(leave.id, 'approve')}>
              <CheckCircle size={13} /> Approve
            </button>
            <button className="btn btn-danger btn-sm"
              onClick={() => onReview(leave.id, 'reject')}>
              <XCircle size={13} /> Reject
            </button>
          </div>
        </td>
      )}
      {isManager && leave.status !== 'pending' && <td>—</td>}
    </tr>
  );
}

export default function Leave() {
  const { user } = useAuth();
  const isManager = user?.role === 'manager';

  const [leaves,      setLeaves]    = useState([]);
  const [loading,     setLoading]   = useState(true);
  const [showModal,   setModal]     = useState(false);
  const [filter,      setFilter]    = useState('all');
  const [balance,     setBalance]   = useState(20);

  const fetchLeaves = async () => {
    setLoading(true);
    try {
      if (isManager) {
        const res = await api.get('/leave');
        setLeaves(res.data);
      } else {
        const [lRes, meRes] = await Promise.all([
          api.get('/leave/my'),
          api.get('/auth/me')
        ]);
        setLeaves(lRes.data);
        setBalance(meRes.data.leave_balance ?? 20);
      }
    } catch (err) {
      toast.error('Failed to load leaves');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { fetchLeaves(); }, []);

  const handleReview = async (id, action) => {
    try {
      const res = await api.put(`/leave/${id}/review`, { action });
      toast.success(res.data.message);
      fetchLeaves();
    } catch (err) {
      toast.error(err.response?.data?.error || 'Failed');
    }
  };

  const filtered = filter === 'all' ? leaves : leaves.filter(l => l.status === filter);

  const pending  = leaves.filter(l => l.status === 'pending').length;
  const approved = leaves.filter(l => l.status === 'approved').length;

  return (
    <div>
      <div className="page-header">
        <div>
          <h1 className="page-title">{isManager ? 'Leave Requests' : 'My Leave'}</h1>
          <p className="page-subtitle">
            {isManager ? 'Review and manage employee leave' : `${balance} annual days remaining`}
          </p>
        </div>
        {!isManager && (
          <button className="btn btn-primary" onClick={() => setModal(true)}>
            <Plus size={15} /> Apply for Leave
          </button>
        )}
      </div>

      <div className="stat-grid" style={{ marginBottom: 20 }}>
        <div className="stat-card">
          <div className="stat-value" style={{ color: 'var(--warning)' }}>{pending}</div>
          <div className="stat-label">Pending</div>
        </div>
        <div className="stat-card">
          <div className="stat-value" style={{ color: 'var(--success)' }}>{approved}</div>
          <div className="stat-label">Approved</div>
        </div>
        {!isManager && (
          <div className="stat-card">
            <div className="stat-value" style={{ color: 'var(--accent)' }}>{balance}</div>
            <div className="stat-label">Days Remaining</div>
          </div>
        )}
      </div>

      <div className="card">
        <div className="card-header">
          <h2 className="card-title">
            {isManager ? 'All Requests' : 'My Requests'}
          </h2>
          <div style={{ display: 'flex', gap: 4, background: 'var(--bg-elevated)',
            borderRadius: 'var(--radius-sm)', padding: 4 }}>
            {['all','pending','approved','rejected'].map(f => (
              <button key={f} onClick={() => setFilter(f)}
                className={`btn btn-sm ${filter === f ? 'btn-primary' : 'btn-ghost'}`}
                style={{ border: 'none', textTransform: 'capitalize' }}>
                {f}
              </button>
            ))}
          </div>
        </div>

        {loading ? (
          <div style={{ display: 'flex', justifyContent: 'center', padding: 40 }}>
            <div className="spinner" style={{ width: 32, height: 32 }} />
          </div>
        ) : (
          <div className="table-wrap">
            <table>
              <thead>
                <tr>
                  {isManager && <th>Employee</th>}
                  <th>Type</th>
                  <th>Period</th>
                  <th>Reason</th>
                  <th>Status</th>
                  <th>Applied</th>
                  {isManager && <th>Action</th>}
                </tr>
              </thead>
              <tbody>
                {filtered.length === 0 ? (
                  <tr><td colSpan={isManager ? 7 : 5}>
                    <div className="empty-state">No leave requests</div>
                  </td></tr>
                ) : filtered.map(leave => (
                  <LeaveRow
                    key={leave.id}
                    leave={leave}
                    isManager={isManager}
                    onReview={handleReview}
                  />
                ))}
              </tbody>
            </table>
          </div>
        )}
      </div>

      {showModal && (
        <ApplyLeaveModal
          leaveBalance={balance}
          onClose={() => setModal(false)}
          onSuccess={fetchLeaves}
        />
      )}
    </div>
  );
}
