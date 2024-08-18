#pragma once
/**
 * @file jwt.cpp
 * @brief Implementation of the JSON Web Token (JWT) functions.
 */
#include "jwt.hpp"
#include <iostream>

void JWT::verifyJWT(std::string jwt, nlohmann::json& settings, std::string BE_IP) {
    auto decoded = jwt::decode(jwt);
    auto verifier = jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{settings["jwt_secret"].get<std::string>()})
        .with_issuer(BE_IP)
        .with_audience(BE_IP);
    verifier.verify(decoded);
    return;
}

nlohmann::json JWT::getRoles(std::string jwt) {
    auto decoded = jwt::decode(jwt);
    return nlohmann::json::parse(decoded.get_payload_claim("roles").as_string());
}

int16_t JWT::getSitePermissionFlags(const std::string& jwt) {
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

int16_t JWT::getUserID(const std::string& jwt) {
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

std::string JWT::generateJWT(nlohmann::json& settings, std::string BE_IP, int user_id, std::unique_ptr<APIs>& sqlapi) {
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

//if the user is not a site admin and dont got the permission
bool JWT::isPermissioned(std::string jwt, int problem_id, std::unique_ptr<APIs>& API) {
    try {
        // Check if the user has site-wide permission
        if (!(JWT::getSitePermissionFlags(jwt) & 1)) {
            nlohmann::json roles = JWT::getRoles(jwt);
            if (roles.empty()) {
                return false;
            }

            std::string query = "SELECT * FROM problem_role WHERE problem_id = ? AND role_name IN (";
            for (size_t i = 0; i < roles.size(); ++i) {
                query += "?";
                if (i < roles.size() - 1) query += ", ";
            }
            query += ") AND permission_flags & 1 <> 0";

            // Prepare the statement
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, problem_id);
            for (size_t i = 0; i < roles.size(); ++i) {
                pstmt->setString(i + 2, roles[i].get<std::string>());
            }

            // Execute the query
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (!res->next()) {
                return false;
            }
        }
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}