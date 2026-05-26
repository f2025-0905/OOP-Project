#include "../include/controllers/AuthController.h"
#include "../include/middleware/AuthMiddleware.h"
#include "../include/models/User.h"
#include <crow.h>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
using namespace std;
using json = nlohmann::json;

// ─────────────────────────────────────────────
//  StaffController  —  handles /api/staff routes
//  All routes require a manager role
//
//  GET    /api/staff          — list all employees
//  GET    /api/staff/:id      — get one employee
//  POST   /api/staff          — add a new employee
//  PUT    /api/staff/:id      — update employee info
//  DELETE /api/staff/:id      — remove an employee
// ─────────────────────────────────────────────
class StaffController {
private:
    string         dbConnection;
    AuthMiddleware authMiddleware;

    // Check that the request comes from a manager.
    // Returns true and sets userId; on failure writes the response directly.
    bool checkManagerAccess (const crow::request& req, crow::response& res) {
        string token = authMiddleware.extractToken(req);
        if (!authMiddleware.isManager(token)) {
            res = crow::response(403, "{\"error\":\"Manager access required\"}");
            return false;
        }
        return true;
    }

public:
    // Constructor
    StaffController (string connStr, string jwtSecret)
        : authMiddleware(jwtSecret) {
        dbConnection = connStr;
    }

    // ── GET /api/staff ────────────────────────
    // Returns a JSON array of all employees
    crow::response getAllStaff (const crow::request& req) {
        crow::response res;
        if (!checkManagerAccess(req, res)) return res;

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto rows = txn.exec(
                "SELECT id, name, email, role FROM users ORDER BY name"
            );

            json staffList = json::array();
            for (auto& row : rows) {
                json emp;
                emp["id"]    = row[0].as<int>();
                emp["name"]  = row[1].as<string>();
                emp["email"] = row[2].as<string>();
                emp["role"]  = row[3].as<string>();
                staffList.push_back(emp);
            }

            return crow::response(200, staffList.dump());

        } catch (exception& e) {
            cerr << "getAllStaff error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── GET /api/staff/:id ────────────────────
    // Returns one employee by id
    crow::response getStaffById (const crow::request& req, int id) {
        crow::response res;
        if (!checkManagerAccess(req, res)) return res;

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto result = txn.exec_params(
                "SELECT id, name, email, role FROM users WHERE id = $1",
                id
            );

            if (result.empty()) {
                return crow::response(404, "{\"error\":\"Employee not found\"}");
            }

            json emp;
            emp["id"]    = result[0][0].as<int>();
            emp["name"]  = result[0][1].as<string>();
            emp["email"] = result[0][2].as<string>();
            emp["role"]  = result[0][3].as<string>();

            return crow::response(200, emp.dump());

        } catch (exception& e) {
            cerr << "getStaffById error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── PUT /api/staff/:id ────────────────────
    // Update name, email, or role of an employee
    crow::response updateStaff (const crow::request& req, int id) {
        crow::response res;
        if (!checkManagerAccess(req, res)) return res;

        try {
            json body = json::parse(req.body);

            string name  = body.value("name",  "");
            string email = body.value("email", "");
            string role  = body.value("role",  "");

            if (name.empty() || email.empty()) {
                return crow::response(400, "{\"error\":\"Name and email are required\"}");
            }

            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto result = txn.exec_params(
                "UPDATE users SET name=$1, email=$2, role=$3 "
                "WHERE id=$4 RETURNING id",
                name, email, role, id
            );

            if (result.empty()) {
                return crow::response(404, "{\"error\":\"Employee not found\"}");
            }

            txn.commit();
            return crow::response(200, "{\"message\":\"Employee updated successfully\"}");

        } catch (exception& e) {
            cerr << "updateStaff error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }

    // ── DELETE /api/staff/:id ─────────────────
    // Remove an employee from the database
    crow::response deleteStaff (const crow::request& req, int id) {
        crow::response res;
        if (!checkManagerAccess(req, res)) return res;

        try {
            pqxx::connection conn(dbConnection);
            pqxx::work txn(conn);

            auto result = txn.exec_params(
                "DELETE FROM users WHERE id = $1 RETURNING id",
                id
            );

            if (result.empty()) {
                return crow::response(404, "{\"error\":\"Employee not found\"}");
            }

            txn.commit();
            return crow::response(200, "{\"message\":\"Employee removed successfully\"}");

        } catch (exception& e) {
            cerr << "deleteStaff error: " << e.what() << endl;
            return crow::response(500, "{\"error\":\"Internal server error\"}");
        }
    }
};
