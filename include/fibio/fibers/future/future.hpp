//
//  future.hpp
//  fibio
//
//  Base on Boost.Fiber at https://github.com/olk/boost-fiber
//
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef fibio_fibers_future_future_hpp
#define fibio_fibers_future_future_hpp

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>
#include <fibio/utility.hpp>
#include <fibio/fibers/exceptions.hpp>
#include <fibio/fibers/future/future_status.hpp>
#include <fibio/fibers/future/detail/shared_state_object.hpp>

namespace fibio {
namespace fibers {

template <typename>
class packaged_task;

template <typename R>
class promise;

template <typename R>
class future;

template <typename R>
class shared_future;

namespace detail {

template <typename T>
struct is_future : std::integral_constant<bool, false>
{
};

template <typename T>
struct is_future<future<T>> : std::integral_constant<bool, true>
{
};

template <typename T>
struct is_future<shared_future<T>> : std::integral_constant<bool, true>
{
};

template <std::size_t N, typename... Futures>
struct any_waiter;

template <typename... Futures>
struct all_waiter;

template <std::size_t N, typename... Futures>
struct async_any_waiter;

template <typename... Futures>
struct async_all_waiter;

} // End of namespace detail

template <typename Iterator>
auto wait_for_any(Iterator begin, Iterator end) ->
    typename std::enable_if<!detail::is_future<Iterator>::value, Iterator>::type;

template <typename Iterator>
auto wait_for_all(Iterator begin, Iterator end) ->
    typename std::enable_if<!detail::is_future<Iterator>::value>::type;

template <typename Iterator>
auto async_wait_for_any(Iterator begin, Iterator end) ->
    typename std::enable_if<!detail::is_future<Iterator>::value, future<Iterator>>::type;

template <typename Iterator>
auto async_wait_for_all(Iterator begin, Iterator end) ->
    typename std::enable_if<!detail::is_future<Iterator>::value, future<void>>::type;

template <typename R>
class future : private boost::noncopyable
{
private:
    typedef typename detail::shared_state<R>::ptr_t ptr_t;

    template <typename>
    friend class packaged_task;

    friend class promise<R>;

    friend class shared_future<R>;

    template <std::size_t N, typename... Futures>
    friend struct detail::any_waiter;
    template <typename... Futures>
    friend struct detail::all_waiter;

    template <typename Iterator>
    friend auto wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, Iterator>::type;

    template <typename Iterator>
    friend auto wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value>::type;

    template <std::size_t N, typename... Futures>
    friend struct detail::async_any_waiter;
    template <typename... Futures>
    friend struct detail::async_all_waiter;

    template <typename Iterator>
    friend auto async_wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<Iterator>>::type;

    template <typename Iterator>
    friend auto async_wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<void>>::type;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    ptr_t state_;

    future(const future&) = delete;

    future& operator=(const future&) = delete;

    future(ptr_t const& p) : state_(p) {}

public:
    /// Constructs a future with no shared state
    /// after construction, valid() == false
    future() noexcept : state_() {}

    /// Abandon ownership if any
    ~future() {}

    /// Constructs a future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    future(future&& other) noexcept : state_() { swap(other); }

    /// Releases any shared state and move-assigns the contents of other to *this
    /// after the assignment, other.valid() == false and this->valid() will yield
    /// the same value as other.valid() before the assignment
    future& operator=(future&& other) noexcept
    {
        future tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    /// Exchange the shared states of two futures
    void swap(future& other) noexcept { state_.swap(other.state_); }

    operator safe_bool() const noexcept { return valid() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return !valid(); }

    /// Checks if the future refers to a shared state
    /// this is the case only for futures returned by
    /// promise::get_future(), packaged_task::get_future()
    /// or async() until the first time get()or share() is called
    bool valid() const noexcept { return 0 != state_.get(); }

    shared_future<R> share();

    /// The get method waits until the future has a valid result and
    /// (depending on which template is used) retrieves it
    /// it effectively calls wait() in order to wait for the result
    /// the value stored in the shared state
    /// if it satisfies the requirements of MoveAssignable, the value is moved,
    /// otherwise it is copied
    /// valid() == false after a call to this method.
    /// detect the case when valid == false before the call and throw a
    /// future_error with an error condition of future_errc::no_state
    R get()
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        ptr_t tmp;
        tmp.swap(state_);
        return tmp->get();
    }

