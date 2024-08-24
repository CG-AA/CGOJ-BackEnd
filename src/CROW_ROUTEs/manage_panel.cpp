#include "manage_panel.hpp"

void ROUTE_manage_panel(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API){
    problemsRoute(app, settings, IP, API);
    problemRoute(app, settings, IP, API);
    testcaseRoute(app, settings, IP, API);
}

