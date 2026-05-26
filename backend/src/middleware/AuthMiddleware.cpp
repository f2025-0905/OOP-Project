#include "../include/middleware/AuthMiddleware.h"
#include <jwt-cpp/jwt.h>
#include <iostream>
using namespace std;

// ─────────────────────────────────────────────
//  verifyToken
//  Decodes the JWT and returns the user's role.
//  Returns "" when the token is bad or expired.
// ─────────────────────────────────────────────
string AuthMiddleware::verifyToken (const string& token) {
    // Empty token — nothing to check
    if (token.empty()) {
        return "";
    }

    try {
        // Decode and verify the token signature
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{secretKey})
                            .with_issuer("shiftwise");

        verifier.verify(decoded);

        // Pull the "role" claim out of the token payload
        string role = decoded.get_payload_claim("role").as_string();
        return role;

    } catch (exception& e) {
        // Token is invalid, expired, or tampered
        cerr << "Token verification failed: " << e.what() << endl;
        return "";
    }
}