    /// Blocks until the result becomes available
    /// valid() == true after the call
    void wait() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        state_->wait();
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    template <class Rep, class Period>
    future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_for(timeout_duration);
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    future_status wait_until(clock_type::time_point const& timeout_time) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_until(timeout_time);
    }

    template <typename F>
    future<typename std::result_of<F(future&)>::type> then(F&& func);
};

template <typename R>
class future<R&> : private boost::noncopyable
{
private:
    typedef typename detail::shared_state<R&>::ptr_t ptr_t;

    template <typename>
    friend class packaged_task;

    friend class promise<R&>;

    friend class shared_future<R&>;

    template <std::size_t N, typename... Futures>
    friend struct detail::any_waiter;
    template <typename... Futures>
    friend struct detail::all_waiter;

    template <typename Iterator>
    friend auto wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, Iterator>::type;

    template <typename Iterator>
    friend auto wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value>::type;

    template <std::size_t N, typename... Futures>
    friend struct detail::async_any_waiter;
    template <typename... Futures>
    friend struct detail::async_all_waiter;

    template <typename Iterator>
    friend auto async_wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<Iterator>>::type;

    template <typename Iterator>
    friend auto async_wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<void>>::type;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    ptr_t state_;

    future(const future&) = delete;

    future& operator=(const future&) = delete;

    future(ptr_t const& p) : state_(p) {}

public:
    /// Constructs a future with no shared state
    /// after construction, valid() == false
    future() noexcept : state_() {}

    /// Abandon ownership if any
    ~future() {}

    /// Constructs a future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    future(future&& other) noexcept : state_() { swap(other); }

    /// Releases any shared state and move-assigns the contents of other to *this
    /// after the assignment, other.valid() == false and this->valid() will yield
    /// the same value as other.valid() before the assignment
    future& operator=(future&& other) noexcept
    {
        future tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    /// Exchange the shared states of two futures
    void swap(future& other) noexcept { state_.swap(other.state_); }

    operator safe_bool() const noexcept { return valid() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return !valid(); }

    /// Checks if the future refers to a shared state
    /// this is the case only for futures returned by
    /// promise::get_future(), packaged_task::get_future()
    /// or async() until the first time get()or share() is called
    bool valid() const noexcept { return 0 != state_.get(); }

    shared_future<R&> share();

    /// The get method waits until the future has a valid result and
    /// (depending on which template is used) retrieves it
    /// it effectively calls wait() in order to wait for the result
    /// the value stored in the shared state
    /// if it satisfies the requirements of MoveAssignable, the value is moved,
    /// otherwise it is copied
    /// valid() == false after a call to this method.
    /// detect the case when valid == false before the call and throw a
    /// future_error with an error condition of future_errc::no_state
    R& get()
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        ptr_t tmp;
        tmp.swap(state_);
        return tmp->get();
    }

    /// Blocks until the result becomes available
    /// valid() == true after the call
    void wait() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        state_->wait();
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    template <class Rep, class Period>
    future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_for(timeout_duration);
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    future_status wait_until(clock_type::time_point const& timeout_time) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_until(timeout_time);
    }

    template <typename F>
    future<typename std::result_of<F(future&)>::type> then(F&& func);
};

template <>
class future<void> : private boost::noncopyable
{
private:
    typedef detail::shared_state<void>::ptr_t ptr_t;

    template <typename>
    friend class packaged_task;

    friend class promise<void>;

    friend class shared_future<void>;

    template <std::size_t N, typename... Futures>
    friend struct detail::any_waiter;
    template <typename... Futures>
    friend struct detail::all_waiter;

    template <typename Iterator>
    friend auto wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, Iterator>::type;

    template <typename Iterator>
    friend auto wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value>::type;

    template <std::size_t N, typename... Futures>
    friend struct detail::async_any_waiter;
    template <typename... Futures>
    friend struct detail::async_all_waiter;

    template <typename Iterator>
    friend auto async_wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<Iterator>>::type;

    template <typename Iterator>
    friend auto async_wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<void>>::type;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    ptr_t state_;

