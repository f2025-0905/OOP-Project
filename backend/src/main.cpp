#include <crow.h>
#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

// Include all controllers
#include "controllers/AuthController.cpp"
#include "controllers/StaffController.cpp"
#include "controllers/LeaveController.cpp"
#include "controllers/ScheduleController.cpp"
#include "controllers/DashboardController.cpp"

// ─────────────────────────────────────────────
//  main  —  starts the Crow HTTP server and
//  registers every API route
// ─────────────────────────────────────────────
int main () {
    // ── Read environment variables ────────────
    // These are set in Render.com's dashboard (never hardcode secrets!)
    string dbUrl    = getenv("DATABASE_URL") ? getenv("DATABASE_URL") : "";
    string jwtSecret = getenv("JWT_SECRET")  ? getenv("JWT_SECRET")  : "default-secret";
    string portStr  = getenv("PORT")         ? getenv("PORT")        : "8080";
    int    port     = stoi(portStr);

    if (dbUrl.empty()) {
        cerr << "ERROR: DATABASE_URL environment variable is not set." << endl;
        return 1;
    }

    cout << "Starting ShiftWise backend on port " << port << endl;

    // ── Create controller instances ───────────
    // Each controller handles a group of related routes.
    // We pass the DB connection string and JWT secret to every one.
    AuthController     authCtrl    (dbUrl, jwtSecret);
    StaffController    staffCtrl   (dbUrl, jwtSecret);
    LeaveController    leaveCtrl   (dbUrl, jwtSecret);
    ScheduleController schedCtrl   (dbUrl, jwtSecret);
    DashboardController dashCtrl   (dbUrl, jwtSecret);

    // ── Set up the web application ────────────
    crow::SimpleApp app;

    // CORS middleware — needed so the React frontend can talk to this server
    app.before_handle([](crow::request& req, crow::response& res, crow::App<>::context&) {
        res.add_header("Access-Control-Allow-Origin",  "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        if (req.method == crow::HTTPMethod::Options) {
            res.code = 204;
            res.end();
        }
    });

    // ── Auth routes ───────────────────────────
    CROW_ROUTE(app, "/api/auth/signup").methods("POST"_method)
    ([&authCtrl](const crow::request& req) {
        return authCtrl.signup(req);
    });

    CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)
    ([&authCtrl](const crow::request& req) {
        return authCtrl.login(req);
    });

    // ── Staff routes ──────────────────────────
    CROW_ROUTE(app, "/api/staff").methods("GET"_method)
    ([&staffCtrl](const crow::request& req) {
        return staffCtrl.getAllStaff(req);
    });

    CROW_ROUTE(app, "/api/staff/<int>").methods("GET"_method)
    ([&staffCtrl](const crow::request& req, int id) {
        return staffCtrl.getStaffById(req, id);
    });

    CROW_ROUTE(app, "/api/staff/<int>").methods("PUT"_method)
    ([&staffCtrl](const crow::request& req, int id) {
        return staffCtrl.updateStaff(req, id);
    });

    CROW_ROUTE(app, "/api/staff/<int>").methods("DELETE"_method)
    ([&staffCtrl](const crow::request& req, int id) {
        return staffCtrl.deleteStaff(req, id);
    });

    // ── Leave routes ──────────────────────────
    CROW_ROUTE(app, "/api/leave").methods("GET"_method)
    ([&leaveCtrl](const crow::request& req) {
        return leaveCtrl.getAllLeave(req);
    });

    CROW_ROUTE(app, "/api/leave/my").methods("GET"_method)
    ([&leaveCtrl](const crow::request& req) {
        return leaveCtrl.getMyLeave(req);
    });

    CROW_ROUTE(app, "/api/leave").methods("POST"_method)
    ([&leaveCtrl](const crow::request& req) {
        return leaveCtrl.submitLeave(req);
    });

    CROW_ROUTE(app, "/api/leave/<int>/approve").methods("PUT"_method)
    ([&leaveCtrl](const crow::request& req, int id) {
        return leaveCtrl.approveLeave(req, id);
    });

    CROW_ROUTE(app, "/api/leave/<int>/reject").methods("PUT"_method)
    ([&leaveCtrl](const crow::request& req, int id) {
        return leaveCtrl.rejectLeave(req, id);
    });

    // ── Schedule routes ───────────────────────
    CROW_ROUTE(app, "/api/schedule").methods("GET"_method)
    ([&schedCtrl](const crow::request& req) {
        return schedCtrl.getAllShifts(req);
    });

    CROW_ROUTE(app, "/api/schedule/my").methods("GET"_method)
    ([&schedCtrl](const crow::request& req) {
        return schedCtrl.getMyShifts(req);
    });

    CROW_ROUTE(app, "/api/schedule").methods("POST"_method)
    ([&schedCtrl](const crow::request& req) {
        return schedCtrl.createShift(req);
    });

    CROW_ROUTE(app, "/api/schedule/<int>").methods("PUT"_method)
    ([&schedCtrl](const crow::request& req, int id) {
        return schedCtrl.updateShift(req, id);
    });

    CROW_ROUTE(app, "/api/schedule/<int>").methods("DELETE"_method)
    ([&schedCtrl](const crow::request& req, int id) {
        return schedCtrl.deleteShift(req, id);
    });

    // ── Dashboard routes ──────────────────────
    CROW_ROUTE(app, "/api/dashboard/stats").methods("GET"_method)
    ([&dashCtrl](const crow::request& req) {
        return dashCtrl.getManagerStats(req);
    });

    CROW_ROUTE(app, "/api/dashboard/my").methods("GET"_method)
    ([&dashCtrl](const crow::request& req) {
        return dashCtrl.getEmployeeStats(req);
    });

    // Health-check route (Render pings this to confirm the server is alive)
    CROW_ROUTE(app, "/health")
    ([]() {
        return crow::response(200, "{\"status\":\"ok\"}");
    });

    // ── Start the server ──────────────────────
    app.port(port)
       .multithreaded()
       .run();

    return 0;
}
