#include "controllers/Controllers.h"
#include "middleware/AuthMiddleware.h"
#include <pqxx/pqxx>
#include <iostream>

bool ScheduleController::hasLeaveConflict(const std::string& employeeId,
                                           const std::string& shiftDate) {
    pqxx::work txn(db);
    auto result = txn.exec_params(
        "SELECT id FROM leave_requests "
        "WHERE employee_id=$1 AND status='approved' "
        "AND $2::date BETWEEN start_date AND end_date",
        employeeId, shiftDate
    );
    return !result.empty();
}

crow::response ScheduleController::getWeeklySchedule(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            std::string weekStart = req.url_params.get("week") ? req.url_params.get("week") : "";
            if (weekStart.empty()) {
                return crow::response(400, R"({"error":"week parameter required (YYYY-MM-DD)"})");
            }

            pqxx::work txn(db);
            auto result = txn.exec_params(
                "SELECT s.id, s.shift_date, s.start_time, s.end_time, s.status, s.notes, "
                "u.id as emp_id, u.first_name, u.last_name, "
                "p.id as pos_id, p.name as pos_name, p.color as pos_color "
                "FROM shifts s "
                "JOIN users u ON s.employee_id = u.id "
                "LEFT JOIN positions p ON s.position_id = p.id "
                "WHERE s.shift_date BETWEEN $1::date AND ($1::date + interval '6 days') "
                "ORDER BY s.shift_date ASC, s.start_time ASC",
                weekStart
            );

            json shifts = json::array();
            for (auto& row : result) {
                shifts.push_back({
                    {"id",         row["id"].as<std::string>()},
                    {"shift_date", row["shift_date"].as<std::string>()},
                    {"start_time", row["start_time"].as<std::string>()},
                    {"end_time",   row["end_time"].as<std::string>()},
                    {"status",     row["status"].as<std::string>()},
                    {"notes",      row["notes"].is_null() ? "" : row["notes"].as<std::string>()},
                    {"employee", {
                        {"id",         row["emp_id"].as<std::string>()},
                        {"first_name", row["first_name"].as<std::string>()},
                        {"last_name",  row["last_name"].as<std::string>()}
                    }},
                    {"position", row["pos_id"].is_null() ? nullptr : json({
                        {"id",    row["pos_id"].as<std::string>()},
                        {"name",  row["pos_name"].as<std::string>()},
                        {"color", row["pos_color"].as<std::string>()}
                    })}
                });
            }

            txn.commit();
            auto res = crow::response(200, shifts.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response ScheduleController::createShift(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);

            std::string employeeId = body["employee_id"].get<std::string>();
            std::string shiftDate  = body["shift_date"].get<std::string>();
            std::string startTime  = body["start_time"].get<std::string>();
            std::string endTime    = body["end_time"].get<std::string>();
            std::string positionId = body.value("position_id", "");
            std::string notes      = body.value("notes", "");

            // Conflict check
            if (hasLeaveConflict(employeeId, shiftDate)) {
                return crow::response(409,
                    R"({"error":"Employee has approved leave on this date","conflict":"leave"})");
            }

            pqxx::work txn(db);
            auto result = txn.exec_params(
                "INSERT INTO shifts (employee_id, position_id, shift_date, start_time, end_time, notes, created_by) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id",
                employeeId,
                positionId.empty() ? nullptr : &positionId,
                shiftDate, startTime, endTime,
                notes.empty() ? nullptr : &notes,
                ctx.userId
            );

            json response = {
                {"id",      result[0]["id"].as<std::string>()},
                {"message", "Shift created"}
            };

            txn.commit();
            auto res = crow::response(201, response.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response ScheduleController::updateShift(const crow::request& req, std::string id) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);
            pqxx::work txn(db);

            // Only draft shifts can be edited
            auto check = txn.exec_params("SELECT status FROM shifts WHERE id=$1", id);
            if (check.empty()) return crow::response(404, R"({"error":"Shift not found"})");
            if (check[0]["status"].as<std::string>() == "published") {
                return crow::response(403, R"({"error":"Cannot edit a published shift"})");
            }

            txn.exec_params(
                "UPDATE shifts SET "
                "start_time  = COALESCE(NULLIF($1,'')::time, start_time), "
                "end_time    = COALESCE(NULLIF($2,'')::time, end_time), "
                "notes       = $3 "
                "WHERE id=$4",
                body.value("start_time",""), body.value("end_time",""),
                body.value("notes",""), id
            );

            txn.commit();
            return crow::response(200, R"({"message":"Shift updated"})");
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response ScheduleController::deleteShift(const crow::request& req, std::string id) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            txn.exec_params("DELETE FROM shifts WHERE id=$1", id);
            txn.commit();
            return crow::response(200, R"({"message":"Shift deleted"})");
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response ScheduleController::publishShifts(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);
            std::string weekStart = body["week_start"].get<std::string>();

            pqxx::work txn(db);
            auto result = txn.exec_params(
                "UPDATE shifts SET status='published' "
                "WHERE status='draft' "
                "AND shift_date BETWEEN $1::date AND ($1::date + interval '6 days') "
                "RETURNING id",
                weekStart
            );

            json response = {
                {"published_count", (int)result.size()},
                {"message", "Shifts published successfully"}
            };

            txn.commit();
            auto res = crow::response(200, response.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response ScheduleController::getMyShifts(const crow::request& req) {
    return AuthMiddleware::requireAuth(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            std::string weekStart = req.url_params.get("week") ? req.url_params.get("week") : "";

            pqxx::work txn(db);
            pqxx::result result;

            if (!weekStart.empty()) {
                result = txn.exec_params(
                    "SELECT s.id, s.shift_date, s.start_time, s.end_time, s.status, s.notes, "
                    "p.name as pos_name, p.color as pos_color "
                    "FROM shifts s "
                    "LEFT JOIN positions p ON s.position_id = p.id "
                    "WHERE s.employee_id=$1 AND s.status='published' "
                    "AND s.shift_date BETWEEN $2::date AND ($2::date + interval '6 days') "
                    "ORDER BY s.shift_date ASC, s.start_time ASC",
                    ctx.userId, weekStart
                );
            } else {
                result = txn.exec_params(
                    "SELECT s.id, s.shift_date, s.start_time, s.end_time, s.status, s.notes, "
                    "p.name as pos_name, p.color as pos_color "
                    "FROM shifts s "
                    "LEFT JOIN positions p ON s.position_id = p.id "
                    "WHERE s.employee_id=$1 AND s.status='published' "
                    "AND s.shift_date >= CURRENT_DATE "
                    "ORDER BY s.shift_date ASC LIMIT 14",
                    ctx.userId
                );
            }

            json shifts = json::array();
            for (auto& row : result) {
                json s = {
                    {"id",         row["id"].as<std::string>()},
                    {"shift_date", row["shift_date"].as<std::string>()},
                    {"start_time", row["start_time"].as<std::string>()},
                    {"end_time",   row["end_time"].as<std::string>()},
                    {"notes",      row["notes"].is_null() ? "" : row["notes"].as<std::string>()}
                };
                if (!row["pos_name"].is_null()) {
                    s["position"] = {
                        {"name",  row["pos_name"].as<std::string>()},
                        {"color", row["pos_color"].as<std::string>()}
                    };
                }
                shifts.push_back(s);
            }

            txn.commit();
            auto res = crow::response(200, shifts.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}
