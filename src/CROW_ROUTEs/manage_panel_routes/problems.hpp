#pragma once
#include "../manage_panel.hpp"

namespace {
crow::response GET(const crow::request& req, std::string jwt, std::unique_ptr<APIs>& API) {
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
    if(JWT::getSitePermissionFlags(jwt) & 1){
        //get all the problems
        query += "LIMIT ? OFFSET ?";
        pstmt = API->prepareStatement(query);
        pstmt->setInt(1, problemsPerPage);
        pstmt->setInt(2, offset);
    } else {
        //get the roles
        nlohmann::json roles = JWT::getRoles(jwt);
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
    return crow::response(200, problems.dump());
}
}// namespace

void problemsRoute (crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& API) {
    CROW_ROUTE(app, "/manage_panel/problems")
    .methods("GET"_method, "POST"_method)
    ([&settings, &API, &IP](const crow::request& req){
        // verify the JWT(user must login first)
        std::string jwt = req.get_header_value("Authorization");
        if (!isLogin(jwt, settings, IP)) {
            return crow::response(401, "Unauthorized");
        }
        if (req.method == "GET"_method) {
            return GET(req, jwt, API);
        } else if (req.method == "POST"_method) {
            return POST(req, jwt, API);
        }

    });
}//problemsRoute