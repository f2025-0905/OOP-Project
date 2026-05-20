#include "middleware/AuthMiddleware.h"
#include <jwt-cpp/jwt.h>
#include <cstdlib>
#include <chrono>

const std::string AuthMiddleware::SECRET_KEY = []() {
    const char* key = std::getenv("JWT_SECRET");
    return key ? std::string(key) : "shiftwise-default-secret-change-in-production";
}();

std::string AuthMiddleware::generateJWT(const std::string& userId,
                                         const std::string& role,
                                         int expiryHours) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(expiryHours);

    return jwt::create()
        .set_issuer("shiftwise")
        .set_type("JWT")
        .set_payload_claim("user_id", jwt::claim(userId))
        .set_payload_claim("role",    jwt::claim(role))
        .set_issued_at(now)
        .set_expires_at(exp)
        .sign(jwt::algorithm::hs256{SECRET_KEY});
}

AuthContext AuthMiddleware::verifyJWT(const std::string& token) {
    AuthContext ctx;
    try {
        auto decoded  = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{SECRET_KEY})
            .with_issuer("shiftwise");

        verifier.verify(decoded);

        ctx.userId = decoded.get_payload_claim("user_id").as_string();
        ctx.role   = decoded.get_payload_claim("role").as_string();
        ctx.valid  = true;
    } catch (...) {
        ctx.valid = false;
    }
    return ctx;
}

AuthContext AuthMiddleware::authenticate(const crow::request& req) {
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        return AuthContext{};
    }
    std::string token = authHeader.substr(7);
    return verifyJWT(token);
}

crow::response AuthMiddleware::requireAuth(
    const crow::request& req,
    std::function<crow::response(const AuthContext&)> handler)
{
    auto ctx = authenticate(req);
    if (!ctx.valid) {
        return crow::response(401, R"({"error":"Unauthorized. Please log in."})");
    }
    return handler(ctx);
}

crow::response AuthMiddleware::requireManager(
    const crow::request& req,
    std::function<crow::response(const AuthContext&)> handler)
{
    auto ctx = authenticate(req);
    if (!ctx.valid) {
        return crow::response(401, R"({"error":"Unauthorized. Please log in."})");
    }
    if (ctx.role != "manager") {
        return crow::response(403, R"({"error":"Forbidden. Manager access required."})");
    }
    return handler(ctx);
}

crow::response AuthMiddleware::requireEmployee(
    const crow::request& req,
    std::function<crow::response(const AuthContext&)> handler)
{
    auto ctx = authenticate(req);
    if (!ctx.valid) {
        return crow::response(401, R"({"error":"Unauthorized. Please log in."})");
    }
    if (ctx.role != "employee") {
        return crow::response(403, R"({"error":"Forbidden. Employee access required."})");
    }
    return handler(ctx);
}
