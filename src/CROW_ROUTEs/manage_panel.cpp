#include "manage_panel.hpp"
#include "../Programs/jwt.hpp"

namespace {

}

void ROUTE_manage_panel(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API){
    CROW_ROUTE(app, "/manage_panel")
    .methods("GET"_method, "POST"_method, "DELETE"_method)
    ([&settings, &API, &IP](const crow::request& req){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        try {
            verifyJWT(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }
        nlohmann::json roles = getRoles(jwt);
        if (req.method == crow::HTTPMethod::GET) {
            
        }
        else if (req.method == crow::HTTPMethod::POST) {

        }
        else if (req.method == crow::HTTPMethod::DELETE) {

        }
    });
}