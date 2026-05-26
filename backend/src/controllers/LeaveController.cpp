#include "../include/middleware/AuthMiddleware.h"
#include <crow.h>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <iostream>
using namespace std;
using json = nlohmann::json;

// ─────────────────────────────────────────────
//  LeaveController  —  handles /api/leave routes
//
//  GET  /api/leave             — all leave requests (manager)
//  GET  /api/leave/my          — own leave requests (employee)
//  POST /api/leave             — submit a new leave request
//  PUT  /api/leave/:id/approve — approve a request (manager)
//  PUT  /api/leave/:id/reject  — reject  a request (manager)
// ─────────────────────────────────────────────
class LeaveController {
private:
    string         dbConnection;
    AuthMiddleware authMiddleware;

    // Get the user id stored inside the JWT
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
    LeaveController (string connStr, string jwtSecret)
        : authMiddleware(jwtSecret) {
        dbConnection = connStr;
    }

    // ── GET /api/leave ────────────────────────
    // Manager sees all leave requests
    crow::response getAllLeave (const crow::request& req) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            return crow::response(403, "{\"error\":\"Manager access required\"}");
        }

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto rows = txn.exec(
                "SELECT lr.id, u.name, lr.start_date, lr.end_date, "
                "       lr.reason, lr.status "
                "FROM leave_requests lr "
                "JOIN users u ON lr.user_id = u.id "
                "ORDER BY lr.created_at DESC"
            );

            json list = json::array();
            for (auto& row : rows) {
                json req_obj;
                req_obj["id"]        = row[0].as<int>();
                req_obj["staffName"] = row[1].as<string>();
                req_obj["startDate"] = row[2].as<string>();
                req_obj["endDate"]   = row[3].as<string>();
                req_obj["reason"]    = row[4].as<string>();
                req_obj["status"]    = row[5].as<string>();
                list.push_back(req_obj);
            }

            return crow::response(200, list.dump());

        } catch (exception& e) {
            cerr << "getAllLeave error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── GET /api/leave/my ─────────────────────
    // Employee sees only their own requests
    crow::response getMyLeave (const crow::request& req) {
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

            auto rows = txn.exec_params(
                "SELECT id, start_date, end_date, reason, status "
                "FROM leave_requests WHERE user_id = $1 "
                "ORDER BY created_at DESC",
                userId
            );

            json list = json::array();
            for (auto& row : rows) {
                json entry;
                entry["id"]        = row[0].as<int>();
                entry["startDate"] = row[1].as<string>();
                entry["endDate"]   = row[2].as<string>();
                entry["reason"]    = row[3].as<string>();
                entry["status"]    = row[4].as<string>();
                list.push_back(entry);
            }

            return crow::response(200, list.dump());

        } catch (exception& e) {
            cerr << "getMyLeave error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── POST /api/leave ───────────────────────
    // Employee submits a new leave request
    // Body: { startDate, endDate, reason }
    crow::response submitLeave (const crow::request& req) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isAuthenticated(token)) {
            return crow::response(401, "{\"error\":\"Please log in first\"}");
        }

        int userId = getUserIdFromToken(req);
        if (userId < 0) {
            return crow::response(401, "{\"error\":\"Invalid token\"}");
        }

        try {
            json body = json::parse(req.body);

            string startDate = body.value("startDate", "");
            string endDate   = body.value("endDate",   "");
            string reason    = body.value("reason",    "");

            if (startDate.empty() || endDate.empty()) {
                return crow::response(400, "{\"error\":\"Start date and end date are required\"}");
            }

            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            txn.exec_params(
                "INSERT INTO leave_requests (user_id, start_date, end_date, reason, status) "
                "VALUES ($1, $2, $3, $4, 'pending')",
                userId, startDate, endDate, reason
            );

            txn.commit();
            return crow::response(201, "{\"message\":\"Leave request submitted successfully\"}");

        } catch (exception& e) {
            cerr << "submitLeave error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── PUT /api/leave/:id/approve ────────────
    crow::response approveLeave (const crow::request& req, int leaveId) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            return crow::response(403, "{\"error\":\"Manager access required\"}");
        }

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto result = txn.exec_params(
                "UPDATE leave_requests SET status = 'approved' "
                "WHERE id = $1 RETURNING id",
                leaveId
            );

            if (result.empty()) {
                return crow::response(404, "{\"error\":\"Leave request not found\"}");
            }

            txn.commit();
            return crow::response(200, "{\"message\":\"Leave request approved\"}");

        } catch (exception& e) {
            cerr << "approveLeave error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── PUT /api/leave/:id/reject ─────────────
    crow::response rejectLeave (const crow::request& req, int leaveId) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            return crow::response(403, "{\"error\":\"Manager access required\"}");
        }

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto result = txn.exec_params(
                "UPDATE leave_requests SET status = 'rejected' "
                "WHERE id = $1 RETURNING id",
                leaveId
            );

            if (result.empty()) {
                return crow::response(404, "{\"error\":\"Leave request not found\"}");
            }

            txn.commit();
            return crow::response(200, "{\"message\":\"Leave request rejected\"}");

        } catch (exception& e) {
            cerr << "rejectLeave error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }
};
