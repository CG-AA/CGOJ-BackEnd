/**
 * @file jwt.cpp
 * @brief Implementation of the JSON Web Token (JWT) functions.
 */
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

nlohmann::json getRoles(std::string jwt) {
    auto decoded = jwt::decode(jwt);
    return nlohmann::json::parse(decoded.get_payload_claim("roles").as_string());
}

int16_t getSitePermissionFlags(const std::string& jwt) {
    try {
        auto decoded = jwt::decode(jwt);
        if (!decoded.has_payload_claim("site_permission_flags")) {
            throw std::runtime_error("Claim 'site_permission_flags' not found");
        }
        return static_cast<int16_t>(decoded.get_payload_claim("site_permission_flags").as_integer());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to decode JWT: ") + e.what());
    }
}

int16_t getUserID(const std::string& jwt) {
    try {
        auto decoded = jwt::decode(jwt);
        auto subject = decoded.get_subject();
        auto json = nlohmann::json::parse(subject);
        
        if (!json.is_number_integer()) {
            throw std::runtime_error("Subject is not an integer");
        }
        
        return static_cast<int16_t>(json.get<int>());
    } catch (const std::exception& e) {
        // Handle the error appropriately, e.g., log it or rethrow
        throw std::runtime_error(std::string("Failed to decode JWT or parse subject: ") + e.what());
    }
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

    query = "SELECT r.permission_flags FROM roles r JOIN user_roles ur ON r.name = ur.role_name WHERE ur.user_id = ?";
    pstmt = sqlapi->prepareStatement(query);
    pstmt->setInt(1, user_id);
    res.reset(pstmt->executeQuery());
    int16_t site_permission_flags = 0;
    while (res->next()) {
        int16_t current_permission = res->getInt("permission_flags");
        // Combine the current permission with the accumulated permissions using bitwise OR.
        site_permission_flags |= current_permission;
    }

    std::string token = jwt::create()
        .set_issuer(BE_IP)
        .set_subject(std::to_string(user_id))
        .set_audience(BE_IP)
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
        .set_payload_claim("roles", jwt::claim(roles.dump()))
        .set_payload_claim("site_permission_flags", jwt::claim(std::to_string(site_permission_flags)))
        .sign(jwt::algorithm::hs256{settings["jwt_secret"].get<std::string>()});
    return token;
}