/**
 * @file main.cpp
 * @brief The main file for the CGBE.
 */

#include <crow.h>
#include <crow/middlewares/cors.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

#include "src/API/api.hpp"
#include "src/API/sand_box_api.hpp"

#include "src/CROW_ROUTEs/register.hpp"
#include "src/CROW_ROUTEs/login.hpp"
#include "src/CROW_ROUTEs/problems.hpp"
#include "src/CROW_ROUTEs/problem.hpp"
#include "src/CROW_ROUTEs/manage_panel.hpp"
#include "src/CROW_ROUTEs/submit.hpp"

#include "src/Programs/get_ip.hpp"

#include "src/include/lrucache.hpp"

/** Configuration settings for the application. */
nlohmann::json settings;
/** Pointer to the APIs class. */
std::unique_ptr<APIs> api, modify_api, submission_api;
/** Pointer to the SandBoxAPI class. */
std::unique_ptr<sand_box_api> sandbox_api;
/** The CROW application object. */
crow::App<crow::CORSHandler> app;
/** The IP address of the BE. */
std::string IP;
/** Cache hit flag for problems_everyone_cache. */
std::atomic<bool> problems_everyone_cache_hit{false};

/** Cache for problems available to everyone */
cache::lru_cache<int8_t, nlohmann::json> problems_everyone_cache(100);
/** Cache for specific problem data */
cache::lru_cache<int16_t, nlohmann::json> problem_cache(1000);

std::vector<std::string> accepted_languages;

/**
 * @brief Loads the settings from the default settings file and the local settings file.
 * 
 * This function reads the JSON data from the default settings file and the local settings file,
 * merges them together, and returns the resulting JSON object.
 * 
 * @param defaultSettingsFile The path to the default settings file.
 * @param localSettingsFileName The path to the local settings file.
 * @return The merged JSON object containing the settings.
 * @throws std::ifstream::failure if there is an error opening or reading the settings files.
 * @throws nlohmann::json::parse_error if there is an error parsing the JSON data.
 */
nlohmann::json loadSettings(const std::string& defaultSettingsFileName, const std::string& localSettingsFileName) {
    // Load settings from the default file
    nlohmann::json defaultSettings;
    std::ifstream defaultFile(defaultSettingsFileName);
    try {
        defaultFile >> defaultSettings;
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Error parsing default settings file: " << e.what() << std::endl;
        throw;
    }

    // Load settings from the .local file
    std::ifstream localFile(localSettingsFileName);
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


/**
 * @brief Creates and initializes an instance of APIs based on the provided settings.
 * 
 * This function takes a JSON object containing settings for MySQL database connection and creates an instance of APIs class.
 * The settings should include the host, user, password, database, and port information for the MySQL connection.
 * 
 * @param settings The JSON object containing the MySQL connection settings.
 * @return A unique pointer to the created APIs instance.
 */
auto setupSqlAPI(const nlohmann::json& settings) {
    return std::make_unique<APIs>(
        settings["MySQL"]["host"].get<std::string>(),
        settings["MySQL"]["user"].get<std::string>(),
        settings["MySQL"]["password"].get<std::string>(),
        settings["MySQL"]["database"].get<std::string>(),
        settings["MySQL"]["port"].get<int>()
    );
}

auto setupSandboxAPI(const nlohmann::json& settings) {
    return std::make_unique<sand_box_api>(
        settings["SandBox"]["host"].get<std::string>(),
        settings["SandBox"]["port"].get<int>(),
        settings["SandBox"]["token"].get<std::string>()
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

/**
 * @brief Sets up Cross-Origin Resource Sharing (CORS) for the application.
 * This function configures the CORS middleware to allow specified headers, methods, and origins.
 * It is used to enable cross-origin requests from the client-side to the server-side.
 */
void setupCORS() {
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
        .global()
            .headers("Authorization", "content-Type")
            .methods("POST"_method, "OPTIONS"_method)
            .origin(settings["CGFE_origin"].get<std::string>());
}

void setupAcceptedLanguages() {
    std::string query = "SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = 'problem_submissions' AND COLUMN_NAME = 'language';";
    std::unique_ptr<sql::PreparedStatement> pstmt(api->prepareStatement(query));
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        std::string column_type = res->getString("COLUMN_TYPE");
        std::string type = column_type.substr(5, column_type.size() - 6);
        std::string delimiter = "','";
        size_t pos = 0;
        while ((pos = type.find(delimiter)) != std::string::npos) {
            accepted_languages.push_back(type.substr(0, pos));
            type.erase(0, pos + delimiter.length());
        }
        accepted_languages.push_back(type);
    } else {
        std::cerr << "Error: Could not fetch accepted languages from the database." << std::endl;
    }
}

/**
 * @brief Sets up the routes for the application.
 * This function registers various routes for handling different requests.
 */
void setupRoutes() {
    ROUTE_problems(app, settings, IP, api, problems_everyone_cache, problems_everyone_cache_hit);
    ROUTE_problem(app, settings, IP, api, problem_cache);
    ROUTE_Register(app, settings, IP, api);
    ROUTE_Login(app, settings, IP, api);
    ROUTE_manage_panel(app, settings, IP, modify_api, api);
    ROUTE_Submit(app, settings, IP, api, submission_api, accepted_languages, sandbox_api);
}

/**
 * The main function of the program.
 * It initializes the IP address, loads settings, sets up CORS, sets up routes,
 * sets up APIs, and runs the application.
 */
int main()
{
    IP = getPublicIP();
    settings = loadSettings("settings", "settings.local");
    // crow::ssl_context_t ctx(crow::ssl_context_t::tlsv13);
    // setupSSL(ctx);
    setupCORS();
    setupRoutes();
    api = setupSqlAPI(settings);
    modify_api = setupSqlAPI(settings);
    submission_api = setupSqlAPI(settings);
    sandbox_api = setupSandboxAPI(settings);
    setupAcceptedLanguages();

    app.port(settings["port"].get<int>()).multithreaded().run();// .ssl(std::move(ctx))
}