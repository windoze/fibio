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
    template<typename SuspendedItem>
    inline bool is_this_fiber(fiber_ptr_t f, const SuspendedItem &si) {
        return f==si.f_;
    }

    struct mutex_object/* : std::enable_shared_from_this<mutex_object>*/ {
        void lock(fiber_ptr_t this_fiber);
        bool try_lock(fiber_ptr_t this_fiber);
        void unlock(fiber_ptr_t this_fiber);
        
        spinlock mtx_;
        fiber_ptr_t owner_;
        waiting_queue_t suspended_;
    };
    
    struct recursive_mutex_object/* : std::enable_shared_from_this<recursive_mutex_object>*/ {
        recursive_mutex_object()
        : level_(0)
        {}
        
        void lock(fiber_ptr_t this_fiber);
        bool try_lock(fiber_ptr_t this_fiber);
        void unlock(fiber_ptr_t this_fiber);
        
        spinlock mtx_;
        size_t level_;
        fiber_ptr_t owner_;
        waiting_queue_t suspended_;
    };
    
    struct timed_mutex_object/* : std::enable_shared_from_this<timed_mutex_object>*/ {
        void lock(fiber_ptr_t this_fiber);
        bool try_lock(fiber_ptr_t this_fiber);
        bool try_lock_usec(fiber_ptr_t this_fiber, uint64_t usec);
        void unlock(fiber_ptr_t this_fiber);
        
        spinlock mtx_;
        fiber_ptr_t owner_;
        struct suspended_item {
            fiber_ptr_t f_;
            timer_ptr_t t_;
        };
        std::deque<suspended_item> suspended_;
    };
    
    struct recursive_timed_mutex_object/* : std::enable_shared_from_this<recursive_timed_mutex_object>*/ {
        recursive_timed_mutex_object()
        : level_(0)
        {}
        
        void lock(fiber_ptr_t this_fiber);
        bool try_lock(fiber_ptr_t this_fiber);
        bool try_lock_usec(fiber_ptr_t this_fiber, uint64_t usec);
        void unlock(fiber_ptr_t this_fiber);
        
        spinlock mtx_;
        size_t level_;
        fiber_ptr_t owner_;
        struct suspended_item {
            fiber_ptr_t f_;
            timer_ptr_t t_;
        };
        std::deque<suspended_item> suspended_;
    };
}}} // End of namespace fibio::fibers::detail

#endif /* defined(__fibio__mutex_object__) */
