/**
 * @file jwt.hpp
 * @brief Header file for the JSON Web Token (JWT) functions.
 */
#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include "../API/api.hpp"

/**
 * Verifies the authenticity of a JSON Web Token (JWT) using the provided settings and backend IP address.
 * 
 * @param jwt The JWT to be verified.
 * @param settings The JSON object containing the settings for JWT verification.
 * @param BE_IP The IP address of the backend server.
 * 
 * @throws std::runtime_error if the JWT verification fails.
 */
void verifyJWT(std::string jwt, nlohmann::json& settings, std::string BE_IP);

int16_t getSitePermissionFlags(const std::string& jwt);

int16_t getUserID(const std::string& jwt);

/**
 * Retrieves the roles from a JSON Web Token (JWT).
 *
 * @param jwt The JSON Web Token.
 * @return A JSON object containing the roles extracted from the JWT.
 */
nlohmann::json getRoles(std::string jwt);

/**
 * Generates a JSON Web Token (JWT) for the given user.
 * 
 * @param settings The JSON object containing the JWT secret.
 * @param BE_IP The IP address of the backend server.
 * @param user_id The ID of the user.
 * @param sqlapi A unique pointer to the APIs object for executing SQL queries.
 * @return The generated JWT as a string.
 */
std::string generateJWT(nlohmann::json& settings, std::string IP, int user_id, std::unique_ptr<APIs>& sqlAPI);