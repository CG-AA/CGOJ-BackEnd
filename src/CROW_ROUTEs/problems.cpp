/**
 * @file problems.cpp
 * @brief Implementation of the problems route.
 */
#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

nlohmann::json getProblems(std::unique_ptr<APIs>& API, std::vector<std::string> roles, int problemsPerPage, int offset) {
    nlohmann::json problems;
    std::string query = "SELECT problems.* FROM problems JOIN problem_role ON problems.id = problem_role.problem_id WHERE problem_role.role_name IN (";
    int i = 0;
    for (const std::string& role : roles) {
        query += "?";
        if (i < roles.size() - 1) {
            query += ", ";
        }
        i++;
    }
    query += ") LIMIT ? OFFSET ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
    for (i = 0; i < roles.size(); i++) {
        pstmt->setString(i + 1, roles[i]);
    }
    pstmt->setInt(roles.size() + 1, problemsPerPage);
    pstmt->setInt(roles.size() + 2, offset);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
        nlohmann::json problem;
        problem["id"] = res->getInt("id");
        problem["title"] = res->getString("title");
        problem["difficulty"] = res->getString("difficulty");
        problems.push_back(problem);
    }
    return problems;
}

void ROUTE_problems(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API, cache::lru_cache<int8_t, nlohmann::json>& problems_everyone_cache, std::atomic<bool>& problems_everyone_cache_hit){
    CROW_ROUTE(app, "/problems")
    .methods("GET"_method)
    ([&settings, IP, &API, &problems_everyone_cache, &problems_everyone_cache_hit](const crow::request& req){
        nlohmann::json roles;
        try {
            std::string jwt = req.get_header_value("Authorization");
            if (jwt != "null") {
                verifyJWT(jwt, settings, IP);
                roles = getRoles(jwt);
            }
        } catch (const std::exception& e) {
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

        if (roles.size() || (offset+problemsPerPage > problems_everyone_cache.size() && problems_everyone_cache_hit)) {
            problems = getProblems(API, roles, problemsPerPage, offset);
        } else if(!problems_everyone_cache_hit) {
            problems = getProblems(API, {"everyone"}, problemsPerPage, offset);
            if(!problems.empty()) {
                problems_everyone_cache_hit = true;
            }
        } else {
            for (int i = offset; i < offset + problemsPerPage; i++) {
                problems.push_back(problems_everyone_cache.get(i));
            }
        }

        CROW_LOG_INFO << "Problems: " << problems.dump();
        if (problems.size() == 0) {
            return crow::response(204, "No problems found.");
        } else {
            return crow::response(200, problems.dump());
        }
    });
}