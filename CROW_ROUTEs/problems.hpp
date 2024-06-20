#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"
#include "../include/lrucache.hpp"

void ROUTE_problems(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API, cache::lru_cache<int8_t, nlohmann::json>& problems_everyone_cache, std::atomic<bool>& problems_everyone_cache_hit);