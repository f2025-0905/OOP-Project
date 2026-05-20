import { useState, useEffect } from 'react';
import api from '../utils/api';
import toast from 'react-hot-toast';
import { Plus, Pencil, Trash2, UserCheck, X, Search } from 'lucide-react';

function AddEmployeeModal({ positions, onClose, onSuccess }) {
  const [form, setForm] = useState({
    first_name: '', last_name: '', username: '',
    email: '', password: '', position_id: '', phone: ''
  });
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setLoading(true);
    try {
      await api.post('/staff', form);
      toast.success('Employee added successfully');
      onSuccess();
      onClose();
    } catch (err) {
      toast.error(err.response?.data?.error || 'Failed to add employee');
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
      <div className="modal" style={{ maxWidth: 520 }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 20 }}>
          <h2 className="modal-title" style={{ margin: 0 }}>Add New Employee</h2>
          <button className="btn btn-icon btn-ghost" onClick={onClose}><X size={16} /></button>
        </div>

        <form onSubmit={handleSubmit}>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">First Name *</label>
              <input className="form-input" required {...f('first_name')} />
            </div>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">Last Name *</label>
              <input className="form-input" required {...f('last_name')} />
            </div>
          </div>

          <div className="form-group mt-2">
            <label className="form-label">Username *</label>
            <input className="form-input" required {...f('username')} />
          </div>

          <div className="form-group">
            <label className="form-label">Email Address *</label>
            <input className="form-input" type="email" required {...f('email')} />
          </div>

          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">Password *</label>
              <input className="form-input" type="password" required {...f('password')} />
            </div>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">Phone</label>
              <input className="form-input" type="tel" {...f('phone')} />
            </div>
          </div>

          <div className="form-group mt-2">
            <label className="form-label">Assign Position</label>
            <select className="form-input" {...f('position_id')}>
              <option value="">— No position yet —</option>
              {positions.map(p => (
                <option key={p.id} value={p.id}>{p.name}</option>
              ))}
            </select>
          </div>

          <div style={{ display: 'flex', gap: 8, justifyContent: 'flex-end', marginTop: 8 }}>
            <button type="button" className="btn btn-ghost" onClick={onClose}>Cancel</button>
            <button type="submit" className="btn btn-primary" disabled={loading}>
              {loading ? <div className="spinner" /> : 'Add Employee'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}

function AddPositionModal({ onClose, onSuccess }) {
  const [form, setForm] = useState({ name: '', description: '', color: '#4F8EF7' });
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setLoading(true);
    try {
      await api.post('/positions', form);
      toast.success('Position created');
      onSuccess();
      onClose();
    } catch (err) {
      toast.error(err.response?.data?.error || 'Failed');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="modal-overlay" onClick={e => e.target === e.currentTarget && onClose()}>
      <div className="modal">
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 20 }}>
          <h2 className="modal-title" style={{ margin: 0 }}>Create Position</h2>
          <button className="btn btn-icon btn-ghost" onClick={onClose}><X size={16} /></button>
        </div>
        <form onSubmit={handleSubmit}>
          <div className="form-group">
            <label className="form-label">Position Name *</label>
            <input className="form-input" required value={form.name}
              onChange={e => setForm({...form, name: e.target.value})} />
          </div>
          <div className="form-group">
            <label className="form-label">Description</label>
            <textarea className="form-input" rows={2} value={form.description}
              onChange={e => setForm({...form, description: e.target.value})} />
          </div>
          <div className="form-group">
            <label className="form-label">Color</label>
            <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
              <input type="color" value={form.color}
                onChange={e => setForm({...form, color: e.target.value})}
                style={{ width: 40, height: 36, border: 'none', background: 'none', cursor: 'pointer' }}
              />
              <input className="form-input" value={form.color}
                onChange={e => setForm({...form, color: e.target.value})}
                style={{ fontFamily: 'monospace', flex: 1 }} />
            </div>
          </div>
          <div style={{ display: 'flex', gap: 8, justifyContent: 'flex-end' }}>
            <button type="button" className="btn btn-ghost" onClick={onClose}>Cancel</button>
            <button type="submit" className="btn btn-primary" disabled={loading}>
              {loading ? <div className="spinner" /> : 'Create'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}

export default function Staff() {
  const [employees,  setEmployees]  = useState([]);
  const [positions,  setPositions]  = useState([]);
  const [search,     setSearch]     = useState('');
  const [loading,    setLoading]    = useState(true);
  const [showAddEmp, setAddEmp]     = useState(false);
  const [showAddPos, setAddPos]     = useState(false);
  const [tab, setTab]               = useState('employees');

  const fetchAll = async () => {
    setLoading(true);
    const [e, p] = await Promise.all([api.get('/staff'), api.get('/positions')]);
    setEmployees(e.data);
    setPositions(p.data);
    setLoading(false);
  };

  useEffect(() => { fetchAll(); }, []);

  const deleteEmployee = async (id, name) => {
    if (!window.confirm(`Deactivate ${name}?`)) return;
    await api.delete(`/staff/${id}`);
    toast.success('Employee deactivated');
    fetchAll();
  };

  const deletePosition = async (id, name) => {
    if (!window.confirm(`Delete position "${name}"?`)) return;
    await api.delete(`/positions/${id}`);
    toast.success('Position deleted');
    fetchAll();
  };

  const filtered = employees.filter(e =>
    `${e.first_name} ${e.last_name} ${e.email} ${e.username}`
      .toLowerCase().includes(search.toLowerCase())
  );

  return (
    <div>
      <div className="page-header">
        <div>
          <h1 className="page-title">Staff</h1>
          <p className="page-subtitle">Manage your team and positions</p>
        </div>
        <div style={{ display: 'flex', gap: 8 }}>
          <button className="btn btn-ghost" onClick={() => setAddPos(true)}>
            <Plus size={15} /> New Position
          </button>
          <button className="btn btn-primary" onClick={() => setAddEmp(true)}>
            <Plus size={15} /> Add Employee
          </button>
        </div>
      </div>

      <div style={{ display: 'flex', gap: 4, marginBottom: 20, background: 'var(--bg-surface)',
        border: '1px solid var(--border)', borderRadius: 'var(--radius-sm)', padding: 4, width: 'fit-content' }}>
        {['employees','positions'].map(t => (
          <button key={t} onClick={() => setTab(t)}
            className={`btn ${tab === t ? 'btn-primary' : 'btn-ghost'} btn-sm`}
            style={{ textTransform: 'capitalize', border: 'none' }}>
            {t} {t === 'employees' ? `(${employees.length})` : `(${positions.length})`}
          </button>
        ))}
      </div>

      {tab === 'employees' && (
        <div className="card">
          <div className="card-header">
            <h2 className="card-title">All Employees</h2>
            <div style={{ position: 'relative' }}>
              <Search size={14} style={{ position: 'absolute', left: 10, top: '50%',
                transform: 'translateY(-50%)', color: 'var(--text-muted)' }} />
              <input className="form-input" placeholder="Search..."
                value={search} onChange={e => setSearch(e.target.value)}
                style={{ paddingLeft: 32, width: 200 }} />
            </div>
          </div>

          {loading ? <div className="spinner" style={{ margin: '32px auto', display: 'block' }} /> : (
            <div className="table-wrap">
              <table>
                <thead>
                  <tr>
                    <th>Employee</th>
                    <th>Username</th>
                    <th>Email</th>
                    <th>Position</th>
                    <th>Leave Balance</th>
                    <th>Status</th>
                    <th>Actions</th>
                  </tr>
                </thead>
                <tbody>
                  {filtered.length === 0 ? (
                    <tr><td colSpan={7}>
                      <div className="empty-state">No employees found</div>
                    </td></tr>
                  ) : filtered.map(emp => (
                    <tr key={emp.id}>
                      <td>
                        <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                          <div className="user-avatar" style={{ width: 32, height: 32 }}>
                            {emp.first_name[0]}{emp.last_name[0]}
                          </div>
                          <span style={{ fontWeight: 600 }}>
                            {emp.first_name} {emp.last_name}
                          </span>
                        </div>
                      </td>
                      <td style={{ color: 'var(--text-secondary)' }}>{emp.username}</td>
                      <td style={{ color: 'var(--text-secondary)' }}>{emp.email}</td>
                      <td>
                        {emp.position ? (
                          <span style={{
                            display: 'inline-flex', alignItems: 'center', gap: 6,
                            padding: '3px 10px', borderRadius: 100,
                            background: emp.position.color + '22',
                            color: emp.position.color, fontSize: '0.78rem', fontWeight: 600
                          }}>
                            <span style={{ width: 6, height: 6, borderRadius: '50%',
                              background: emp.position.color, flexShrink: 0 }} />
                            {emp.position.name}
                          </span>
                        ) : (
                          <span style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>Unassigned</span>
                        )}
                      </td>
                      <td>
                        <span style={{ fontWeight: 600 }}>{emp.leave_balance}</span>
                        <span style={{ color: 'var(--text-muted)', fontSize: '0.78rem' }}> days</span>
                      </td>
                      <td>
                        <span className={`badge ${emp.is_active ? 'badge-approved' : 'badge-rejected'}`}>
                          {emp.is_active ? 'Active' : 'Inactive'}
                        </span>
                      </td>
                      <td>
                        <div style={{ display: 'flex', gap: 6 }}>
                          <button className="btn btn-icon btn-ghost btn-sm"
                            title="Deactivate"
                            onClick={() => deleteEmployee(emp.id, `${emp.first_name} ${emp.last_name}`)}>
                            <Trash2 size={14} />
                          </button>
                        </div>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      )}

      {tab === 'positions' && (
        <div className="card">
          <div className="card-header">
            <h2 className="card-title">Positions</h2>
            <button className="btn btn-primary btn-sm" onClick={() => setAddPos(true)}>
              <Plus size={14} /> New
            </button>
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(240px, 1fr))', gap: 12 }}>
            {positions.length === 0 ? (
              <div className="empty-state" style={{ gridColumn: '1/-1' }}>
                <p>No positions yet. Create one to get started.</p>
              </div>
            ) : positions.map(pos => (
              <div key={pos.id} style={{
                background: 'var(--bg-elevated)', borderRadius: 'var(--radius-md)',
                padding: 16, borderLeft: `3px solid ${pos.color}`,
                display: 'flex', flexDirection: 'column', gap: 8
              }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
                  <div>
                    <div style={{ fontWeight: 700, fontSize: '0.9rem' }}>{pos.name}</div>
                    {pos.description && (
                      <div style={{ fontSize: '0.78rem', color: 'var(--text-muted)', marginTop: 2 }}>
                        {pos.description}
                      </div>
                    )}
                  </div>
                  <button className="btn btn-icon btn-ghost btn-sm"
                    onClick={() => deletePosition(pos.id, pos.name)}>
                    <Trash2 size={13} />
                  </button>
                </div>
                <div style={{ fontSize: '0.75rem', color: 'var(--text-secondary)' }}>
                  {pos.employee_count} employee{pos.employee_count !== 1 ? 's' : ''}
                </div>
              </div>
            ))}
          </div>
        </div>
      )}

      {showAddEmp && (
        <AddEmployeeModal positions={positions} onClose={() => setAddEmp(false)} onSuccess={fetchAll} />
      )}
      {showAddPos && (
        <AddPositionModal onClose={() => setAddPos(false)} onSuccess={fetchAll} />
      )}
    </div>
  );
}
