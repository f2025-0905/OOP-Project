import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { Toaster } from 'react-hot-toast';
import { AuthProvider, useAuth } from './hooks/useAuth';

import Login         from './pages/Login';
import Signup        from './pages/Signup';
import ManagerDashboard from './pages/ManagerDashboard';
import Staff         from './pages/Staff';
import Schedule      from './pages/Schedule';
import Leave         from './pages/Leave';
import { MySchedule, Announcements } from './pages/EmployeePages';
import TopNav        from './components/shared/TopNav';

import './styles/global.css';

// Wraps a route — redirects to /login if not authenticated
function RequireAuth({ children, managerOnly = false, employeeOnly = false }) {
  const { user, loading } = useAuth();

  if (loading) return (
    <div style={{ display: 'flex', justifyContent: 'center', alignItems: 'center',
      height: '100vh', background: 'var(--bg-base)' }}>
      <div className="spinner" style={{ width: 36, height: 36 }} />
    </div>
  );

  if (!user) return <Navigate to="/login" replace />;
  if (managerOnly  && user.role !== 'manager')  return <Navigate to="/my-schedule" replace />;
  if (employeeOnly && user.role !== 'employee') return <Navigate to="/dashboard"   replace />;

  return children;
}

// Shell layout with top nav
function AppLayout({ children }) {
  return (
    <div className="app-shell">
      <TopNav />
      <main className="main-content">{children}</main>
    </div>
  );
}

function AppRoutes() {
  const { user } = useAuth();

  return (
    <Routes>
      {/* Public */}
      <Route path="/login"  element={<Login />} />
      <Route path="/signup" element={<Signup />} />

      {/* Root redirect */}
      <Route path="/" element={
        user ? <Navigate to={user.role === 'manager' ? '/dashboard' : '/my-schedule'} replace />
              : <Navigate to="/login" replace />
      } />

      {/* Manager routes */}
      <Route path="/dashboard" element={
        <RequireAuth managerOnly>
          <AppLayout><ManagerDashboard /></AppLayout>
        </RequireAuth>
      } />
      <Route path="/staff" element={
        <RequireAuth managerOnly>
          <AppLayout><Staff /></AppLayout>
        </RequireAuth>
      } />
      <Route path="/schedule" element={
        <RequireAuth managerOnly>
          <AppLayout><Schedule /></AppLayout>
        </RequireAuth>
      } />
      <Route path="/leave" element={
        <RequireAuth managerOnly>
          <AppLayout><Leave /></AppLayout>
        </RequireAuth>
      } />

      {/* Employee routes */}
      <Route path="/my-schedule" element={
        <RequireAuth>
          <AppLayout><MySchedule /></AppLayout>
        </RequireAuth>
      } />
      <Route path="/my-leave" element={
        <RequireAuth employeeOnly>
          <AppLayout><Leave /></AppLayout>
        </RequireAuth>
      } />

      {/* Shared */}
      <Route path="/announcements" element={
        <RequireAuth>
          <AppLayout><Announcements /></AppLayout>
        </RequireAuth>
      } />

      {/* 404 fallback */}
      <Route path="*" element={<Navigate to="/" replace />} />
    </Routes>
  );
}

export default function App() {
  return (
    <AuthProvider>
      <BrowserRouter>
        <AppRoutes />
        <Toaster
          position="top-right"
          toastOptions={{
            style: {
              background: 'var(--bg-elevated)',
              color: 'var(--text-primary)',
              border: '1px solid var(--border)',
              fontFamily: 'var(--font-body)',
              fontSize: '0.875rem'
            },
            success: { iconTheme: { primary: '#34D399', secondary: '#13151C' } },
            error:   { iconTheme: { primary: '#F87171', secondary: '#13151C' } }
          }}
        />
      </BrowserRouter>
    </AuthProvider>
  );
}
