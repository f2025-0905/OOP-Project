#ifndef SHIFT_H
#define SHIFT_H

#include <string>
using namespace std;

// ─────────────────────────────────────────────
//  Shift  —  represents one work shift entry
// ─────────────────────────────────────────────
class Shift {
private:
    int    id;
    int    userId;      // which employee
    string date;        // "YYYY-MM-DD"
    string startTime;   // "HH:MM"
    string endTime;     // "HH:MM"
    string status;      // "scheduled" | "completed" | "cancelled"

public:
    // Default constructor
    Shift () {
        id        = 0;
        userId    = 0;
        date      = "";
        startTime = "";
        endTime   = "";
        status    = "scheduled";
    }

    // Parameterized constructor
    Shift (int shiftId, int empId, string shiftDate,
           string start, string end, string shiftStatus) {
        id        = shiftId;
        userId    = empId;
        date      = shiftDate;
        startTime = start;
        endTime   = end;
        status    = shiftStatus;
    }

    // ── Getters ──────────────────────────────
    int    getId ()        { return id; }
    int    getUserId ()    { return userId; }
    string getDate ()      { return date; }
    string getStartTime () { return startTime; }
    string getEndTime ()   { return endTime; }
    string getStatus ()    { return status; }

    // ── Setters ──────────────────────────────
    void setId (int shiftId)          { id        = shiftId; }
    void setUserId (int empId)        { userId    = empId; }
    void setDate (string shiftDate)   { date      = shiftDate; }
    void setStartTime (string start)  { startTime = start; }
    void setEndTime (string end)      { endTime   = end; }
    void setStatus (string s)         { status    = s; }

    // Convert object to JSON string
    string toJson () {
        return "{"
               "\"id\":"          + to_string(id)     + ","
               "\"userId\":"      + to_string(userId)  + ","
               "\"date\":\""      + date               + "\","
               "\"startTime\":\"" + startTime          + "\","
               "\"endTime\":\""   + endTime            + "\","
               "\"status\":\""    + status             + "\""
               "}";
    }
};

#endif // SHIFT_H
