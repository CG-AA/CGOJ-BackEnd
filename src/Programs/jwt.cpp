#include "jwt.hpp"

/**
 * Verifies the authenticity of a JSON Web Token (JWT) using the provided settings and backend IP address.
 * 
 * @param jwt The JWT to be verified.
 * @param settings The JSON object containing the settings for JWT verification.
 * @param BE_IP The IP address of the backend server.
 * 
 * @throws std::runtime_error if the JWT verification fails.
 */
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

/**
 * Retrieves the roles from a JSON Web Token (JWT).
 *
 * @param jwt The JSON Web Token.
 * @return A JSON object containing the roles extracted from the JWT.
 */
nlohmann::json getRoles(std::string jwt) {
    auto decoded = jwt::decode(jwt);
    return nlohmann::json::parse(decoded.get_payload_claim("roles").as_string());
}

/**
 * Generates a JSON Web Token (JWT) for the given user.
 * 
 * @param settings The JSON object containing the JWT secret.
 * @param BE_IP The IP address of the backend server.
 * @param user_id The ID of the user.
 * @param sqlapi A unique pointer to the APIs object for executing SQL queries.
 * @return The generated JWT as a string.
 */
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