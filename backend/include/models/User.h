#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// ─────────────────────────────────────────────
//  User  —  stores one employee's account info
// ─────────────────────────────────────────────
class User {
private:
    int    id;
    string name;
    string email;
    string password;   // hashed password
    string role;       // "manager" or "employee"

public:
    // Default constructor
    User () {
        id   = 0;
        name = "";
        email    = "";
        password = "";
        role     = "employee";
    }

    // Parameterized constructor
    User (int userId, string userName, string userEmail,
          string userPassword, string userRole) {
        id       = userId;
        name     = userName;
        email    = userEmail;
        password = userPassword;
        role     = userRole;
    }

    // ── Getters ──────────────────────────────
    int    getId ()       { return id; }
    string getName ()     { return name; }
    string getEmail ()    { return email; }
    string getPassword () { return password; }
    string getRole ()     { return role; }

    // ── Setters ──────────────────────────────
    void setId (int userId)             { id       = userId; }
    void setName (string userName)      { name     = userName; }
    void setEmail (string userEmail)    { email    = userEmail; }
    void setPassword (string pass)      { password = pass; }
    void setRole (string userRole)      { role     = userRole; }

    // Convert object to JSON string (used by HTTP responses)
    string toJson () {
        return "{"
               "\"id\":"     + to_string(id)   + ","
               "\"name\":\""  + name            + "\","
               "\"email\":\"" + email           + "\","
               "\"role\":\""  + role            + "\""
               "}";
    }
};

#endif // USER_H
