/**
 * @file api.hpp
 * @brief Header file for the APIs class.
 */

#pragma once

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <mutex>

/**
 * @class APIs
 * @brief A class that provides an interface for interacting with a MySQL database.
 * 
 * not using connection pool because it is not necessary for this project
 * the onlinejudge is designed to be used by a small number of users
 */
class APIs {
private:
    sql::mysql::MySQL_Driver *driver; /**< The MySQL driver object. */
    std::unique_ptr<sql::Connection> con; /**< The MySQL connection object. */
    std::mutex mtx; /**< A mutex to ensure thread safety. */
    std::unique_lock<std::mutex> transactionLock; /**< A lock for transactions. */

public:
    /**
     * @brief Constructs an APIs object with the specified MySQL connection details.
     * @param SQL_host The hostname of the MySQL server.
     * @param SQL_user The username for the MySQL server.
     * @param SQL_password The password for the MySQL server.
     * @param SQL_database The name of the MySQL database.
     * @param SQL_port The port number for the MySQL server.
     */
    APIs(const std::string& SQL_host, const std::string& SQL_user, const std::string& SQL_password, const std::string& SQL_database, int SQL_port);

    /**
     * @brief Executes a read query on the MySQL database.
     * @param query The SQL query to execute.
     * @return A unique pointer to the result set of the query.
     */
    std::unique_ptr<sql::ResultSet> read(const std::string& query);

    /**
     * @brief Executes a write query on the MySQL database.
     * @param query The SQL query to execute.
     * @return The number of rows affected by the query.
     */
    int write(const std::string& query);

    /**
     * @brief Prepares a statement for execution on the MySQL database.
     * @param query The SQL query to prepare.
     * @return A unique pointer to the prepared statement.
     */
    std::unique_ptr<sql::PreparedStatement> prepareStatement(const std::string& query);

    void beginTransaction();

    void commitTransaction();

    void rollbackTransaction();
};
