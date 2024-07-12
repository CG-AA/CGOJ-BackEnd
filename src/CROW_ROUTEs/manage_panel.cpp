#include "manage_panel.hpp"
#include "../Programs/jwt.hpp"

namespace {

}

void ROUTE_manage_panel(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API){
    CROW_ROUTE(app, "/manage_panel/problems")
    .methods("GET"_method)
    ([&settings, &API, &IP](const crow::request& req){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        try {
            verifyJWT(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }

        u_int32_t problemsPerPage = 30, offset = 0;
        if (req.url_params.get("problemsPerPage")) {
            problemsPerPage = std::stoi(req.url_params.get("problemsPerPage"));
        }
        if (req.url_params.get("offset")) {
            offset = std::stoi(req.url_params.get("offset"));
        }
        std::string query = "SELECT p.id, u.name AS owner_name, p.title, p.difficulty "
                            "FROM problems p "
                            "JOIN users u ON p.owner_id = u.id ";
        std::unique_ptr<sql::PreparedStatement> pstmt;

        //if the user is a site admin
        if(getSitePermissionFlags(jwt) & 1){
            //get all the problems
            query += "LIMIT ? OFFSET ?";
            pstmt = API->prepareStatement(query);
            pstmt->setInt(1, problemsPerPage);
            pstmt->setInt(2, offset);
        } else {
            //get the roles
            nlohmann::json roles = getRoles(jwt);
            //get the problems that the user has permission to modify
            query += "JOIN problem_role pr ON p.id = pr.problem_id "
                    "WHERE pr.role_name IN (";
            for (size_t i = 0; i < roles.size(); ++i) {
                query += "?";
                if (i < roles.size() - 1) query += ", ";
            }
            query += ") AND pr.permission_flags & 1 <> 0 "
                    "LIMIT ? OFFSET ?";
            pstmt = API->prepareStatement(query);
            for (size_t i = 0; i < roles.size(); ++i) {
                pstmt->setString(i + 1, roles[i].get<std::string>());
            }
            pstmt->setInt(roles.size() + 1, problemsPerPage);
            pstmt->setInt(roles.size() + 2, offset);
        }

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        nlohmann::json problems;
        while(res->next()) {
            nlohmann::json problem;
            problem["id"] = res->getInt("id");
            problem["owner_name"] = res->getString("owner_name");
            problem["title"] = res->getString("title");
            problem["difficulty"] = res->getInt("difficulty");
            problems.push_back(problem);
        }
    });
    CROW_ROUTE(app, "/manage_panel/problems/<int>")
    .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method)
    ([&settings, &API, &IP](const crow::request& req, int problem_id){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        try {
            verifyJWT(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }
        //if the user is not a site admin and dont got the permission
        if(!(getSitePermissionFlags(jwt) & 1)){
            nlohmann::json roles = getRoles(jwt);
            std::string query = "SELECT * FROM problem_role WHERE problem_id = ? AND role_name IN (";
            for (size_t i = 0; i < roles.size(); ++i) {
                query += "?";
                if (i < roles.size() - 1) query += ", ";
            }
            query += ") AND permission_flags & 1 <> 0";
            std::unique_ptr<sql::PreparedStatement> pstmt(API->prepareStatement(query));
            pstmt->setInt(1, problem_id);
            for (size_t i = 0; i < roles.size(); ++i) {
                pstmt->setString(i + 2, roles[i].get<std::string>());
            }
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (!res->next()) {
                return crow::response(403, "Forbidden");
            }
        }
    });
        
}

// std::string query = R"(
//             SELECT 
//                 p.id AS problem_id, 
//                 p.title AS problem_title, 
//                 p.description, 
//                 p.input_format, 
//                 p.output_format, 
//                 p.difficulty, 
//                 ps.title AS solution_title, 
//                 ps.solution, 
//                 ph.title AS hint_title, 
//                 ph.hint, 
//                 pio.sample_input, 
//                 pio.sample_output, 
//                 GROUP_CONCAT(t.name) AS tags
//             FROM problems p
//             LEFT JOIN problem_solutions ps ON p.id = ps.problem_id
//             LEFT JOIN problem_hints ph ON p.id = ph.problem_id
//             LEFT JOIN problem_sample_IO pio ON p.id = pio.problem_id
//             LEFT JOIN problem_tags pt ON p.id = pt.problem_id
//             LEFT JOIN tags t ON pt.tag_id = t.id
//             WHERE p.id = ?
//             GROUP BY p.id, ps.id, ph.id, pio.id
//         )";