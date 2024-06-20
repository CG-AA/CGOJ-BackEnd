#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

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

void ROUTE_problem(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& sqlAPI, cache::lru_cache<int16_t, nlohmann::json>& problem_cache){
    CROW_ROUTE(app, "/problem/<int>")
    .methods("GET"_method)
    ([&settings, IP, &sqlAPI, &problem_cache](const crow::request& req, int problemId){
        nlohmann::json roles;
        try {
            std::string jwt = req.get_header_value("Authorization");
            verifyJWT(jwt, settings, IP);
            roles = getRoles(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }
        nlohmann::json problem;
        bool cache_hit = problem_cache.exists(problemId);
        if (!cache_hit) {
            roles.push_back("everyone");
            std::string query = "SELECT problems.* FROM problems JOIN problem_role ON problems.id = problem_role.problem_id WHERE problems.id = ? AND problem_role.role_name IN (";
            for (size_t i = 0; i < roles.size(); i++) {
                query += "?";
                if (i < roles.size() - 1) {
                    query += ", ";
                }
            }
            query += ")";

            std::unique_ptr<sql::PreparedStatement> pstmt = sqlAPI->prepareStatement(query);
            pstmt->setInt(1, problemId);
            for (size_t i = 0; i < roles.size(); i++) {
                pstmt->setString(i + 2, roles[i].get<std::string>());
            }
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) {
                problem["id"] = res->getInt("id");
                problem["title"] = res->getString("title");
                problem["description"] = res->getString("description");
                problem["input_format"] = res->getString("input_format");
                problem["output_format"] = res->getString("output_format");
                problem["difficulty"] = res->getString("difficulty");

                // Fetch problem role
                std::string roleQuery = "SELECT role_name, permission_flags FROM problem_role WHERE problem_id = ?";
                std::unique_ptr<sql::PreparedStatement> rolePstmt = sqlAPI->prepareStatement(roleQuery);
                rolePstmt->setInt(1, problemId);
                std::unique_ptr<sql::ResultSet> roleRes(rolePstmt->executeQuery());
                while (roleRes->next()) {
                    nlohmann::json role;
                    role[roleRes->getString("role_name")] = roleRes->getInt("permission_flags");
                    problem["roles"].push_back(role);
                }

                // Fetch sample input and output
                std::string sampleQuery = "SELECT sample_input, sample_output FROM problem_sample_IO WHERE problem_id = ?";
                std::unique_ptr<sql::PreparedStatement> samplePstmt = sqlAPI->prepareStatement(sampleQuery);
                samplePstmt->setInt(1, problemId);
                std::unique_ptr<sql::ResultSet> sampleRes(samplePstmt->executeQuery());
                nlohmann::json samples;
                while (sampleRes->next()) {
                    nlohmann::json sample;
                    sample["sample_input"] = sampleRes->getString("sample_input");
                    sample["sample_output"] = sampleRes->getString("sample_output");
                    samples.push_back(sample);
                }
                problem["samples"] = samples;

                // Fetch tags
                std::string tagQuery = "SELECT tag_name FROM problem_tags WHERE problem_id = ?";
                std::unique_ptr<sql::PreparedStatement> tagPstmt = sqlAPI->prepareStatement(tagQuery);
                tagPstmt->setInt(1, problemId);
                std::unique_ptr<sql::ResultSet> tagRes(tagPstmt->executeQuery());
                nlohmann::json tags;
                while (tagRes->next()) {
                    tags.push_back(tagRes->getString("tag_name"));
                }
                problem["tags"] = tags;

                // Fetch solutions
                bool permission = view_solutions_permission(settings, roles, problem);
                if(permission){
                    std::string solutionQuery = "SELECT title, solution FROM solutions WHERE problem_id = ?";
                    std::unique_ptr<sql::PreparedStatement> solutionPstmt = sqlAPI->prepareStatement(solutionQuery);
                    solutionPstmt->setInt(1, problemId);
                    std::unique_ptr<sql::ResultSet> solutionRes(solutionPstmt->executeQuery());
                    nlohmann::json solutions;
                    while (solutionRes->next()) {
                        nlohmann::json solution;
                        solution["title"] = solutionRes->getString("title");
                        solution["solution"] = solutionRes->getString("solution");
                        solutions.push_back(solution);
                    }
                    problem["solutions"] = solutions;
                }
                
                problem_cache.put(problemId, problem);
                return crow::response(200, problem);
            } else {
                return crow::response(404, "Problem not found");
            }
        } else {
            nlohmann::json problem_copy = problem_cache.get(problemId);
            bool permission = view_solutions_permission(settings, roles, problem_copy);
            if(permission){
                return crow::response(200, problem_copy);
            } else {
                problem_copy.erase("solutions");
                return crow::response(200, problem_copy);
            }
        }
    });
}