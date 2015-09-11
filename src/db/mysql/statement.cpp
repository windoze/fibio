//
//  statement.cpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#include <fibio/fibers/future/async.hpp>
#include "connection.hpp"
#include "statement.hpp"
#include "resultset.hpp"

namespace fibio {
namespace db {
namespace mysql {

MySQL_Statement::MySQL_Statement(sql::Connection* conn,
                                 sql::Statement* impl,
                                 std::shared_ptr<fibers::foreign_thread_pool> executor)
: conn_(conn), impl_(impl), executor_(executor)
{
}

MySQL_Statement::~MySQL_Statement()
{
}

sql::Connection* MySQL_Statement::getConnection()
{
    return conn_;
}

void MySQL_Statement::cancel()
{
    (*executor_)([&]() { impl_->cancel(); });
}

void MySQL_Statement::clearWarnings()
{
    (*executor_)([&]() { impl_->clearWarnings(); });
}

void MySQL_Statement::close()
{
    (*executor_)([&]() { impl_->close(); });
}

bool MySQL_Statement::execute(const sql::SQLString& sql)
{
    return (*executor_)([&]() { return impl_->execute(sql); });
}

sql::ResultSet* MySQL_Statement::executeQuery(const sql::SQLString& sql)
{
    sql::ResultSet* rs= (*executor_)([&]() { return impl_->executeQuery(sql); });
    return new MySQL_ResultSet(this, rs, executor_);
}

int MySQL_Statement::executeUpdate(const sql::SQLString& sql)
{
    return (*executor_)([&]() { return impl_->executeUpdate(sql); });
}

size_t MySQL_Statement::getFetchSize()
{
    return (*executor_)([&]() { return impl_->getFetchSize(); });
}

unsigned int MySQL_Statement::getMaxFieldSize()
{
    return (*executor_)([&]() { return impl_->getMaxFieldSize(); });
}

uint64_t MySQL_Statement::getMaxRows()
{
    return (*executor_)([&]() { return impl_->getMaxRows(); });
}

bool MySQL_Statement::getMoreResults()
{
    return (*executor_)([&]() { return impl_->getMoreResults(); });
}

unsigned int MySQL_Statement::getQueryTimeout()
{
    return (*executor_)([&]() { return impl_->getQueryTimeout(); });
}

sql::ResultSet* MySQL_Statement::getResultSet()
{
    sql::ResultSet* rs = (*executor_)([&]() { return impl_->getResultSet(); });
    return new MySQL_ResultSet(this, rs, executor_);
}

sql::ResultSet::enum_type MySQL_Statement::getResultSetType()
{
    return (*executor_)([&]() { return impl_->getResultSetType(); });
}

uint64_t MySQL_Statement::getUpdateCount()
{
    return (*executor_)([&]() { return impl_->getUpdateCount(); });
}

const sql::SQLWarning* MySQL_Statement::getWarnings()
{
    return (*executor_)([&]() { return impl_->getWarnings(); });
}

void MySQL_Statement::setCursorName(const sql::SQLString& name)
{
    (*executor_)([&]() { impl_->setCursorName(name); });
}

void MySQL_Statement::setEscapeProcessing(bool enable)
{
    (*executor_)([&]() { impl_->setEscapeProcessing(enable); });
}

void MySQL_Statement::setFetchSize(size_t rows)
{
    (*executor_)([&]() { impl_->setFetchSize(rows); });
}

void MySQL_Statement::setMaxFieldSize(unsigned int max)
{
    (*executor_)([&]() { impl_->setMaxFieldSize(max); });
}

void MySQL_Statement::setMaxRows(unsigned int max)
{
    (*executor_)([&]() { impl_->setMaxRows(max); });
}

void MySQL_Statement::setQueryTimeout(unsigned int seconds)
{
    (*executor_)([&]() { impl_->setQueryTimeout(seconds); });
}

sql::Statement* MySQL_Statement::setResultSetType(sql::ResultSet::enum_type type)
{
    (*executor_)([&]() { impl_->setResultSetType(type); });
    return this;
}

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio
