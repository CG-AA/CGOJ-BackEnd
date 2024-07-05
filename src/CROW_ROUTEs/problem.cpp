/**
 * @file problem.cpp
 * @brief Implementation of the problem route.
 */
#include "problems.hpp"
#include "../Programs/jwt.hpp"

#include <jwt-cpp/jwt.h>

namespace{
bool have_permisson(int user_permission, std::string permission_name, nlohmann::json& settings){
    try {
        int index = settings["permission_flags"]["problems"][permission_name];
        return (user_permission & (1 << index)) != 0;
    } catch (const std::exception& e) {
        throw std::runtime_error("Permission not found");
    }
}

bool view_solutions_permission(nlohmann::json& settings, nlohmann::json& roles, nlohmann::json& problem){
    for(auto i : problem["roles"]){
        if(have_permisson(i.begin().value(), "view_solutions", settings) && roles.contains(i.begin().key())){
            return true;
        }
    }
    return false;
}

nlohmann::json get_problem(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = "SELECT * FROM problems WHERE id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
        nlohmann::json problem;
        problem["id"] = res->getInt("id");
        problem["owner_id"] = res->getInt("owner_id");
        problem["name"] = res->getString("name");
        problem["description"] = res->getString("description");
        problem["input_format"] = res->getString("input_format");
        problem["output_format"] = res->getString("output_format");
        problem["difficulty"] = res->getString("difficulty");
        return problem;
    } else {
        throw std::runtime_error("Problem not found");
    }
}

nlohmann::json get_problem_sample_IO(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = "SELECT * FROM problem_sample_IO WHERE problem_id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json sample_io = nlohmann::json::array();
    while (res->next()) {
        nlohmann::json io;
        io["input"] = res->getString("input");
        io["output"] = res->getString("output");
        sample_io.push_back(io);
    }
    return sample_io;
}

nlohmann::json get_problem_tags(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = 
        "SELECT t.name "
        "FROM tags t "
        "JOIN problem_tags pt ON t.id = pt.tag_id "
        "WHERE pt.problem_id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json tags = nlohmann::json::array();
    while (res->next()) {
        tags.push_back(res->getString("name"));
    }
    return tags;
}

nlohmann::json get_problem_test_cases(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = "SELECT * FROM problem_test_cases WHERE problem_id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json test_cases = nlohmann::json::array();
    while (res->next()) {
        nlohmann::json test_case;
        test_case["id"] = res->getInt("id");
        test_case["input"] = res->getString("input");
        test_case["output"] = res->getString("output");
        test_case["time_limit"] = res->getInt("time_limit");
        test_case["memory_limit"] = res->getInt("memory_limit");
        test_case["score"] = res->getInt("score");  // 0 >= score <= 10,000
        test_cases.push_back(test_case);
    }
    return test_cases;
}

nlohmann::json get_problem_solution(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = "SELECT * FROM problem_solutions WHERE problem_id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json solutions = nlohmann::json::array();
    while (res->next()) {
        nlohmann::json solution;
        solution["title"] = res->getString("title");
        solution["solution"] = res->getString("solution");
        solutions.push_back(solution);
    }
    return solutions;
}

nlohmann::json get_problem_hints(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = "SELECT * FROM problem_hints WHERE problem_id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json hints = nlohmann::json::array();
    while (res->next()) {
        nlohmann::json hint;
        hint["title"] = res->getString("title");
        hint["hint"] = res->getString("hint");
        hints.push_back(hint);
    }
    return hints;
}

nlohmann::json get_problem_submissions(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = "SELECT * FROM problem_submissions WHERE problem_id = ? ORDER BY submission_time DESC;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json submissions = nlohmann::json::array();
    while (res->next()) {
        nlohmann::json submission;
        submission["id"] = res->getInt("id");
        submission["user_id"] = res->getInt("user_id");
        submission["submission_time"] = res->getString("submission_time");
        submission["code"] = res->getString("code");
        submission["status"] = res->getString("status");
        submission["time_taken"] = res->getInt("time_taken");
        submission["memory_taken"] = res->getInt("memory_taken");
        submission["language"] = res->getString("language");
        submissions.push_back(submission);
    }
    return submissions;
}

nlohmann::json get_problem_submissions_subtasks(std::unique_ptr<APIs>& sqlAPI, int submissionId) {
    std::string query = "SELECT * FROM problem_submissions_subtasks WHERE submission_id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, submissionId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json subtasks = nlohmann::json::array();
    while (res->next()) {
        nlohmann::json subtask;
        subtask["submission_id"] = res->getInt("submission_id");
        subtask["id"] = res->getInt("id");
        subtask["status"] = res->getString("status");
        subtask["time_taken"] = res->getInt("time_taken");
        subtask["memory_taken"] = res->getInt("memory_taken");
        subtasks.push_back(subtask);
    }
    return subtasks;
}

nlohmann::json get_problem_roles(std::unique_ptr<APIs>& sqlAPI, int problemId) {
    std::string query = "SELECT rp.name, rp.color, pr.permission_flags "
                        "FROM problem_role pr "
                        "JOIN roles_problem rp ON pr.role_name = rp.name "
                        "WHERE pr.problem_id = ?;";
    std::unique_ptr<sql::PreparedStatement> pstmt(sqlAPI->prepareStatement(query));
    pstmt->setInt(1, problemId);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    nlohmann::json roles = nlohmann::json::array();
    while (res->next()) {
        nlohmann::json role;
        role["name"] = res->getString("name");
        role["color"] = res->getString("color");
        role["permission_flags"] = res->getInt("permission_flags");
        roles.push_back(role);
    }
    return roles;
}
} // namespace





void ROUTE_problem(crow::App<crow::CORSHandler>& app, nlohmann::json& settings, std::string IP, std::unique_ptr<APIs>& sqlAPI, cache::lru_cache<int16_t, nlohmann::json>& problem_cache){
    CROW_ROUTE(app, "/problem/<int>")
    .methods("GET"_method)
    ([&settings, IP, &sqlAPI, &problem_cache](const crow::request& req, int problemId){
        nlohmann::json roles;
        try {
            std::string jwt = req.get_header_value("Authorization");
            if (jwt != "null") {
                verifyJWT(jwt, settings, IP);
                roles = getRoles(jwt);
            }
        } catch (const std::exception& e) {
        }
        //do a cache hit
        if(problem_cache.exists(problemId)){
            nlohmann::json problem = problem_cache.get(problemId);
            if(!view_solutions_permission(settings, roles, problem))
                problem.erase("solutions");
            return crow::response(200, problem_cache.get(problemId));
        }


    });
}