#pragma once
#include "../manage_panel.hpp"

namespace {
crow::response PUT(const crow::request& req, std::string jwt, std::unique_ptr<APIs>& API, nlohmann::json& settings, int problem_id) {
    // update the problem
    //check if the table correct
    nlohmann::json body = nlohmann::json::parse(req.body);
    if(settings["problem_tables"].find(body["table"]) == settings["problem_tables"].end()){
        return crow::response(400, "Invalid table");
    }
    //check if the column correct
    if(settings["problem_tables"][body["table"]].find(body["column"]) == settings["problem_tables"][body["table"]].end()){
        return crow::response(400, "Invalid column");
    }
    if(body["table"] == "problems"){
        //update the problem
        std::string query = "UPDATE problems SET " + body["column"].get<std::string>() + " = ? WHERE id = ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
        pstmt->setString(1, body["value"].get<std::string>());
        pstmt->setInt(2, problem_id);
        pstmt->execute();
    } else {
        //update the problem
        std::string query = "UPDATE " + body["table"].get<std::string>() + " SET " + body["column"].get<std::string>() + " = ? WHERE problem_id = ?";
        std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
        if(body["value"].is_string()){
            pstmt->setString(1, body["value"].get<std::string>());
        } else {
            pstmt->setInt(1, body["value"].get<int>());
        }
        pstmt->setInt(2, problem_id);
        pstmt->execute();
    }
    return crow::response(200, "Problem updated");
}
}//namespace

void problemRoute(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API) {
    CROW_ROUTE(app, "/manage_panel/problems/<int>")
    .methods("PUT"_method, "DELETE"_method)
    ([&settings, &API, &IP](const crow::request& req, int problem_id){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        if (!isLogin(jwt, settings, IP)) {
            return crow::response(401, "Unauthorized");
        }
        //if the user is not a site admin and dont got the permission
        if (!isPermissioned(jwt, problem_id, API)) {
            return crow::response(403, "Forbidden");
        }
        if (req.method == "PUT"_method) {
        } else if (req.method == "DELETE"_method) {
            // delete the problem
            std::string query;

            // Delete from problem_submissions_subtasks
            query = "DELETE pst FROM problem_submissions_subtasks pst "
                    "JOIN problem_submissions ps ON pst.submission_id = ps.id "
                    "WHERE ps.problem_id = ?";
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, problem_id);
            pstmt->execute();

            // Define an array of table names related to a problem
            const char* tables[] = {
                "problem_submissions",
                "problem_sample_IO",
                "problem_solutions",
                "problem_hints",
                "problem_test_cases",
                "problem_tags",
                "problem_role"
            };
            for (auto& table : tables) {
                std::string query = "DELETE FROM " + std::string(table) + " WHERE problem_id = ?";
                pstmt = API->prepareStatement(query);
                pstmt->setInt(1, problem_id);
                pstmt->execute();
            }
            // Finally, delete from problems
            query = "DELETE FROM problems WHERE id = ?";
            pstmt = API->prepareStatement(query);
            pstmt->setInt(1, problem_id);
            pstmt->execute();

            return crow::response(200, "Problem deleted");
        }
    });
}//problemRoute