//
//  resultset.cpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#include <fibio/fibers/future/async.hpp>
#include <cppconn/statement.h>
#include "resultset.hpp"

namespace fibio {
namespace db {
namespace mysql {

MySQL_ResultSet::MySQL_ResultSet(sql::Statement* stmt,
                                 sql::ResultSet* impl,
                                 std::shared_ptr<fibers::foreign_thread_pool> executor)
: stmt_(stmt), impl_(impl), executor_(executor)
{
}

MySQL_ResultSet::~MySQL_ResultSet()
{
}

bool MySQL_ResultSet::absolute(int row)
{
    return (*executor_)([&]() { return impl_->absolute(row); });
}

void MySQL_ResultSet::afterLast()
{
    (*executor_)([&]() { impl_->afterLast(); });
}

void MySQL_ResultSet::beforeFirst()
{
    (*executor_)([&]() { impl_->beforeFirst(); });
}

void MySQL_ResultSet::cancelRowUpdates()
{
    (*executor_)([&]() { impl_->cancelRowUpdates(); });
}

void MySQL_ResultSet::clearWarnings()
{
    (*executor_)([&]() { impl_->clearWarnings(); });
}

void MySQL_ResultSet::close()
{
    (*executor_)([&]() { impl_->close(); });
}

uint32_t MySQL_ResultSet::findColumn(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->findColumn(columnLabel); });
}

bool MySQL_ResultSet::first()
{
    return (*executor_)([&]() { return impl_->first(); });
}

std::istream* MySQL_ResultSet::getBlob(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->getBlob(columnIndex); });
}

std::istream* MySQL_ResultSet::getBlob(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->getBlob(columnLabel); });
}

bool MySQL_ResultSet::getBoolean(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->getBoolean(columnIndex); });
}

bool MySQL_ResultSet::getBoolean(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->getBoolean(columnLabel); });
}

int MySQL_ResultSet::getConcurrency()
{
    return (*executor_)([&]() { return impl_->getConcurrency(); });
}

sql::SQLString MySQL_ResultSet::getCursorName()
{
    return (*executor_)([&]() { return impl_->getCursorName(); });
}

long double MySQL_ResultSet::getDouble(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->getDouble(columnIndex); });
}

long double MySQL_ResultSet::getDouble(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->getDouble(columnLabel); });
}

int MySQL_ResultSet::getFetchDirection()
{
    return (*executor_)([&]() { return impl_->getFetchDirection(); });
}

size_t MySQL_ResultSet::getFetchSize()
{
    return (*executor_)([&]() { return impl_->getFetchSize(); });
}

int MySQL_ResultSet::getHoldability()
{
    return (*executor_)([&]() { return impl_->getHoldability(); });
}

int32_t MySQL_ResultSet::getInt(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->getInt(columnIndex); });
}

int32_t MySQL_ResultSet::getInt(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->getInt(columnLabel); });
}

uint32_t MySQL_ResultSet::getUInt(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->getUInt(columnIndex); });
}
uint32_t MySQL_ResultSet::getUInt(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->getUInt(columnLabel); });
}

int64_t MySQL_ResultSet::getInt64(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->getInt64(columnIndex); });
}
int64_t MySQL_ResultSet::getInt64(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->getInt64(columnLabel); });
}

uint64_t MySQL_ResultSet::getUInt64(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->getUInt64(columnIndex); });
}

uint64_t MySQL_ResultSet::getUInt64(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->getUInt64(columnLabel); });
}

sql::ResultSetMetaData* MySQL_ResultSet::getMetaData() const
{
    // NOTE: Seems ResultSetMetaData members don't communicate with server?
    return (*executor_)([&]() { return impl_->getMetaData(); });
}

size_t MySQL_ResultSet::getRow() const
{
    return (*executor_)([&]() { return impl_->getRow(); });
}

sql::RowID* MySQL_ResultSet::getRowId(uint32_t columnIndex)
{
    return (*executor_)([&]() { return impl_->getRowId(columnIndex); });
}

sql::RowID* MySQL_ResultSet::getRowId(const sql::SQLString& columnLabel)
{
    return (*executor_)([&]() { return impl_->getRowId(columnLabel); });
}

const sql::Statement* MySQL_ResultSet::getStatement() const
{
    return stmt_;
}

sql::SQLString MySQL_ResultSet::getString(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->getString(columnIndex); });
}

sql::SQLString MySQL_ResultSet::getString(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->getString(columnLabel); });
}

sql::ResultSet::enum_type MySQL_ResultSet::getType() const
{
    return (*executor_)([&]() { return impl_->getType(); });
}

void MySQL_ResultSet::getWarnings()
{
    (*executor_)([&]() { impl_->getWarnings(); });
}

void MySQL_ResultSet::insertRow()
{
    (*executor_)([&]() { impl_->insertRow(); });
}

bool MySQL_ResultSet::isAfterLast() const
{
    return (*executor_)([&]() { return impl_->isAfterLast(); });
}

bool MySQL_ResultSet::isBeforeFirst() const
{
    return (*executor_)([&]() { return impl_->isBeforeFirst(); });
}

bool MySQL_ResultSet::isClosed() const
{
    return (*executor_)([&]() { return impl_->isClosed(); });
}

bool MySQL_ResultSet::isFirst() const
{
    return (*executor_)([&]() { return impl_->isFirst(); });
}

bool MySQL_ResultSet::isLast() const
{
    return (*executor_)([&]() { return impl_->isLast(); });
}

bool MySQL_ResultSet::isNull(uint32_t columnIndex) const
{
    return (*executor_)([&]() { return impl_->isNull(columnIndex); });
}

bool MySQL_ResultSet::isNull(const sql::SQLString& columnLabel) const
{
    return (*executor_)([&]() { return impl_->isNull(columnLabel); });
}

bool MySQL_ResultSet::last()
{
    return (*executor_)([&]() { return impl_->last(); });
}

bool MySQL_ResultSet::next()
{
    return (*executor_)([&]() { return impl_->next(); });
}

void MySQL_ResultSet::moveToCurrentRow()
{
    (*executor_)([&]() { impl_->moveToCurrentRow(); });
}

void MySQL_ResultSet::moveToInsertRow()
{
    (*executor_)([&]() { impl_->moveToInsertRow(); });
}

bool MySQL_ResultSet::previous()
{
    return (*executor_)([&]() { return impl_->previous(); });
}

void MySQL_ResultSet::refreshRow()
{
    (*executor_)([&]() { impl_->refreshRow(); });
}

bool MySQL_ResultSet::relative(int rows)
{
    return (*executor_)([&]() { return impl_->relative(rows); });
}

bool MySQL_ResultSet::rowDeleted()
{
    return (*executor_)([&]() { return impl_->rowDeleted(); });
}

bool MySQL_ResultSet::rowInserted()
{
    return (*executor_)([&]() { return impl_->rowInserted(); });
}

bool MySQL_ResultSet::rowUpdated()
{
    return (*executor_)([&]() { return impl_->rowUpdated(); });
}

void MySQL_ResultSet::setFetchSize(size_t rows)
{
    (*executor_)([&]() { impl_->setFetchSize(rows); });
}

size_t MySQL_ResultSet::rowsCount() const
{
    return (*executor_)([&]() { return impl_->rowsCount(); });
}

bool MySQL_ResultSet::wasNull() const
{
    return (*executor_)([&]() { return impl_->wasNull(); });
}

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio
