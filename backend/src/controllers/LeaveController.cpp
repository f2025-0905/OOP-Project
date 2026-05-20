#include "controllers/Controllers.h"
#include "middleware/AuthMiddleware.h"
#include <pqxx/pqxx>
#include <iostream>
#include <ctime>

int LeaveController::calculateWorkingDays(const std::string& startDate,
                                           const std::string& endDate) {
    // Simple inclusive day count (can enhance for weekday-only logic)
    // Using DB for accuracy
    return 1; // placeholder; actual calc done in DB query below
}

void LeaveController::markShiftsAsLeave(const std::string& employeeId,
                                         const std::string& startDate,
                                         const std::string& endDate,
                                         pqxx::work& txn) {
    // When leave is approved, remove draft shifts in that range
    txn.exec_params(
        "DELETE FROM shifts "
        "WHERE employee_id=$1 AND status='draft' "
        "AND shift_date BETWEEN $2::date AND $3::date",
        employeeId, startDate, endDate
    );
}

crow::response LeaveController::applyLeave(const crow::request& req) {
    return AuthMiddleware::requireAuth(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);

            std::string startDate = body["start_date"].get<std::string>();
            std::string endDate   = body["end_date"].get<std::string>();
            std::string leaveType = body.value("leave_type", "annual");
            std::string reason    = body.value("reason", "");

            pqxx::work txn(db);

            // Check leave balance for annual leave
            if (leaveType == "annual") {
                auto balRes = txn.exec_params(
                    "SELECT leave_balance FROM employee_profiles WHERE user_id=$1", ctx.userId
                );
                if (!balRes.empty()) {
                    int balance = balRes[0]["leave_balance"].as<int>();

                    auto daysRes = txn.exec_params(
                        "SELECT ($2::date - $1::date + 1) as days",
                        startDate, endDate
                    );
                    int days = daysRes[0]["days"].as<int>();

                    if (days > balance) {
                        return crow::response(400,
                            "{\"error\":\"Insufficient leave balance. You have " +
                            std::to_string(balance) + " days remaining\"}");
                    }
                }
            }

            // Check for overlapping pending/approved requests
            auto overlap = txn.exec_params(
                "SELECT id FROM leave_requests "
                "WHERE employee_id=$1 AND status IN ('pending','approved') "
                "AND NOT (end_date < $2::date OR start_date > $3::date)",
                ctx.userId, startDate, endDate
            );
            if (!overlap.empty()) {
                return crow::response(409, R"({"error":"You already have a leave request for this period"})");
            }

            auto result = txn.exec_params(
                "INSERT INTO leave_requests (employee_id, start_date, end_date, leave_type, reason) "
                "VALUES ($1,$2,$3,$4,$5) RETURNING id, created_at",
                ctx.userId, startDate, endDate, leaveType, reason
            );

            txn.commit();

            json response = {
                {"id",         result[0]["id"].as<std::string>()},
                {"start_date", startDate},
                {"end_date",   endDate},
                {"leave_type", leaveType},
                {"status",     "pending"},
                {"message",    "Leave request submitted"}
            };

            auto res = crow::response(201, response.dump());
            res.set_header("Content-Type", "application/json");
            return res;

        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response LeaveController::getMyLeaves(const crow::request& req) {
    return AuthMiddleware::requireAuth(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            auto result = txn.exec_params(
                "SELECT lr.id, lr.start_date, lr.end_date, lr.leave_type, lr.reason, "
                "lr.status, lr.created_at, lr.reviewed_at, "
                "u.first_name as reviewer_first, u.last_name as reviewer_last "
                "FROM leave_requests lr "
                "LEFT JOIN users u ON lr.reviewed_by = u.id "
                "WHERE lr.employee_id=$1 "
                "ORDER BY lr.created_at DESC",
                ctx.userId
            );

            json leaves = json::array();
            for (auto& row : result) {
                json lr = {
                    {"id",         row["id"].as<std::string>()},
                    {"start_date", row["start_date"].as<std::string>()},
                    {"end_date",   row["end_date"].as<std::string>()},
                    {"leave_type", row["leave_type"].as<std::string>()},
                    {"reason",     row["reason"].is_null() ? "" : row["reason"].as<std::string>()},
                    {"status",     row["status"].as<std::string>()},
                    {"created_at", row["created_at"].as<std::string>()}
                };
                if (!row["reviewer_first"].is_null()) {
                    lr["reviewed_by"] = row["reviewer_first"].as<std::string>() + " "
                                       + row["reviewer_last"].as<std::string>();
                    lr["reviewed_at"] = row["reviewed_at"].as<std::string>();
                }
                leaves.push_back(lr);
            }

            txn.commit();
            auto res = crow::response(200, leaves.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response LeaveController::getAllLeaves(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            std::string status = req.url_params.get("status") ? req.url_params.get("status") : "all";

            pqxx::work txn(db);
            pqxx::result result;

            if (status == "all") {
                result = txn.exec(
                    "SELECT lr.id, lr.start_date, lr.end_date, lr.leave_type, lr.reason, "
                    "lr.status, lr.created_at, "
                    "u.id as emp_id, u.first_name, u.last_name, u.email "
                    "FROM leave_requests lr "
                    "JOIN users u ON lr.employee_id = u.id "
                    "ORDER BY CASE lr.status WHEN 'pending' THEN 0 ELSE 1 END, lr.created_at DESC"
                );
            } else {
                result = txn.exec_params(
                    "SELECT lr.id, lr.start_date, lr.end_date, lr.leave_type, lr.reason, "
                    "lr.status, lr.created_at, "
                    "u.id as emp_id, u.first_name, u.last_name, u.email "
                    "FROM leave_requests lr "
                    "JOIN users u ON lr.employee_id = u.id "
                    "WHERE lr.status=$1 ORDER BY lr.created_at DESC",
                    status
                );
            }

            json leaves = json::array();
            for (auto& row : result) {
                leaves.push_back({
                    {"id",         row["id"].as<std::string>()},
                    {"start_date", row["start_date"].as<std::string>()},
                    {"end_date",   row["end_date"].as<std::string>()},
                    {"leave_type", row["leave_type"].as<std::string>()},
                    {"reason",     row["reason"].is_null() ? "" : row["reason"].as<std::string>()},
                    {"status",     row["status"].as<std::string>()},
                    {"created_at", row["created_at"].as<std::string>()},
                    {"employee", {
                        {"id",         row["emp_id"].as<std::string>()},
                        {"first_name", row["first_name"].as<std::string>()},
                        {"last_name",  row["last_name"].as<std::string>()},
                        {"email",      row["email"].as<std::string>()}
                    }}
                });
            }

            txn.commit();
            auto res = crow::response(200, leaves.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response LeaveController::reviewLeave(const crow::request& req, std::string id) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);
            std::string action = body["action"].get<std::string>(); // "approve" or "reject"

            if (action != "approve" && action != "reject") {
                return crow::response(400, R"({"error":"Action must be 'approve' or 'reject'"})");
            }

            pqxx::work txn(db);

            auto leaveRes = txn.exec_params(
                "SELECT employee_id, start_date, end_date, leave_type, status "
                "FROM leave_requests WHERE id=$1",
                id
            );

            if (leaveRes.empty()) return crow::response(404, R"({"error":"Leave request not found"})");
            if (leaveRes[0]["status"].as<std::string>() != "pending") {
                return crow::response(409, R"({"error":"Leave request already reviewed"})");
            }

            std::string employeeId = leaveRes[0]["employee_id"].as<std::string>();
            std::string startDate  = leaveRes[0]["start_date"].as<std::string>();
            std::string endDate    = leaveRes[0]["end_date"].as<std::string>();
            std::string leaveType  = leaveRes[0]["leave_type"].as<std::string>();

            std::string newStatus = (action == "approve") ? "approved" : "rejected";

            txn.exec_params(
                "UPDATE leave_requests SET status=$1, reviewed_by=$2, reviewed_at=NOW() WHERE id=$3",
                newStatus, ctx.userId, id
            );

            if (action == "approve") {
                // Deduct leave balance for annual/sick leave
                if (leaveType == "annual" || leaveType == "sick") {
                    txn.exec_params(
                        "UPDATE employee_profiles SET "
                        "leave_balance = leave_balance - ($1::date - $2::date + 1) "
                        "WHERE user_id=$3 AND leave_balance > 0",
                        endDate, startDate, employeeId
                    );
                }
                // Remove conflicting draft shifts
                markShiftsAsLeave(employeeId, startDate, endDate, txn);
            }

            txn.commit();

            json response = {
                {"status",  newStatus},
                {"message", "Leave request " + newStatus}
            };

            auto res = crow::response(200, response.dump());
            res.set_header("Content-Type", "application/json");
            return res;

        } catch (const std::exception& e) {
            std::cerr << "Review leave error: " << e.what() << std::endl;
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}
