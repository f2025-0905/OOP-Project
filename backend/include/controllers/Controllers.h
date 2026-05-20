#pragma once
#include <crow.h>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================
// Staff Controller
// ============================================
class StaffController {
private:
    pqxx::connection& db;
public:
    explicit StaffController(pqxx::connection& conn) : db(conn) {}

    crow::response getAllEmployees(const crow::request& req);          // GET  /api/staff
    crow::response getEmployee(const crow::request& req, std::string id); // GET  /api/staff/:id
    crow::response createEmployee(const crow::request& req);          // POST /api/staff
    crow::response updateEmployee(const crow::request& req, std::string id); // PUT  /api/staff/:id
    crow::response deleteEmployee(const crow::request& req, std::string id); // DEL  /api/staff/:id
    crow::response assignPosition(const crow::request& req, std::string id); // PUT  /api/staff/:id/position

    crow::response getAllPositions(const crow::request& req);          // GET  /api/positions
    crow::response createPosition(const crow::request& req);          // POST /api/positions
    crow::response updatePosition(const crow::request& req, std::string id);
    crow::response deletePosition(const crow::request& req, std::string id);
};

// ============================================
// Schedule Controller
// ============================================
class ScheduleController {
private:
    pqxx::connection& db;

    bool hasLeaveConflict(const std::string& employeeId,
                           const std::string& shiftDate);
public:
    explicit ScheduleController(pqxx::connection& conn) : db(conn) {}

    crow::response getWeeklySchedule(const crow::request& req);        // GET  /api/schedule?week=YYYY-MM-DD
    crow::response getEmployeeSchedule(const crow::request& req, std::string empId); // GET /api/schedule/employee/:id
    crow::response createShift(const crow::request& req);              // POST /api/schedule/shift
    crow::response updateShift(const crow::request& req, std::string id);
    crow::response deleteShift(const crow::request& req, std::string id);
    crow::response publishShifts(const crow::request& req);            // POST /api/schedule/publish
    crow::response getMyShifts(const crow::request& req);              // GET  /api/schedule/my
};

// ============================================
// Leave Controller
// ============================================
class LeaveController {
private:
    pqxx::connection& db;

    int calculateWorkingDays(const std::string& startDate,
                              const std::string& endDate);
    void markShiftsAsLeave(const std::string& employeeId,
                            const std::string& startDate,
                            const std::string& endDate,
                            pqxx::work& txn);
public:
    explicit LeaveController(pqxx::connection& conn) : db(conn) {}

    crow::response applyLeave(const crow::request& req);               // POST /api/leave
    crow::response getMyLeaves(const crow::request& req);              // GET  /api/leave/my
    crow::response getAllLeaves(const crow::request& req);             // GET  /api/leave (manager)
    crow::response reviewLeave(const crow::request& req, std::string id); // PUT  /api/leave/:id/review
    crow::response cancelLeave(const crow::request& req, std::string id);
};

// ============================================
// Dashboard Controller
// ============================================
class DashboardController {
private:
    pqxx::connection& db;
public:
    explicit DashboardController(pqxx::connection& conn) : db(conn) {}

    crow::response getManagerDashboard(const crow::request& req);      // GET /api/dashboard/manager
    crow::response getEmployeeDashboard(const crow::request& req);     // GET /api/dashboard/employee
    crow::response getAnnouncements(const crow::request& req);         // GET /api/announcements
    crow::response createAnnouncement(const crow::request& req);       // POST /api/announcements
    crow::response deleteAnnouncement(const crow::request& req, std::string id);
};