    future(const future&) = delete;

    future& operator=(const future&) = delete;

    future(ptr_t const& p) : state_(p) {}

public:
    /// Constructs a future with no shared state
    /// after construction, valid() == false
    future() noexcept : state_() {}

    /// abandon ownership if any
    ~future() {}

    /// Constructs a future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    future(future&& other) noexcept : state_() { swap(other); }

    /// Releases any shared state and move-assigns the contents of other to *this
    /// after the assignment, other.valid() == false and this->valid() will yield
    /// the same value as other.valid() before the assignment
    future& operator=(future&& other) noexcept
    {
        future tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    /// Exchange the shared states of two futures
    void swap(future& other) noexcept { state_.swap(other.state_); }

    operator safe_bool() const noexcept { return valid() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return !valid(); }

    /// Checks if the future refers to a shared state
    /// this is the case only for futures returned by
    /// promise::get_future(), packaged_task::get_future()
    /// or async() until the first time get()or share() is called
    bool valid() const noexcept { return 0 != state_.get(); }

    shared_future<void> share();

    /// The get method waits until the future has a valid result and
    /// (depending on which template is used) retrieves it
    /// it effectively calls wait() in order to wait for the result
    /// the value stored in the shared state
    /// if it satisfies the requirements of MoveAssignable, the value is moved,
    /// otherwise it is copied
    /// valid() == false after a call to this method.
    /// detect the case when valid == false before the call and throw a
    /// future_error with an error condition of future_errc::no_state
    void get()
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        ptr_t tmp;
        tmp.swap(state_);
        tmp->get();
    }

    /// Blocks until the result becomes available
    /// valid() == true after the call
    void wait() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        state_->wait();
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    template <class Rep, class Period>
    future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_for(timeout_duration);
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    future_status wait_until(clock_type::time_point const& timeout_time) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_until(timeout_time);
    }

    template <typename F>
    future<typename std::result_of<F(future&)>::type> then(F&& func);
};

template <typename R>
void swap(future<R>& l, future<R>& r)
{
    l.swap(r);
}

template <typename R>
class shared_future
{
private:
    typedef typename detail::shared_state<R>::ptr_t ptr_t;

    friend class future<R>;

    template <std::size_t N, typename... Futures>
    friend struct detail::any_waiter;
    template <typename... Futures>
    friend struct detail::all_waiter;

    template <typename Iterator>
    friend auto wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, Iterator>::type;

    template <typename Iterator>
    friend auto wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value>::type;

    template <std::size_t N, typename... Futures>
    friend struct detail::async_any_waiter;
    template <typename... Futures>
    friend struct detail::async_all_waiter;

    template <typename Iterator>
    friend auto async_wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<Iterator>>::type;

    template <typename Iterator>
    friend auto async_wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<void>>::type;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    ptr_t state_;

    explicit shared_future(ptr_t const& p) : state_(p) {}

public:
    /// Constructs a shared_future with no shared state
    /// after construction, valid() == false
    shared_future() noexcept : state_() {}

    /// If *this is the last object referring to the shared state,
    /// destroys the shared state otherwise does nothing
    ~shared_future() {}

    /// Constructs a shared future that refers to the same shared state,
    /// as other, if there's any
    shared_future(shared_future const& other) : state_(other.state_) {}

    /// Constructs a shared_future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    shared_future(future<R>&& other) noexcept : state_() { state_.swap(other.state_); }

    /// Constructs a shared_future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    shared_future(shared_future&& other) noexcept : state_() { swap(other); }

