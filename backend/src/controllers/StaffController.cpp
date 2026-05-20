#include "controllers/Controllers.h"
#include "middleware/AuthMiddleware.h"
#include <bcrypt/BCrypt.hpp>
#include <pqxx/pqxx>
#include <iostream>

// ============================================
// EMPLOYEE ENDPOINTS
// ============================================

crow::response StaffController::getAllEmployees(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            auto result = txn.exec(
                "SELECT u.id, u.username, u.email, u.first_name, u.last_name, "
                "u.is_active, u.created_at, "
                "ep.leave_balance, ep.phone, ep.hire_date, "
                "p.id as position_id, p.name as position_name, p.color as position_color "
                "FROM users u "
                "LEFT JOIN employee_profiles ep ON u.id = ep.user_id "
                "LEFT JOIN positions p ON ep.position_id = p.id "
                "WHERE u.role = 'employee' "
                "ORDER BY u.first_name ASC"
            );

            json employees = json::array();
            for (auto& row : result) {
                json emp = {
                    {"id",           row["id"].as<std::string>()},
                    {"username",     row["username"].as<std::string>()},
                    {"email",        row["email"].as<std::string>()},
                    {"first_name",   row["first_name"].as<std::string>()},
                    {"last_name",    row["last_name"].as<std::string>()},
                    {"is_active",    row["is_active"].as<bool>()},
                    {"created_at",   row["created_at"].as<std::string>()},
                    {"leave_balance",row["leave_balance"].is_null() ? 20 : row["leave_balance"].as<int>()},
                    {"phone",        row["phone"].is_null() ? "" : row["phone"].as<std::string>()},
                    {"hire_date",    row["hire_date"].is_null() ? "" : row["hire_date"].as<std::string>()}
                };
                if (!row["position_id"].is_null()) {
                    emp["position"] = {
                        {"id",    row["position_id"].as<std::string>()},
                        {"name",  row["position_name"].as<std::string>()},
                        {"color", row["position_color"].as<std::string>()}
                    };
                } else {
                    emp["position"] = nullptr;
                }
                employees.push_back(emp);
            }

            txn.commit();
            auto res = crow::response(200, employees.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response StaffController::createEmployee(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);

            std::vector<std::string> required = {"username","email","password","first_name","last_name"};
            for (auto& f : required) {
                if (!body.contains(f) || body[f].get<std::string>().empty()) {
                    return crow::response(400, "{\"error\":\"" + f + " is required\"}");
                }
            }

            std::string username  = body["username"].get<std::string>();
            std::string email     = body["email"].get<std::string>();
            std::string password  = body["password"].get<std::string>();
            std::string firstName = body["first_name"].get<std::string>();
            std::string lastName  = body["last_name"].get<std::string>();
            std::string phone     = body.value("phone", "");

            std::string hash = BCrypt::generateHash(password);

            pqxx::work txn(db);

            auto dup = txn.exec_params(
                "SELECT id FROM users WHERE username=$1 OR email=$2", username, email
            );
            if (!dup.empty()) {
                return crow::response(409, R"({"error":"Username or email already exists"})");
            }

            auto userRes = txn.exec_params(
                "INSERT INTO users (username, email, password_hash, first_name, last_name, role) "
                "VALUES ($1,$2,$3,$4,$5,'employee') RETURNING id",
                username, email, hash, firstName, lastName
            );
            std::string userId = userRes[0]["id"].as<std::string>();

            // Create employee profile
            txn.exec_params(
                "INSERT INTO employee_profiles (user_id, phone) VALUES ($1, $2)",
                userId, phone.empty() ? nullptr : &phone
            );

            // Assign position if provided
            if (body.contains("position_id") && !body["position_id"].is_null()) {
                std::string posId = body["position_id"].get<std::string>();
                txn.exec_params(
                    "UPDATE employee_profiles SET position_id=$1 WHERE user_id=$2",
                    posId, userId
                );
            }

            txn.commit();

            json response = {
                {"id",         userId},
                {"username",   username},
                {"email",      email},
                {"first_name", firstName},
                {"last_name",  lastName},
                {"role",       "employee"},
                {"message",    "Employee created successfully"}
            };

            auto res = crow::response(201, response.dump());
            res.set_header("Content-Type", "application/json");
            return res;

        } catch (const std::exception& e) {
            std::cerr << "Create employee error: " << e.what() << std::endl;
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response StaffController::updateEmployee(const crow::request& req, std::string id) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);
            pqxx::work txn(db);

            if (body.contains("first_name") || body.contains("last_name") || body.contains("email")) {
                txn.exec_params(
                    "UPDATE users SET "
                    "first_name = COALESCE($1, first_name), "
                    "last_name  = COALESCE($2, last_name), "
                    "email      = COALESCE($3, email) "
                    "WHERE id = $4",
                    body.value("first_name",""), body.value("last_name",""),
                    body.value("email",""), id
                );
            }

            if (body.contains("phone") || body.contains("leave_balance")) {
                txn.exec_params(
                    "UPDATE employee_profiles SET "
                    "phone         = COALESCE(NULLIF($1,''), phone), "
                    "leave_balance = COALESCE($2, leave_balance) "
                    "WHERE user_id = $3",
                    body.value("phone",""),
                    body.contains("leave_balance") ? std::to_string(body["leave_balance"].get<int>()) : "",
                    id
                );
            }

            txn.commit();
            return crow::response(200, R"({"message":"Employee updated successfully"})");
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response StaffController::deleteEmployee(const crow::request& req, std::string id) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            // Soft delete
            txn.exec_params("UPDATE users SET is_active=false WHERE id=$1 AND role='employee'", id);
            txn.commit();
            return crow::response(200, R"({"message":"Employee deactivated"})");
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response StaffController::assignPosition(const crow::request& req, std::string id) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);
            std::string positionId = body["position_id"].get<std::string>();

            pqxx::work txn(db);
            txn.exec_params(
                "UPDATE employee_profiles SET position_id=$1 WHERE user_id=$2",
                positionId, id
            );
            txn.commit();
            return crow::response(200, R"({"message":"Position assigned"})");
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

