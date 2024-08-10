#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"

void ROUTE_profile(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API);