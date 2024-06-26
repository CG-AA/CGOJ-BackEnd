#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"

/**
 * @class IPrateLimit
 * @brief Class representing IP rate limiting functionality.
 * 
 * This class is responsible for managing IP rate limiting for a given email address.
 * It stores an array of email addresses and provides a constructor to initialize the class
 * with an optional email address.
 */
class IPrateLimit {
public:
    std::array<std::string, 3> emails; /**< Array of email addresses. */
    
    /**
     * @brief Constructor for IPrateLimit class.
     * 
     * @param email The email address to be added to the array.
     */
    IPrateLimit(std::string email = "");
};

class RateLimit {
public:
    std::recursive_mutex mtx;
    int last_cleanup_day = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count() / 24;
    std::map<std::string, IPrateLimit> rateLimit;

    void cleanup();
    bool check_rate_limit(std::string ip, std::string email);
};

void ROUTE_Register(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& sqlAPI);