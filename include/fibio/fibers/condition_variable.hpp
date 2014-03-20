//
//  condition_variable.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_condition_variable_hpp
#define fibio_condition_variable_hpp

#include <memory>
#include <chrono>
#include <condition_variable>
#include <fibio/fibers/detail/forward.hpp>
#include <fibio/fibers/mutex.hpp>

namespace fibio { namespace fibers {
    using std::cv_status;

    struct condition_variable {
        condition_variable();
        condition_variable(const condition_variable&) = delete;
        
        void wait(std::unique_lock<mutex>& lock);
        
        template< class Predicate >
        void wait(std::unique_lock<mutex>& lock, Predicate pred) {
            while (!pred()) {
                wait(lock);
            }
        }
        
        template< class Rep, class Period >
        cv_status wait_for(std::unique_lock<mutex>& lock, const std::chrono::duration<Rep,Period>& timeout_duration) {
            return wait_usec(lock,
                             std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count());
        }
        
        template< class Rep, class Period, class Predicate >
        bool wait_for(std::unique_lock<mutex>& lock,
                      const std::chrono::duration<Rep, Period>& rel_time,
                      Predicate pred)
        {
            while (!pred()){
                if (cv_status::timeout == wait_for( lock, rel_time))
                    return pred();
            }
            return true;
        }
        
        template< class Clock, class Duration >
        cv_status wait_until(std::unique_lock<mutex>& lock, const std::chrono::time_point<Clock,Duration>& timeout_time) {
            return wait_usec(lock,
                             std::chrono::duration_cast<std::chrono::microseconds>(timeout_time - std::chrono::system_clock::now()).count());
        }
        
        template< class Clock, class Duration, class Predicate >
        bool wait_until(std::unique_lock<mutex>& lock,
                        const std::chrono::time_point<Clock, Duration>& timeout_time,
                        Predicate pred)
        {
            while (!pred()){
                if (cv_status::timeout == wait_until(lock, timeout_time))
                    return pred();
            }
            return true;
        }
        
        void notify_one();
        void notify_all();
        
        cv_status wait_usec(std::unique_lock<mutex>& lock, uint64_t usec);
        std::shared_ptr<detail::condition_variable_object> m_;
    };

    void notify_all_at_thread_exit(condition_variable &cond, unique_lock<mutex> lk);
    
    namespace detail {
        template<typename MutexType>
        struct lock_on_exit {
            MutexType* m;
            lock_on_exit()
            : m(0)
            {}
            
            void activate(MutexType& m_) {
                m_.unlock();
                m=&m_;
            }
            
            ~lock_on_exit() {
                if(m) {
                    m->lock();
                }
            }
        };
    }
    
    class condition_variable_any {
        mutex internal_mutex;
        condition_variable cond;
        
    public:
        condition_variable_any(const condition_variable_any &)=delete;
        
        condition_variable_any(){}
        ~condition_variable_any(){}
        
        template<typename lock_type>
        void wait(lock_type& m) {
            detail::lock_on_exit<lock_type> guard;
            unique_lock<mutex> lk(internal_mutex);
            guard.activate(m);
            cond.wait(lk);
        }
        
        template<typename lock_type,typename predicate_type>
        void wait(lock_type& m,predicate_type pred) {
            while(!pred()) wait(m);
        }
        
        template <class lock_type, class Clock, class Duration>
        cv_status wait_until(lock_type& lock, const std::chrono::time_point<Clock, Duration>& t) {
            detail::lock_on_exit<lock_type> guard;
            unique_lock<mutex> lk(internal_mutex);
            guard.activate(lock);
            return cond.wait_until(lk, t);
        }
        
        template <class lock_type, class Clock, class Duration, class Predicate>
        bool wait_until(lock_type& lock, const std::chrono::time_point<Clock, Duration>& t, Predicate pred) {
            while (!pred()) {
                if (wait_until(lock, t) == cv_status::timeout)
                    return pred();
            }
            return true;
        }
        
        
        template <class lock_type, class Rep, class Period>
        cv_status wait_for(lock_type& lock, const std::chrono::duration<Rep, Period>& d) {
            detail::lock_on_exit<lock_type> guard;
            unique_lock<mutex> lk(internal_mutex);
            guard.activate(lock);
            return cond.wait_for(lk, d);
        }
        
        
        template <class lock_type, class Rep, class Period, class Predicate>
        bool wait_for(lock_type& lock, const std::chrono::duration<Rep, Period>& d, Predicate pred) {
            while (!pred()) {
                if (wait_for(lock, d) == cv_status::timeout)
                    return pred();
            }
            return true;
        }
        
        void notify_one() noexcept {
            cond.notify_one();
        }
        
        void notify_all() noexcept {
            cond.notify_all();
        }
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::condition_variable;
    using fibers::condition_variable_any;
    using fibers::cv_status;
    using fibers::notify_all_at_thread_exit;
}   // End of namespace fibio

#endif