// ============================================
// POSITION ENDPOINTS
// ============================================

crow::response StaffController::getAllPositions(const crow::request& req) {
    return AuthMiddleware::requireAuth(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            auto result = txn.exec(
                "SELECT p.id, p.name, p.description, p.color, p.created_at, "
                "COUNT(ep.user_id) as employee_count "
                "FROM positions p "
                "LEFT JOIN employee_profiles ep ON p.id = ep.position_id "
                "GROUP BY p.id ORDER BY p.name ASC"
            );

            json positions = json::array();
            for (auto& row : result) {
                positions.push_back({
                    {"id",             row["id"].as<std::string>()},
                    {"name",           row["name"].as<std::string>()},
                    {"description",    row["description"].is_null() ? "" : row["description"].as<std::string>()},
                    {"color",          row["color"].as<std::string>()},
                    {"employee_count", row["employee_count"].as<int>()},
                    {"created_at",     row["created_at"].as<std::string>()}
                });
            }

            txn.commit();
            auto res = crow::response(200, positions.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response StaffController::createPosition(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);
            std::string name  = body["name"].get<std::string>();
            std::string desc  = body.value("description", "");
            std::string color = body.value("color", "#3B82F6");

            pqxx::work txn(db);
            auto result = txn.exec_params(
                "INSERT INTO positions (name, description, color, created_by) "
                "VALUES ($1,$2,$3,$4) RETURNING id",
                name, desc, color, ctx.userId
            );

            json response = {
                {"id",    result[0]["id"].as<std::string>()},
                {"name",  name},
                {"color", color}
            };

            txn.commit();
            auto res = crow::response(201, response.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response StaffController::deletePosition(const crow::request& req, std::string id) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            txn.exec_params("DELETE FROM positions WHERE id=$1", id);
            txn.commit();
            return crow::response(200, R"({"message":"Position deleted"})");
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}
