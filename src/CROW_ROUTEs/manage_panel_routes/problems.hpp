#pragma once
#include "../manage_panel.hpp"

namespace {
crow::response GET(const crow::request& req, std::string jwt, std::unique_ptr<APIs>& API) {
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
    if(JWT::getSitePermissionFlags(jwt) & 1){
        //get all the problems
        query += "LIMIT ? OFFSET ?";
        pstmt = API->prepareStatement(query);
        pstmt->setInt(1, problemsPerPage);
        pstmt->setInt(2, offset);
    } else {
        //get the roles
        nlohmann::json roles = JWT::getRoles(jwt);
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
    return crow::response(200, problems.dump());
}

crow::response POST(const crow::request& req, std::string jwt, std::unique_ptr<APIs>& API) {
    try {
        // Parse the request body
        nlohmann::json body = nlohmann::json::parse(req.body);

        // Validate difficulty
        std::string difficulty = body["difficulty"].get<std::string>();
        if (difficulty != "Easy" && difficulty != "Medium" && difficulty != "Hard") {
            return crow::response(400, "Invalid difficulty");
        }

        // Insert the problem
        std::string query = R"(
        INSERT INTO problems (owner_id, title, description, input_format, output_format, difficulty)
        VALUES (?, ?, ?, ?, ?, ?);
        )";
        std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
        pstmt->setInt(1, JWT::getUserID(jwt));
        pstmt->setString(2, body["title"].get<std::string>());
        pstmt->setString(3, body["description"].get<std::string>());
        pstmt->setString(4, body["input_format"].get<std::string>());
        pstmt->setString(5, body["output_format"].get<std::string>());
        pstmt->setString(6, difficulty);
        pstmt->execute();

        // Get the problem_id
        query = "SELECT id FROM problems WHERE title = ?;";
        pstmt = API->prepareStatement(query);
        pstmt->setString(1, body["title"].get<std::string>());
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) {
            return crow::response(500, "Internal server error");
        }
        int problem_id = res->getInt("id");

        // Insert sample IO
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
            pstmt = API->prepareStatement(query);
            pstmt->setInt(1, problem_id);
            pstmt->setString(2, sample_inputs[i].get<std::string>());
            pstmt->setString(3, sample_outputs[i].get<std::string>());
            pstmt->execute();
        }

        // Insert tags
        nlohmann::json tags = body["tags"];
        query = R"(
        INSERT INTO problem_tags (problem_id, tag_id)
        VALUES (?, ?);
        )";
        for (const auto& tag : tags) {
            pstmt = API->prepareStatement(query);
            pstmt->setInt(1, problem_id);
            pstmt->setInt(2, tag.get<int>());
            pstmt->execute();
        }

        // Insert test cases
        nlohmann::json testcases = body["testcases"];
        int total_score = 0;
        for (const auto& testcase : testcases) {
            if (testcase["time_limit"].get<int>() <= 0 || testcase["memory_limit"].get<int>() <= 0 || testcase["score"].get<int>() <= 0) {
                return crow::response(400, "Invalid test case");
            }
            total_score += testcase["score"].get<int>();
        }
        if (total_score != 10000) {
            return crow::response(400, "Total score of test cases must be 10000");
        }
        query = R"(
        INSERT INTO problem_test_cases (problem_id, input, output, time_limit, memory_limit, score)
        VALUES (?, ?, ?, ?, ?, ?);
        )";
        for (const auto& testcase : testcases) {
            pstmt = API->prepareStatement(query);
            pstmt->setInt(1, problem_id);
            pstmt->setString(2, testcase["input"].get<std::string>());
            pstmt->setString(3, testcase["output"].get<std::string>());
            pstmt->setInt(4, testcase["time_limit"].get<int>());
            pstmt->setInt(5, testcase["memory_limit"].get<int>());
            pstmt->setInt(6, testcase["score"].get<int>());
            pstmt->execute();
        }

        // Insert roles
        nlohmann::json roles = body["roles"];
        query = R"(
        INSERT INTO problem_role (problem_id, role_name, permission_flags)
        VALUES (?, ?, ?);
        )";
        for (const auto& role : roles) {
            pstmt = API->prepareStatement(query);
            pstmt->setInt(1, problem_id);
            pstmt->setString(2, role["role_name"].get<std::string>());
            pstmt->setInt(3, role["permission_flags"].get<int>());
            pstmt->execute();
        }

        return crow::response(200, "Problem created");
    } catch (const std::exception& e) {
        return crow::response(500, std::string("Internal server error: ") + e.what());
    }
}
}// namespace

void problemsRoute (crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API) {
    CROW_ROUTE(app, "/manage_panel/problems")
    .methods("GET"_method, "POST"_method)
    ([&settings, &API, &IP](const crow::request& req){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        if (!isLogin(jwt, settings, IP)) {
            return crow::response(401, "Unauthorized");
        }
        if (req.method == "GET"_method) {
            return GET(req, jwt, API);
        } else /*if (req.method == "POST"_method)*/ {
            return POST(req, jwt, API);
        }

    });
}//problemsRoute