    /// Releases any shared state and move-assigns the contents of other to *this
    /// after the assignment, other.valid() == false and this->valid() will yield
    /// the same value as other.valid() before the assignment
    shared_future& operator=(shared_future&& other) noexcept
    {
        shared_future tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    shared_future& operator=(shared_future const& other) noexcept
    {
        shared_future tmp(other);
        swap(tmp);
        return *this;
    }

    shared_future& operator=(future<R>&& other) noexcept
    {
        shared_future tmp(other);
        swap(tmp);
        return *this;
    }

    /// Exchange the shared states of two shared_futures
    void swap(shared_future& other) noexcept { state_.swap(other.state_); }

    operator safe_bool() const noexcept { return valid() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return !valid(); }

    /// Checks if the shared_future refers to a shared state
    /// this is the case only for shared_futures returned by
    /// promise::get_shared_future(), packaged_task::get_shared_future()
    /// or async() until the first time get()or share() is called
    bool valid() const noexcept { return 0 != state_.get(); }

    /// The get method waits until the shared_future has a valid result and
    /// (depending on which template is used) retrieves it
    /// it effectively calls wait() in order to wait for the result
    /// the value stored in the shared state
    /// if it satisfies the requirements of MoveAssignable, the value is moved,
    /// otherwise it is copied
    /// valid() == false after a call to this method.
    /// detect the case when valid == false before the call and throw a
    /// future_error with an error condition of future_errc::no_state
    R const& get() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->get();
    }

    /// Blocks until the result becomes available
    /// valid() == true after the call
    void wait() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        state_->wait();
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    template <class Rep, class Period>
    future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_for(timeout_duration);
    }

    /// blocks until the result becomes available or timeout
    /// valid() == true after the call
    future_status wait_until(clock_type::time_point const& timeout_time) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_until(timeout_time);
    }

    template <typename F>
    future<typename std::result_of<F(shared_future&)>::type> then(F&& func);
};

template <typename R>
class shared_future<R&>
{
private:
    typedef typename detail::shared_state<R&>::ptr_t ptr_t;

    friend class future<R&>;

    template <std::size_t N, typename... Futures>
    friend struct detail::any_waiter;
    template <typename... Futures>
    friend struct detail::all_waiter;

    template <typename Iterator>
    friend auto wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, Iterator>::type;

    template <typename Iterator>
    friend auto wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value>::type;

    template <std::size_t N, typename... Futures>
    friend struct detail::async_any_waiter;
    template <typename... Futures>
    friend struct detail::async_all_waiter;

    template <typename Iterator>
    friend auto async_wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<Iterator>>::type;

    template <typename Iterator>
    friend auto async_wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<void>>::type;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    ptr_t state_;

    explicit shared_future(ptr_t const& p) : state_(p) {}

public:
    /// Constructs a shared_future with no shared state
    /// after construction, valid() == false
    shared_future() noexcept : state_() {}

    /// If *this is the last object referring to the shared state,
    /// destroys the shared state otherwise does nothing
    ~shared_future() {}

    /// Constructs a shared future that refers to the same shared state,
    /// as other, if there's any
    shared_future(shared_future const& other) : state_(other.state_) {}

    /// Constructs a shared_future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    shared_future(future<R&>&& other) noexcept : state_() { state_.swap(other.state_); }

    /// Constructs a shared_future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    shared_future(shared_future&& other) noexcept : state_() { swap(other); }

    /// Releases any shared state and move-assigns the contents of other to *this
    /// after the assignment, other.valid() == false and this->valid() will yield
    /// the same value as other.valid() before the assignment
    shared_future& operator=(shared_future&& other) noexcept
    {
        shared_future tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    shared_future& operator=(shared_future const& other) noexcept
    {
        shared_future tmp(other);
        swap(tmp);
        return *this;
    }

    shared_future& operator=(future<R&>&& other) noexcept
    {
        shared_future tmp(other);
        swap(tmp);
        return *this;
    }

    /// Exchange the shared states of two shared_futures
    void swap(shared_future& other) noexcept { state_.swap(other.state_); }

    operator safe_bool() const noexcept { return valid() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return !valid(); }

    /// Checks if the shared_future refers to a shared state
    /// this is the case only for shared_futures returned by
    /// promise::get_shared_future(), packaged_task::get_shared_future()
    /// or async() until the first time get()or share() is called
    bool valid() const noexcept { return 0 != state_.get(); }

    /// The get method waits until the shared_future has a valid result and
    /// (depending on which template is used) retrieves it
    /// it effectively calls wait() in order to wait for the result
    /// the value stored in the shared state
    /// if it satisfies the requirements of MoveAssignable, the value is moved,
    /// otherwise it is copied
    /// valid() == false after a call to this method.
    /// detect the case when valid == false before the call and throw a
    /// future_error with an error condition of future_errc::no_state
    R& get() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->get();
    }

