#include "../include/middleware/AuthMiddleware.h"
#include <crow.h>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <iostream>
using namespace std;
using json = nlohmann::json;

// ─────────────────────────────────────────────
//  DashboardController  —  /api/dashboard routes
//
//  GET /api/dashboard/stats   — manager summary numbers
//  GET /api/dashboard/my      — employee's own summary
// ─────────────────────────────────────────────
class DashboardController {
private:
    string         dbConnection;
    AuthMiddleware authMiddleware;

    // Read userId from JWT
    int getUserIdFromToken (const crow::request& req) {
        string token = authMiddleware.extractToken(req);
        if (token.empty()) return -1;
        try {
            auto decoded = jwt::decode(token);
            return stoi(decoded.get_payload_claim("userId").as_string());
        } catch (...) {
            return -1;
        }
    }

public:
    // Constructor
    DashboardController (string connStr, string jwtSecret)
        : authMiddleware(jwtSecret) {
        dbConnection = connStr;
    }

    // ── GET /api/dashboard/stats ──────────────
    // Returns high-level numbers for the manager:
    //   total staff, shifts today, pending leave requests
    crow::response getManagerStats (const crow::request& req) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            return crow::response(403, "{\"error\":\"Manager access required\"}");
        }

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            // Count total employees
            auto staffCount = txn.exec(
                "SELECT COUNT(*) FROM users WHERE role = 'employee'"
            );

            // Count shifts scheduled for today
            auto todayShifts = txn.exec(
                "SELECT COUNT(*) FROM shifts WHERE date = CURRENT_DATE"
            );

            // Count leave requests still waiting for a decision
            auto pendingLeave = txn.exec(
                "SELECT COUNT(*) FROM leave_requests WHERE status = 'pending'"
            );

            json stats;
            stats["totalStaff"]     = staffCount[0][0].as<int>();
            stats["shiftsToday"]    = todayShifts[0][0].as<int>();
            stats["pendingLeave"]   = pendingLeave[0][0].as<int>();

            return crow::response(200, stats.dump());

        } catch (exception& e) {
            cerr << "getManagerStats error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── GET /api/dashboard/my ─────────────────
    // Returns the logged-in employee's own summary:
    //   upcoming shifts count, approved leave days
    crow::response getEmployeeStats (const crow::request& req) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isAuthenticated(token)) {
            return crow::response(401, "{\"error\":\"Please log in first\"}");
        }

        int userId = getUserIdFromToken(req);
        if (userId < 0) {
            return crow::response(401, "{\"error\":\"Invalid token\"}");
        }

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            // Upcoming shifts for this employee
            auto upcomingShifts = txn.exec_params(
                "SELECT COUNT(*) FROM shifts "
                "WHERE user_id = $1 AND date >= CURRENT_DATE AND status = 'scheduled'",
                userId
            );

            // Approved leave requests
            auto approvedLeave = txn.exec_params(
                "SELECT COUNT(*) FROM leave_requests "
                "WHERE user_id = $1 AND status = 'approved'",
                userId
            );

            // Pending leave requests
            auto pendingLeave = txn.exec_params(
                "SELECT COUNT(*) FROM leave_requests "
                "WHERE user_id = $1 AND status = 'pending'",
                userId
            );

            json stats;
            stats["upcomingShifts"] = upcomingShifts[0][0].as<int>();
            stats["approvedLeave"]  = approvedLeave[0][0].as<int>();
            stats["pendingLeave"]   = pendingLeave[0][0].as<int>();

            return crow::response(200, stats.dump());

        } catch (exception& e) {
            cerr << "getEmployeeStats error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }
};
