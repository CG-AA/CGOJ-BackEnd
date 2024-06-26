/**
 * @file problems.hpp
 * Header file for defining the route for fetching a list of problems accessible to the user in a coding problems platform.
 * 
 * This file declares the setup for a route that handles fetching a paginated list of problems based on user roles and permissions. It utilizes JWT for authentication, checks user roles, and fetches problems from either a cache or the database.
 * 
 * Dependencies:
 * - crow.h: Main header file for the CROW web server library, used for routing.
 * - crow/middlewares/cors.h: Header file for CORS middleware in CROW, enabling cross-origin requests.
 * - nlohmann/json.hpp: Header file for the JSON library for modern C++, used for JSON manipulation.
 * - api.hpp: Header file for the API interface, used for database interactions.
 * - lrucache.hpp: Header file for the LRU (Least Recently Used) cache implementation, used for caching problem details.
 * 
 * Function Declarations:
 * - ROUTE_problems: Declares the function for setting up the route to fetch a list of problems. It requires the CROW application object, application settings, client IP, API object, problems cache, and a cache hit flag as parameters.
 * 
 * Usage:
 * Include this header file in the server application to set up and handle the route for fetching a list of problems. Ensure all dependencies are properly included and linked in the project.
 */
#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"
#include "../include/lrucache.hpp"

/**
 * @brief Declares the route for fetching a list of problems accessible to the user.
 * 
 * This function sets up a GET route at "/problems" in a coding problems platform. It is designed to handle fetching a paginated list of problems based on user roles and permissions. The route utilizes JWT for authentication, checks user roles, and fetches problems from either a cache or the database, aiming to optimize response times and reduce database load.
 * 
 * @param app Reference to the CROW application object, used to define the route.
 * @param settings Reference to a JSON object containing application settings, including database and authentication configurations.
 * @param IP String representing the client's IP address, used for logging and security checks.
 * @param API Unique pointer to the APIs object, facilitating database interactions for fetching problem details.
 * @param problems_everyone_cache Reference to an LRU cache object, used for caching problem details accessible to all users to enhance performance.
 * @param problems_everyone_cache_hit Atomic boolean flag indicating whether the cache for problems accessible to everyone has been populated, ensuring cache is only filled once to avoid unnecessary database queries.
 * 
 * The function integrates with the CROW web server library for routing, nlohmann::json for JSON manipulation, and custom caching mechanisms to efficiently serve problem lists to users based on their permissions.
 * 
 * Usage:
 * This function should be called during the server initialization phase to set up the "/problems" route. It requires proper initialization of all parameters, including the application object, settings, API object, and cache.
 */
void ROUTE_problems(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API, cache::lru_cache<int8_t, nlohmann::json>& problems_everyone_cache, std::atomic<bool>& problems_everyone_cache_hit);