    /// Blocks until the result becomes available
    /// valid() == true after the call
    void wait() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        state_->wait();
    }

    /// blocks until the result becomes available or timeout
    /// valid() == true after the call
    template <class Rep, class Period>
    future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_for(timeout_duration);
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    future_status wait_until(clock_type::time_point const& timeout_time) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_until(timeout_time);
    }

    template <typename F>
    future<typename std::result_of<F(shared_future&)>::type> then(F&& func);
};

template <>
class shared_future<void>
{
private:
    typedef detail::shared_state<void>::ptr_t ptr_t;

    friend class future<void>;

    template <std::size_t N, typename... Futures>
    friend struct detail::any_waiter;
    template <typename... Futures>
    friend struct detail::all_waiter;

    template <typename Iterator>
    friend auto wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, Iterator>::type;

    template <typename Iterator>
    friend auto wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value>::type;

    template <std::size_t N, typename... Futures>
    friend struct detail::async_any_waiter;
    template <typename... Futures>
    friend struct detail::async_all_waiter;

    template <typename Iterator>
    friend auto async_wait_for_any(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<Iterator>>::type;

    template <typename Iterator>
    friend auto async_wait_for_all(Iterator begin, Iterator end) ->
        typename std::enable_if<!detail::is_future<Iterator>::value, future<void>>::type;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    ptr_t state_;

    shared_future(ptr_t const& p) : state_(p) {}

public:
    /// Constructs a shared_future with no shared state
    /// after construction, valid() == false
    shared_future() noexcept : state_() {}

    /// If *this is the last object referring to the shared state,
    /// destroys the shared state otherwise does nothing
    ~shared_future() {}

    /// Constructs a shared future that refers to the same shared state,
    /// as other, if there's any
    shared_future(shared_future const& other) : state_(other.state_) {}

    /// Constructs a shared_future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    shared_future(future<void>&& other) noexcept : state_() { state_.swap(other.state_); }

    /// Constructs a shared_future with the shared state of other using move semantics
    /// after construction, other.valid() == false
    shared_future(shared_future&& other) noexcept : state_() { swap(other); }

    /// Releases any shared state and move-assigns the contents of other to *this
    /// after the assignment, other.valid() == false and this->valid() will yield
    /// the same value as other.valid() before the assignment
    shared_future& operator=(shared_future&& other) noexcept
    {
        shared_future tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    shared_future& operator=(shared_future const& other) noexcept
    {
        shared_future tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    shared_future& operator=(future<void>&& other) noexcept
    {
        shared_future tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    /// Exchange the shared states of two shared_futures
    void swap(future<void>& other) noexcept { state_.swap(other.state_); }

    /// Exchange the shared states of two shared_futures
    void swap(shared_future& other) noexcept { state_.swap(other.state_); }

    operator safe_bool() const noexcept { return valid() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return !valid(); }

    /// Checks if the shared_future refers to a shared state
    /// this is the case only for shared_futures returned by
    /// promise::get_shared_future(), packaged_task::get_shared_future()
    /// or async() until the first time get()or share() is called
    bool valid() const noexcept { return 0 != state_.get(); }

    /// The get method waits until the shared_future has a valid result and
    /// (depending on which template is used) retrieves it
    /// it effectively calls wait() in order to wait for the result
    /// the value stored in the shared state
    /// if it satisfies the requirements of MoveAssignable, the value is moved,
    /// otherwise it is copied
    /// valid() == false after a call to this method.
    /// detect the case when valid == false before the call and throw a
    /// future_error with an error condition of future_errc::no_state
    void get() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        state_->get();
    }

    /// Blocks until the result becomes available
    /// valid() == true after the call
    void wait() const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        state_->wait();
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    template <class Rep, class Period>
    future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_for(timeout_duration);
    }

    /// Blocks until the result becomes available or timeout
    /// valid() == true after the call
    future_status wait_until(clock_type::time_point const& timeout_time) const
    {
        if (!valid()) {
            BOOST_THROW_EXCEPTION(future_uninitialized());
        }
        return state_->wait_until(timeout_time);
    }

    template <typename F>
    future<typename std::result_of<F(shared_future&)>::type> then(F&& func);
};

template <typename R>
void swap(shared_future<R>& l, shared_future<R>& r)
{
    l.swap(r);
}

/// Transfer the shared state of *this to a shared_future object
/// multiple shared_future objects may reference the same shared state,
/// which is not possible with future
/// after calling share on a future, valid() == false
/// detect the case when valid == false before the call and throw a
/// future_error with an error condition of future_errc::no_state
template <typename R>
shared_future<R> future<R>::share()
{
    if (!valid()) {
        BOOST_THROW_EXCEPTION(future_uninitialized());
    }
    return shared_future<R>(std::move(*this));
}

/// Transfer the shared state of *this to a shared_future object
/// multiple shared_future objects may reference the same shared state,
/// which is not possible with future
/// after calling share on a future, valid() == false
/// detect the case when valid == false before the call and throw a
/// future_error with an error condition of future_errc::no_state
template <typename R>
shared_future<R&> future<R&>::share()
{
    if (!valid()) {
        BOOST_THROW_EXCEPTION(future_uninitialized());
    }
    return shared_future<R&>(std::move(*this));
}

/// Transfer the shared state of *this to a shared_future object
/// multiple shared_future objects may reference the same shared state,
/// which is not possible with future
/// after calling share on a future, valid() == false
/// detect the case when valid == false before the call and throw a
/// future_error with an error condition of future_errc::no_state
inline shared_future<void> future<void>::share()
{
    if (!valid()) {
        BOOST_THROW_EXCEPTION(future_uninitialized());
    }
    return shared_future<void>(std::move(*this));
}

namespace detail {
template <typename Ret>
struct any_state : std::enable_shared_from_this<any_state<Ret>>
{
    typedef std::shared_ptr<any_state> ptr;
    mutex mtx;
    condition_variable cv;
    std::atomic<bool> flag{false};
    Ret ret{};

    void set_value(Ret v)
    {
        unique_lock<mutex> lk(mtx);
        flag = true;
        ret = v;
        cv.notify_all();
    }

    Ret wait()
    {
        unique_lock<mutex> lk(mtx);
        cv.wait(lk, [this]() -> bool { return flag; });
        return ret;
    }
};

template <std::size_t N, typename... Futures>
struct any_waiter;

template <std::size_t N, typename F>
struct any_waiter<N, F>
{
    static void setup(typename any_state<size_t>::ptr s, F& f)
    {
        f.state_->add_external_waiter([s]() { s->set_value(N); });
    }
};

template <std::size_t N, typename F, typename... Futures>
struct any_waiter<N, F, Futures...>
{
    static void setup(typename any_state<size_t>::ptr s, F& f, Futures&... fs)
    {
        any_waiter<N, F>::setup(s, f);
        any_waiter<N + 1, Futures...>::setup(s, fs...);
    }
};

template <typename... Futures>
struct all_waiter;

template <typename F>
struct all_waiter<F>
{
    static void setup(condition_variable& cv, std::atomic<size_t>& acquired, size_t count, F& f)
    {
        f.state_->add_external_waiter([&cv, &acquired, count]() {
            if (++acquired == count) cv.notify_all();
        });
    }
};

template <typename F, typename... Futures>
struct all_waiter<F, Futures...>
{
    static void
    setup(condition_variable& cv, std::atomic<size_t>& acquired, size_t count, F& f, Futures&... fs)
    {
        all_waiter<F>::setup(cv, acquired, count, f);
        all_waiter<Futures...>::setup(cv, acquired, count, fs...);
    }
};

template <typename... Futures, std::size_t... Indices>
std::size_t wait_for_any2(std::tuple<Futures&...>&& futures, utility::tuple_indices<Indices...>)
{
    auto sp = std::make_shared<detail::any_state<size_t>>();
    detail::any_waiter<0, Futures...>::setup(sp, std::get<Indices>(futures)...);
    return sp->wait();
}

template <typename... Futures, std::size_t... Indices>
std::size_t wait_for_any2(std::tuple<Futures&...>& futures, utility::tuple_indices<Indices...>)
{
    auto sp = std::make_shared<detail::any_state<size_t>>();
    detail::any_waiter<0, Futures...>::setup(sp, std::get<Indices>(futures)...);
    return sp->wait();
}

template <typename... Futures, std::size_t... Indices>
void wait_for_all2(std::tuple<Futures&...>& futures, utility::tuple_indices<Indices...>)
{
    mutex mtx;
    condition_variable cv;
    constexpr size_t count = sizeof...(Futures);
    std::atomic<size_t> acquired(0);
    detail::all_waiter<Futures...>::setup(cv, acquired, count, std::get<Indices>(futures)...);
    unique_lock<mutex> lk(mtx);
    cv.wait(lk, [&]() -> bool { return acquired == count; });
}

template <typename... Futures, std::size_t... Indices>
void wait_for_all2(std::tuple<Futures&...>&& futures, utility::tuple_indices<Indices...>)
{
    mutex mtx;
    condition_variable cv;
    constexpr size_t count = sizeof...(Futures);
    std::atomic<size_t> acquired(0);
    detail::all_waiter<Futures...>::setup(cv, acquired, count, std::get<Indices>(futures)...);
    unique_lock<mutex> lk(mtx);
    cv.wait(lk, [&]() -> bool { return acquired == count; });
}
} // End of namespace detail

template <typename... Futures>
std::size_t wait_for_any(Futures&... futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    auto sp = std::make_shared<detail::any_state<size_t>>();
    detail::any_waiter<0, Futures...>::setup(sp, futures...);
    return sp->wait();
}

template <typename... Futures>
std::size_t wait_for_any(std::tuple<Futures&...>&& futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    typedef typename utility::make_tuple_indices<sizeof...(Futures)>::type index_type;
    return detail::wait_for_any2(std::forward<std::tuple<Futures&...>>(futures), index_type());
}

template <typename... Futures>
std::size_t wait_for_any(std::tuple<Futures&...>& futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    typedef typename utility::make_tuple_indices<sizeof...(Futures)>::type index_type;
    return detail::wait_for_any2(futures, index_type());
}

template <typename Iterator>
auto wait_for_any(Iterator begin, Iterator end) ->
    typename std::enable_if<!detail::is_future<Iterator>::value, Iterator>::type
{
    static_assert(detail::is_future<typename std::iterator_traits<Iterator>::value_type>::value,
                  "Iterator must refer to future type");
    auto sp = std::make_shared<detail::any_state<Iterator>>();
    for (Iterator i = begin; i != end; ++i) {
        i->state_->add_external_waiter([sp, i]() { sp->set_value(i); });
    }
    return sp->wait();
}

template <typename... Futures>
void wait_for_all(Futures&... futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    mutex mtx;
    condition_variable cv;
    constexpr size_t count = sizeof...(Futures);
    std::atomic<size_t> acquired(0);
    detail::all_waiter<Futures...>::setup(cv, acquired, count, futures...);
    unique_lock<mutex> lk(mtx);
    cv.wait(lk, [&]() -> bool { return acquired == count; });
}

template <typename... Futures>
void wait_for_all(std::tuple<Futures&...>& futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    typedef typename utility::make_tuple_indices<sizeof...(Futures)>::type index_type;
    detail::wait_for_all2(futures, index_type());
}

template <typename... Futures>
void wait_for_all(std::tuple<Futures&...>&& futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    typedef typename utility::make_tuple_indices<sizeof...(Futures)>::type index_type;
    detail::wait_for_all2(std::forward<std::tuple<Futures&...>>(futures), index_type());
}

template <typename Iterator>
auto wait_for_all(Iterator begin, Iterator end) ->
    typename std::enable_if<!detail::is_future<Iterator>::value>::type
{
    static_assert(detail::is_future<typename std::iterator_traits<Iterator>::value_type>::value,
                  "Iterator must refer to future type");
    mutex mtx;
    condition_variable cv;
    size_t count = 0;
    std::atomic<size_t> acquired(0);
    for (Iterator i = begin; i != end; ++i) {
        i->state_->add_external_waiter([&]() {
            acquired++;
            cv.notify_all();
        });
        count++;
    }
    unique_lock<mutex> lk(mtx);
    cv.wait(lk, [&]() -> bool { return acquired == count; });
}

} // End of namespace fibers

using fibers::future;
using fibers::shared_future;
using fibers::wait_for_any;
using fibers::wait_for_all;

} // End of namespace fibio

#endif
