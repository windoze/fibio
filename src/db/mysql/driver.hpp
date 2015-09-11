//
//  driver.hpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_db_mysql_driver_hpp
#define fibio_db_mysql_driver_hpp

#include <mysql_driver.h>

namespace fibio {
namespace db {
namespace mysql {

class MySQL_Driver : public sql::Driver, public std::enable_shared_from_this<MySQL_Driver>
{
public:
    virtual ~MySQL_Driver() override;

    virtual sql::Connection* connect(const sql::SQLString& hostName,
                                     const sql::SQLString& userName,
                                     const sql::SQLString& password) override;

    virtual sql::Connection* connect(sql::ConnectOptionsMap& options) override;

    virtual int getMajorVersion() override;

    virtual int getMinorVersion() override;

    virtual int getPatchVersion() override;

    virtual const sql::SQLString& getName() override;

    virtual void threadInit() override;

    virtual void threadEnd() override;

    // private:
    MySQL_Driver(sql::mysql::MySQL_Driver* impl,
                 std::shared_ptr<fibers::foreign_thread_pool> executor);
    // Executor is shared across all ops for this driver instance
    std::shared_ptr<fibers::foreign_thread_pool> executor_;
    // We don't own the driver instance
    sql::mysql::MySQL_Driver* impl_;
};

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio

#endif // !defined(fibio_db_mysql_driver_hpp)
