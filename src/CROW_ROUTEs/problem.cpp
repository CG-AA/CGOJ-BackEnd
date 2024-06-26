/**
 * @file problem.cpp
 * This file contains the implementation of the problem-related routes and permissions for a coding problems platform.
 * It utilizes the CROW web server library for routing and nlohmann::json for JSON manipulation.
 *
 * Functions:
 * - have_permission: Checks if a user has a specific permission based on their permission flags.
 * - view_solutions_permission: Determines if the roles associated with a problem allow viewing its solutions.
 * - ROUTE_problem: Defines the route for fetching problem details, including title, description, samples, tags, and solutions if permitted.
 *
 * Dependencies:
 * - problems.hpp: Header file for problem-related declarations.
 * - ../Programs/jwt.hpp: Header file for JWT (JSON Web Token) related functions.
 * - jwt-cpp/jwt.h: JWT library for C++.
 * - nlohmann/json.hpp: JSON library for modern C++.
 *
 * Usage:
 * This file is part of a backend server application. It should be compiled with the rest of the project and run on a server.
 * The ROUTE_problem function sets up an endpoint that responds to GET requests with problem details based on the problem ID provided in the URL.
 * Permissions are checked to determine if solutions should be included in the response.
 *
 * Example:
 * GET /problem/1 -> Fetches details for problem with ID 1. If the requester has permission, solutions will be included in the response.
 */
#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

/**
 * @brief Checks if a user has a specific permission.
 * 
 * @param user_permission The permission flags of the user as an integer.
 * @param permission_name The name of the permission to check.
 * @param settings A JSON object containing the mapping of permission names to their respective flags.
 * @return true If the user has the specified permission.
 * @return false If the user does not have the specified permission.
 * @throws std::runtime_error If the permission name is not found in the settings.
 */
bool have_permisson(int user_permission, std::string permission_name, nlohmann::json& settings){
    try {
        int index = settings["permission_flags"]["problems"][permission_name];
        return (user_permission & (1 << index)) != 0;
    } catch (const std::exception& e) {
        throw std::runtime_error("Permission not found");
    }
}

/**
 * @brief Determines if the roles associated with a problem allow viewing its solutions.
 * 
 * @param settings A JSON object containing the application settings, including permissions.
 * @param roles A JSON object containing the roles of the current user.
 * @param problem A JSON object containing the problem details, including roles with access.
 * @return true If viewing solutions is permitted for at least one of the user's roles.
 * @return false If none of the user's roles permit viewing solutions.
 */
bool view_solutions_permission(nlohmann::json& settings, nlohmann::json& roles, nlohmann::json& problem){
    for(auto i : problem["roles"]){
        if(have_permisson(i.begin().value(), "view_solutions", settings) && roles.contains(i.begin().key())){
            return true;
        }
    }
    return false;
}

/**
 * @brief Sets up the route for fetching problem details.
 * 
 * This function defines a GET route for retrieving details of a specific problem by its ID.
 * It checks for authorization, fetches problem details from the database or cache, and determines if solutions should be included based on permissions.
 * 
 * @param app The CROW application object used to define the route.
 * @param settings A JSON object containing application settings.
 * @param IP The IP address of the client making the request.
 * @param sqlAPI A pointer to the SQL API object for database operations.
 * @param problem_cache A cache object for storing and retrieving problem details.
 */
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