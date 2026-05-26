#include "../include/controllers/AuthController.h"
#include "../include/models/User.h"
#include <crow.h>
#include <pqxx/pqxx>
#include <jwt-cpp/jwt.h>
#include <bcrypt/BCrypt.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
using namespace std;
using json = nlohmann::json;

// ─────────────────────────────────────────────
//  Private helpers
// ─────────────────────────────────────────────

string AuthController::hashPassword (string plainText) {
    // BCrypt automatically adds a salt — safe for storing in the DB
    return BCrypt::generateHash(plainText);
}

bool AuthController::checkPassword (string plainText, string hash) {
    return BCrypt::validatePassword(plainText, hash);
}

string AuthController::createToken (int userId, string role) {
    // Token expires in 24 hours
    auto token = jwt::create()
                     .set_issuer("shiftwise")
                     .set_type("JWT")
                     .set_payload_claim("userId", jwt::claim(to_string(userId)))
                     .set_payload_claim("role",   jwt::claim(role))
                     .set_expires_at(chrono::system_clock::now() + chrono::hours{24})
                     .sign(jwt::algorithm::hs256{jwtSecret});
    return token;
}

// ─────────────────────────────────────────────
//  POST /api/auth/signup
// ─────────────────────────────────────────────
crow::response AuthController::signup (const crow::request& req) {
    try {
        // Parse request body
        json body = json::parse(req.body);

        string name     = body.value("name",     "");
        string email    = body.value("email",    "");
        string password = body.value("password", "");
        string role     = body.value("role",     "employee");

        // Basic validation
        if (name.empty() || email.empty() || password.empty()) {
            return crow::response(400, "{\"error\":\"Name, email and password are required\"}");
        }

        // Connect to database
        pqxx::connection conn(dbConnection);
        pqxx::work txn(conn);

        // Check if email already exists
        auto existing = txn.exec_params(
            "SELECT id FROM users WHERE email = $1",
            email
        );

        if (!existing.empty()) {
            return crow::response(409, "{\"error\":\"Email already registered\"}");
        }

        // Hash the password before storing
        string hashedPwd = hashPassword(password);

        // Insert new user into the database
        auto result = txn.exec_params(
            "INSERT INTO users (name, email, password, role) "
            "VALUES ($1, $2, $3, $4) RETURNING id",
            name, email, hashedPwd, role
        );

        txn.commit();

        int newId = result[0][0].as<int>();

        // Build User object and generate token
        User newUser(newId, name, email, hashedPwd, role);
        string token = createToken(newId, role);

        // Return success response with token
        json response;
        response["message"] = "Account created successfully";
        response["token"]   = token;
        response["user"]    = json::parse(newUser.toJson());

        return crow::response(201, response.dump());

    } catch (exception& e) {
        cerr << "Signup error: " << e.what() << endl;
        return crow::response(500, "{\"error\":\"Internal server error\"}");
    }
}

// ─────────────────────────────────────────────
//  POST /api/auth/login
// ─────────────────────────────────────────────
crow::response AuthController::login (const crow::request& req) {
    try {
        // Parse request body
        json body = json::parse(req.body);

        string email    = body.value("email",    "");
        string password = body.value("password", "");

        if (email.empty() || password.empty()) {
            return crow::response(400, "{\"error\":\"Email and password are required\"}");
        }

        // Look up the user in the database
        pqxx::connection conn(dbConnection);
        pqxx::work txn(conn);

        auto result = txn.exec_params(
            "SELECT id, name, email, password, role FROM users WHERE email = $1",
            email
        );

        if (result.empty()) {
            return crow::response(401, "{\"error\":\"Invalid email or password\"}");
        }

        // Build User object from the DB row
        int    userId   = result[0][0].as<int>();
        string userName = result[0][1].as<string>();
        string userHash = result[0][3].as<string>();
        string userRole = result[0][4].as<string>();

        // Verify the password
        if (!checkPassword(password, userHash)) {
            return crow::response(401, "{\"error\":\"Invalid email or password\"}");
        }

        // Create JWT token
        string token = createToken(userId, userRole);

        User loggedIn(userId, userName, email, userHash, userRole);

        json response;
        response["message"] = "Login successful";
        response["token"]   = token;
        response["user"]    = json::parse(loggedIn.toJson());

        return crow::response(200, response.dump());

    } catch (exception& e) {
        cerr << "Login error: " << e.what() << endl;
        return crow::response(500, "{\"error\":\"Internal server error\"}");
    }
}
