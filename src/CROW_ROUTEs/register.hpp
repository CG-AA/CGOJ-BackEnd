/**
 * @file register.hpp
 * @brief Header file for the user registration route.
 */
#pragma once

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include "../API/api.hpp"

/**
 * @class IPrateLimit
 * @brief Record emails for rate limiting.
 * 
 * @note Only used in RateLimit class and accessible by the same file.
 */
namespace { // Anonymous namespace to limit scope to this file
/**
 * @brief Constructs an instance of the IPrateLimit class.
 * 
 * This constructor initializes the `IPrateLimit` object with the provided email.
 * The email is stored in the `emails` array at index 0.
 *
 * @param email The email to be stored in the `emails` array.
 * 
 * @note Only accessable by the same file.
 */
class IPrateLimit {
public:
    std::array<std::string, 3> emails;
    IPrateLimit(std::string email = "");
};
}

/**
 * @class RateLimit
 * @brief Rate limiting for registration.
 * 
 * This class provides rate limiting functionality for the user registration process.
 */
class RateLimit {
public:
    /**
     * @brief if a given IP address and email combination is within the rate limit.
     * 
     * This method ensures thread-safe access to the rate limit data and performs a cleanup
     * before checking the rate limit for the given IP and email. If the IP is not found in the
     * rate limit data, it adds the IP and email to the data and allows the request. If the IP
     * is found, it checks if the email is already associated with the IP. If the email is found,
     * or if there is an empty slot for a new email, the request is allowed. Otherwise, the request
     * is denied, indicating the rate limit has been reached for the given IP.
     * 
     * @param ip The IP address of the user making the request.
     * @param email The email address of the user making the request.
     * @return true if the request is within the rate limit; false otherwise.
     */
    bool check_rate_limit(std::string ip, std::string email);
private:
    std::recursive_mutex mtx; /**< The mutex used to synchronize access to the rate limit data. */
    int last_cleanup_day = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count() / 24; /**< The last day when the rate limit data was cleaned up. */
    std::map<std::string, IPrateLimit> rateLimit; /**< The map that stores the rate limit information for each IP address and email combination. */

    /**
     * @brief Cleans up the rate limit data.
     *
     * This function clears the rate limit data if the current day is different from the last cleanup day.
     * It is intended to be called periodically to ensure that the rate limit data remains up to date.
     */
    void cleanup();
};

/**
 * @brief Handles the user registration process.
 * 
 * This function sets up a POST route at "/register" to handle user registration requests. It includes rate limiting, user data validation, password hashing, and JWT token generation.
 * 
 * @param app Reference to the CROW application object, used to define the route.
 * @param settings Reference to a JSON object containing application settings.
 * @param IP String representing the client's IP address, used for logging.
 * @param api Unique pointer to the APIs object, facilitating database interactions.
 * 
 * @attention Email ownership verification should be implemented to prevent unauthorized registrations.
 */
void ROUTE_Register(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& api);