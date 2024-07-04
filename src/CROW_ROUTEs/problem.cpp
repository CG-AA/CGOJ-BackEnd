/**
 * @file problem.cpp
 * @brief Implementation of the problem route.
 */
#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

namespace{
bool have_permisson(int user_permission, std::string permission_name, nlohmann::json& settings){
    try {
        int index = settings["permission_flags"]["problems"][permission_name];
        return (user_permission & (1 << index)) != 0;
    } catch (const std::exception& e) {
        throw std::runtime_error("Permission not found");
    }
}

bool view_solutions_permission(nlohmann::json& settings, nlohmann::json& roles, nlohmann::json& problem){
    for(auto i : problem["roles"]){
        if(have_permisson(i.begin().value(), "view_solutions", settings) && roles.contains(i.begin().key())){
            return true;
        }
    }
    return false;
}

nlohmann::json get_problem(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = "SELECT * FROM problems WHERE id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        nlohmann::json problem;
        problem["id"] = res->getInt("id");
        problem["owner_id"] = res->getInt("owner_id");
        problem["name"] = res->getString("name");
        problem["description"] = res->getString("description");
        problem["input_format"] = res->getString("input_format");
        problem["output_format"] = res->getString("output_format");
        problem["difficulty"] = res->getString("difficulty");
        return problem;
    } else {
        throw std::runtime_error("Problem not found");
    }
}

nlohmann::json get_problem_details(std::unique_ptr<APIs>& sqlAPI, int problemId) {

}
}



void ROUTE_problem(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& sqlAPI, cache::lru_cache<int16_t, nlohmann::json>& problem_cache){
    CROW_ROUTE(app, "/problem/<int>")
    .methods("GET"_method)
    ([&settings, IP, &sqlAPI, &problem_cache](const crow::request& req, int problemId){
        nlohmann::json roles;
        try {
            std::string jwt = req.get_header_value("Authorization");
            if (jwt != "null") {
                verifyJWT(jwt, settings, IP);
                roles = getRoles(jwt);
            }
        } catch (const std::exception& e) {
        }
        //do a cache hit
        if(problem_cache.exists(problemId)){
            nlohmann::json problem = problem_cache.get(problemId);
            if(!view_solutions_permission(settings, roles, problem))
                problem.erase("solutions");
            return crow::response(200, problem_cache.get(problemId));
        }


    });
}