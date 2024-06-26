
#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

/**
 * @brief Sets up the route for fetching a list of problems accessible to the user.
 * 
 * This function defines a GET route at "/problems" to retrieve a paginated list of problems that the user has access to, based on their roles.
 * It utilizes JWT for authentication and authorization, checks user roles, and fetches problems from either a cache or the database depending on the user's permissions and the requested page.
 * 
 * @param app The CROW application object used to define the route.
 * @param settings A JSON object containing application settings, including JWT secret keys and database configurations.
 * @param IP The IP address of the client making the request, used for logging and additional security checks.
 * @param API A unique pointer to the APIs object, used for database operations.
 * @param problems_everyone_cache An LRU cache object for storing and retrieving problems accessible to all users ("everyone" role) to reduce database load.
 * @param problems_everyone_cache_hit An atomic boolean flag indicating whether the cache for problems accessible to everyone has been populated.
 * 
 * The route handler performs the following steps:
 * 1. Extracts the JWT from the Authorization header and verifies it. If verification fails, returns a 401 Unauthorized response.
 * 2. Parses the JWT to extract user roles.
 * 3. Handles pagination through "page" and "problemsPerPage" query parameters.
 * 4. If the user has specific roles or requests a page beyond the cache limit, fetches problems from the database filtered by user roles.
 * 5. If the user has no specific roles and requests a page within the cache limit, attempts to fetch problems from the cache.
 * 6. If the cache is not yet populated, fetches problems accessible to everyone from the database, populates the cache, and sets the cache hit flag.
 * 7. Constructs a JSON response with the list of problems and returns it with a 200 OK status. If no problems are found, returns a 404 Not Found response.
 */
void ROUTE_problems(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API, cache::lru_cache<int8_t, nlohmann::json>& problems_everyone_cache, std::atomic<bool>& problems_everyone_cache_hit){
    CROW_ROUTE(app, "/problems")
    .methods("GET"_method)
    ([&settings, IP, &API, &problems_everyone_cache, &problems_everyone_cache_hit](const crow::request& req){
        nlohmann::json roles;
        try {
            std::string jwt = req.get_header_value("Authorization");
            verifyJWT(jwt, settings, IP);
            roles = getRoles(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }
        // Handle page query parameter
        int page = 1, problemsPerPage = 10;
        if (req.url_params.get("page")) {
            page = std::stoi(req.url_params.get("page"));
        }
        if (req.url_params.get("problemsPerPage")) {
            problemsPerPage = std::stoi(req.url_params.get("problemsPerPage"));
        }
        int offset = (page - 1) * problemsPerPage;

        nlohmann::json problems;

        if (roles.size() || offset+problemsPerPage > 30) {
            roles.push_back("everyone");
            std::string query = "SELECT problems.* FROM problems JOIN problem_role ON problems.id = problem_role.problem_id WHERE problem_role.role_name IN (";
            for (size_t i = 0; i < roles.size(); i++) {
                query += "?";
                if (i < roles.size() - 1) {
                    query += ", ";
                }
            }
            query += ")";
            query += " LIMIT " + std::to_string(problemsPerPage) + " OFFSET " + std::to_string(offset);
            std::unique_ptr<sql::PreparedStatement> pstmt = API->prepareStatement(query);
            for (size_t i = 0; i < roles.size(); i++) {
                pstmt->setString(i + 1, roles[i].get<std::string>());
            }
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                nlohmann::json problem;
                problem["id"] = res->getInt("id");
                problem["name"] = res->getString("name");
                problem["difficulty"] = res->getString("difficulty");
                problems.push_back(problem);
            }
        } else if(!problems_everyone_cache_hit) {
            std::string query = "SELECT problems.* FROM problems JOIN problem_role ON problems.id = problem_role.problem_id WHERE problem_role.role_name IN ( ? ) LIMIT 30";
            std::unique_ptr<sql::PreparedStatement> pstmt = API->prepareStatement(query);
            pstmt->setString(1, "everyone");
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            while (res->next()) {
                nlohmann::json problem;
                problem["id"] = res->getInt("id");
                problem["name"] = res->getString("name");
                problem["difficulty"] = res->getString("difficulty");
                problems.push_back(problem);
                problems_everyone_cache.put(res->getInt("id"), problem);
            }
            problems_everyone_cache_hit = true;
        } else {
            for (int i = offset; i < offset + problemsPerPage; i++) {
                problems.push_back(problems_everyone_cache.get(i));
            }
        }


        if (problems.size() == 0) {
            return crow::response(404, "No problems found");
        } else {
            return crow::response(200, problems);
        }
    });
}