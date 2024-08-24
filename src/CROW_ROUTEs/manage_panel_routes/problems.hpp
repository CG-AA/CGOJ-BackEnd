#pragma once
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include "../../API/api.hpp"
#include "../../Programs/jwt.hpp"

#define badReq(reason) { \
    std::ostringstream oss; \
    oss << "{\"error\": \"" << reason << "\"}"; \
    API->rollbackTransaction(); \
    return crow::response(400, oss.str()); \
}    
namespace {
inline crow::response GET(const crow::request& req, std::string jwt, std::unique_ptr<APIs>& API) {
    u_int32_t problemsPerPage = 30, offset = 0, problemsCount = 0;
    if (req.url_params.get("problemsPerPage")) {
        problemsPerPage = std::stoi(req.url_params.get("problemsPerPage"));
    }
    if (req.url_params.get("offset")) {
        offset = std::stoi(req.url_params.get("offset"));
    }
    std::string query = "SELECT p.id, u.name AS owner_name, p.title, p.difficulty "
                        "FROM problems p "
                        "JOIN users u ON p.owner_id = u.id ";
    std::string countQuery = "SELECT COUNT(*) "
                            "FROM problems p "
                            "JOIN users u ON p.owner_id = u.id ";

    std::unique_ptr<sql::PreparedStatement> pstmt;
    std::unique_ptr<sql::PreparedStatement> countPstmt;

    //if the user is a site admin
    if(JWT::getSitePermissionFlags(jwt) & 1){
        //get all the problems
        query += "LIMIT ? OFFSET ?";
        pstmt = API->prepareStatement(query);
        pstmt->setInt(1, problemsPerPage);
        pstmt->setInt(2, offset);
        //get the count of all the problems
        countPstmt = API->prepareStatement(countQuery);
    } else {
        std::string roleFilter = "JOIN problem_role pr ON p.id = pr.problem_id "
                                "WHERE pr.role_name IN (";
        std::string permissionFilter = ") AND pr.permission_flags & 1 <> 0 ";
        //get the roles
        nlohmann::json roles = JWT::getRoles(jwt);
        //get the problems that the user has permission to modify
        query += roleFilter;
        countQuery += roleFilter;
        for (size_t i = 0; i < roles.size(); ++i) {
            query += "?";
            countQuery += "?";
            if (i < roles.size() - 1) {
                query += ", ";
                countQuery += ", ";
            }
        }
        query += permissionFilter;
        query += "LIMIT ? OFFSET ?";
        pstmt = API->prepareStatement(query);
        countPstmt = API->prepareStatement(countQuery);
        for (size_t i = 0; i < roles.size(); ++i) {
            pstmt->setString(i + 1, roles[i].get<std::string>());
            countPstmt->setString(i + 1, roles[i].get<std::string>());
        }
        pstmt->setInt(roles.size() + 1, problemsPerPage);
        pstmt->setInt(roles.size() + 2, offset);
    }
    nlohmann::json res, problems;
    try{
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while(res->next()) {
            nlohmann::json problem;
            problem["id"] = res->getInt("id");
            problem["owner_name"] = res->getString("owner_name");
            problem["title"] = res->getString("title");
            problem["difficulty"] = res->getInt("difficulty");
            problems.push_back(problem);
        }
        std::unique_ptr<sql::ResultSet> countRes(countPstmt->executeQuery());
        if(countRes->next()){
            problemsCount = countRes->getInt(1);
        } else {
            problemsCount = -1;
        }
    } catch (const std::exception& e) {
        return crow::response(500, "Internal server error");
    }
    nlohmann::json res;
    res["problems"] = problems;
    res["problemsCount"] = problemsCount;

    if (problems.size() == 0) {
        return crow::response(204, "No problems found.");
    } else {
        return crow::response(200, res.dump());
    }
}

inline crow::response POST(const crow::request& req, std::string jwt, std::unique_ptr<APIs>& API, const nlohmann::json& setting) {
    try {
        // Parse the request body
        nlohmann::json body = nlohmann::json::parse(req.body);
        // Validate difficulty
        std::string difficulty = body["problem"]["difficulty"].get<std::string>();
        try {
            if (std::find(setting["valid_difficulties"].begin(), setting["valid_difficulties"].end(), difficulty) == setting["valid_difficulties"].end()) {
                badReq("Invalid difficulty");
            }
        } catch (const std::exception& e) {
            badReq(e.what());
        }
        API->beginTransaction();

        // Insert the problem
        std::string query = R"(
        INSERT INTO problems (owner_id, title, description, input_format, output_format, difficulty)
        VALUES (?, ?, ?, ?, ?, ?);
        )";
        std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
        pstmt->setInt(1, JWT::getUserID(jwt));
        try {
            pstmt->setString(2, body["problem"]["title"].get<std::string>());
            pstmt->setString(3, body["problem"]["description"].get<std::string>());
            pstmt->setString(4, body["problem"]["input_format"].get<std::string>());
            pstmt->setString(5, body["problem"]["output_format"].get<std::string>());
            pstmt->setString(6, body["problem"]["difficulty"].get<std::string>());
            pstmt->execute();
        } catch (const std::exception& e) {
            badReq(e.what());
        }

        // Get the problem_id
        query = "SELECT id FROM problems WHERE title = ?;";
        pstmt = API->prepareStatement(query);
        pstmt->setString(1, body["problem"]["title"].get<std::string>());
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) {
            return crow::response(500, "Internal server error");
        }
        int problem_id = res->getInt("id");

