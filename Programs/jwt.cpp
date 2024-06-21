#include "jwt.hpp"

void verifyJWT(std::string jwt, nlohmann::json& settings, std::string BE_IP) {
    try {
        auto decoded = jwt::decode(jwt);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{settings["jwt_secret"].get<std::string>()})
            .with_issuer(BE_IP)
            .with_audience(BE_IP);

        verifier.verify(decoded);
        return;
    } catch (const std::exception& e) {
        std::string error = "Failed to verify JWT: " + std::string(e.what());
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }
}

// returns the roles from the JWT
nlohmann::json getRoles(std::string jwt, nlohmann::json& settings, std::string BE_IP) {
    auto decoded = jwt::decode(jwt);
    return nlohmann::json::parse(decoded.get_payload_claim("roles").as_string());
}

std::string generateJWT(nlohmann::json& settings, std::string BE_IP, int user_id, std::unique_ptr<APIs>& sqlapi) {
    std::string query = "SELECT role_name FROM user_roles WHERE user_id = ?";
    std::unique_ptr<sql::PreparedStatement> pstmt = sqlapi->prepareStatement(query);
    pstmt->setInt(1, user_id);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json roles;
    while(res->next()) {
        roles.push_back(res->getString("role_name"));
    }
    
    std::string token = jwt::create()
        .set_issuer(BE_IP)
        .set_subject(std::to_string(user_id))
        .set_audience(BE_IP)
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
        .set_payload_claim("roles", jwt::claim(roles.dump()))
        .sign(jwt::algorithm::hs256{settings["jwt_secret"].get<std::string>()});
    return token;
}