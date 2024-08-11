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


void problemRoute(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API) {
    CROW_ROUTE(app, "/manage_panel/problems/<int>")
    .methods("GET"_method, "PUT"_method, "DELETE"_method)
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
        if (req.method == "GET"_method) { // get the problem details except the testcases and submissions
            std::string query = R"(
                SELECT 
                    p.id AS problem_id, 
                    p.title AS problem_title, 
                    p.description, 
                    p.input_format, 
                    p.output_format, 
                    p.difficulty, 
                    ps.title AS solution_title, 
                    ps.solution, 
                    ph.title AS hint_title, 
                    ph.hint, 
                    pio.sample_input, 
                    pio.sample_output, 
                    GROUP_CONCAT(t.name) AS tags
                FROM problems p
                LEFT JOIN problem_solutions ps ON p.id = ps.problem_id
                LEFT JOIN problem_hints ph ON p.id = ph.problem_id
                LEFT JOIN problem_sample_IO pio ON p.id = pio.problem_id
                LEFT JOIN problem_tags pt ON p.id = pt.problem_id
                LEFT JOIN tags t ON pt.tag_id = t.id
                WHERE p.id = ?
                GROUP BY p.id, ps.id, ph.id, pio.id
            )";
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, problem_id);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (!res->next()) {
                return crow::response(404, "Problem not found");
            }
            nlohmann::json problem;
            nlohmann::json solutions = nlohmann::json::array();
            nlohmann::json hints = nlohmann::json::array();
            nlohmann::json sample_ios = nlohmann::json::array();
            std::set<std::string> tagsSet;

            do {
                // For tags, since they are concatenated into a single string, split them and insert into a set to ensure uniqueness
                std::string tagsConcatenated = res->getString("tags");
                std::istringstream tagsStream(tagsConcatenated);
                std::string tag;
                while (std::getline(tagsStream, tag, ',')) {
                    tagsSet.insert(tag);
                }

                // Assuming the structure of the result set allows for checking if the current row has a new solution, hint, or sample I/O
                if (res->getString("solution_title").length()) {
                    nlohmann::json solution;
                    solution["title"] = res->getString("solution_title");
                    solution["solution"] = res->getString("solution");
                    solutions.push_back(solution);
                }

                if (res->getString("hint_title").length()) {
                    nlohmann::json hint;
                    hint["title"] = res->getString("hint_title");
                    hint["hint"] = res->getString("hint");
                    hints.push_back(hint);
                }

                if (res->getString("sample_input").length() || res->getString("sample_output").length()) {
                    nlohmann::json sample_io;
                    sample_io["sample_input"] = res->getString("sample_input");
                    sample_io["sample_output"] = res->getString("sample_output");
                    sample_ios.push_back(sample_io);
                }
            } while (res->next());

            // Convert the tags set to a JSON array
            nlohmann::json tags = nlohmann::json::array();
            for (const auto& tag : tagsSet) {
                tags.push_back(tag);
            }

            problem["solutions"] = solutions;
            problem["hints"] = hints;
            problem["sample_ios"] = sample_ios;
            problem["tags"] = tags;

            return crow::response(200, problem.dump());
        } else if (req.method == "PUT"_method) {
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

