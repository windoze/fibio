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

namespace fibio {
namespace fibers {

using std::cv_status;

struct condition_variable
{
    /// constructor
    condition_variable() = default;

    /**
     * notifies one waiting fiber
     */
    void notify_one();

    /**
     * notifies all waiting fibers
     */
    void notify_all();

    /**
     * blocks the current fiber until the condition variable is woken up
     */
    void wait(std::unique_lock<mutex>& lock);

    /**
     * blocks the current fiber until the condition variable is woken up
     * and the predicate returns `true`
     */
    template <class Predicate>
    void wait(std::unique_lock<mutex>& lock, Predicate pred)
    {
        while (!pred()) {
            wait(lock);
        }
    }

    /**
     * blocks the current thread until the condition variable is woken up
     * or after the specified timeout duration
     */
    template <class Rep, class Period>
    cv_status wait_for(std::unique_lock<mutex>& lock,
                       const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        return wait_rel(lock, std::chrono::duration_cast<detail::duration_t>(timeout_duration));
    }

    /**
     * blocks the current thread until the condition variable is woken up
     * or after the specified timeout duration and the predicate returns `true`
     */
    template <class Rep, class Period, class Predicate>
    bool wait_for(std::unique_lock<mutex>& lock,
                  const std::chrono::duration<Rep, Period>& rel_time,
                  Predicate pred)
    {
        while (!pred()) {
            if (cv_status::timeout == wait_for(lock, rel_time)) return pred();
        }
        return true;
    }

    /**
     * blocks the current thread until the condition variable is woken up
     * or until specified time point has been reached
     */
    template <class Clock, class Duration>
    cv_status wait_until(std::unique_lock<mutex>& lock,
                         const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        return wait_for(lock, timeout_time - Clock::now());
    }

    /**
     * blocks the current thread until the condition variable is woken up
     * or until specified time point has been reached and the predicate
     * returns `true`
     */
    template <class Clock, class Duration, class Predicate>
    bool wait_until(std::unique_lock<mutex>& lock,
                    const std::chrono::time_point<Clock, Duration>& timeout_time,
                    Predicate pred)
    {
        while (!pred()) {
            if (cv_status::timeout == wait_until(lock, timeout_time)) return pred();
        }
        return true;
    }

private:
    /// non-copyable
    condition_variable(const condition_variable&) = delete;

    void operator=(const condition_variable&) = delete;

    cv_status wait_rel(std::unique_lock<mutex>& lock, detail::duration_t d);

    void timeout_handler(detail::fiber_ptr_t this_fiber,
                         detail::timer_t* t,
                         cv_status& ret,
                         boost::system::error_code ec);

    detail::spinlock mtx_;

    struct suspended_item
    {
        mutex* m_;
        detail::fiber_ptr_t f_;
        detail::timer_t* t_;

        bool operator==(detail::fiber_ptr_t f) const { return f_ == f; }
    };

    std::deque<suspended_item> suspended_;
};

/**
 * schedules a call to notify_all to be invoked when this fiber is completely finished
 */
void notify_all_at_fiber_exit(condition_variable& cond, unique_lock<mutex> lk);

namespace detail {

template <typename MutexType>
struct lock_on_exit
{
    MutexType* m;

    lock_on_exit() : m(0) {}

    void activate(MutexType& m_)
    {
        m_.unlock();
        m = &m_;
    }

    ~lock_on_exit()
    {
        if (m) {
            m->lock();
        }
    }
};

} // End of namespace detail

class condition_variable_any
{
public:
    /// constructor
    condition_variable_any() {}

    /// destructor
    ~condition_variable_any() {}

    /**
     * notifies one waiting fiber
     */
    void notify_one() noexcept { cond_.notify_one(); }

    /**
     * notifies all waiting fibers
     */
    void notify_all() noexcept { cond_.notify_all(); }

    /**
     * blocks the current fiber until the condition variable is woken up
     */
    template <typename lock_type>
    void wait(lock_type& m)
    {
        detail::lock_on_exit<lock_type> guard;
        unique_lock<mutex> lk(mtx_);
        guard.activate(m);
        cond_.wait(lk);
    }

    /**
     * blocks the current fiber until the condition variable is woken up
     * and the predicate returns `true`
     */
    template <typename lock_type, typename predicate_type>
    void wait(lock_type& m, predicate_type pred)
    {
        while (!pred()) wait(m);
    }

    /**
     * blocks the current thread until the condition variable is woken up
     * or after the specified timeout duration
     */
    template <class lock_type, class Rep, class Period>
    cv_status wait_for(lock_type& lock, const std::chrono::duration<Rep, Period>& d)
    {
        detail::lock_on_exit<lock_type> guard;
        unique_lock<mutex> lk(mtx_);
        guard.activate(lock);
        return cond_.wait_for(lk, d);
    }

    /**
     * blocks the current thread until the condition variable is woken up
     * or after the specified timeout duration and the predicate returns `true`
     */
    template <class lock_type, class Rep, class Period, class Predicate>
    bool wait_for(lock_type& lock, const std::chrono::duration<Rep, Period>& d, Predicate pred)
    {
        while (!pred()) {
            if (wait_for(lock, d) == cv_status::timeout) return pred();
        }
        return true;
    }

    /**
     * blocks the current thread until the condition variable is woken up
     * or until specified time point has been reached
     */
    template <class lock_type, class Clock, class Duration>
    cv_status wait_until(lock_type& lock, const std::chrono::time_point<Clock, Duration>& t)
    {
        detail::lock_on_exit<lock_type> guard;
        unique_lock<mutex> lk(mtx_);
        guard.activate(lock);
        return cond_.wait_until(lk, t);
    }

    /**
     * blocks the current thread until the condition variable is woken up
     * or until specified time point has been reached and the predicate
     * returns `true`
     */
    template <class lock_type, class Clock, class Duration, class Predicate>
    bool
    wait_until(lock_type& lock, const std::chrono::time_point<Clock, Duration>& t, Predicate pred)
    {
        while (!pred()) {
            if (wait_until(lock, t) == cv_status::timeout) return pred();
        }
        return true;
    }

private:
    /// non-copyable
    condition_variable_any(const condition_variable_any&) = delete;

    void operator=(const condition_variable_any&) = delete;

    mutex mtx_;
    condition_variable cond_;
};

} // End of namespace fibers

using fibers::condition_variable;
using fibers::condition_variable_any;
using fibers::cv_status;
using fibers::notify_all_at_fiber_exit;

} // End of namespace fibio

#endif
