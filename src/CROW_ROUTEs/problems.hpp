/**
 * @file problems.hpp
 * @brief Declaration of the problems route.
 */
#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"
#include "../include/lrucache.hpp"

/**
 * @brief Configures a route for accessing a list of problems.
 * 
 * This function sets up a route "/problems" on the provided Crow application instance. It handles GET requests to fetch a list of problems, supporting pagination and role-based filtering. The function performs JWT verification, role extraction, and conditionally queries the database or cache based on the roles and pagination parameters.
 * 
 * @param app Reference to the Crow application instance configured with CORSHandler middleware.
 * @param settings JSON object containing configuration settings, used for JWT verification and role-based access control.
 * @param IP String representing the IP address for additional security checks in JWT verification.
 * @param API Unique pointer to an APIs instance, used for database operations.
 * @param problems_everyone_cache Reference to an LRU cache instance for caching problems accessible to everyone.
 * @param problems_everyone_cache_hit Atomic boolean flag indicating whether the cache for problems accessible to everyone has been populated.
 * 
 * The function begins by verifying the JWT from the request header and extracting roles. It then processes query parameters for pagination. Based on the roles and pagination, it either queries the database for problems or retrieves them from the cache. The function supports a special case where problems accessible to everyone are cached to improve performance. It returns a JSON response with the list of problems or an error message if no problems are found.
 */
void ROUTE_problems(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API, cache::lru_cache<int8_t, nlohmann::json>& problems_everyone_cache, std::atomic<bool>& problems_everyone_cache_hit);