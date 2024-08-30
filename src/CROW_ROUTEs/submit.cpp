#include "submit.hpp"
#include "../API/sand_box_api.hpp"

#define JSON_ERROR(message) nlohmann::json({{"error", message}})

void ROUTE_Submit(crow::App<crow::CORSHandler>& app, nlohmann::json& settings , std::string IP, std::unique_ptr<APIs>& sqlAPI, std::unique_ptr<APIs>& submissionAPI, std::vector<std::string> accepted_languages, std::unique_ptr<sand_box_api>& sandboxAPI) {
    CROW_ROUTE(app, "/submit")
    .methods("POST"_method)
    ([&](const crow::request& req){
        // Parse the request body
        nlohmann::json body = nlohmann::json::parse(req.body);
        // // Check permissions
        // std::string jwt = req.get_header_value("Authorization");
        // try {
        //     JWT::verifyJWT(jwt, settings, IP);
        // } catch (const std::exception& e) {
        //     return crow::response(401, JSON_ERROR(e.what()));
        // }
        // try {
        //     if(!JWT::isPermissioned(jwt, body["problem_id"].get<int>(), sqlAPI, settings["permission_flags"]["submit"].get<int>())){
        //         return crow::response(403, JSON_ERROR("Permission denied"));
        //     }
        // } catch (const std::exception& e) {
        //     return crow::response(500, JSON_ERROR(e.what()));
        // }

        // // Validate the request body
        // if (!body.contains("source_code") || !body.contains("language") || !body.contains("problem_id")) {
        //     return crow::response(400, JSON_ERROR("Missing required fields"));
        // }
        // // Get the source code, language, and problem ID
        // std::string source_code = body["source_code"].get<std::string>();
        // std::string language = body["language"].get<std::string>();
        // int problem_id = body["problem_id"].get<int>();
        // // Validate the language
        // if (std::find(accepted_languages.begin(), accepted_languages.end(), language) == accepted_languages.end()) {
        //     return crow::response(400, JSON_ERROR("Invalid language"));
        // }
        // int submission_id;
        // // insert the submission and mark it as pending
        // try {
        //     std::string query = "INSERT INTO problem_submissions (problem_id, user_id, submission_time, code, score, status, time_taken, memory_taken, language) VALUES (?, ?, NOW(), ?, ?, ?, ?, ?, ?);";
        //     submissionAPI->beginTransaction();
        //     std::unique_ptr<sql::PreparedStatement> pstmt(submissionAPI->prepareStatement(query));
        //     pstmt->setInt(1, problem_id);
        //     pstmt->setInt(2, JWT::getUserID(jwt));
        //     pstmt->setString(3, source_code);
        //     pstmt->setInt(4, 0);
        //     pstmt->setString(5, "Pending");
        //     pstmt->setInt(6, 0);
        //     pstmt->setInt(7, 0);
        //     pstmt->setString(8, language);
        //     pstmt->execute();
        //     // get the submission ID
        //     query = "SELECT LAST_INSERT_ID() AS id;";
        //     std::unique_ptr<sql::PreparedStatement> pstmt2(submissionAPI->prepareStatement(query));
        //     std::unique_ptr<sql::ResultSet> res(pstmt2->executeQuery());
        //     res->next();
        //     submission_id = res->getInt("id");
        //     submissionAPI->commitTransaction();
        // } catch (const std::exception& e) {
        //     submissionAPI->rollbackTransaction();
        //     return crow::response(500, JSON_ERROR(e.what()));
        // }
        // // fetch the test cases
        // nlohmann::json test_cases;
        // try {
        //     std::string query = "SELECT id, input, output, time_limit, memory_limit"
        //                         "FROM problem_test_cases"
        //                         "WHERE problem_id = ?;";
        //     std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
        //     pstmt->setInt(1, problem_id);
        //     std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        //     while (res->next()) {
        //         int test_case_id = res->getInt("id");
        //         std::string input = res->getString("input");
        //         std::string output = res->getString("output");
        //         int time_limit = res->getInt("time_limit");
        //         int memory_limit = res->getInt("memory_limit");
        //         nlohmann::json test_case = {
        //             {"id", test_case_id},
        //             {"in", input},
        //             {"ou", output},
        //             {"ti", time_limit},
        //             {"me", memory_limit}
        //         };
        //         test_cases.push_back(test_case);
        //     }
        // } catch (const std::exception& e) {
        //     return crow::response(500, JSON_ERROR(e.what()));
        // }
        // // Send the submission to the sandbox
        // nlohmann::json payload = {
        //     {"source_code", source_code},
        //     {"language", language},
        //     {"test_cases", test_cases}
        // };
        // std::string response = sandboxAPI->POST(payload);
        // // expect : json object with each test case id and status, time_taken, memory_taken
        // nlohmann::json result = nlohmann::json::parse(response);
        // // Save to the database
        // //update the submission status
        // try {
        //     std::string query = "UPDATE problem_submissions SET status = ?, score = ?, time_taken = ?, memory_taken = ? WHERE id = ?;";
        //     submissionAPI->beginTransaction();
        //     std::unique_ptr<sql::PreparedStatement> pstmt(submissionAPI->prepareStatement(query));
        //     pstmt->setString(1, result["status"].get<std::string>());
        //     pstmt->setInt(2, result["score"].get<int>());
        //     pstmt->setInt(3, result["time_taken"].get<int>());
        //     pstmt->setInt(4, result["memory_taken"].get<int>());
        //     pstmt->setInt(5, submission_id);
        //     pstmt->execute();
        //     submissionAPI->commitTransaction();
        // } catch (const std::exception& e) {
        //     submissionAPI->rollbackTransaction();
        //     // set the submission status to Rejected
        //     try {
        //         std::string query = "UPDATE problem_submissions SET status = 'Rejected' WHERE id = ?;";
        //         std::unique_ptr<sql::PreparedStatement> pstmt(submissionAPI->prepareStatement(query));
        //         pstmt->setInt(1, submission_id);
        //         pstmt->execute();
        //     } catch (const std::exception& e) {
        //         return crow::response(500, JSON_ERROR(e.what()));
        //     }
        //     return crow::response(500, JSON_ERROR(e.what()));
        // }
        // // upload the subtasks
        // try {
        //     std::string query = "INSERT INTO problem_submissions_subtasks (submission_id, test_case_id, status, time_taken, memory_taken) VALUES (?, ?, ?, ?, ?);";
        //     submissionAPI->beginTransaction();
        //     std::unique_ptr<sql::PreparedStatement> pstmt(submissionAPI->prepareStatement(query));
        //     for (const auto& subtask : result["subtasks"]) {
        //         pstmt->setInt(1, submission_id);
        //         pstmt->setInt(2, subtask["id"].get<int>());
        //         pstmt->setString(3, subtask["status"].get<std::string>());
        //         pstmt->setInt(4, subtask["time_taken"].get<int>());
        //         pstmt->setInt(5, subtask["memory_taken"].get<int>());
        //         pstmt->execute();
        //     }
        //     submissionAPI->commitTransaction();
        // } catch (const std::exception& e) {
        //     submissionAPI->rollbackTransaction();
        //     return crow::response(500, JSON_ERROR(e.what()));
        // }
        // return crow::response(200, result.dump());

        // testing connection /w sandbox
        std::string response = sandboxAPI->POST("hi");
        CROW_LOG_INFO << response;
        return crow::response(200, response);
    });
}