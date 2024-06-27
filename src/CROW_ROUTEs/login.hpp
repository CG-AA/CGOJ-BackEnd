/**
 * @file login.hpp
 * @brief Header file for the login route.
 * 
 * This file declares the ROUTE_Login function, which is used to define a route
 * for user login using the CROW library. It sets up a POST route that accepts
 * user credentials (email and password), validates them, and returns a JSON Web Token (JWT)
 * if the credentials are valid.
 */

#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"

/**
 * @brief Implements the login functionality as a POST route using the CROW library.
 * 
 * This function sets up a POST route at "/login" for user authentication. It expects
 * a JSON payload in the request body containing the user's email and password. The function
 * then queries the database for a user with the provided email. If a user is found, it
 * validates the password using BCrypt. Upon successful validation, it generates a JSON Web Token (JWT)
 * using the user's ID, the application settings, and the server IP address. The JWT is then returned
 * to the client. If the email or password is invalid, it returns an error message.
 * 
 * @param app The CROW application object used to define the route.
 * @param settings A JSON object containing the application settings, including permissions.
 * @param IP The IP address of the backend server.
 * @param sqlAPI A pointer to the SQL API object for database operations.
 * 
 * @note The route is defined to listen for POST requests only.
 */
void ROUTE_Login(crow::App<crow::CORSHandler>& app, nlohmann::json& settings , std::string IP, std::unique_ptr<APIs>& sqlAPI);