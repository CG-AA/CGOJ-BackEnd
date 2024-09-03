#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>

// show the languages that are accepted
void ROUTE_Languages(crow::App<crow::CORSHandler>& app, std::vector<std::string>& accepted_languages);