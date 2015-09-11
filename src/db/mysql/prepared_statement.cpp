//
//  prepared_statement.cpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#include <fibio/fibers/future/async.hpp>
#include "connection.hpp"
#include "prepared_statement.hpp"
#include "resultset.hpp"

namespace fibio {
namespace db {
namespace mysql {

MySQL_PreparedStatement::MySQL_PreparedStatement(
    sql::Connection* conn,
    sql::PreparedStatement* impl,
    std::shared_ptr<fibers::foreign_thread_pool> executor)
: conn_(conn), impl_(impl), executor_(executor)
{
}

MySQL_PreparedStatement::~MySQL_PreparedStatement()
{
}

sql::Connection* MySQL_PreparedStatement::getConnection()
{
    return conn_;
}

void MySQL_PreparedStatement::cancel()
{
    (*executor_)([&]() { impl_->cancel(); });
}

void MySQL_PreparedStatement::clearWarnings()
{
    (*executor_)([&]() { impl_->clearWarnings(); });
}

void MySQL_PreparedStatement::close()
{
    (*executor_)([&]() { impl_->close(); });
}

bool MySQL_PreparedStatement::execute(const sql::SQLString& sql)
{
    return (*executor_)([&]() { return impl_->execute(sql); });
}

sql::ResultSet* MySQL_PreparedStatement::executeQuery(const sql::SQLString& sql)
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->executeQuery(sql); });
    return new MySQL_ResultSet(this, rs, executor_);
}

int MySQL_PreparedStatement::executeUpdate(const sql::SQLString& sql)
{
    return (*executor_)([&]() { return impl_->executeUpdate(sql); });
}

size_t MySQL_PreparedStatement::getFetchSize()
{
    return (*executor_)([&]() { return impl_->getFetchSize(); });
}

unsigned int MySQL_PreparedStatement::getMaxFieldSize()
{
    return (*executor_)([&]() { return impl_->getMaxFieldSize(); });
}

uint64_t MySQL_PreparedStatement::getMaxRows()
{
    return (*executor_)([&]() { return impl_->getMaxRows(); });
}

bool MySQL_PreparedStatement::getMoreResults()
{
    return (*executor_)([&]() { return impl_->getMoreResults(); });
}

unsigned int MySQL_PreparedStatement::getQueryTimeout()
{
    return (*executor_)([&]() { return impl_->getQueryTimeout(); });
}

sql::ResultSet* MySQL_PreparedStatement::getResultSet()
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->getResultSet(); });
    return new MySQL_ResultSet(this, rs, executor_);
}

sql::ResultSet::enum_type MySQL_PreparedStatement::getResultSetType()
{
    return (*executor_)([&]() { return impl_->getResultSetType(); });
}

uint64_t MySQL_PreparedStatement::getUpdateCount()
{
    return (*executor_)([&]() { return impl_->getUpdateCount(); });
}

const sql::SQLWarning* MySQL_PreparedStatement::getWarnings()
{
    return (*executor_)([&]() { return impl_->getWarnings(); });
}

void MySQL_PreparedStatement::setCursorName(const sql::SQLString& name)
{
    (*executor_)([&]() { impl_->setCursorName(name); });
}

void MySQL_PreparedStatement::setEscapeProcessing(bool enable)
{
    (*executor_)([&]() { impl_->setEscapeProcessing(enable); });
}

void MySQL_PreparedStatement::setFetchSize(size_t rows)
{
    (*executor_)([&]() { impl_->setFetchSize(rows); });
}

void MySQL_PreparedStatement::setMaxFieldSize(unsigned int max)
{
    (*executor_)([&]() { impl_->setMaxFieldSize(max); });
}

void MySQL_PreparedStatement::setMaxRows(unsigned int max)
{
    (*executor_)([&]() { impl_->setMaxRows(max); });
}

void MySQL_PreparedStatement::setQueryTimeout(unsigned int seconds)
{
    (*executor_)([&]() { impl_->setQueryTimeout(seconds); });
}

sql::PreparedStatement* MySQL_PreparedStatement::setResultSetType(sql::ResultSet::enum_type type)
{
    (*executor_)([&]() { impl_->setResultSetType(type); });
    return this;
}

void MySQL_PreparedStatement::clearParameters()
{
    (*executor_)([&]() { impl_->clearParameters(); });
}

bool MySQL_PreparedStatement::execute()
{
    return (*executor_)([&]() { return impl_->execute(); });
}

sql::ResultSet* MySQL_PreparedStatement::executeQuery()
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->executeQuery(); });
    return new MySQL_ResultSet(this, rs, executor_);
}

int MySQL_PreparedStatement::executeUpdate()
{
    return (*executor_)([&]() { return impl_->executeUpdate(); });
}

sql::ResultSetMetaData* MySQL_PreparedStatement::getMetaData()
{
    // NOTE: Seems ResultSetMetaData members don't communicate with server?
    return (*executor_)([&]() { return impl_->getMetaData(); });
}

sql::ParameterMetaData* MySQL_PreparedStatement::getParameterMetaData()
{
    // NOTE: Seems ParameterMetaData doesn't talk with server
    return (*executor_)([&]() { return impl_->getParameterMetaData(); });
}

void MySQL_PreparedStatement::setBigInt(unsigned int parameterIndex, const sql::SQLString& value)
{
    (*executor_)([&]() { impl_->setBigInt(parameterIndex, value); });
}

void MySQL_PreparedStatement::setBlob(unsigned int parameterIndex, std::istream* blob)
{
    (*executor_)([&]() { impl_->setBlob(parameterIndex, blob); });
}

void MySQL_PreparedStatement::setBoolean(unsigned int parameterIndex, bool value)
{
    (*executor_)([&]() { impl_->setBoolean(parameterIndex, value); });
}

void MySQL_PreparedStatement::setDateTime(unsigned int parameterIndex, const sql::SQLString& value)
{
    (*executor_)([&]() { impl_->setDateTime(parameterIndex, value); });
}

void MySQL_PreparedStatement::setDouble(unsigned int parameterIndex, double value)
{
    (*executor_)([&]() { impl_->setDouble(parameterIndex, value); });
}

void MySQL_PreparedStatement::setInt(unsigned int parameterIndex, int32_t value)
{
    (*executor_)([&]() { impl_->setInt(parameterIndex, value); });
}

void MySQL_PreparedStatement::setUInt(unsigned int parameterIndex, uint32_t value)
{
    (*executor_)([&]() { impl_->setUInt(parameterIndex, value); });
}

void MySQL_PreparedStatement::setInt64(unsigned int parameterIndex, int64_t value)
{
    (*executor_)([&]() { impl_->setInt64(parameterIndex, value); });
}

void MySQL_PreparedStatement::setUInt64(unsigned int parameterIndex, uint64_t value)
{
    (*executor_)([&]() { impl_->setUInt64(parameterIndex, value); });
}

void MySQL_PreparedStatement::setNull(unsigned int parameterIndex, int sqlType)
{
    (*executor_)([&]() { impl_->setNull(parameterIndex, sqlType); });
}

void MySQL_PreparedStatement::setString(unsigned int parameterIndex, const sql::SQLString& value)
{
    (*executor_)([&]() { impl_->setString(parameterIndex, value); });
}

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio
