//
//  mutex.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_mutex_hpp
#define fibio_mutex_hpp

#include <deque>
#include <memory>
#include <chrono>
#include <mutex>
#include <fibio/fibers/detail/forward.hpp>
#include <fibio/fibers/detail/spinlock.hpp>

namespace fibio { namespace fibers {
    class mutex {
    public:
        /// constructor
        mutex()=default;
        
        /**
         * locks the mutex, blocks if the mutex is not available
         */
        void lock();

        /**
         * tries to lock the mutex, returns if the mutex is not available
         */
        bool try_lock();
        
        /**
         * unlocks the mutex
         */
        void unlock();
    private:
        /// non-copyable
        mutex(const mutex&) = delete;
        void operator=(const mutex&) = delete;
        detail::spinlock mtx_;
        detail::fiber_ptr_t owner_;
        std::deque<detail::fiber_ptr_t> suspended_;
        friend struct condition_variable;
    };
    
    class timed_mutex {
    public:
        /// constructor
        timed_mutex()=default;

        /**
         * locks the mutex, blocks if the mutex is not available
         */
        void lock();

        /**
         * tries to lock the mutex, returns if the mutex is not available
         */
        bool try_lock();

        /**
         * tries to lock the mutex, returns if the mutex has been
         * unavailable for the specified timeout duration
         */
        template<class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep,Period>& timeout_duration ) {
            return try_lock_rel(std::chrono::duration_cast<detail::duration_t>(timeout_duration));
        }
        
        /**
         * tries to lock the mutex, returns if the mutex has been
         * unavailable until specified time point has been reached
         */
        template< class Clock, class Duration >
        bool try_lock_until(const std::chrono::time_point<Clock,Duration>& timeout_time ) {
            return try_lock_for(timeout_time-Clock::now());
        }
        
        /**
         * unlocks the mutex
         */
        void unlock();
    private:
        /// non-copyable
        timed_mutex(const timed_mutex&) = delete;
        void operator=(const timed_mutex&) = delete;
        bool try_lock_rel(detail::duration_t d);
        void timeout_handler(detail::fiber_ptr_t this_fiber,
                             boost::system::error_code ec);
        
        detail::spinlock mtx_;
        detail::fiber_ptr_t owner_;
        struct suspended_item {
            detail::fiber_ptr_t f_;
            detail::timer_t *t_;
            bool operator==(detail::fiber_ptr_t f) const { return f_==f; }
        };
        std::deque<suspended_item> suspended_;
    };
    
    class recursive_mutex {
    public:
        /// constructor
        recursive_mutex()=default;

        /**
         * locks the mutex, blocks if the mutex is not available
         */
        void lock();

        /**
         * tries to lock the mutex, returns if the mutex is not available
         */
        bool try_lock();

        /**
         * unlocks the mutex
         */
        void unlock();
        
    private:
        /// non-copyable
        recursive_mutex(const recursive_mutex&) = delete;
        void operator=(const recursive_mutex&) = delete;

        detail::spinlock mtx_;
        size_t level_=0;
        detail::fiber_ptr_t owner_;
        std::deque<detail::fiber_ptr_t> suspended_;
    };
    
    class recursive_timed_mutex {
    public:
        /// constructor
        recursive_timed_mutex()=default;

        /**
         * locks the mutex, blocks if the mutex is not available
         */
        void lock();
        
        /**
         * tries to lock the mutex, returns if the mutex is not available
         */
        bool try_lock();
        
        /**
         * unlocks the mutex
         */
        void unlock();
        
        /**
         * tries to lock the mutex, returns if the mutex has been
         * unavailable for the specified timeout duration
         */
        template<class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep,Period>& timeout_duration) {
            return try_lock_rel(std::chrono::duration_cast<detail::duration_t>(timeout_duration));
        }
        
        /**
         * tries to lock the mutex, returns if the mutex has been
         * unavailable until specified time point has been reached
         */
        template<class Clock, class Duration>
        bool try_lock_until(const std::chrono::time_point<Clock,Duration>& timeout_time) {
            return try_lock_for(timeout_time-Clock::now());
        }
        
    private:
        /// non-copyable
        recursive_timed_mutex(const recursive_timed_mutex&) = delete;
        void operator=(const recursive_timed_mutex&) = delete;
        bool try_lock_rel(detail::duration_t d);
        void timeout_handler(detail::fiber_ptr_t this_fiber, boost::system::error_code ec);
        detail::spinlock mtx_;
        size_t level_;
        detail::fiber_ptr_t owner_;
        struct suspended_item {
            detail::fiber_ptr_t f_;
            detail::timer_t *t_;
            bool operator==(detail::fiber_ptr_t f) const { return f_==f; }
        };
        std::deque<suspended_item> suspended_;
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::mutex;
    using fibers::timed_mutex;
    using fibers::recursive_mutex;
    using fibers::recursive_timed_mutex;
    
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
