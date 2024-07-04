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
}

nlohmann::json get_problem(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = 
        "SELECT "
        "p.*, "
        "psio.sample_input, "
        "psio.sample_output, "
        "pt.tag_name, "
        "ptc.input AS test_case_input, "
        "ptc.output AS test_case_output, "
        "ptc.time_limit, "
        "ptc.memory_limit, "
        "s.title AS solution_title, "
        "s.solution, "
        "h.hint, "
        "h.context, "
        "pr.role_name, "
        "pr.permission_flags "
        "FROM problems p "
        "LEFT JOIN problem_sample_IO psio ON p.id = psio.problem_id "
        "LEFT JOIN problem_tags pt ON p.id = pt.problem_id "
        "LEFT JOIN problem_test_cases ptc ON p.id = ptc.problem_id "
        "LEFT JOIN solutions s ON p.id = s.problem_id "
        "LEFT JOIN hints h ON p.id = h.problem_id "
        "LEFT JOIN problem_role pr ON p.id = pr.problem_id "
        "WHERE p.id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json problem;
    problem["id"] = res->getInt("id");
    problem["name"] = res->getString("name");
    problem["difficulty"] = res->getString("difficulty");
    problem["description"] = res->getString("description");
    problem["sample_input"] = res->getString("sample_input");
    problem["sample_output"] = res->getString("sample_output");
    problem["tag_name"] = res->getString("tag_name");
    problem["test_case_input"] = res->getString("test_case_input");
    problem["test_case_output"] = res->getString("test_case_output");
    problem["time_limit"] = res->getInt("time_limit");
    problem["memory_limit"] = res->getInt("memory_limit");
    problem["solution_title"] = res->getString("solution_title");
    problem["solution"] = res->getString("solution");
    problem["hint"] = res->getString("hint");
    problem["context"] = res->getString("context");
    problem["roles"][res->getString("role_name")] = res->getInt("permission_flags");
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