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
crow::response GET(const crow::request& req, std::string jwt, std::unique_ptr<APIs>& API, int problem_id) {
    try{
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
    } catch (const std::exception& e) {
        badReq(e.what());
    }
}//GET
crow::response POST(const crow::request& req, std::string jwt, std::unique_ptr<APIs>& API, int problem_id) {
    try{
    API->beginTransaction();
    //replace all the testcases
    std::string query = "DELETE FROM problem_test_cases WHERE problem_id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
    pstmt->setInt(1, problem_id);
    pstmt->execute();
    nlohmann::json testcases = nlohmann::json::parse(req.body);
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
    API->commitTransaction();
    return crow::response(200, "Test cases updated");
    } catch (const std::exception& e) {
        badReq(e.what());
    }
}//POST
}//namespace

inline void testcaseRoute(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API, std::unique_ptr<APIs>& modifyAPI) {
    CROW_ROUTE(app, "/manage_panel/problems/<int>/testcases")
    .methods("GET"_method, "POST"_method, "PUT"_method)
    ([&settings, &API, &modifyAPI, IP](const crow::request& req, int problem_id){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        try {
            JWT::verifyJWT(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, "Unauthorized");
        }
        if (!JWT::isPermissioned(jwt, problem_id, API, settings["permission_flags"]["problems"]["edit"].get<int>())) {
            return crow::response(403, "Forbidden");
        }
        if (req.method == "GET"_method) {
            return GET(req, jwt, API, problem_id);
        } else if (req.method == "POST"_method) {
            return POST(req, jwt, modifyAPI, problem_id);
        }
    });
}//testcaseRoute