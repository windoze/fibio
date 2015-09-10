//
//  test_fstream.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <fibio/iostream.hpp>
#include <fibio/fiberize.hpp>

using namespace fibio;

void test_fstream()
{
    {
        ofstream f("/tmp/test_file");
        f << "Hello, World!\n";
        f.flush();
    }

    {
        ifstream f("/tmp/test_file");
        std::string l;
        std::getline(f, l);
        assert(l == "Hello, World!");
    }
}

int fibio::main(int argc, char* argv[])
{
    fiber_group fg;
    fg.create_fiber(test_fstream);
    fg.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
