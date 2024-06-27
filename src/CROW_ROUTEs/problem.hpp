/**
 * @file problem.hpp
 * @brief Header file for the problem route.
 * 
 */
#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../include/lrucache.hpp"
#include "../API/api.hpp"

namespace {
/**
 * @brief Checks if the user has the specified permission.
 * 
 * @param user_permission The permission flags of the user.
 * @param permission_name The name of the permission to check.
 * @param settings The application settings JSON object.
 * @return true if the user has the permission, false otherwise.
 * 
 * @throws std::runtime_error if the permission is not found in the settings.
 * 
 * @note The function uses bitwise operations to check the permission flags.
 * @note only accessable by the same file
 */
bool have_permisson(int user_permission, std::string permission_name, nlohmann::json& settings);

/**
 * @brief Checks if the user has permission to view solutions for a problem.
 * 
 * @param settings The application settings JSON object.
 * @param roles The roles of the user.
 * @param problem The problem JSON object.
 * @return true if the user has permission to view solutions, false otherwise.
 * 
 * @note The function checks the permissions for each role associated with the problem.
 * @note only accessable by the same file
 */
bool view_solutions_permission(nlohmann::json& settings, nlohmann::json& roles, nlohmann::json& problem);
}

/**
 * @brief Configures a route for accessing problem details.
 * 
 * This function sets up a route "/problem/<int>" on the provided Crow application instance. It handles GET requests to fetch problem details based on the problem ID. The function checks for authorization, validates roles, and retrieves problem details from a cache or database. It also handles permissions for viewing solutions.
 * 
 * @param app Reference to the Crow application instance configured with CORSHandler middleware.
 * @param settings JSON object containing configuration settings, used for JWT verification and role-based access control.
 * @param IP String representing the IP address for additional security checks in JWT verification.
 * @param sqlAPI Unique pointer to an APIs instance, used for database operations.
 * @param problem_cache Reference to an LRU cache instance for caching problem details.
 */
void ROUTE_problem(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& sqlAPI, cache::lru_cache<int16_t, nlohmann::json>& problem_cache);