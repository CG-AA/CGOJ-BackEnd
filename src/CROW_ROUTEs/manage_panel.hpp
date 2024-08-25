#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"
#include "../Programs/jwt.hpp"

#include "manage_panel_routes/problems.hpp"
#include "manage_panel_routes/problem.hpp"
#include "manage_panel_routes/testcases.hpp"

void ROUTE_manage_panel(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& modifyAPI, std::unique_ptr<APIs>& API);