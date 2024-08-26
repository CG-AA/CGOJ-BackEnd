#pragma once
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"
#include "../Programs/jwt.hpp"

void ROUTE_Judge(crow::App<crow::CORSHandler>& app, nlohmann::json& settings , std::string IP, std::unique_ptr<APIs>& sqlAPI, std::vector<std::string> accepted_languages);