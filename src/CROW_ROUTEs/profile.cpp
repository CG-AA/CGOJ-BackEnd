// #include "profile.hpp"
// #include "../Programs/jwt.hpp"

// void ROUTE_profile(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API){
//     CROW_ROUTE(app, "/profile/<int>")
//     .methods("GET"_method)
//     ([&](const crow::request& req, crow::response& res, int userId){
//         bool ownProfile = false;
//         try {
//             JWT::verifyJWT(req.get_header_value("Authorization"), settings, IP);
//             if(JWT::getUserID(req.get_header_value("Authorization")) == userId){
//                 ownProfile = true;
//             }
//         } catch (const std::exception& e) {
//             return crow::response(401, "User must be logged in to view profile");
//         }
//     });
// }