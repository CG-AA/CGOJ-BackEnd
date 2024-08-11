#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"
#include "../Programs/jwt.hpp"

#include "manage_panel_routes/problems.hpp"

bool isLogin(std::string jwt, nlohmann::json& settings, std::string IP);
bool isPermissioned (std::string jwt, int problem_id, std::unique_ptr<APIs>& API);

void ROUTE_manage_panel(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API);