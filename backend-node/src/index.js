require('dotenv').config();
const express = require('express');
const cors    = require('cors');

const { requireAuth, requireManager } = require('./middleware/auth');
const auth     = require('./controllers/auth');
const staff    = require('./controllers/staff');
const schedule = require('./controllers/schedule');
const leave    = require('./controllers/leave');
const dashboard = require('./controllers/dashboard');

const app = express();

app.use(cors({ origin: '*', methods: ['GET','POST','PUT','DELETE','OPTIONS'] }));
app.use(express.json());

// ── Auth ────────────────────────────────────────────
app.post('/api/auth/login',           auth.login);
app.post('/api/auth/signup',          auth.signup);
app.get( '/api/auth/me',              requireAuth, auth.getMe);
app.post('/api/auth/change-password', requireAuth, auth.changePassword);

// ── Staff ───────────────────────────────────────────
app.get(   '/api/staff',                  requireManager, staff.getAllEmployees);
app.post(  '/api/staff',                  requireManager, staff.createEmployee);
app.put(   '/api/staff/:id',              requireManager, staff.updateEmployee);
app.delete('/api/staff/:id',              requireManager, staff.deleteEmployee);
app.put(   '/api/staff/:id/position',     requireManager, staff.assignPosition);

app.get(   '/api/positions',              requireAuth,    staff.getAllPositions);
app.post(  '/api/positions',              requireManager, staff.createPosition);
app.delete('/api/positions/:id',          requireManager, staff.deletePosition);

// ── Schedule ────────────────────────────────────────
app.get(   '/api/schedule',               requireManager, schedule.getWeeklySchedule);
app.get(   '/api/schedule/my',            requireAuth,    schedule.getMyShifts);
app.post(  '/api/schedule/shift',         requireManager, schedule.createShift);
app.put(   '/api/schedule/shift/:id',     requireManager, schedule.updateShift);
app.delete('/api/schedule/shift/:id',     requireManager, schedule.deleteShift);
app.post(  '/api/schedule/publish',       requireManager, schedule.publishShifts);

// ── Leave ───────────────────────────────────────────
app.post('/api/leave',                    requireAuth,    leave.applyLeave);
app.get( '/api/leave/my',                 requireAuth,    leave.getMyLeaves);
app.get( '/api/leave',                    requireManager, leave.getAllLeaves);
app.put( '/api/leave/:id/review',         requireManager, leave.reviewLeave);

// ── Dashboard & Announcements ───────────────────────
app.get(   '/api/dashboard/manager',      requireManager, dashboard.getManagerDashboard);
app.get(   '/api/dashboard/employee',     requireAuth,    dashboard.getEmployeeDashboard);
app.get(   '/api/announcements',          requireAuth,    dashboard.getAnnouncements);
app.post(  '/api/announcements',          requireManager, dashboard.createAnnouncement);
app.delete('/api/announcements/:id',      requireManager, dashboard.deleteAnnouncement);

// ── Health check ────────────────────────────────────
app.get('/health', (_, res) => res.json({ status: 'ok', service: 'shiftwise-node' }));

const PORT = process.env.PORT || 8080;
app.listen(PORT, () => console.log(`ShiftWise Node backend running on port ${PORT}`));
