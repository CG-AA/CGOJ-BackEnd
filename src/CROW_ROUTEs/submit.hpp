#pragma once
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"
#include "../API/sand_box_api.hpp"
#include "../Programs/jwt.hpp"

void ROUTE_Submit(crow::App<crow::CORSHandler>& app, nlohmann::json& settings , std::string IP, std::unique_ptr<APIs>& sqlAPI, std::unique_ptr<APIs>& submissionAPI, std::vector<std::string> accepted_languages, std::unique_ptr<sand_box_api>& sandboxAPI);