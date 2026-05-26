#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include <crow.h>
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

// ─────────────────────────────────────────────
//  AuthContext  —  stores info extracted from
//  a verified JWT token
// ─────────────────────────────────────────────
struct AuthContext {
    string userId;
    string role;
    bool   valid = false;   // false means token was missing or bad
};

// ─────────────────────────────────────────────
//  AuthMiddleware  —  handles JWT creation and
//  verification for every protected route
// ─────────────────────────────────────────────
class AuthMiddleware {
private:
    // Secret key used to sign and verify all tokens.
    // Value is defined once in AuthMiddleware.cpp
    static const string SECRET_KEY;

public:
    // Read the Authorization header and verify the JWT.
    // Returns an AuthContext — check .valid before using it.
    static AuthContext authenticate (const crow::request& req) ;

    // Protect a route — any logged-in user may pass.
    // Calls handler(ctx) when the token is valid,
    // returns 401 automatically when it is not.
    static crow::response requireAuth (
        const crow::request& req,
        function<crow::response(const AuthContext&)> handler
    ) ;

    // Protect a route — only managers may pass.
    // Returns 403 when the user is authenticated but not a manager.
    static crow::response requireManager (
        const crow::request& req,
        function<crow::response(const AuthContext&)> handler
    ) ;

    // Protect a route — only employees may pass.
    // Returns 403 when the user is authenticated but not an employee.
    static crow::response requireEmployee (
        const crow::request& req,
        function<crow::response(const AuthContext&)> handler
    ) ;

    // Create a signed JWT for the given userId and role.
    // Token expires after expiryHours (default: 24 hours).
    static string generateJWT (const string& userId,
                                const string& role,
                                int expiryHours = 24) ;

    // Decode and verify a JWT string.
    // Returns an AuthContext with valid = false if anything is wrong.
    static AuthContext verifyJWT (const string& token) ;
};

#endif // AUTH_MIDDLEWARE_H