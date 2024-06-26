/**
 * @file register.cpp
 * Implementation file for handling user registration in a web application.
 * 
 * This file includes the implementation of the route for user registration, including rate limiting, user data validation, password hashing, and JWT token generation.
 * 
 * Dependencies:
 * - register.hpp: Header file for registration route declaration.
 * - ../Programs/jwt.hpp: Header file for JWT (JSON Web Token) related functions.
 * - cppconn/resultset.h & cppconn/prepared_statement.h: MySQL Connector/C++ for executing SQL queries.
 * - vmime/vmime.hpp: VMime library for handling email verification (not fully implemented).
 * - bcrypt/BCrypt.hpp: BCrypt library for hashing passwords securely.
 * 
 * Classes:
 * - IPrateLimit: Handles rate limiting by storing recent email attempts.
 * - RateLimit: Manages rate limiting for registration requests, including cleanup based on day changes.
 * 
 * Functions:
 * - ROUTE_Register: Sets up the route for user registration. It performs rate limiting, user data validation, password hashing with BCrypt, and JWT token generation for successful registrations.
 * 
 * Usage:
 * This implementation is part of the backend server of a web application. It should be compiled with the rest of the project and run on a server.
 * The ROUTE_Register function needs to be called during the server setup to enable the "/register" endpoint for user registration.
 */
#include "register.hpp"

#include "../Programs/jwt.hpp"

#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <vmime/vmime.hpp>
#include <bcrypt/BCrypt.hpp>

/**
 * @brief Constructs an instance of the IPrateLimit class.
 * 
 * This constructor initializes the `IPrateLimit` object with the provided email.
 * The email is stored in the `emails` array at index 0.
 *
 * @param email The email to be stored in the `emails` array.
 */
IPrateLimit::IPrateLimit(std::string email = "") {
    emails.fill("");
    emails[0] = email;
}

/**
 * @brief Cleans up the rate limit data.
 *
 * This function clears the rate limit data if the current day is different from the last cleanup day.
 * It is intended to be called periodically to ensure that the rate limit data remains up to date.
 */
void RateLimit::cleanup() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    int today = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count() / 24;
    if (today != last_cleanup_day) {
        rateLimit.clear();
        last_cleanup_day = today;
    }
}

/**
 * @brief Checks the rate limit for a given IP and email.
 * 
 * This function checks the rate limit for a specific IP and email combination.
 * It ensures that the rate limit is not exceeded and allows or denies access accordingly.
 * 
 * @param ip The IP address of the client.
 * @param email The email address of the client.
 * @return True if the rate limit is not exceeded and access is allowed, false otherwise.
 */
bool RateLimit::check_rate_limit(std::string ip, std::string email) {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    cleanup();
    auto it = rateLimit.find(ip);
    if (it == rateLimit.end()) {
        rateLimit[ip] = IPrateLimit(email);
        return true;
    } else {
        for(auto& e : it->second.emails) {
            if (e == email) {
                return true;
            }
            if (e == "") {
                e = email;
                return true;
            }
        }
        return false;
    }
}

RateLimit rateLimit;

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
 * The function performs the following operations:
 * - Rate limiting: Uses the `rateLimit` object to prevent abuse from frequent requests.
 * - User data validation: Checks for the uniqueness of the email address in the database.
 * - Password hashing: Utilizes BCrypt to securely hash the user's password.
 * - User insertion: Inserts the new user into the database with a unique tag.
 * - JWT token generation: Generates a JWT token for the newly registered user.
 * 
 * Exceptions:
 * - SQL exceptions are caught and logged, returning a 500 server error response.
 * - Standard exceptions are also caught and logged, returning a 500 server error response.
 * 
 * @note Email ownership verification is mentioned as a necessary step but is not implemented in this function.
 * 
 * Usage:
 * This function should be called during the server setup phase to enable the "/register" endpoint for handling user registration requests.
 */
void ROUTE_Register(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& api) {
    CROW_ROUTE(app, "/register")
    .methods("POST"_method)
    ([&settings, IP, &api](const crow::request& req){
        nlohmann::json body = nlohmann::json::parse(req.body);

        // rate limiting
        if (!rateLimit.check_rate_limit(req.remote_ip_address, body["email"].get<std::string>())) {
            crow::response res(429, "{\"error\": \"Too many requests\"}");
            return res;
        }

        int new_tag = 0;
        int user_id = 0;
        try {
            // tag selection
            std::string query = "SELECT COALESCE(MAX(tag), 0) + 1 AS new_tag FROM users WHERE name = ?;";
            std::unique_ptr<sql::PreparedStatement> pstmt = api->prepareStatement(query);
            pstmt->setString(1, body["username"].get<std::string>());
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) {
                new_tag = res->getInt("new_tag");
            }
            // check email uniqueness
            query = "SELECT COUNT(*) AS count FROM users WHERE email = ?;";
            pstmt = api->prepareStatement(query);
            pstmt->setString(1, body["email"].get<std::string>());
            res = std::unique_ptr<sql::ResultSet>(pstmt->executeQuery());
            if (res->next()) {
                if (res->getInt("count") > 0) {
                    crow::response res(400, "{\"error\": \"Email already in use\"}");
                    return res;
                }
            }

            // check email ownership using vmime
            /*
                need to be implemented

                possable solution:
                    1. gmail/sendgrid SMTP
                    2. local SMTP server
            */

            // hash password using bcrypt
            // library url: https://github.com/trusch/libbcrypt
            std::string hashed_password = BCrypt::generateHash(body["password"].get<std::string>());

            // insert user
            query = "INSERT INTO users (name, tag, email, password) VALUES (?, ?, ?, ?);";
            pstmt = api->prepareStatement(query);
            pstmt->setString(1, body["username"].get<std::string>());
            pstmt->setInt(2, new_tag);
            pstmt->setString(3, body["email"].get<std::string>());
            pstmt->setString(4, hashed_password);
            pstmt->executeUpdate();

            //get user_id
            query = "SELECT id FROM users WHERE email = ?;";
            pstmt = api->prepareStatement(query);
            pstmt->setString(1, body["email"].get<std::string>());
            res = std::unique_ptr<sql::ResultSet>(pstmt->executeQuery());
            if (res->next()) {
                user_id = res->getInt("id");
            } else {
                crow::response res(500);
                return res;
            }

        } catch (sql::SQLException &e) {
            CROW_LOG_ERROR << "SQL Exception in " << __FILE__;
            CROW_LOG_ERROR << "(" << __FUNCTION__ << ") on line " << __LINE__;
            CROW_LOG_ERROR << "Error: " << e.what();
            CROW_LOG_ERROR << " (MySQL error code: " << e.getErrorCode();
            CROW_LOG_ERROR << ", SQLState: " << e.getSQLState() << ")";
            crow::response res(500);
            return res;
        } catch (std::exception &e) {
            CROW_LOG_ERROR << "Exception in " << __FILE__;
            CROW_LOG_ERROR << "(" << __FUNCTION__ << ") on line " << __LINE__;
            CROW_LOG_ERROR << "Error: " << e.what();
            crow::response res(500);
            return res;
        }

        std::string token = generateJWT(settings, IP, user_id, api);
        crow::response res(200, "{\"jwt\": \"" + token + "\"}");
        return res;
    });
}