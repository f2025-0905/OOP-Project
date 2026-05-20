# ShiftWise — Employee Scheduling System
**Final Year OOP Project | C++ Backend · React Frontend · PostgreSQL**

---

## Architecture Overview

```
shiftwise/
├── backend/          ← C++ (Crow HTTP + libpqxx + JWT)
│   ├── src/
│   │   ├── main.cpp                     ← All routes registered here
│   │   ├── controllers/
│   │   │   ├── AuthController.cpp       ← Login, Signup, Me
│   │   │   ├── StaffController.cpp      ← Employees & Positions
│   │   │   ├── ScheduleController.cpp   ← Shifts, publish
│   │   │   ├── LeaveController.cpp      ← Apply, approve, reject
│   │   │   └── DashboardController.cpp  ← Widgets, announcements
│   │   └── middleware/
│   │       └── AuthMiddleware.cpp       ← JWT verify, role guards
│   ├── include/                         ← Header files
│   ├── CMakeLists.txt
│   └── Dockerfile
├── frontend/         ← React 18
│   ├── src/
│   │   ├── App.js                       ← Router + auth guards
│   │   ├── pages/                       ← All 5 modules
│   │   ├── components/shared/Sidebar.jsx
│   │   ├── hooks/useAuth.js             ← Auth context
│   │   ├── utils/api.js                 ← Axios + JWT interceptors
│   │   └── styles/global.css            ← Full design system
│   └── Dockerfile
└── database/
    └── schema.sql                       ← Run this first
```

---

## OOP Concepts Used

| Concept | Where |
|---|---|
| **Inheritance** | `Employee : User`, `Manager : User`, `LeaveRequest : Request` |
| **Polymorphism** | `ShiftEntry::getStatus()`, `ShiftEntry::canEdit()`, `Request::approve()` |
| **Abstraction** | `ShiftEntry` (abstract), `Request` (abstract) |
| **Encapsulation** | All class members private/protected with getters/setters |
| **Dependency Injection** | Controllers receive `pqxx::connection&` in constructor |

---

## Environment Variables

### Backend
| Variable | Description | Example |
|---|---|---|
| `DATABASE_URL` | PostgreSQL connection string | `postgresql://user:pass@host/db` |
| `JWT_SECRET` | Secret key for token signing | `my-super-secret-key-2024` |
| `PORT` | HTTP port | `8080` |

### Frontend
| Variable | Description | Example |
|---|---|---|
| `REACT_APP_API_URL` | Backend URL | `https://shiftwise-api.onrender.com` |

---

## Local Development

### 1. Database
```bash
# Create database and run schema
psql -U postgres -c "CREATE DATABASE shiftwise;"
psql -U postgres -d shiftwise -f database/schema.sql
```

### 2. Backend
```bash
cd backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

export DATABASE_URL="postgresql://postgres:password@localhost/shiftwise"
export JWT_SECRET="dev-secret-key"
./shiftwise
# Runs on http://localhost:8080
```

### 3. Frontend
```bash
cd frontend
npm install
echo "REACT_APP_API_URL=http://localhost:8080" > .env.local
npm start
# Runs on http://localhost:3000
```

---

## Deployment on Render.com

### Step 1 — PostgreSQL Database
1. Go to Render dashboard → **New** → **PostgreSQL**
2. Name: `shiftwise-db`
3. Copy the **External Database URL**
4. Run `schema.sql` in the Render database console

### Step 2 — Backend (Web Service)
1. **New** → **Web Service** → connect your GitHub repo
2. Set root directory: `backend`
3. Environment: **Docker**
4. Add environment variables:
   - `DATABASE_URL` → paste from Step 1
   - `JWT_SECRET` → any long random string
5. Deploy. Copy the **service URL** (e.g. `https://shiftwise-api.onrender.com`)

### Step 3 — Frontend (Static Site or Web Service)
1. **New** → **Web Service** → same repo
2. Set root directory: `frontend`
3. Environment: **Docker**
4. Add environment variable:
   - `REACT_APP_API_URL` → backend URL from Step 2
5. Deploy

### Step 4 — Create First Manager Account
```
POST https://your-backend.onrender.com/api/auth/signup
{
  "username": "admin",
  "email":    "admin@company.com",
  "password": "securepassword123",
  "first_name": "John",
  "last_name":  "Doe"
}
```
Or just use the Signup page in the frontend.

---

## API Endpoints Reference

### Auth
| Method | Endpoint | Auth | Description |
|---|---|---|---|
| POST | `/api/auth/login` | — | Login (employee or manager) |
| POST | `/api/auth/signup` | — | Create manager account |
| GET  | `/api/auth/me` | Any | Get current user |
| POST | `/api/auth/change-password` | Any | Change password |

### Staff (Manager only)
| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/staff` | All employees |
| POST | `/api/staff` | Add employee |
| PUT | `/api/staff/:id` | Update employee |
| DELETE | `/api/staff/:id` | Deactivate employee |
| PUT | `/api/staff/:id/position` | Assign position |
| GET | `/api/positions` | All positions |
| POST | `/api/positions` | Create position |
| DELETE | `/api/positions/:id` | Delete position |

### Schedule
| Method | Endpoint | Auth | Description |
|---|---|---|---|
| GET | `/api/schedule?week=YYYY-MM-DD` | Manager | Weekly schedule |
| POST | `/api/schedule/shift` | Manager | Create shift |
| PUT | `/api/schedule/shift/:id` | Manager | Edit draft shift |
| DELETE | `/api/schedule/shift/:id` | Manager | Delete shift |
| POST | `/api/schedule/publish` | Manager | Publish week |
| GET | `/api/schedule/my` | Employee | Own shifts |

### Leave
| Method | Endpoint | Auth | Description |
|---|---|---|---|
| POST | `/api/leave` | Employee | Apply for leave |
| GET | `/api/leave/my` | Employee | Own leave requests |
| GET | `/api/leave` | Manager | All leave requests |
| PUT | `/api/leave/:id/review` | Manager | Approve or reject |

### Dashboard & Announcements
| Method | Endpoint | Auth | Description |
|---|---|---|---|
| GET | `/api/dashboard/manager` | Manager | 3 widgets + stats |
| GET | `/api/dashboard/employee` | Employee | Upcoming shifts + balance |
| GET | `/api/announcements` | Any | All announcements |
| POST | `/api/announcements` | Manager | Post announcement |
| DELETE | `/api/announcements/:id` | Manager | Delete announcement |

---

## Module Summary

| Module | Manager Features | Employee Features |
|---|---|---|
| **Auth** | Login, Signup | Login |
| **Dashboard** | Working now, On leave, Announcement wall | — |
| **Staff** | Add/edit/deactivate employees, create positions, assign positions | — |
| **Schedule** | Build weekly grid, assign shifts, publish | View published shifts by week |
| **Leave** | View all requests, approve/reject | Apply, view history, see balance |
| **Announcements** | Post, delete | Read |

---

## Features Implemented
- JWT authentication with role-based access (manager / employee)
- Leave balance auto-deduction on approval
- Conflict detection: warns if scheduling someone on approved leave
- Draft vs published shift states
- Announcement wall with priority levels (normal / high / urgent)
- Soft-delete for employees (deactivate, not delete)
- Leave approval auto-removes conflicting draft shifts
