#include "manage_panel.hpp"
#include "../Programs/jwt.hpp"

namespace {
bool isLogin(std::string jwt, nlohmann::json& settings, std::string IP){
    try {
        verifyJWT(jwt, settings, IP);
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}
//if the user is not a site admin and dont got the permission
bool isPermissioned (std::string jwt, int problem_id, std::unique_ptr<APIs>& API) {
    if(!(getSitePermissionFlags(jwt) & 1)){
        nlohmann::json roles = getRoles(jwt);
        std::string query = "SELECT * FROM problem_role WHERE problem_id = ? AND role_name IN (";
        for (size_t i = 0; i < roles.size(); ++i) {
            query += "?";
            if (i < roles.size() - 1) query += ", ";
        }
        query += ") AND permission_flags & 1 <> 0";
        std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
        pstmt->setInt(1, problem_id);
        for (size_t i = 0; i < roles.size(); ++i) {
            pstmt->setString(i + 2, roles[i].get<std::string>());
        }
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) {
            return false;
        }
    }
    return true;
}
}

void ROUTE_manage_panel(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API){
    CROW_ROUTE(app, "/manage_panel/problems")
    .methods("GET"_method)
    ([&settings, &API, &IP](const crow::request& req){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        if (!isLogin(jwt, settings, IP)) {
            return crow::response(401, "Unauthorized");
        }

        u_int32_t problemsPerPage = 30, offset = 0;
        if (req.url_params.get("problemsPerPage")) {
            problemsPerPage = std::stoi(req.url_params.get("problemsPerPage"));
        }
        if (req.url_params.get("offset")) {
            offset = std::stoi(req.url_params.get("offset"));
        }
        std::string query = "SELECT p.id, u.name AS owner_name, p.title, p.difficulty "
                            "FROM problems p "
                            "JOIN users u ON p.owner_id = u.id ";
        std::unique_ptr<sql::PreparedStatement> pstmt;

        //if the user is a site admin
        if(getSitePermissionFlags(jwt) & 1){
            //get all the problems
            query += "LIMIT ? OFFSET ?";
            pstmt = API->prepareStatement(query);
            pstmt->setInt(1, problemsPerPage);
            pstmt->setInt(2, offset);
        } else {
            //get the roles
            nlohmann::json roles = getRoles(jwt);
            //get the problems that the user has permission to modify
            query += "JOIN problem_role pr ON p.id = pr.problem_id "
                    "WHERE pr.role_name IN (";
            for (size_t i = 0; i < roles.size(); ++i) {
                query += "?";
                if (i < roles.size() - 1) query += ", ";
            }
            query += ") AND pr.permission_flags & 1 <> 0 "
                    "LIMIT ? OFFSET ?";
            pstmt = API->prepareStatement(query);
            for (size_t i = 0; i < roles.size(); ++i) {
                pstmt->setString(i + 1, roles[i].get<std::string>());
            }
            pstmt->setInt(roles.size() + 1, problemsPerPage);
            pstmt->setInt(roles.size() + 2, offset);
        }

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        nlohmann::json problems;
        while(res->next()) {
            nlohmann::json problem;
            problem["id"] = res->getInt("id");
            problem["owner_name"] = res->getString("owner_name");
            problem["title"] = res->getString("title");
            problem["difficulty"] = res->getInt("difficulty");
            problems.push_back(problem);
        }
    });
    CROW_ROUTE(app, "/manage_panel/problems/<int>")
    .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method)
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
            // Assuming the initial part of fetching the problem details remains the same
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
        } else if (req.method == "POST"_method) {   // insert a new problem
            //get body
            nlohmann::json body = nlohmann::json::parse(req.body);
            //insert the problem
            std::string query = R"(
            INSERT INTO problems (owner_id, title, description, input_format, output_format, difficulty)
            VALUES (?, ?, ?, ?, ?, ?);
            )";
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, getUserID(jwt));
            pstmt->setString(2, body["title"].get<std::string>());
            pstmt->setString(3, body["description"].get<std::string>());
            pstmt->setString(4, body["input_format"].get<std::string>());
            pstmt->setString(5, body["output_format"].get<std::string>());
            //difficulty must be one of ["Easy", "Medium", "Hard]
            std::string difficulty = body["difficulty"].get<std::string>();
            if (difficulty != "Easy" && difficulty != "Medium" && difficulty != "Hard") {
                return crow::response(400, "Invalid difficulty");
            }
            pstmt->setString(6, difficulty);
            pstmt->execute();
            //insert sample IO
            //two json arrays: sample_inputs and sample_outputs
            nlohmann::json sample_inputs = body["sample_inputs"];
            nlohmann::json sample_outputs = body["sample_outputs"];
            if (sample_inputs.size() != sample_outputs.size()) {
                return crow::response(400, "Sample inputs and outputs must have the same size");
            }
            query = R"(
            INSERT INTO problem_sample_IO (problem_id, sample_input, sample_output)
            VALUES (?, ?, ?);
            )";
            for (size_t i = 0; i < sample_inputs.size(); ++i) {
                std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
                pstmt->setInt(1, problem_id);
                pstmt->setString(2, sample_inputs[i].get<std::string>());
                pstmt->setString(3, sample_outputs[i].get<std::string>());
                pstmt->execute();
            }
            //insert tags
            nlohmann::json tags = body["tags"];
            query = R"(
            INSERT INTO problem_tags (problem_id, tag_id)
            VALUES (?, ?);
            )";
            try {
                for (const auto& tag : tags) {
                    std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
                    pstmt->setInt(1, problem_id);
                    pstmt->setInt(2, tag.get<int>());
                    pstmt->execute();
                }
            } catch (const std::exception& e) {
                return crow::response(400, "Invalid tag");
            }
            //insert test cases
            
            nlohmann::json testcases = body["testcases"];
            query = R"(
            INSERT INTO problem_test_cases (problem_id, input, output, time_limit, memory_limit, score)
            VALUES (?, ?, ?, ?, ?, ?);
            )";
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

        } else if (req.method == "PUT"_method) {
            // update the problem
        } else if (req.method == "DELETE"_method) {
            // delete the problem
        }
    });
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
        } else if (req.method == "PUT"_method) {
        } else if (req.method == "DELETE"_method) {
        }
    });
}

