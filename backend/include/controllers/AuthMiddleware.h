#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include <string>
#include <crow.h>
using namespace std;

// ─────────────────────────────────────────────
//  AuthMiddleware  —  checks JWT on every request
//  that needs a logged-in user
// ─────────────────────────────────────────────
class AuthMiddleware {
private:
    string secretKey;   // same key used to sign tokens

public:
    // Constructor — receives the JWT secret from main
    AuthMiddleware (string secret) {
        secretKey = secret;
    }

    // Extract the token string from "Authorization: Bearer <token>"
    string extractToken (const crow::request& req) {
        string authHeader = req.get_header_value("Authorization");
        if (authHeader.substr(0, 7) == "Bearer ") {
            return authHeader.substr(7);   // everything after "Bearer "
        }
        return "";   // no token found
    }

    // Verify the token and return the user's role.
    // Returns "" if the token is invalid or missing.
    string verifyToken (const string& token) ;

    // Convenience: returns true if the token belongs to a manager
    bool isManager (const string& token) {
        return verifyToken(token) == "manager";
    }

    // Convenience: returns true if the token belongs to any valid user
    bool isAuthenticated (const string& token) {
        return !verifyToken(token).empty();
    }
};

#endif // AUTH_MIDDLEWARE_H
