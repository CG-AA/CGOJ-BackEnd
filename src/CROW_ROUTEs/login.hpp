/**
 * @file login.hpp
 * @brief Header file for the login route.
 */

#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"

/**
 * @brief Defines the route for user login using the CROW library.
 * @param app The CROW application object used to set up the route.
 * @param settings The JSON object containing application settings, including JWT secret and expiration settings.
 * @param IP The IP address of the client making the request, used for logging or additional security checks.
 * @param sqlAPI A unique pointer to the SQL API object, used for database operations.
 */
void ROUTE_Login(crow::App<crow::CORSHandler>& app, nlohmann::json& settings , std::string IP, std::unique_ptr<APIs>& sqlAPI);