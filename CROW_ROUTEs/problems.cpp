#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

// To get the problems(and their basic info) that the user has access to
void ROUTE_problems(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& sqlAPI){
    CROW_ROUTE(app, "/problems")
    .methods("GET"_method)
    ([&settings, IP, &sqlAPI](const crow::request& req){
        nlohmann::json roles;
        try {
            std::string jwt = req.get_header_value("Authorization");
            verifyJWT(jwt, settings, IP);
            roles = getRoles(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }
        roles.push_back("everyone");
        std::string query = "SELECT problems.* FROM problems JOIN problem_role ON problems.id = problem_role.problem_id WHERE problem_role.role_name IN (";
        for (size_t i = 0; i < roles.size(); i++) {
            query += "?";
            if (i < roles.size() - 1) {
                query += ", ";
            }
        }
        query += ")";

        // Handle page query parameter
        int page = 1, problemsPerPage = 10;
        if (req.url_params.get("page")) {
            page = std::stoi(req.url_params.get("page"));
        }
        if (req.url_params.get("problemsPerPage")) {
            problemsPerPage = std::stoi(req.url_params.get("problemsPerPage"));
        }
        int offset = (page - 1) * problemsPerPage; // assuming 10 problems per page
        query += " LIMIT " + std::to_string(problemsPerPage) + " OFFSET " + std::to_string(offset);

        std::unique_ptr<sql::PreparedStatement> pstmt = sqlAPI->prepareStatement(query);
        for (size_t i = 0; i < roles.size(); i++) {
            pstmt->setString(i + 1, roles[i].get<std::string>());
        }
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        nlohmann::json problems;
        while (res->next()) {
            nlohmann::json problem;
            problem["id"] = res->getInt("id");
            problem["name"] = res->getString("name");
            problem["difficulty"] = res->getString("difficulty");
            problems.push_back(problem);
        }
        if (problems.size() == 0) {
            return crow::response(404, "No problems found");
        } else {
            return crow::response(200, problems);
        }
    });
}