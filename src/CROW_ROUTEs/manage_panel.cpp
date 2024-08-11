#include "manage_panel.hpp"

bool isLogin(std::string jwt, nlohmann::json& settings, std::string IP){
    try {
        JWT::verifyJWT(jwt, settings, IP);
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}
//if the user is not a site admin and dont got the permission
bool isPermissioned(std::string jwt, int problem_id, std::unique_ptr<APIs>& API) {
    try {
        // Check if the user has site-wide permission
        if (!(JWT::getSitePermissionFlags(jwt) & 1)) {
            nlohmann::json roles = JWT::getRoles(jwt);
            if (roles.empty()) {
                return false;
            }

            std::string query = "SELECT * FROM problem_role WHERE problem_id = ? AND role_name IN (";
            for (size_t i = 0; i < roles.size(); ++i) {
                query += "?";
                if (i < roles.size() - 1) query += ", ";
            }
            query += ") AND permission_flags & 1 <> 0";

            // Prepare the statement
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, problem_id);
            for (size_t i = 0; i < roles.size(); ++i) {
                pstmt->setString(i + 2, roles[i].get<std::string>());
            }

            // Execute the query
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (!res->next()) {
                return false;
            }
        }
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

void testcaseRoute(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API) {
    CROW_ROUTE(app, "/manage_panel/problems/<int>/testcases")
    .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method)
    ([&settings, &API, &IP](const crow::request& req, int problem_id){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        if (!isLogin(jwt, settings, IP)) {
            return crow::response(401, "Unauthorized");
        }
        if (!isPermissioned(jwt, problem_id, API)) {
            return crow::response(403, "Forbidden");
        }
        if (req.method == "GET"_method) {    
            std::string query = R"(
                SELECT *
                FROM problem_test_cases
                WHERE problem_id = ?;
            )";
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, problem_id);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            nlohmann::json testcases;
            while(res->next()) {
                nlohmann::json testcase;
                testcase["id"] = res->getInt("id");
                testcase["problem_id"] = res->getInt("problem_id");
                testcase["input"] = res->getString("input");
                testcase["output"] = res->getString("output");
                testcase["time_limit"] = res->getInt("time_limit");
                testcase["memory_limit"] = res->getInt("memory_limit");
                testcase["score"] = res->getInt("score");
                testcases.push_back(testcase);
            }
            return crow::response(200, testcases.dump());
        } else if (req.method == "POST"_method) {
            //replace all the testcases
            std::string query = "DELETE FROM problem_test_cases WHERE problem_id = ?;";
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, problem_id);
            pstmt->execute();
            nlohmann::json testcases = nlohmann::json::parse(req.body);
            query = R"(
            INSERT INTO problem_test_cases (problem_id, input, output, time_limit, memory_limit, score)
            VALUES (?, ?, ?, ?, ?, ?)";
            for (const auto& testcase : testcases) {
                std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
                pstmt->setInt(1, problem_id);
                pstmt->setString(2, testcase["input"].get<std::string>());
                pstmt->setString(3, testcase["output"].get<std::string>());
                pstmt->setInt(4, testcase["time_limit"].get<int>());
                pstmt->setInt(5, testcase["memory_limit"].get<int>());
                pstmt->setInt(6, testcase["score"].get<int>());
                pstmt->execute();
            }
            return crow::response(200, "Test cases updated");
        } else if (req.method == "DELETE"_method) {
            //delete all the testcases
            std::string query = "DELETE FROM problem_test_cases WHERE problem_id = ?";
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, problem_id);
            pstmt->execute();
            return crow::response(200, "Test cases deleted");
        }
    });
}//testcaseRoute

void ROUTE_manage_panel(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API){
    problemsRoute(app, settings, IP, API);
    problemRoute(app, settings, IP, API);
    testcaseRoute(app, settings, IP, API);
}

