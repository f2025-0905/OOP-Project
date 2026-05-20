#include <crow.h>
#include <pqxx/pqxx>
#include <iostream>
#include <cstdlib>
#include "controllers/AuthController.h"
#include "controllers/Controllers.h"

int main() {
    // ============================================
    // Database connection
    // ============================================
    std::string dbUrl = std::getenv("DATABASE_URL") ? std::getenv("DATABASE_URL") : "";
    if (dbUrl.empty()) {
        std::cerr << "ERROR: DATABASE_URL environment variable not set\n";
        return 1;
    }

    pqxx::connection db(dbUrl);
    std::cout << "Connected to PostgreSQL\n";

    // ============================================
    // Controllers (Dependency Injection via ref)
    // ============================================
    AuthController     authCtrl(db);
    StaffController    staffCtrl(db);
    ScheduleController scheduleCtrl(db);
    LeaveController    leaveCtrl(db);
    DashboardController dashCtrl(db);

    // ============================================
    // Crow app
    // ============================================
    crow::SimpleApp app;

    // CORS middleware
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization")
        .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
        .origin("*");

    // ============================================
    // AUTH ROUTES
    // ============================================
    CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)(
        [&](const crow::request& req) { return authCtrl.login(req); });

    CROW_ROUTE(app, "/api/auth/signup").methods("POST"_method)(
        [&](const crow::request& req) { return authCtrl.signup(req); });

    CROW_ROUTE(app, "/api/auth/me").methods("GET"_method)(
        [&](const crow::request& req) { return authCtrl.getMe(req); });

    CROW_ROUTE(app, "/api/auth/change-password").methods("POST"_method)(
        [&](const crow::request& req) { return authCtrl.changePassword(req); });

    // ============================================
    // STAFF ROUTES
    // ============================================
    CROW_ROUTE(app, "/api/staff").methods("GET"_method)(
        [&](const crow::request& req) { return staffCtrl.getAllEmployees(req); });

    CROW_ROUTE(app, "/api/staff").methods("POST"_method)(
        [&](const crow::request& req) { return staffCtrl.createEmployee(req); });

    CROW_ROUTE(app, "/api/staff/<string>").methods("PUT"_method)(
        [&](const crow::request& req, std::string id) { return staffCtrl.updateEmployee(req, id); });

    CROW_ROUTE(app, "/api/staff/<string>").methods("DELETE"_method)(
        [&](const crow::request& req, std::string id) { return staffCtrl.deleteEmployee(req, id); });

    CROW_ROUTE(app, "/api/staff/<string>/position").methods("PUT"_method)(
        [&](const crow::request& req, std::string id) { return staffCtrl.assignPosition(req, id); });

    // POSITION ROUTES
    CROW_ROUTE(app, "/api/positions").methods("GET"_method)(
        [&](const crow::request& req) { return staffCtrl.getAllPositions(req); });

    CROW_ROUTE(app, "/api/positions").methods("POST"_method)(
        [&](const crow::request& req) { return staffCtrl.createPosition(req); });

    CROW_ROUTE(app, "/api/positions/<string>").methods("DELETE"_method)(
        [&](const crow::request& req, std::string id) { return staffCtrl.deletePosition(req, id); });

    // ============================================
    // SCHEDULE ROUTES
    // ============================================
    CROW_ROUTE(app, "/api/schedule").methods("GET"_method)(
        [&](const crow::request& req) { return scheduleCtrl.getWeeklySchedule(req); });

    CROW_ROUTE(app, "/api/schedule/my").methods("GET"_method)(
        [&](const crow::request& req) { return scheduleCtrl.getMyShifts(req); });

    CROW_ROUTE(app, "/api/schedule/shift").methods("POST"_method)(
        [&](const crow::request& req) { return scheduleCtrl.createShift(req); });

    CROW_ROUTE(app, "/api/schedule/shift/<string>").methods("PUT"_method)(
        [&](const crow::request& req, std::string id) { return scheduleCtrl.updateShift(req, id); });

    CROW_ROUTE(app, "/api/schedule/shift/<string>").methods("DELETE"_method)(
        [&](const crow::request& req, std::string id) { return scheduleCtrl.deleteShift(req, id); });

    CROW_ROUTE(app, "/api/schedule/publish").methods("POST"_method)(
        [&](const crow::request& req) { return scheduleCtrl.publishShifts(req); });

    // ============================================
    // LEAVE ROUTES
    // ============================================
    CROW_ROUTE(app, "/api/leave").methods("POST"_method)(
        [&](const crow::request& req) { return leaveCtrl.applyLeave(req); });

    CROW_ROUTE(app, "/api/leave/my").methods("GET"_method)(
        [&](const crow::request& req) { return leaveCtrl.getMyLeaves(req); });

    CROW_ROUTE(app, "/api/leave").methods("GET"_method)(
        [&](const crow::request& req) { return leaveCtrl.getAllLeaves(req); });

    CROW_ROUTE(app, "/api/leave/<string>/review").methods("PUT"_method)(
        [&](const crow::request& req, std::string id) { return leaveCtrl.reviewLeave(req, id); });

    // ============================================
    // DASHBOARD ROUTES
    // ============================================
    CROW_ROUTE(app, "/api/dashboard/manager").methods("GET"_method)(
        [&](const crow::request& req) { return dashCtrl.getManagerDashboard(req); });

    CROW_ROUTE(app, "/api/dashboard/employee").methods("GET"_method)(
        [&](const crow::request& req) { return dashCtrl.getEmployeeDashboard(req); });

    CROW_ROUTE(app, "/api/announcements").methods("GET"_method)(
        [&](const crow::request& req) { return dashCtrl.getAnnouncements(req); });

    CROW_ROUTE(app, "/api/announcements").methods("POST"_method)(
        [&](const crow::request& req) { return dashCtrl.createAnnouncement(req); });

    CROW_ROUTE(app, "/api/announcements/<string>").methods("DELETE"_method)(
        [&](const crow::request& req, std::string id) { return dashCtrl.deleteAnnouncement(req, id); });

    // Health check
    CROW_ROUTE(app, "/health")([](){ return crow::response(200, R"({"status":"ok"})"); });

    // ============================================
    // Start server
    // ============================================
    int port = std::getenv("PORT") ? std::stoi(std::getenv("PORT")) : 8080;
    std::cout << "ShiftWise backend running on port " << port << "\n";
    app.port(port).multithreaded().run();

    return 0;
}
