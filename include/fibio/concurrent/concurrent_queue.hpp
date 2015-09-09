//
//  concurrent_queue.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_concurrent_queue_hpp
#define fibio_concurrent_queue_hpp

#include <limits>
#include <deque>
#include <queue>
#include <iterator>
#include <fibio/fibers/mutex.hpp>
#include <fibio/fibers/condition_variable.hpp>

namespace fibio {
namespace concurrent {

/**
 * Similiar to N3533 concurrent_queue with some differences
 * Don't use `busy` here as this is *not* a lock-free queue
 */

enum class queue_op_status
{
    success,
    empty,
    full,
    closed,
};

template <typename T, typename LockType, typename CVType, typename Container = std::deque<T>>
struct basic_concurrent_queue
{
    typedef basic_concurrent_queue<T, LockType, CVType, Container> this_type;
    typedef std::queue<T, Container> queue_type;
    typedef typename LockType::mutex_type mutex_type;

    typedef typename queue_type::container_type container_type;
    typedef typename queue_type::value_type value_type;
    typedef typename queue_type::size_type size_type;
    typedef typename queue_type::reference reference;
    typedef typename queue_type::const_reference const_reference;

    /**
     * Constructor construct a concurrent queue
     * @param capacity capacity of the queue, default to be unlimited
     * @param auto_open true indicates the queue is created in open state
     */
    inline explicit basic_concurrent_queue(size_type capacity
                                           = std::numeric_limits<size_type>::max(),
                                           bool auto_open = true)
    : opened_(auto_open), capacity_(capacity)
    {
    }

    /**
     * Open the queue, only opened queue can accept new elements
     */
    inline bool open()
    {
        LockType lock(the_mutex_);
        opened_ = true;
        return opened_;
    }

    /**
     * Close the queue, closed queue cannot have new elements pushed in
     */
    inline void close()
    {
        LockType lock(the_mutex_);
        opened_ = false;
        full_cv_.notify_all();
        empty_cv_.notify_all();
    }

    /**
     * Returns true if the queue is open
     */
    inline bool is_open() const
    {
        LockType lock(the_mutex_);
        return opened_;
    }

    /**
     * Push an element into the queue, block if the queue is full
     */
    inline queue_op_status push(const T& data)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        // Wait until queue is closed or not full
        while ((the_queue_.size() >= capacity_) && opened_) {
            full_cv_.wait(lock);
        }
        if (!opened_) {
            // Queue closed
            return queue_op_status::closed;
        }
        the_queue_.push(data);
        empty_cv_.notify_one();
        return queue_op_status::success;
    }

    /**
     * Push an element into the queue, block if the queue is full
     */
    inline queue_op_status push(T&& data)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        // Wait until queue is closed or not full
        while ((the_queue_.size() >= capacity_) && opened_) {
            full_cv_.wait(lock);
        }
        if (!opened_) {
            // Queue closed
            return queue_op_status::closed;
        }
        the_queue_.push(std::move(data));
        empty_cv_.notify_one();
        return queue_op_status::success;
    }

    /**
     * Push an element into the queue, block if the queue is full
     * std::back_inserter support
     */
    inline void push_back(const T& data) { push(data); }

    /**
     * Push an element into the queue, block if the queue is full
     * std::back_inserter support
     */
    inline void push_back(T&& data) { push(std::move(data)); }

