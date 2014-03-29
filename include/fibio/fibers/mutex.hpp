//
//  mutex.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_mutex_hpp
#define fibio_mutex_hpp

#include <memory>
#include <chrono>
#include <mutex>
#include <fibio/fibers/detail/forward.hpp>

namespace fibio { namespace fibers {
    struct mutex {
        mutex();
        mutex(const mutex&) = delete;
        
        void lock();
        void unlock();
        bool try_lock();
        
        std::shared_ptr<detail::mutex_object> m_;
    };
    
    struct timed_mutex {
        timed_mutex();
        timed_mutex(const timed_mutex&) = delete;
        
        void lock();
        void unlock();
        bool try_lock();
        
        template<class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep,Period>& timeout_duration ) {
            return try_lock_usec(std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count());
        }
        
        template< class Clock, class Duration >
        bool try_lock_until(const std::chrono::time_point<Clock,Duration>& timeout_time ) {
            return try_lock_usec(std::chrono::duration_cast<std::chrono::microseconds>(timeout_time - std::chrono::steady_clock::now()).count());
        }
        
        bool try_lock_usec(uint64_t usec);
        std::shared_ptr<detail::timed_mutex_object> m_;
    };
    
    struct recursive_mutex {
        recursive_mutex();
        recursive_mutex(const recursive_mutex&) = delete;
        
        void lock();
        void unlock();
        bool try_lock();
        
        std::shared_ptr<detail::recursive_mutex_object> m_;
    };
    
    struct timed_recursive_mutex {
        timed_recursive_mutex();
        timed_recursive_mutex(const timed_recursive_mutex&) = delete;
        
        void lock();
        void unlock();
        bool try_lock();
        
        template<class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep,Period>& timeout_duration) {
            return try_lock_usec(std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count());
        }
        
        template<class Clock, class Duration>
        bool try_lock_until(const std::chrono::time_point<Clock,Duration>& timeout_time) {
            return try_lock_usec(std::chrono::duration_cast<std::chrono::microseconds>(timeout_time - std::chrono::steady_clock::now()).count());
        }
        
        bool try_lock_usec(uint64_t usec);
        std::shared_ptr<detail::timed_recursive_mutex_object> m_;
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::mutex;
    using fibers::timed_mutex;
    using fibers::recursive_mutex;
    using fibers::timed_recursive_mutex;
    
    using std::lock_guard;
    using std::unique_lock;
    using std::defer_lock_t;
    using std::try_to_lock_t;
    using std::adopt_lock_t;
    using std::defer_lock;
    using std::try_to_lock;
    using std::adopt_lock;
    using std::try_lock;
    using std::lock;
}   // End of namespace fibio

#endif
