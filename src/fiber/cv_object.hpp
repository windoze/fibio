//
//  cv_object.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef __fibio__cv_object__
#define __fibio__cv_object__

#include <memory>
#include <deque>
#include <mutex>
#include <fibio/fibers/condition_variable.hpp>
#include "fiber_object.hpp"
#include "mutex_object.hpp"

namespace fibio { namespace fibers { namespace detail {
    struct condition_variable_object/* : std::enable_shared_from_this<condition_variable_object> */{
        void wait(mutex_object *m, fiber_ptr_t this_fiber);
        cv_status wait_rel(mutex_object *m, fiber_ptr_t this_fiber, duration_t d);
        void notify_one();
        void notify_all();
        
        spinlock mtx_;
        struct suspended_item {
            mutex_object *m_;
            fiber_ptr_t f_;
            timer_t *t_;
        };
        std::deque<suspended_item> suspended_;
    };
}}} // End of namespace fibio::fibers::detail

#endif /* defined(__fibio__cv_object__) */
