/**
 * @file problem.hpp
 * Header file for problem-related routes and functionalities in a coding problems platform.
 * 
 * This file declares the setup for a route that handles fetching problem details, including title, description, samples, tags, and solutions based on user permissions.
 * It utilizes the CROW web server library for routing, nlohmann::json for JSON manipulation, and a custom LRU cache implementation for caching problem details.
 *
 * Dependencies:
 * - crow.h: Main header file for the CROW web server library.
 * - crow/middlewares/cors.h: Header file for CORS middleware in CROW, enabling cross-origin requests.
 * - nlohmann/json.hpp: Header file for the JSON library for modern C++, used for JSON manipulation.
 * - lrucache.hpp: Header file for the LRU (Least Recently Used) cache implementation.
 * - api.hpp: Header file for the API interface, used for database interactions.
 *
 * Functions:
 * - ROUTE_problem: Declares the function for setting up the route to fetch problem details. It requires the CROW application object, application settings, client IP, SQL API object, and problem cache as parameters.
 *
 * Usage:
 * Include this header file in the server application to set up and handle the route for fetching problem details. Ensure all dependencies are properly included and linked in the project.
 */
#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../include/lrucache.hpp"
#include "../API/api.hpp"

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
void ROUTE_problem(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& sqlAPI, cache::lru_cache<int16_t, nlohmann::json>& problem_cache);