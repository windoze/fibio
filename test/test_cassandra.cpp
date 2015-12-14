//
// test_cassandra.cpp
// fibio
//
// Created by Chen Xu on 15/12/12.
// Copyright (c) 2015 0d0a.com. All rights reserved. 
//

#include <boost/uuid/uuid_io.hpp>
#include <fibio/fiberize.hpp>
#include <fibio/db/cassandra.hpp>

int fibio::main(int argc, char** argv)
{
    namespace cass=fibio::db::cassandra;

    cass::cluster c("127.0.0.1");
    cass::session s = c.connect();
    cass::statement stat("SELECT now() FROM system.local;", 0);
    cass::result_set rs = s.execute(stat);
    std::cout << rs.size() << std::endl;
    for (auto&& r : rs) {
        cass::value v=r[0];
        std::cout << v.get_uuid() << "\n";
    }
    return 0;
}
