/**
 * @file api.cpp
 * @brief Implementation of the APIs class.
 */

#include "api.hpp"

/**
 * @brief Constructs an instance of the APIs class.
 * @param SQL_host The host name of the MySQL server.
 * @param SQL_user The username for connecting to the MySQL server.
 * @param SQL_password The password for connecting to the MySQL server.
 * @param SQL_database The name of the MySQL database.
 * @param SQL_port The port number of the MySQL server.
 */
APIs::APIs(const std::string& SQL_host, const std::string& SQL_user, const std::string& SQL_password, const std::string& SQL_database, int SQL_port){
    driver = sql::mysql::get_mysql_driver_instance();
    std::string hostWithPort = SQL_host + ":" + std::to_string(SQL_port);
    con = std::unique_ptr<sql::Connection>(driver->connect(hostWithPort, SQL_user, SQL_password));
    con->setSchema(SQL_database);
}

/**
 * @brief Executes a read query on the MySQL database.
 * @param query The SQL query to execute.
 * @return A unique pointer to the result set of the query.
 */
std::unique_ptr<sql::ResultSet> APIs::read(const std::string& query) {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));
    return res;
}

/**
 * @brief Executes a write query on the MySQL database.
 * @param query The SQL query to execute.
 * @return The number of rows affected by the query.
 */
int APIs::write(const std::string& query) {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    int updateCount = stmt->executeUpdate(query);
    return updateCount;
}

/**
 * @brief Prepares a statement for execution on the MySQL database.
 * @param query The SQL query to prepare.
 * @return A unique pointer to the prepared statement.
 */
std::unique_ptr<sql::PreparedStatement> APIs::prepareStatement(const std::string& query) {
    return std::unique_ptr<sql::PreparedStatement>(con->prepareStatement(query));
}

