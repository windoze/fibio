//
//  driver.cpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#include <fibio/fibers/future/async.hpp>
#include <mysql_driver.h>
#include "driver.hpp"
#include "connection.hpp"

namespace fibio {
namespace db {
namespace mysql {

MySQL_Driver::MySQL_Driver(sql::mysql::MySQL_Driver* impl,
                           std::shared_ptr<foreign_thread_pool> executor)
: executor_(executor), impl_(impl)
{
    threadInit();
}

MySQL_Driver::~MySQL_Driver()
{
    threadEnd();
}

sql::Connection* MySQL_Driver::connect(const sql::SQLString& hostName,
                                       const sql::SQLString& userName,
                                       const sql::SQLString& password)
{
    sql::Connection* conn
        = (*executor_)([&]() { return impl_->connect(hostName, userName, password); });
    return new MySQL_Connection(this, conn, executor_);
}

sql::Connection* MySQL_Driver::connect(sql::ConnectOptionsMap& options)
{
    sql::Connection* conn = (*executor_)([&]() { return impl_->connect(options); });
    return new MySQL_Connection(this, conn, executor_);
}

int MySQL_Driver::getMajorVersion()
{
    return (*executor_)([&]() { return impl_->getMajorVersion(); });
}

int MySQL_Driver::getMinorVersion()
{
    return (*executor_)([&]() { return impl_->getMinorVersion(); });
}

int MySQL_Driver::getPatchVersion()
{
    return (*executor_)([&]() { return impl_->getPatchVersion(); });
}

const sql::SQLString& MySQL_Driver::getName()
{
    return (*executor_)([&]() -> const sql::SQLString& { return impl_->getName(); });
}

void MySQL_Driver::threadInit()
{
    (*executor_)([&]() { impl_->threadInit(); });
}

void MySQL_Driver::threadEnd()
{
    (*executor_)([&]() { impl_->threadEnd(); });
}

std::shared_ptr<MySQL_Driver>
create_driver_instance(sql::mysql::MySQL_Driver* drv,
                       std::shared_ptr<fibers::foreign_thread_pool> executor)
{
    auto md = std::make_shared<MySQL_Driver>(drv, executor);
    return md;
}

std::shared_ptr<sql::Driver> get_driver_instance_by_name(const char* const clientlib)
{
    std::shared_ptr<fibers::foreign_thread_pool> executor
        = std::make_shared<fibers::foreign_thread_pool>();
    sql::mysql::MySQL_Driver* drv = (*executor)(sql::mysql::get_driver_instance_by_name, clientlib);
    return create_driver_instance(drv, executor);
}

std::shared_ptr<sql::Driver> get_driver_instance()
{
    std::shared_ptr<fibers::foreign_thread_pool> executor
        = std::make_shared<fibers::foreign_thread_pool>();
    sql::mysql::MySQL_Driver* drv = (*executor)(sql::mysql::get_driver_instance);
    return create_driver_instance(drv, executor);
}

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio
