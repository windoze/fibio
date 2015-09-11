//
//  mysql.hpp
//  fibio
//
//  Created by Chen Xu on 15/09/11.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_db_mysql_hpp
#define fibio_db_mysql_hpp

#include <memory>

namespace sql {
class Driver;
}

namespace fibio {
namespace db {
namespace mysql {

std::shared_ptr<sql::Driver> get_driver_instance_by_name(const char* const clientlib);
std::shared_ptr<sql::Driver> get_driver_instance();

inline std::shared_ptr<sql::Driver> get_mysql_driver_instance()
{
    return get_driver_instance();
}

} // End of namespace mysql
} // End of namespace db
} // End of namespace fibio

#endif // !defined(fibio_db_mysql_hpp)