        // Insert sample IO
        query = R"(
        INSERT INTO problem_sample_IO (problem_id, sample_input, sample_output)
        VALUES (?, ?, ?);
        )";
        try{
            for (const auto& sample : body["problem_sample_IO"]) {
                pstmt = API->prepareStatement(query);
                pstmt->setInt(1, problem_id);
                pstmt->setString(2, sample["input"].get<std::string>());
                pstmt->setString(3, sample["output"].get<std::string>());
                pstmt->execute();
            }
        } catch (const std::exception& e) {
            badReq(e.what());
        }

        // Insert tags
        query = R"(
        INSERT INTO problem_tags (problem_id, tag_name)
        VALUES (?, ?);
        )";
        //check if the tag exists
        try {
            for (const auto& tag : body["problem_tags"]) {
                pstmt = API->prepareStatement("SELECT * FROM tags WHERE name = ?;");
                pstmt->setString(1, tag.get<std::string>());
                res.reset(pstmt->executeQuery());
                if (!res->next()) {
                    // Insert the tag if it does not exist
                    try{
                        pstmt = API->prepareStatement("INSERT INTO tags (name) VALUES (?);");
                        pstmt->setString(1, tag.get<std::string>());
                        pstmt->execute();
                    } catch (const std::exception& e) {
                        badReq(e.what());
                    }
                }
            }
        } catch (const std::exception& e) {
            badReq(e.what());
        }
        try{
            for (const auto& tag : body["problem_tags"]) {
                pstmt = API->prepareStatement(query);
                pstmt->setInt(1, problem_id);
                pstmt->setString(2, tag.get<std::string>());
                pstmt->execute();
            }
        } catch (const std::exception& e) {
            badReq(e.what());
        }

        // Insert test cases
        query = R"(
        INSERT INTO problem_test_cases (problem_id, input, output, time_limit, memory_limit, score)
        VALUES (?, ?, ?, ?, ?, ?);
        )";
        try {
            int TTS = 0;
            for (const auto& testcase : body["problem_test_cases"]) {
                std::string I = testcase["input"].get<std::string>(),
                            O = testcase["output"].get<std::string>();
                int TL = testcase["time_limit"].get<int>(),
                    ML = testcase["memory_limit"].get<int>(),
                    S = testcase["score"].get<int>();
                if(TL < 0 || ML < 0 || S < 0){
                    badReq("Invalid test case");
                }
                TTS += S;
                pstmt = API->prepareStatement(query);
                pstmt->setInt(1, problem_id);
                pstmt->setString(2, I);
                pstmt->setString(3, O);
                pstmt->setInt(4, TL);
                pstmt->setInt(5, ML);
                pstmt->setInt(6, S);
                pstmt->execute();
            }
            if(TTS != 10000){
                badReq("Total test case score must be 10000");
            }
        } catch (const std::exception& e) {
            badReq(e.what());
        }

        // Insert roles
        query = R"(
        INSERT INTO problem_role (problem_id, role_name, permission_flags)
        VALUES (?, ?, ?);
        )";
        //check if the role exists
        try {
            for (const auto& role : body["problem_roles"]) {
                pstmt = API->prepareStatement("SELECT * FROM roles WHERE name = ?;");
                pstmt->setString(1, role["role_name"].get<std::string>());
                res.reset(pstmt->executeQuery());
                if (!res->next()) {
                    // Insert the role if it does not exist
                    try{
                        pstmt = API->prepareStatement("INSERT INTO roles (name) VALUES (?);");
                        pstmt->setString(1, role["role_name"].get<std::string>());
                        pstmt->execute();
                    } catch (const std::exception& e) {
                        badReq(e.what());
                    }
                }
            }
        } catch (const std::exception& e) {
            badReq(e.what());
        }
        try {
            for (const auto& role : body["problem_roles"]) {
                pstmt = API->prepareStatement(query);
                pstmt->setInt(1, problem_id);
                pstmt->setString(2, role["role_name"].get<std::string>());
                pstmt->setInt(3, role["permission_flags"].get<int>());
                pstmt->execute();
            }
        } catch (const std::exception& e) {
            badReq(e.what());
        }

        API->commitTransaction();
        return crow::response(200, R"({"message": "Problem created successfully"})");
    } catch (const std::exception& e) {
        CROW_LOG_ERROR << "Exception occurred: " << e.what();
        API->rollbackTransaction();
        return crow::response(500, R"({"error": "Internal server error"})");
    }
}
}// namespace

inline void problemsRoute (crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API) {
    CROW_ROUTE(app, "/manage_panel/problems")
    .methods("GET"_method, "POST"_method)
    ([&settings, &API, IP](const crow::request& req){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        try {
            JWT::verifyJWT(jwt, settings, IP);
        } catch (const std::exception& e) {
            CROW_LOG_INFO << e.what();
            return crow::response(401, "Unauthorized");
        }
        if (req.method == "GET"_method) {
            return GET(req, jwt, API);
        } else /*if (req.method == "POST"_method)*/ {
            return POST(req, jwt, API, settings);
        }

    });
}//problemsRoute