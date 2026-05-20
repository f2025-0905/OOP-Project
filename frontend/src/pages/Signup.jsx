import { useState } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import { useAuth } from '../hooks/useAuth';
import toast from 'react-hot-toast';
import { UserPlus } from 'lucide-react';

export default function Signup() {
  const { signup } = useAuth();
  const navigate   = useNavigate();
  const [form, setForm] = useState({
    first_name: '', last_name: '', username: '', email: '', password: '', confirm: ''
  });
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (Object.values(form).some(v => !v)) {
      toast.error('Please fill in all fields');
      return;
    }
    if (form.password !== form.confirm) {
      toast.error('Passwords do not match');
      return;
    }
    if (form.password.length < 8) {
      toast.error('Password must be at least 8 characters');
      return;
    }

    setLoading(true);
    try {
      const { confirm, ...data } = form;
      await signup(data);
      toast.success('Account created! Welcome to ShiftWise.');
      navigate('/dashboard');
    } catch (err) {
      toast.error(err.response?.data?.error || 'Signup failed');
    } finally {
      setLoading(false);
    }
  };

  const f = (key) => ({ value: form[key], onChange: e => setForm({...form, [key]: e.target.value}) });

  return (
    <div className="auth-page">
      <div className="auth-bg-orb" style={{
        width: 350, height: 350, background: '#4F8EF7',
        top: '-100px', right: '-80px'
      }} />
      <div className="auth-bg-orb" style={{
        width: 250, height: 250, background: '#FBBF24',
        bottom: '-60px', left: '-60px'
      }} />

      <div className="auth-card" style={{ maxWidth: 480 }}>
        <div className="auth-logo">
          <h1>Shift<span>Wise</span></h1>
          <p>Create a Manager Account</p>
        </div>

        <form onSubmit={handleSubmit}>
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">First Name</label>
              <input className="form-input" placeholder="John" {...f('first_name')} />
            </div>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">Last Name</label>
              <input className="form-input" placeholder="Doe" {...f('last_name')} />
            </div>
          </div>

          <div className="form-group mt-2">
            <label className="form-label">Username</label>
            <input className="form-input" placeholder="johndoe" {...f('username')} />
          </div>

          <div className="form-group">
            <label className="form-label">Email</label>
            <input className="form-input" type="email" placeholder="john@company.com" {...f('email')} />
          </div>

          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">Password</label>
              <input className="form-input" type="password" placeholder="Min 8 chars" {...f('password')} />
            </div>
            <div className="form-group" style={{ marginBottom: 0 }}>
              <label className="form-label">Confirm</label>
              <input className="form-input" type="password" placeholder="Repeat" {...f('confirm')} />
            </div>
          </div>

          <button
            type="submit"
            className="btn btn-primary w-full"
            style={{ justifyContent: 'center', marginTop: 20, height: 44 }}
            disabled={loading}
          >
            {loading ? <div className="spinner" /> : <><UserPlus size={16} /> Create Manager Account</>}
          </button>
        </form>

        <div className="auth-divider">already have an account?</div>

        <p style={{ textAlign: 'center', fontSize: '0.85rem', color: 'var(--text-secondary)' }}>
          <Link to="/login" style={{ color: 'var(--accent)', fontWeight: 600 }}>
            Sign in instead
          </Link>
        </p>
      </div>
    </div>
  );
}
