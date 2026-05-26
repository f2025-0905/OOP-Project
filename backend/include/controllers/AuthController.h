#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <crow.h>
#include <pqxx/pqxx>
#include <string>
using namespace std;

// ─────────────────────────────────────────────
//  AuthController  —  handles /api/auth routes
//   POST /api/auth/signup
//   POST /api/auth/login
// ─────────────────────────────────────────────
class AuthController {
private:
    string dbConnection;   // PostgreSQL connection string
    string jwtSecret;      // secret used to sign JWT tokens

    // Hash a plain password using a simple method (bcrypt wrapper)
    string hashPassword (string plainText) ;

    // Compare a plain password with a stored hash
    bool checkPassword (string plainText, string hash) ;

    // Create a signed JWT for the given user id and role
    string createToken (int userId, string role) ;

public:
    // Constructor — inject DB connection string and JWT secret
    AuthController (string connStr, string secret) {
        dbConnection = connStr;
        jwtSecret    = secret;
    }

    // Register a new user account
    // Expects JSON body: { name, email, password, role }
    crow::response signup (const crow::request& req) ;

    // Login with email + password, returns JWT on success
    // Expects JSON body: { email, password }
    crow::response login (const crow::request& req) ;
};

#endif // AUTH_CONTROLLER_H
