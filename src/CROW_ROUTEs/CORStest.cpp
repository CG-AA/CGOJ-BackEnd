#include "CORStest.hpp"

void ROUTE_CORStest(crow::App<crow::CORSHandler>& app, nlohmann::json& settings) {
    CROW_ROUTE(app, "/CORStest")
    .methods("POST"_method)
    ([](const crow::request& req){
        return "CORStest";
    });

    // CROW_ROUTE(app, "/CORStest")
    // .methods("OPTIONS"_method)
    // ([](const crow::request& req){
    //     crow::response res;
    //     res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
    //     res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    //     res.add_header("Access-Control-Allow-Origin", settings["CGFE_origin"].get<std::string>());
    //     res.code = 200;
    //     return res;
    // });
}