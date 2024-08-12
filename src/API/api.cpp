/**
 * @file api.cpp
 * @brief Implementation of the APIs class.
 */

#include "api.hpp"

APIs::APIs(const std::string& SQL_host, const std::string& SQL_user, const std::string& SQL_password, const std::string& SQL_database, int SQL_port) 
: transactionLock(mtx, std::defer_lock) {
    driver = sql::mysql::get_mysql_driver_instance();
    std::string hostWithPort = SQL_host + ":" + std::to_string(SQL_port);
    con = std::unique_ptr<sql::Connection>(driver->connect(hostWithPort, SQL_user, SQL_password));
    con->setSchema(SQL_database);
}

std::unique_ptr<sql::ResultSet> APIs::read(const std::string& query) {
    std::lock_guard<std::mutex> lock(mtx);
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query));
    return res;
}

int APIs::write(const std::string& query) {
    std::lock_guard<std::mutex> lock(mtx);
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    int updateCount = stmt->executeUpdate(query);
    return updateCount;
}

std::unique_ptr<sql::PreparedStatement> APIs::prepareStatement(const std::string& query) {
    std::lock_guard<std::mutex> lock(mtx);
    return std::unique_ptr<sql::PreparedStatement>(con->prepareStatement(query));
}

void APIs::beginTransaction() {
    transactionLock.lock();
    con->setAutoCommit(false);
}

void APIs::commitTransaction() {
    con->commit();
    con->setAutoCommit(true);
    transactionLock.unlock();
}

void APIs::rollbackTransaction() {
    con->rollback();
    con->setAutoCommit(true);
    transactionLock.unlock();
}