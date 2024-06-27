
#include "register.hpp"

#include "../Programs/jwt.hpp"

#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <vmime/vmime.hpp>
#include <bcrypt/BCrypt.hpp>

namespace{
IPrateLimit::IPrateLimit(std::string email = "") {
    emails.fill("");
    emails[0] = email;
}
}

void RateLimit::cleanup() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    int today = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count() / 24;
    if (today != last_cleanup_day) {
        rateLimit.clear();
        last_cleanup_day = today;
    }
}

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