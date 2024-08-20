#include "permissions.hpp"
#include "../Programs/jwt.hpp"

void ROUTE_permissions(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API){
    CROW_ROUTE(app, "/permissions")
    .methods("GET"_method)
    ([&settings, &API, IP](const crow::request& req){
        std::string jwt = req.get_header_value("Authorization");
        try {
            JWT::verifyJWT(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }
        nlohmann::json roles = JWT::getRoles(jwt);

    });
}