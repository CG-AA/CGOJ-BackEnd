#pragma once

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

class APIs {
private:
    sql::mysql::MySQL_Driver *driver;
    std::unique_ptr<sql::Connection> con;

public:
    
    APIs(const std::string& host, const std::string& user, const std::string& password, const std::string& database, int port);
    std::unique_ptr<sql::ResultSet> read(const std::string& query);
    int write(const std::string& query);
    std::unique_ptr<sql::PreparedStatement> prepareStatement(const std::string& query);

};