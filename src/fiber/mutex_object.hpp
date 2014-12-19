//
//  mutex_object.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef __fibio__mutex_object__
#define __fibio__mutex_object__

#include <memory>
#include <deque>
#include "fiber_object.hpp"

namespace fibio { namespace fibers { namespace detail {
    struct mutex_object {
        void lock(fiber_ptr_t this_fiber);
        bool try_lock(fiber_ptr_t this_fiber);
        void unlock(fiber_ptr_t this_fiber);
        
        spinlock mtx_;
        fiber_ptr_t owner_;
        std::deque<fiber_ptr_t> suspended_;
    };
    
    struct recursive_mutex_object {
        recursive_mutex_object()
        : level_(0)
        {}
        
        void lock(fiber_ptr_t this_fiber);
        bool try_lock(fiber_ptr_t this_fiber);
        void unlock(fiber_ptr_t this_fiber);
        
        spinlock mtx_;
        size_t level_;
        fiber_ptr_t owner_;
        std::deque<fiber_ptr_t> suspended_;
    };
    
    struct timed_mutex_object {
        void lock(fiber_ptr_t this_fiber);
        bool try_lock(fiber_ptr_t this_fiber);
        bool try_lock_rel(fiber_ptr_t this_fiber, duration_t d);
        void unlock(fiber_ptr_t this_fiber);
        
        spinlock mtx_;
        fiber_ptr_t owner_;
        struct suspended_item {
            fiber_ptr_t f_;
            timer_t *t_;
            bool operator==(fiber_ptr_t f) const { return f_==f; }
        };
        std::deque<suspended_item> suspended_;
    };
    
    struct recursive_timed_mutex_object {
        recursive_timed_mutex_object()
        : level_(0)
        {}
        
        void lock(fiber_ptr_t this_fiber);
        bool try_lock(fiber_ptr_t this_fiber);
        bool try_lock_rel(fiber_ptr_t this_fiber, duration_t d);
        void unlock(fiber_ptr_t this_fiber);
        
        spinlock mtx_;
        size_t level_;
        fiber_ptr_t owner_;
        struct suspended_item {
            fiber_ptr_t f_;
            timer_t *t_;
            bool operator==(fiber_ptr_t f) const { return f_==f; }
        };
        std::deque<suspended_item> suspended_;
    };
}}} // End of namespace fibio::fibers::detail

#endif /* defined(__fibio__mutex_object__) */
