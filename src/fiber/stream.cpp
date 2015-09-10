//
//  stream.cpp
//  fibio
//
//  Created by Chen Xu on 15-9-10.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#include <fibio/stream/fstream.hpp>

namespace fibio {
namespace stream {
namespace detail {

std::shared_ptr<fibers::foreign_thread_pool> get_default_executor() {
    static std::shared_ptr<fibers::foreign_thread_pool> default_executor;
    static std::once_flag executor_flag;
    std::call_once(executor_flag, [&](){
        default_executor.reset(new fibers::foreign_thread_pool(2));
    });
    return default_executor;
}

} // End of namespace detail
} // End of namespace stream
} // End of namespace fibio
