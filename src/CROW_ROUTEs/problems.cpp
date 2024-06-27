/**
 * @file problems.cpp
 * @brief Implementation of the problems route.
 */
#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

void ROUTE_problems(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API, cache::lru_cache<int8_t, nlohmann::json>& problems_everyone_cache, std::atomic<bool>& problems_everyone_cache_hit){
    CROW_ROUTE(app, "/problems")
    .methods("GET"_method)
    ([&settings, IP, &API, &problems_everyone_cache, &problems_everyone_cache_hit](const crow::request& req){
        nlohmann::json roles;
        try {
            std::string jwt = req.get_header_value("Authorization");
            verifyJWT(jwt, settings, IP);
            roles = getRoles(jwt);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }
        // Handle page query parameter
        int page = 1, problemsPerPage = 10;
        if (req.url_params.get("page")) {
            page = std::stoi(req.url_params.get("page"));
        }
        if (req.url_params.get("problemsPerPage")) {
            problemsPerPage = std::stoi(req.url_params.get("problemsPerPage"));
        }
        int offset = (page - 1) * problemsPerPage;

        nlohmann::json problems;

        if (roles.size() || offset+problemsPerPage > 30) {
            roles.push_back("everyone");
            std::string query = "SELECT problems.* FROM problems JOIN problem_role ON problems.id = problem_role.problem_id WHERE problem_role.role_name IN (";
            for (size_t i = 0; i < roles.size(); i++) {
                query += "?";
                if (i < roles.size() - 1) {
                    query += ", ";
                }
            }
            query += ")";
            query += " LIMIT " + std::to_string(problemsPerPage) + " OFFSET " + std::to_string(offset);
            std::unique_ptr<sql::PreparedStatement> pstmt = API->prepareStatement(query);
            for (size_t i = 0; i < roles.size(); i++) {
                pstmt->setString(i + 1, roles[i].get<std::string>());
            }
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                nlohmann::json problem;
                problem["id"] = res->getInt("id");
                problem["name"] = res->getString("name");
                problem["difficulty"] = res->getString("difficulty");
                problems.push_back(problem);
            }
        } else if(!problems_everyone_cache_hit) {
            std::string query = "SELECT problems.* FROM problems JOIN problem_role ON problems.id = problem_role.problem_id WHERE problem_role.role_name IN ( ? ) LIMIT 30";
            std::unique_ptr<sql::PreparedStatement> pstmt = API->prepareStatement(query);
            pstmt->setString(1, "everyone");
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                nlohmann::json problem;
                problem["id"] = res->getInt("id");
                problem["name"] = res->getString("name");
                problem["difficulty"] = res->getString("difficulty");
                problems.push_back(problem);
                problems_everyone_cache.put(res->getInt("id"), problem);
            }
            problems_everyone_cache_hit = true;
        } else {
            for (int i = offset; i < offset + problemsPerPage; i++) {
                problems.push_back(problems_everyone_cache.get(i));
            }
        }


        if (problems.size() == 0) {
            return crow::response(404, "No problems found");
        } else {
            return crow::response(200, problems);
        }
    });
}