#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>

void ROUTE_CORStest(crow::App<crow::CORSHandler>& app, nlohmann::json& settings);
