/**
 * @file login.cpp
 * @brief Implementation of the login route.
 */
#include "login.hpp"

#include <bcrypt/BCrypt.hpp>
#include "../Programs/jwt.hpp"

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
                std::string JWT = JWT::generateJWT(settings, IP, res->getInt("id"), sqlAPI);
                crow::response response(200, "{\"JWT\": \"" + JWT + "\", \"name\": \"" + res->getString("name") + "\"}");
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