import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../../hooks/useAuth';
import { LogOut } from 'lucide-react';

const managerLinks = [
  { label: 'Dashboard',      href: '/dashboard'      },
  { label: 'Staff',          href: '/staff'          },
  { label: 'Schedule',       href: '/schedule'       },
  { label: 'Leave',          href: '/leave'          },
  { label: 'Announcements',  href: '/announcements'  },
];

const employeeLinks = [
  { label: 'My Schedule',    href: '/my-schedule'    },
  { label: 'Leave',          href: '/my-leave'       },
  { label: 'Announcements',  href: '/announcements'  },
];

export default function TopNav() {
  const { user, logout } = useAuth();
  const location = useLocation();
  const links = user?.role === 'manager' ? managerLinks : employeeLinks;
  const initials = user ? (user.first_name[0] + user.last_name[0]).toUpperCase() : '?';

  return (
    <nav className="topnav">
      <div className="topnav-logo">
        Shift<span>Wise</span>
      </div>
      <div className="topnav-links">
        {links.map(link => (
          <Link
            key={link.href}
            to={link.href}
            className={`topnav-link ${location.pathname === link.href ? 'active' : ''}`}
          >
            {link.label}
          </Link>
        ))}
      </div>
      <div className="topnav-right">
        <div style={{ textAlign: 'right' }}>
          <div className="user-chip-name">{user?.first_name} {user?.last_name}</div>
          <div className="user-chip-role" style={{ textTransform: 'capitalize' }}>{user?.role}</div>
        </div>
        <div className="user-avatar">{initials}</div>
        <button className="btn btn-ghost btn-sm" onClick={logout}>
          <LogOut size={13} /> Sign out
        </button>
      </div>
    </nav>
  );
}
