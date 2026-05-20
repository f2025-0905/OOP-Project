#include "controllers/AuthController.h"
#include "middleware/AuthMiddleware.h"
#include <bcrypt/BCrypt.hpp>
#include <pqxx/pqxx>
#include <iostream>

crow::response AuthController::login(const crow::request& req) {
    try {
        auto body = json::parse(req.body);

        if (!body.contains("username") || !body.contains("password")) {
            return crow::response(400, R"({"error":"Username and password required"})");
        }

        std::string username = body["username"].get<std::string>();
        std::string password = body["password"].get<std::string>();

        pqxx::work txn(db);
        auto result = txn.exec_params(
            "SELECT id, username, email, first_name, last_name, role, password_hash, is_active "
            "FROM users WHERE username = $1 OR email = $1",
            username
        );

        if (result.empty()) {
            return crow::response(401, R"({"error":"Invalid credentials"})");
        }

        auto row = result[0];
        std::string storedHash = row["password_hash"].as<std::string>();
        bool isActive = row["is_active"].as<bool>();

        if (!isActive) {
            return crow::response(403, R"({"error":"Account is deactivated"})");
        }

        if (!BCrypt::validatePassword(password, storedHash)) {
            return crow::response(401, R"({"error":"Invalid credentials"})");
        }

        std::string userId = row["id"].as<std::string>();
        std::string role   = row["role"].as<std::string>();
        std::string token  = AuthMiddleware::generateJWT(userId, role);

        json response = {
            {"token", token},
            {"user", {
                {"id",         userId},
                {"username",   row["username"].as<std::string>()},
                {"email",      row["email"].as<std::string>()},
                {"first_name", row["first_name"].as<std::string>()},
                {"last_name",  row["last_name"].as<std::string>()},
                {"role",       role}
            }}
        };

        txn.commit();
        auto res = crow::response(200, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    } catch (const std::exception& e) {
        std::cerr << "Login error: " << e.what() << std::endl;
        return crow::response(500, R"({"error":"Internal server error"})");
    }
}

crow::response AuthController::signup(const crow::request& req) {
    try {
        auto body = json::parse(req.body);

        std::vector<std::string> required = {"username","email","password","first_name","last_name"};
        for (auto& field : required) {
            if (!body.contains(field) || body[field].get<std::string>().empty()) {
                return crow::response(400, "{\"error\":\"" + field + " is required\"}");
            }
        }

        std::string username   = body["username"].get<std::string>();
        std::string email      = body["email"].get<std::string>();
        std::string password   = body["password"].get<std::string>();
        std::string firstName  = body["first_name"].get<std::string>();
        std::string lastName   = body["last_name"].get<std::string>();

        if (password.length() < 8) {
            return crow::response(400, R"({"error":"Password must be at least 8 characters"})");
        }

        std::string passwordHash = BCrypt::generateHash(password);

        pqxx::work txn(db);

        // Check duplicate
        auto dupCheck = txn.exec_params(
            "SELECT id FROM users WHERE username=$1 OR email=$2",
            username, email
        );
        if (!dupCheck.empty()) {
            return crow::response(409, R"({"error":"Username or email already exists"})");
        }

        // Insert manager
        auto result = txn.exec_params(
            "INSERT INTO users (username, email, password_hash, first_name, last_name, role) "
            "VALUES ($1, $2, $3, $4, $5, 'manager') RETURNING id",
            username, email, passwordHash, firstName, lastName
        );

        std::string userId = result[0]["id"].as<std::string>();
        std::string token  = AuthMiddleware::generateJWT(userId, "manager");

        txn.commit();

        json response = {
            {"token", token},
            {"user", {
                {"id",         userId},
                {"username",   username},
                {"email",      email},
                {"first_name", firstName},
                {"last_name",  lastName},
                {"role",       "manager"}
            }}
        };

        auto res = crow::response(201, response.dump());
        res.set_header("Content-Type", "application/json");
        return res;

    } catch (const std::exception& e) {
        std::cerr << "Signup error: " << e.what() << std::endl;
        return crow::response(500, R"({"error":"Internal server error"})");
    }
}

crow::response AuthController::getMe(const crow::request& req) {
    return AuthMiddleware::requireAuth(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            auto result = txn.exec_params(
                "SELECT u.id, u.username, u.email, u.first_name, u.last_name, u.role, u.created_at, "
                "ep.position_id, ep.leave_balance, p.name as position_name "
                "FROM users u "
                "LEFT JOIN employee_profiles ep ON u.id = ep.user_id "
                "LEFT JOIN positions p ON ep.position_id = p.id "
                "WHERE u.id = $1",
                ctx.userId
            );

            if (result.empty()) {
                return crow::response(404, R"({"error":"User not found"})");
            }

            auto row = result[0];
            json user = {
                {"id",           row["id"].as<std::string>()},
                {"username",     row["username"].as<std::string>()},
                {"email",        row["email"].as<std::string>()},
                {"first_name",   row["first_name"].as<std::string>()},
                {"last_name",    row["last_name"].as<std::string>()},
                {"role",         row["role"].as<std::string>()},
                {"created_at",   row["created_at"].as<std::string>()}
            };

            if (!row["position_id"].is_null()) {
                user["position_id"]   = row["position_id"].as<std::string>();
                user["position_name"] = row["position_name"].as<std::string>();
                user["leave_balance"] = row["leave_balance"].as<int>();
            }

            txn.commit();
            auto res = crow::response(200, user.dump());
            res.set_header("Content-Type", "application/json");
            return res;

        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response AuthController::changePassword(const crow::request& req) {
    return AuthMiddleware::requireAuth(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);
            std::string oldPass = body["old_password"].get<std::string>();
            std::string newPass = body["new_password"].get<std::string>();

            if (newPass.length() < 8) {
                return crow::response(400, R"({"error":"New password must be at least 8 characters"})");
            }

            pqxx::work txn(db);
            auto result = txn.exec_params(
                "SELECT password_hash FROM users WHERE id=$1", ctx.userId
            );

            if (!BCrypt::validatePassword(oldPass, result[0]["password_hash"].as<std::string>())) {
                return crow::response(401, R"({"error":"Incorrect current password"})");
            }

            std::string newHash = BCrypt::generateHash(newPass);
            txn.exec_params("UPDATE users SET password_hash=$1 WHERE id=$2", newHash, ctx.userId);
            txn.commit();

            return crow::response(200, R"({"message":"Password updated successfully"})");

        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}
