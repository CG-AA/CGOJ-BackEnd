#include <crow.h>
#include <crow/middlewares/cors.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

#include "API/api.hpp"

#include "CROW_ROUTEs/register.hpp"
#include "CROW_ROUTEs/login.hpp"
#include "CROW_ROUTEs/CORStest.hpp"
#include "CROW_ROUTEs/problems.hpp"

#include "Programs/get_ip.hpp"

#include <include/lrucache.hpp>

nlohmann::json settings;
std::unique_ptr<APIs> api;
crow::App<crow::CORSHandler> app;
std::string IP;
std::atomic<bool> problems_everyone_cache_hit = false;

cache::lru_cache<int8_t, nlohmann::json> problems_everyone_cache(100);

nlohmann::json loadSettings(const std::string& defaultSettingsFile, const std::string& localSettingsFile) {
    // Load settings from the default file
    nlohmann::json defaultSettings;
    std::ifstream defaultFile(defaultSettingsFile);
    try {
        defaultFile >> defaultSettings;
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Error parsing default settings file: " << e.what() << std::endl;
        throw;
    }

    // Load settings from the .local file
    std::ifstream localFile(localSettingsFile);
    nlohmann::json localSettings;
    if (localFile.good()) { // Check if the .local file exists
        try {
            localFile >> localSettings;
        } catch (nlohmann::json::parse_error& e) {
            std::cerr << "Error parsing local settings file: " << e.what() << std::endl;
            throw;
        }
        for (auto& element : localSettings.items()) {
            defaultSettings[element.key()] = element.value();
        }
    }


    return defaultSettings;
}

auto setupAPIs(const nlohmann::json& settings) {
    return std::make_unique<APIs>(
        settings["MySQL"]["host"].get<std::string>(),
        settings["MySQL"]["user"].get<std::string>(),
        settings["MySQL"]["password"].get<std::string>(),
        settings["MySQL"]["database"].get<std::string>(),
        settings["MySQL"]["port"].get<int>()
    );
}

// void setupSSL(crow::ssl_context_t& ctx) {
//     ctx.set_options(crow::ssl_context_t::default_workarounds
//                   | crow::ssl_context_t::single_dh_use
//                   | crow::ssl_context_t::tlsv13);//TLSv1.3
//     //password is for decrypting the private key
//     ctx.set_password_callback([](std::size_t, crow::ssl_context_t::password_purpose) {
//         return settings["ssl_password"].get<std::string>();
//     });
//     ctx.use_certificate_chain_file(settings["ssl_certificate_file"].get<std::string>());
//     ctx.use_private_key_file(settings["ssl_private_key_file"].get<std::string>(), crow::ssl_context_t::pem);
// }

void setupCORS() {
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
        .global()
            .headers("Authorization", "content-Type")
            .methods("POST"_method, "OPTIONS"_method)
            .origin(settings["CGFE_origin"].get<std::string>())
        .prefix("/problems")
            .headers("Authorization", "content-Type")
            .methods("GET"_method, "OPTIONS"_method);
}

void setupRoutes() {
    ROUTE_CORStest(app, settings);
    ROUTE_problems(app, settings, IP, api, problems_everyone_cache, problems_everyone_cache_hit);
    ROUTE_Register(app, settings, IP, api);
    ROUTE_Login(app, settings, IP, api);
}

int main()
{
    IP = getPublicIP();
    settings = loadSettings("settings", "settings.local");
    // crow::ssl_context_t ctx(crow::ssl_context_t::tlsv13);
    // setupSSL(ctx);
    setupCORS();
    setupRoutes();
    api = setupAPIs(settings);

    app.port(settings["port"].get<int>()).multithreaded().run();// .ssl(std::move(ctx))
}