/**
 * @file login.cpp
 * @brief Defines the route for user login using the CROW library.
 * 
 * This file contains the implementation of the ROUTE_Login function, which sets up a POST route for user login.
 * It receives user credentials (email and password) in the request body, validates them against the database, and returns a JSON Web Token (JWT) if the credentials are valid.
 * 
 * @note This file requires the presence of a properly configured SQL API object for database access.
 */
#include "login.hpp"

#include <bcrypt/BCrypt.hpp>
#include "../Programs/jwt.hpp"

/**
 * @brief Defines the route for user login using the CROW library.
 * 
 * This function sets up a POST route for user login. It receives user credentials (email and password) in the request body,
 * validates them against the database, and returns a JSON Web Token (JWT) if the credentials are valid.
 * 
 * @param app The CROW application object used to set up the route.
 * @param settings The JSON object containing application settings, including JWT secret and expiration settings.
 * @param IP The IP address of the client making the request, used for logging or additional security checks.
 * @param sqlAPI A unique pointer to the SQL API object, used for database operations.
 * 
 * @return A crow::response object with a 200 status code and the JWT in the response body if credentials are valid.
 *         Returns a crow::response object with a 400 status code and an error message if credentials are invalid.
 * 
 * @note This function uses BCrypt for password validation and requires a properly configured SQL API object for database access.
 *       It assumes the presence of a 'users' table with 'email' and 'password' columns.
 */
void ROUTE_Login(crow::App<crow::CORSHandler>& app, nlohmann::json& settings , std::string IP, std::unique_ptr<APIs>& sqlAPI) {
    CROW_ROUTE(app, "/login")
    .methods("POST"_method)
    ([&settings, &sqlAPI, IP](const crow::request& req) {
        nlohmann::json body = nlohmann::json::parse(req.body);
        std::string query = "SELECT * FROM users WHERE email = ?;";
        std::unique_ptr<sql::PreparedStatement> pstmt = sqlAPI->prepareStatement(query);
        pstmt->setString(1, body["email"].get<std::string>());
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            if (BCrypt::validatePassword(body["password"].get<std::string>(), res->getString("password"))) {
                std::string JWT = generateJWT(settings, IP, res->getInt("id"), sqlAPI);
                crow::response response(200, "{\"JWT\": \"" + JWT + "\"}");
                return response;
            } else {
                crow::response response(400, "{\"error\": \"Invalid email or password\"}");
                return response;
            }
        } else {
            crow::response response(400, "{\"error\": \"Invalid email or password\"}");
            return response;
        }
    });
}