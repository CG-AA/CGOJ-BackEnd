#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include "../API/api.hpp"

void verifyJWT(std::string jwt, nlohmann::json& settings, std::string BE_IP);
std::string generateJWT(nlohmann::json& settings, std::string IP, int user_id, std::unique_ptr<APIs>& sqlAPI);
nlohmann::json getRoles(std::string jwt, nlohmann::json& settings, std::string BE_IP);