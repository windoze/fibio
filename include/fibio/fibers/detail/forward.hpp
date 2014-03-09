//
//  forward.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_forward_hpp
#define fibio_forward_hpp

namespace fibio { namespace fibers { namespace detail {
    struct scheduler_object;
    struct fiber_object;
    struct mutex_object;
    struct recursive_mutex_object;
    struct timed_mutex_object;
    struct timed_recursive_mutex_object;
    struct condition_variable_object;
}}} // End of namespace fibio::fiber::detail

#endif
