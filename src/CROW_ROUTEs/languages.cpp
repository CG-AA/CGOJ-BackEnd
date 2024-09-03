#include "languages.hpp"
#include <nlohmann/json.hpp>

void ROUTE_Languages(crow::App<crow::CORSHandler>& app, std::vector<std::string>& accepted_languages) {
    CROW_ROUTE(app, "/languages")
    .methods("GET"_method)
    ([&accepted_languages](const crow::request& req){
        return crow::response(200, nlohmann::json(accepted_languages).dump());
    });
}