    /**
     * Push some elements into the queue, returns when all elements are pushed or queue is full
     * @return return iterator next to last pushed element
     */
    template <typename InIterator>
    inline InIterator push_some(InIterator first, InIterator last)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return first;
        }
        for (; first != last && the_queue_.size() < capacity_; ++first) {
            the_queue_.push(*first);
            empty_cv_.notify_one();
        }
        return first;
    }

    /**
     * Push some elements into the queue, blocks until all elements are pushed
     * @return return queue_op_status::close if queue is closed
     */
    template <typename InIterator>
    inline queue_op_status push_all(InIterator first, InIterator last)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        while (first != last) {
            the_queue_.push(*first);
            empty_cv_.notify_one();
            ++first;
        }
        return queue_op_status::success;
    }

    /**
     * Try push an elements into the queue without blocking
     * @return return queue_op_status::success if element is pushed, other values indicate failure
     */
    inline queue_op_status try_push(const T& data)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        // Check if the queue is full
        if (the_queue_.size() >= capacity_) {
            return queue_op_status::full;
        }
        the_queue_.push(std::move(data));
        empty_cv_.notify_one();
        return queue_op_status::success;
    }

    /**
     * Try push an elements into the queue without blocking
     * @return return queue_op_status::success if element is pushed, other values indicate failure
     */
    inline queue_op_status try_push(T&& data)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        // Check if the queue is full
        if (the_queue_.size() >= capacity_) {
            return queue_op_status::full;
        }
        the_queue_.push(std::move(data));
        empty_cv_.notify_one();
        return queue_op_status::success;
    }

    /**
     * Try push an elements into the queue, wait for `timeout_duration`.
     * @return return queue_op_status::success if element is pushed, other values indicate failure
     */
    template <class Rep, class Period>
    inline queue_op_status try_push_for(const T& data,
                                        const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        // Wait until queue is closed or not full
        std::cv_status ret = cv_status::no_timeout;
        while ((the_queue_.size() >= capacity_) && opened_) {
            ret = full_cv_.wait_for(lock, timeout_duration);
        }
        if (!opened_) {
            // Queue closed
            return queue_op_status::closed;
        }
        if (ret == cv_status::no_timeout) {
            the_queue_.push(std::move(data));
            empty_cv_.notify_one();
            return queue_op_status::success;
        }
        return queue_op_status::full;
    }

    /**
     * Try push an elements into the queue, wait for `timeout_duration`.
     * @return return queue_op_status::success if element is pushed, other values indicate failure
     */
    template <class Rep, class Period>
    inline queue_op_status try_push_for(T&& data,
                                        const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        // Wait until queue is closed or not full
        std::cv_status ret = cv_status::no_timeout;
        while ((the_queue_.size() >= capacity_) && opened_) {
            ret = full_cv_.wait_for(lock, timeout_duration);
        }
        if (!opened_) {
            // Queue closed
            return queue_op_status::closed;
        }
        if (ret == cv_status::no_timeout) {
            the_queue_.push(std::move(data));
            empty_cv_.notify_one();
            return queue_op_status::success;
        }
        return queue_op_status::full;
    }

    /**
     * Try push an elements into the queue, wait until `timeout_time` reached.
     * @return return queue_op_status::success if element is pushed, other values indicate failure
     */
    template <class Clock, class Duration>
    inline queue_op_status
    try_push_until(const T& data, const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        // Wait until queue is closed or not full
        std::cv_status ret = cv_status::no_timeout;
        while ((the_queue_.size() >= capacity_) && opened_) {
            ret = full_cv_.wait_until(lock, timeout_time);
        }
        if (!opened_) {
            // Queue closed
            return queue_op_status::closed;
        }
        if (ret == cv_status::no_timeout) {
            the_queue_.push(std::move(data));
            empty_cv_.notify_one();
            return queue_op_status::success;
        }
        return queue_op_status::full;
    }

    /**
     * Try push an elements into the queue, wait until `timeout_time` reached.
     * @return return queue_op_status::success if element is pushed, other values indicate failure
     */
    template <class Clock, class Duration>
    inline queue_op_status
    try_push_until(T&& data, const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        LockType lock(the_mutex_);
        if (!opened_) {
            // Cannot push into a closed queue
            return queue_op_status::closed;
        }
        // Wait until queue is closed or not full
        std::cv_status ret = cv_status::no_timeout;
        while ((the_queue_.size() >= capacity_) && opened_) {
            ret = full_cv_.wait_for(lock, timeout_time);
        }
        if (!opened_) {
            // Queue closed
            return queue_op_status::closed;
        }
        if (ret == cv_status::no_timeout) {
            the_queue_.push(std::move(data));
            empty_cv_.notify_one();
            return queue_op_status::success;
        }
        return queue_op_status::full;
    }

    /**
     * Blocks until an element is popped from the queue
     * @return return queue_op_status::success if element is popped, other values indicate failure
     */
    inline queue_op_status pop(T& popped_value)
    {
        LockType lock(the_mutex_);
        // Wait only if the queue is open and empty
        while (the_queue_.empty() && opened_) {
            empty_cv_.wait(lock);
        }
        if (the_queue_.empty()) {
            // Last loop ensure queue will not empty only if queue is closed
            // So here we have an empty and closed queue
            return queue_op_status::closed;
        }
        std::swap(popped_value, the_queue_.front());
        the_queue_.pop();
        full_cv_.notify_one();
        return queue_op_status::success;
    }

    /**
     * Try to pop an element from the queue without blocking
     * @return return queue_op_status::success if element is popped, other values indicate failure
     */
    inline queue_op_status try_pop(T& popped_value)
    {
        LockType lock(the_mutex_);
        if (the_queue_.empty()) {
            return queue_op_status::empty;
        }
        std::swap(popped_value, the_queue_.front());
        the_queue_.pop();
        full_cv_.notify_one();
        return queue_op_status::success;
    }

    /**
     * Try to pop an element from the queue, wait for `timeout_duration`
     * @return return queue_op_status::success if element is popped, other values indicate failure
     */
    template <class Rep, class Period>
    inline queue_op_status try_pop_for(T& popped_value,
                                       const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        LockType lock(the_mutex_);
        // Wait only if the queue is open and empty
        std::cv_status ret = cv_status::no_timeout;
        while (the_queue_.empty() && opened_) {
            ret = empty_cv_.wait_for(lock, timeout_duration);
        }
        if (the_queue_.empty()) {
            // Last loop ensure queue will not empty only if queue is closed
            // So here we have an empty and closed queue
            return queue_op_status::closed;
        }
        if (ret == cv_status::no_timeout) {
            std::swap(popped_value, the_queue_.front());
            the_queue_.pop();
            full_cv_.notify_one();
            return queue_op_status::success;
        }
        return queue_op_status::empty;
    }

    /**
     * Try to pop an element from the queue, wait until `timeout_time` reached
     * @return return queue_op_status::success if element is popped, other values indicate failure
     */
    template <class Clock, class Duration>
    inline queue_op_status
    try_pop_until(T& popped_value, const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        LockType lock(the_mutex_);
        // Wait only if the queue is open and empty
        std::cv_status ret = cv_status::no_timeout;
        while (the_queue_.empty() && opened_) {
            ret = empty_cv_.wait_until(lock, timeout_time);
        }
        if (the_queue_.empty()) {
            // Last loop ensure queue will not empty only if queue is closed
            // So here we have an empty and closed queue
            return queue_op_status::closed;
        }
        if (ret == cv_status::no_timeout) {
            std::swap(popped_value, the_queue_.front());
            the_queue_.pop();
            full_cv_.notify_one();
            return queue_op_status::success;
        }
        return queue_op_status::empty;
    }

    /**
     * Pop elements from the queue and push them into an output iterator
     * Returns until `nelem` elements are popped or queue becomes empty
     * @param oi output iterator receives popped elements
     * @nelem at most `nelem` elements should be popped
     */
    // Pop at most nelem items
    template <typename OutIterator>
    inline OutIterator pop_some(OutIterator oi,
                                size_type nelem = std::numeric_limits<size_type>::max())
    {
        LockType lock(the_mutex_);
        if (the_queue_.empty()) {
            return false;
        }
        nelem = std::min(the_queue_.size(), nelem);
        size_type i = 0;
        for (; i < nelem; i++) {
            std::swap(*oi, the_queue_.top());
            the_queue_.pop();
            oi++;
            full_cv_.notify_one();
        }
        return oi;
    }

    /**
     * Returns true indicates the queue is empty
     * NOTE: The return value is just a snapshot, queue state may change
     *       when caller gets the returned value
     */
    inline bool empty() const
    {
        LockType lock(the_mutex_);
        return the_queue_.empty();
    }

    /**
     * Returns true indicates the queue is full
     * NOTE: The return value is just a snapshot, queue state may change
     *       when caller gets the returned value
     */
    inline bool full() const
    {
        LockType lock(the_mutex_);
        return the_queue_.size() >= capacity_;
    }

    /**
     * Returns the number of elements holding in the queue
     * NOTE: The return value is just a snapshot, queue state may change
     *       when caller gets the returned value
     */
    inline size_type size() const
    {
        LockType lock(the_mutex_);
        return the_queue_.size();
    }

    /**
     * Returns the max number of elements the queue can hold
     */
    inline size_type capacity() const { return capacity_; }

    /**
     * Minimal range-based for loop support
     * It's not a fully functional iterator and should not be used directly
     */
    struct iterator : std::iterator<std::input_iterator_tag, T>
    {
        iterator(iterator&& other) = default;

        bool operator!=(const iterator& other) const
        {
            // Only ended iterators are equal
            return !(ended() && other.ended());
        }

        iterator& operator++()
        {
            popped_ = queue_->pop(value_);
            if (popped_ != queue_op_status::success) queue_ = 0;
            return *this;
        }

        value_type& operator*() { return value_; }

        value_type* operator->() { return &value_; }

    private:
        bool ended() const { return !queue_; }

        iterator() : queue_(0), popped_(queue_op_status::success) {}

        iterator(this_type* queue) : queue_(queue), popped_(queue_op_status::success)
        {
            operator++();
        }

        iterator(const iterator& other) = delete;

        iterator& operator=(const iterator& other) = delete;

        this_type* queue_;
        value_type value_;
        queue_op_status popped_;
        friend struct basic_concurrent_queue;
    };

    /**
     * Minimal range-based for loop support
     * Returns an iterator to the first element of the container.
     */
    iterator begin() { return iterator(this); }

    /**
     * Minimal range-based for loop support
     * Returns an iterator indicates the queue is empty and closed.
     */
    iterator end() const { return iterator(); }

private:
    // Non-copyable, non-movable
    basic_concurrent_queue(const basic_concurrent_queue&) = delete;

    basic_concurrent_queue(basic_concurrent_queue&&) = delete;

    void operator=(const basic_concurrent_queue&) = delete;

    bool opened_;
    const size_t capacity_;
    mutable mutex_type the_mutex_;
    CVType full_cv_;
    CVType empty_cv_;
    queue_type the_queue_;
};

template <typename T, typename Container = std::deque<T>>
using concurrent_queue = concurrent::
    basic_concurrent_queue<T, unique_lock<fibers::mutex>, fibers::condition_variable, Container>;
} // End of namespace concurrent
} // End of namespace fibio

#endif
