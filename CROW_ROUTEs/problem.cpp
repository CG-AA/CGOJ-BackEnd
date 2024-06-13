#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

bool have_permisson(int flags, std::string permission_name, nlohmann::json& settings){
    try {
        int index = settings["permission_flags"]["problems"][permission_name];
        return (flags & (1 << index)) != 0;
    } catch (const std::exception& e) {
        throw std::runtime_error("Permission not found");
    }
}

void ROUTE_problems(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& sqlAPI){
    CROW_ROUTE(app, "/problems/<int>")
    .methods("GET"_method)
    ([&settings, IP, &sqlAPI](const crow::request& req, int problemId){
        nlohmann::json roles;
        try {
            std::string jwt = req.get_header_value("Authorization");
            verifyJWT(jwt, settings, IP);
            roles = getRoles(jwt, settings, IP);
        } catch (const std::exception& e) {
            return crow::response(401, e.what());
        }
        roles.push_back("everyone");
        std::string query = "SELECT problems.* FROM problems JOIN problem_role ON problems.id = problem_role.problem_id WHERE problems.id = ? AND problem_role.role_name IN (";
        for (size_t i = 0; i < roles.size(); i++) {
            query += "?";
            if (i < roles.size() - 1) {
                query += ", ";
            }
        }
        query += ")";

        std::unique_ptr<sql::PreparedStatement> pstmt = sqlAPI->prepareStatement(query);
        pstmt->setInt(1, problemId);
        for (size_t i = 0; i < roles.size(); i++) {
            pstmt->setString(i + 2, roles[i].get<std::string>());
        }
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            nlohmann::json problem;
            problem["id"] = res->getInt("id");
            problem["title"] = res->getString("title");
            problem["description"] = res->getString("description");
            problem["input_format"] = res->getString("input_format");
            problem["output_format"] = res->getString("output_format");
            problem["difficulty"] = res->getString("difficulty");

            // Fetch sample input and output
            std::string sampleQuery = "SELECT sample_input, sample_output FROM problem_sample_IO WHERE problem_id = ?";
            std::unique_ptr<sql::PreparedStatement> samplePstmt = sqlAPI->prepareStatement(sampleQuery);
            samplePstmt->setInt(1, problemId);
            std::unique_ptr<sql::ResultSet> sampleRes(samplePstmt->executeQuery());
            nlohmann::json samples;
            while (sampleRes->next()) {
                nlohmann::json sample;
                sample["sample_input"] = sampleRes->getString("sample_input");
                sample["sample_output"] = sampleRes->getString("sample_output");
                samples.push_back(sample);
            }
            problem["samples"] = samples;

            // Fetch tags
            std::string tagQuery = "SELECT tag_name FROM problem_tags WHERE problem_id = ?";
            std::unique_ptr<sql::PreparedStatement> tagPstmt = sqlAPI->prepareStatement(tagQuery);
            tagPstmt->setInt(1, problemId);
            std::unique_ptr<sql::ResultSet> tagRes(tagPstmt->executeQuery());
            nlohmann::json tags;
            while (tagRes->next()) {
                tags.push_back(tagRes->getString("tag_name"));
            }
            problem["tags"] = tags;

            // Fetch solutions
            


            return crow::response(200, problem);
        } else {
            return crow::response(404, "Problem not found");
        }
    });
}