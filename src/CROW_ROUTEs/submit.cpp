#include "submit.hpp"

#define JSON_ERROR(message) nlohmann::json({{"error", message}})

void ROUTE_Judge(crow::App<crow::CORSHandler>& app, nlohmann::json& settings , std::string IP, std::unique_ptr<APIs>& sqlAPI, std::unique_ptr<APIs>& submissionAPI, std::vector<std::string> accepted_languages) {
    CROW_ROUTE(app, "/judge")
    .methods("POST"_method)
    ([&](const crow::request& req){
        // Parse the request body
        nlohmann::json body = nlohmann::json::parse(req.body);
        // Check permissions
        std::string jwt = req.get_header_value("Authorization");
        try {
            JWT::verifyJWT(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, JSON_ERROR(e.what()));
        }
        try {
            if(!JWT::isPermissioned(jwt, body["problem_id"].get<int>(), sqlAPI, settings["permission_flags"]["submit"].get<int>())){
                return crow::response(403, JSON_ERROR("Permission denied"));
            }
        } catch (const std::exception& e) {
            return crow::response(500, JSON_ERROR(e.what()));
        }

        // Validate the request body
        if (!body.contains("source_code") || !body.contains("language") || !body.contains("problem_id")) {
            return crow::response(400, JSON_ERROR("Missing required fields"));
        }
        // Get the source code, language, and problem ID
        std::string source_code = body["source_code"].get<std::string>();
        std::string language = body["language"].get<std::string>();
        int problem_id = body["problem_id"].get<int>();
        // Validate the language
        if (std::find(accepted_languages.begin(), accepted_languages.end(), language) == accepted_languages.end()) {
            return crow::response(400, JSON_ERROR("Invalid language"));
        }
        // testing connection /w sandbox
        
}