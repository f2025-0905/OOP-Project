#include "../include/middleware/AuthMiddleware.h"
#include "../include/models/Shift.h"
#include <crow.h>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <iostream>
using namespace std;
using json = nlohmann::json;

// ─────────────────────────────────────────────
//  ScheduleController  —  /api/schedule routes
//
//  GET    /api/schedule          — all shifts (manager)
//  GET    /api/schedule/my       — employee's own shifts
//  POST   /api/schedule          — create a shift (manager)
//  PUT    /api/schedule/:id      — update a shift (manager)
//  DELETE /api/schedule/:id      — delete a shift (manager)
// ─────────────────────────────────────────────
class ScheduleController {
private:
    string         dbConnection;
    AuthMiddleware authMiddleware;

    // Read userId from JWT payload
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
    ScheduleController (string connStr, string jwtSecret)
        : authMiddleware(jwtSecret) {
        dbConnection = connStr;
    }

    // ── GET /api/schedule ─────────────────────
    // Manager sees the full schedule
    crow::response getAllShifts (const crow::request& req) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            return crow::response(403, "{\"error\":\"Manager access required\"}");
        }

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto rows = txn.exec(
                "SELECT s.id, u.name, s.date, s.start_time, s.end_time, s.status "
                "FROM shifts s "
                "JOIN users u ON s.user_id = u.id "
                "ORDER BY s.date, s.start_time"
            );

            json list = json::array();
            for (auto& row : rows) {
                json shift;
                shift["id"]        = row[0].as<int>();
                shift["staffName"] = row[1].as<string>();
                shift["date"]      = row[2].as<string>();
                shift["startTime"] = row[3].as<string>();
                shift["endTime"]   = row[4].as<string>();
                shift["status"]    = row[5].as<string>();
                list.push_back(shift);
            }

            return crow::response(200, list.dump());

        } catch (exception& e) {
            cerr << "getAllShifts error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── GET /api/schedule/my ──────────────────
    // Employee sees only their own upcoming shifts
    crow::response getMyShifts (const crow::request& req) {
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
                "SELECT id, date, start_time, end_time, status "
                "FROM shifts WHERE user_id = $1 "
                "ORDER BY date, start_time",
                userId
            );

            json list = json::array();
            for (auto& row : rows) {
                json shift;
                shift["id"]        = row[0].as<int>();
                shift["date"]      = row[1].as<string>();
                shift["startTime"] = row[2].as<string>();
                shift["endTime"]   = row[3].as<string>();
                shift["status"]    = row[4].as<string>();
                list.push_back(shift);
            }

            return crow::response(200, list.dump());

        } catch (exception& e) {
            cerr << "getMyShifts error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── POST /api/schedule ────────────────────
    // Manager creates a new shift
    // Body: { userId, date, startTime, endTime }
    crow::response createShift (const crow::request& req) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            return crow::response(403, "{\"error\":\"Manager access required\"}");
        }

        try {
            json body = json::parse(req.body);

            int    userId    = body.value("userId",    0);
            string date      = body.value("date",      "");
            string startTime = body.value("startTime", "");
            string endTime   = body.value("endTime",   "");

            if (userId == 0 || date.empty() || startTime.empty() || endTime.empty()) {
                return crow::response(400, "{\"error\":\"userId, date, startTime and endTime are required\"}");
            }

            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            txn.exec_params(
                "INSERT INTO shifts (user_id, date, start_time, end_time, status) "
                "VALUES ($1, $2, $3, $4, 'scheduled')",
                userId, date, startTime, endTime
            );

            txn.commit();
            return crow::response(201, "{\"message\":\"Shift created successfully\"}");

        } catch (exception& e) {
            cerr << "createShift error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── PUT /api/schedule/:id ─────────────────
    // Manager updates an existing shift
    crow::response updateShift (const crow::request& req, int shiftId) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            return crow::response(403, "{\"error\":\"Manager access required\"}");
        }

        try {
            json body = json::parse(req.body);

            string date      = body.value("date",      "");
            string startTime = body.value("startTime", "");
            string endTime   = body.value("endTime",   "");
            string status    = body.value("status",    "scheduled");

            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto result = txn.exec_params(
                "UPDATE shifts SET date=$1, start_time=$2, end_time=$3, status=$4 "
                "WHERE id=$5 RETURNING id",
                date, startTime, endTime, status, shiftId
            );

            if (result.empty()) {
                return crow::response(404, "{\"error\":\"Shift not found\"}");
            }

            txn.commit();
            return crow::response(200, "{\"message\":\"Shift updated successfully\"}");

        } catch (exception& e) {
            cerr << "updateShift error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── DELETE /api/schedule/:id ──────────────
    // Manager removes a shift
    crow::response deleteShift (const crow::request& req, int shiftId) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            return crow::response(403, "{\"error\":\"Manager access required\"}");
        }

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto result = txn.exec_params(
                "DELETE FROM shifts WHERE id = $1 RETURNING id",
                shiftId
            );

            if (result.empty()) {
                return crow::response(404, "{\"error\":\"Shift not found\"}");
            }

            txn.commit();
            return crow::response(200, "{\"message\":\"Shift deleted successfully\"}");

        } catch (exception& e) {
            cerr << "deleteShift error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }
};
