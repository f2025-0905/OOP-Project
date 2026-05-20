#pragma once
#include <crow.h>
#include <pqxx/pqxx>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class AuthController {
private:
    pqxx::connection& db;

    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    std::string generateToken(const std::string& userId, const std::string& role);

public:
    explicit AuthController(pqxx::connection& conn) : db(conn) {}

    // POST /api/auth/login
    crow::response login(const crow::request& req);

    // POST /api/auth/signup  (managers only)
    crow::response signup(const crow::request& req);

    // POST /api/auth/logout
    crow::response logout(const crow::request& req);

    // GET /api/auth/me
    crow::response getMe(const crow::request& req);

    // POST /api/auth/change-password
    crow::response changePassword(const crow::request& req);
};
