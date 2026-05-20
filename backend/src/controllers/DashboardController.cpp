#include "controllers/Controllers.h"
#include "middleware/AuthMiddleware.h"
#include <pqxx/pqxx>

crow::response DashboardController::getManagerDashboard(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);

            // Widget 1: Who is working right now
            auto working = txn.exec(
                "SELECT u.id, u.first_name, u.last_name, "
                "s.start_time, s.end_time, p.name as position "
                "FROM shifts s "
                "JOIN users u ON s.employee_id = u.id "
                "LEFT JOIN positions p ON s.position_id = p.id "
                "WHERE s.shift_date = CURRENT_DATE "
                "AND s.status = 'published' "
                "AND NOW()::time BETWEEN s.start_time AND s.end_time "
                "ORDER BY s.start_time ASC"
            );

            // Widget 2: Who is on leave today
            auto onLeave = txn.exec(
                "SELECT u.id, u.first_name, u.last_name, "
                "lr.leave_type, lr.start_date, lr.end_date "
                "FROM leave_requests lr "
                "JOIN users u ON lr.employee_id = u.id "
                "WHERE lr.status='approved' "
                "AND CURRENT_DATE BETWEEN lr.start_date AND lr.end_date "
                "ORDER BY u.first_name ASC"
            );

            // Stats
            auto stats = txn.exec(
                "SELECT "
                "(SELECT COUNT(*) FROM users WHERE role='employee' AND is_active=true) as total_employees,"
                "(SELECT COUNT(*) FROM leave_requests WHERE status='pending') as pending_leaves,"
                "(SELECT COUNT(*) FROM shifts WHERE shift_date=CURRENT_DATE AND status='published') as shifts_today,"
                "(SELECT COUNT(*) FROM users WHERE role='employee' AND is_active=true "
                " AND id NOT IN (SELECT employee_id FROM leave_requests WHERE status='approved' AND CURRENT_DATE BETWEEN start_date AND end_date)) as available_today"
            );

            json workingArr = json::array();
            for (auto& row : working) {
                workingArr.push_back({
                    {"id",         row["id"].as<std::string>()},
                    {"first_name", row["first_name"].as<std::string>()},
                    {"last_name",  row["last_name"].as<std::string>()},
                    {"start_time", row["start_time"].as<std::string>()},
                    {"end_time",   row["end_time"].as<std::string>()},
                    {"position",   row["position"].is_null() ? "" : row["position"].as<std::string>()}
                });
            }

            json leaveArr = json::array();
            for (auto& row : onLeave) {
                leaveArr.push_back({
                    {"id",         row["id"].as<std::string>()},
                    {"first_name", row["first_name"].as<std::string>()},
                    {"last_name",  row["last_name"].as<std::string>()},
                    {"leave_type", row["leave_type"].as<std::string>()},
                    {"start_date", row["start_date"].as<std::string>()},
                    {"end_date",   row["end_date"].as<std::string>()}
                });
            }

            json response = {
                {"working_now",      workingArr},
                {"on_leave_today",   leaveArr},
                {"stats", {
                    {"total_employees",  stats[0]["total_employees"].as<int>()},
                    {"pending_leaves",   stats[0]["pending_leaves"].as<int>()},
                    {"shifts_today",     stats[0]["shifts_today"].as<int>()},
                    {"available_today",  stats[0]["available_today"].as<int>()}
                }}
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

crow::response DashboardController::getEmployeeDashboard(const crow::request& req) {
    return AuthMiddleware::requireAuth(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);

            // Next 7 shifts
            auto shifts = txn.exec_params(
                "SELECT s.shift_date, s.start_time, s.end_time, p.name as position, p.color "
                "FROM shifts s "
                "LEFT JOIN positions p ON s.position_id = p.id "
                "WHERE s.employee_id=$1 AND s.status='published' "
                "AND s.shift_date >= CURRENT_DATE "
                "ORDER BY s.shift_date ASC LIMIT 7",
                ctx.userId
            );

            // Leave balance
            auto profile = txn.exec_params(
                "SELECT ep.leave_balance, p.name as position "
                "FROM employee_profiles ep "
                "LEFT JOIN positions p ON ep.position_id = p.id "
                "WHERE ep.user_id=$1",
                ctx.userId
            );

            // Pending leave requests
            auto pendingLeaves = txn.exec_params(
                "SELECT COUNT(*) as cnt FROM leave_requests "
                "WHERE employee_id=$1 AND status='pending'",
                ctx.userId
            );

            json upcomingShifts = json::array();
            for (auto& row : shifts) {
                json s = {
                    {"shift_date", row["shift_date"].as<std::string>()},
                    {"start_time", row["start_time"].as<std::string>()},
                    {"end_time",   row["end_time"].as<std::string>()}
                };
                if (!row["position"].is_null()) {
                    s["position"] = row["position"].as<std::string>();
                    s["color"]    = row["color"].as<std::string>();
                }
                upcomingShifts.push_back(s);
            }

            json response = {
                {"upcoming_shifts",  upcomingShifts},
                {"leave_balance",    profile.empty() ? 20 : profile[0]["leave_balance"].as<int>()},
                {"position",         (profile.empty() || profile[0]["position"].is_null()) ? "" : profile[0]["position"].as<std::string>()},
                {"pending_leaves",   pendingLeaves[0]["cnt"].as<int>()}
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

crow::response DashboardController::getAnnouncements(const crow::request& req) {
    return AuthMiddleware::requireAuth(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            auto result = txn.exec(
                "SELECT a.id, a.title, a.body, a.priority, a.created_at, "
                "u.first_name, u.last_name "
                "FROM announcements a "
                "JOIN users u ON a.posted_by = u.id "
                "ORDER BY a.created_at DESC LIMIT 20"
            );

            json announcements = json::array();
            for (auto& row : result) {
                announcements.push_back({
                    {"id",         row["id"].as<std::string>()},
                    {"title",      row["title"].as<std::string>()},
                    {"body",       row["body"].as<std::string>()},
                    {"priority",   row["priority"].as<std::string>()},
                    {"created_at", row["created_at"].as<std::string>()},
                    {"posted_by",  row["first_name"].as<std::string>() + " " + row["last_name"].as<std::string>()}
                });
            }

            txn.commit();
            auto res = crow::response(200, announcements.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response DashboardController::createAnnouncement(const crow::request& req) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            auto body = json::parse(req.body);
            std::string title    = body["title"].get<std::string>();
            std::string bodyText = body["body"].get<std::string>();
            std::string priority = body.value("priority", "normal");

            pqxx::work txn(db);
            auto result = txn.exec_params(
                "INSERT INTO announcements (title, body, priority, posted_by) "
                "VALUES ($1,$2,$3,$4) RETURNING id, created_at",
                title, bodyText, priority, ctx.userId
            );

            txn.commit();

            json response = {
                {"id",         result[0]["id"].as<std::string>()},
                {"title",      title},
                {"body",       bodyText},
                {"priority",   priority},
                {"created_at", result[0]["created_at"].as<std::string>()},
                {"message",    "Announcement posted"}
            };

            auto res = crow::response(201, response.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}

crow::response DashboardController::deleteAnnouncement(const crow::request& req, std::string id) {
    return AuthMiddleware::requireManager(req, [&](const AuthContext& ctx) -> crow::response {
        try {
            pqxx::work txn(db);
            txn.exec_params("DELETE FROM announcements WHERE id=$1", id);
            txn.commit();
            return crow::response(200, R"({"message":"Announcement deleted"})");
        } catch (const std::exception& e) {
            return crow::response(500, R"({"error":"Internal server error"})");
        }
    });
}
