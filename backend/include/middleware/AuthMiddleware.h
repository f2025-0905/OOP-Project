#pragma once
#include <crow.h>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct AuthContext {
    std::string userId;
    std::string role;
    bool valid = false;
};

class AuthMiddleware {
public:
    // Validates JWT from Authorization header
    static AuthContext authenticate(const crow::request& req);

    // Middleware: requires any logged-in user
    static crow::response requireAuth(
        const crow::request& req,
        std::function<crow::response(const AuthContext&)> handler
    );

    // Middleware: requires manager role
    static crow::response requireManager(
        const crow::request& req,
        std::function<crow::response(const AuthContext&)> handler
    );

    // Middleware: requires employee role
    static crow::response requireEmployee(
        const crow::request& req,
        std::function<crow::response(const AuthContext&)> handler
    );

    static std::string generateJWT(const std::string& userId,
                                    const std::string& role,
                                    int expiryHours = 24);

    static AuthContext verifyJWT(const std::string& token);

private:
    static const std::string SECRET_KEY;
};
