#include "api.hpp"

APIs::APIs(const std::string& host, const std::string& user, const std::string& password, const std::string& database, int port) {
    driver = sql::mysql::get_mysql_driver_instance();
    std::string hostWithPort = host + ":" + std::to_string(port);
    con = std::unique_ptr<sql::Connection>(driver->connect(hostWithPort, user, password));
    con->setSchema(database);
}

std::unique_ptr<sql::ResultSet> APIs::read(const std::string& query) {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));
    return res;
}

int APIs::write(const std::string& query) {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    int updateCount = stmt->executeUpdate(query);
    return updateCount;
}

std::unique_ptr<sql::PreparedStatement> APIs::prepareStatement(const std::string& query) {
    return std::unique_ptr<sql::PreparedStatement>(con->prepareStatement(query));
}