//
//  promise.hpp
//  fibio
//
//  Base on Boost.Fiber at https://github.com/olk/boost-fiber
//
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef fibio_fibers_future_promise_hpp
#define fibio_fibers_future_promise_hpp

#include <memory>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/detail/memory.hpp> // boost::allocator_arg_t
#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>

#include <fibio/fibers/exceptions.hpp>
#include <fibio/fibers/fiber.hpp>
#include <fibio/fibers/future/detail/shared_state.hpp>
#include <fibio/fibers/future/detail/shared_state_object.hpp>
#include <fibio/fibers/future/future.hpp>

namespace fibio {
namespace fibers {

template <typename R>
class promise : private boost::noncopyable
{
private:
    typedef typename detail::shared_state<R>::ptr_t ptr_t;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    bool obtained_;
    ptr_t future_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE(promise);

public:
    promise() : obtained_(false), future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<R, std::allocator<promise<R>>> object_t;
        std::allocator<promise<R>> alloc;
        typename object_t::allocator_t a(alloc);
        future_ = ptr_t(
            // placement new
            ::new (a.allocate(1)) object_t(a));
    }

    template <typename Allocator>
    promise(boost::allocator_arg_t, Allocator alloc)
    : obtained_(false), future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<R, Allocator> object_t;
        typename object_t::allocator_t a(alloc);
        future_ = ptr_t(
            // placement new
            ::new (a.allocate(1)) object_t(a));
    }

    ~promise()
    {
        // TODO: abandon ownership if any
        if (future_) future_->owner_destroyed();
    }

    promise(promise&& other) noexcept : obtained_(false), future_()
    {
        // TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap(other);
    }

    promise& operator=(promise&& other) noexcept
    {
        // TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    void swap(promise& other) noexcept
    {
        // TODO: exchange the shared states of two promises
        std::swap(obtained_, other.obtained_);
        future_.swap(other.future_);
    }

    operator safe_bool() const noexcept { return 0 != future_.get() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return 0 == future_.get(); }

    future<R> get_future()
    {
        // TODO: returns a future object associated with the same shared state
        //      exception is thrown if *this has no shared state or get_future
        //      has already been called.
        if (obtained_) BOOST_THROW_EXCEPTION(future_already_retrieved());
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        obtained_ = true;
        return future<R>(future_);
    }

    void set_value(R const& value)
    {
        // TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        future_->set_value(value);
    }

    void set_value(R&& value)
    {
        // TODO: store the value into the shared state and make the state ready
        //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        future_->set_value(std::move(value));
    }

    void set_exception(std::exception_ptr p)
    {
        // TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        future_->set_exception(p);
    }

    void set_value_at_fiber_exit(const R& value)
    {
        ptr_t f = future_;
        this_fiber::at_fiber_exit([value, f]() { f->set_value(value); });
    }

    void set_value_at_fiber_exit(R&& value)
    {
        // FIXME: Way too stupid
        ptr_t f = future_;
        R* p = new R(std::forward<R>(value));
        this_fiber::at_fiber_exit([p, f]() {
            try {
                f->set_value(*p);
            } catch (...) {
                delete p;
                throw;
            }
        });
    }

    void set_exception_at_fiber_exit(std::exception_ptr p)
    {
        ptr_t f = future_;
        this_fiber::at_fiber_exit([p, f]() { f->set_exception(p); });
    }
};

template <typename R>
class promise<R&> : private boost::noncopyable
{
private:
    typedef typename detail::shared_state<R&>::ptr_t ptr_t;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    bool obtained_;
    ptr_t future_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE(promise);

public:
    promise() : obtained_(false), future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<R&, std::allocator<promise<R&>>> object_t;
        std::allocator<promise<R>> alloc;
        typename object_t::allocator_t a(alloc);
        future_ = ptr_t(
            // placement new
            ::new (a.allocate(1)) object_t(a));
    }

    template <typename Allocator>
    promise(boost::allocator_arg_t, Allocator alloc)
    : obtained_(false), future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<R&, Allocator> object_t;
        typename object_t::allocator_t a(alloc);
        future_ = ptr_t(
            // placement new
            ::new (a.allocate(1)) object_t(a));
    }

    ~promise()
    {
        // TODO: abandon ownership if any
        if (future_) future_->owner_destroyed();
    }

    promise(promise&& other) noexcept : obtained_(false), future_()
    {
        // TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap(other);
    }

    promise& operator=(promise&& other) noexcept
    {
        // TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    void swap(promise& other) noexcept
    {
        // TODO: exchange the shared states of two promises
        std::swap(obtained_, other.obtained_);
        future_.swap(other.future_);
    }

    operator safe_bool() const noexcept { return 0 != future_.get() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return 0 == future_.get(); }

    future<R&> get_future()
    {
        // TODO: returns a future object associated with the same shared state
        //      exception is thrown if *this has no shared state or get_future
        //      has already been called.
        if (obtained_) BOOST_THROW_EXCEPTION(future_already_retrieved());
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        obtained_ = true;
        return future<R&>(future_);
    }

    void set_value(R& value)
    {
        // TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        future_->set_value(value);
    }

    void set_exception(std::exception_ptr p)
    {
        // TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        future_->set_exception(p);
    }

    void set_value_at_fiber_exit(R& value)
    {
        ptr_t f = future_;
        this_fiber::at_fiber_exit([&value, f]() { f->set_value(value); });
    }

    void set_exception_at_fiber_exit(std::exception_ptr p)
    {
        ptr_t f = future_;
        this_fiber::at_fiber_exit([p, f]() { f->set_exception(p); });
    }
};

template <>
class promise<void> : private boost::noncopyable
{
private:
    typedef detail::shared_state<void>::ptr_t ptr_t;

    struct dummy
    {
        void nonnull() {}
    };

    typedef void (dummy::*safe_bool)();

    bool obtained_;
    ptr_t future_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE(promise);

public:
    promise() : obtained_(false), future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<void, std::allocator<promise<void>>> object_t;
        std::allocator<promise<void>> alloc;
        object_t::allocator_t a(alloc);
        future_ = ptr_t(
            // placement new
            ::new (a.allocate(1)) object_t(a));
    }

    template <typename Allocator>
    promise(boost::allocator_arg_t, Allocator alloc)
    : obtained_(false), future_()
    {
        // TODO: constructs the promise with an empty shared state
        //       the shared state is allocated using alloc
        //       alloc must meet the requirements of Allocator
        typedef detail::shared_state_object<void, Allocator> object_t;
#if BOOST_MSVC
        object_t::allocator_t a(alloc);
#else
        typename object_t::allocator_t a(alloc);
#endif
        future_ = ptr_t(
            // placement new
            ::new (a.allocate(1)) object_t(a));
    }

    ~promise()
    {
        // TODO: abandon ownership if any
        if (future_) future_->owner_destroyed();
    }

    promise(promise&& other) noexcept : obtained_(false), future_()
    {
        // TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        swap(other);
    }

    promise& operator=(promise&& other) noexcept
    {
        // TODO: take over ownership
        //      other is valid before but in
        //      undefined state afterwards
        promise tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    void swap(promise& other) noexcept
    {
        // TODO: exchange the shared states of two promises
        std::swap(obtained_, other.obtained_);
        future_.swap(other.future_);
    }

    operator safe_bool() const noexcept { return 0 != future_.get() ? &dummy::nonnull : 0; }

    bool operator!() const noexcept { return 0 == future_.get(); }

    future<void> get_future()
    {
        // TODO: returns a future object associated with the same shared state
        //      exception is thrown if *this has no shared state or get_future
        //      has already been called.
        if (obtained_) BOOST_THROW_EXCEPTION(future_already_retrieved());
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        obtained_ = true;
        return future<void>(future_);
    }

    void set_value()
    {
        // TODO: store the value into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        future_->set_value();
    }

    void set_exception(std::exception_ptr p)
    {
        // TODO: store the exception pointer p into the shared state and make the state ready
        //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
        //      associated with the promise object while updating the promise object
        //      an exception is thrown if there is no shared state or the shared state already
        //      stores a value or exception
        if (!future_) BOOST_THROW_EXCEPTION(promise_uninitialized());
        future_->set_exception(p);
    }

    void set_value_at_fiber_exit()
    {
        ptr_t f = future_;
        this_fiber::at_fiber_exit([f]() { f->set_value(); });
    }

    void set_exception_at_fiber_exit(std::exception_ptr p)
    {
        ptr_t f = future_;
        this_fiber::at_fiber_exit([p, f]() { f->set_exception(p); });
    }
};

template <typename R>
void swap(promise<R>& l, promise<R>& r)
{
    l.swap(r);
}

template <typename T>
future<typename std::decay<T>::type> make_ready_future(T&& value)
{
    typedef typename std::decay<T>::type value_type;
    promise<value_type> p;
    p.set_value(std::forward<T>(value));
    return p.get_future();
}

inline future<void> make_ready_future()
{
    promise<void> p;
    p.set_value();
    return p.get_future();
}

namespace detail {

// HACK: C++ forbids variable with `void` type as it is always incomplete
template <typename R, typename F>
auto set_promise_value(promise<typename std::result_of<F(future<R>&)>::type>& p,
                       F&& fn,
                       future<R>& f) ->
    typename std::enable_if<std::is_same<typename std::result_of<F(future<R>&)>::type,
                                         void>::value>::type
{
    std::forward<F>(fn)(f);
    p.set_value();
}

template <typename R, typename F>
auto set_promise_value(promise<typename std::result_of<F(future<R>&)>::type>& p,
                       F&& fn,
                       future<R>& f) ->
    typename std::enable_if<!std::is_same<typename std::result_of<F(future<R>&)>::type,
                                          void>::value>::type
{
    p.set_value(std::forward<F>(fn)(f));
}

template <typename R, typename F>
auto set_promise_value(promise<typename std::result_of<F(shared_future<R>&)>::type>& p,
                       F&& fn,
                       shared_future<R>& f) ->
    typename std::enable_if<std::is_same<typename std::result_of<F(shared_future<R>&)>::type,
                                         void>::value>::type
{
    std::forward<F>(fn)(f);
    p.set_value();
}

template <typename R, typename F>
auto set_promise_value(promise<typename std::result_of<F(shared_future<R>&)>::type>& p,
                       F&& fn,
                       shared_future<R>& f) ->
    typename std::enable_if<!std::is_same<typename std::result_of<F(shared_future<R>&)>::type,
                                          void>::value>::type
{
    p.set_value(std::forward<F>(fn)(f));
}

template <typename Ret>
struct async_any_state : std::enable_shared_from_this<async_any_state<Ret>>
{
    typedef std::shared_ptr<async_any_state> ptr;
    std::atomic<bool> is_set_{false};
    promise<Ret> p;

    void set_value(Ret v)
    {
        if (is_set_) return;
        is_set_ = true;
        p.set_value(v);
    }

    future<Ret> get_future() { return p.get_future(); }
};

template <std::size_t N, typename... Futures>
struct async_any_waiter;

template <std::size_t N, typename F>
struct async_any_waiter<N, F>
{
    static void setup(typename async_any_state<size_t>::ptr s, F& f)
    {
        f.state_->add_external_waiter([s]() { s->set_value(N); });
    }
};

template <std::size_t N, typename F, typename... Futures>
struct async_any_waiter<N, F, Futures...>
{
    static void setup(typename async_any_state<size_t>::ptr s, F& f, Futures&... fs)
    {
        async_any_waiter<N, F>::setup(s, f);
        async_any_waiter<N + 1, Futures...>::setup(s, fs...);
    }
};

template <typename... Futures, std::size_t... Indices>
future<std::size_t> async_wait_for_any2(std::tuple<Futures&...>&& futures,
                                        utility::tuple_indices<Indices...>)
{
    auto sp = std::make_shared<detail::async_any_state<size_t>>();
    detail::async_any_waiter<0, Futures...>::setup(sp, std::get<Indices>(futures)...);
    return sp->get_future();
}

template <typename... Futures, std::size_t... Indices>
future<std::size_t> async_wait_for_any2(std::tuple<Futures&...>& futures,
                                        utility::tuple_indices<Indices...>)
{
    auto sp = std::make_shared<detail::async_any_state<size_t>>();
    detail::async_any_waiter<0, Futures...>::setup(sp, std::get<Indices>(futures)...);
    return sp->get_future();
}

struct async_all_state : std::enable_shared_from_this<async_all_state>
{
    typedef std::shared_ptr<async_all_state> ptr;
    std::atomic<size_t> acquired{0};
    promise<void> p;
    const size_t count_;

    async_all_state(size_t c) : count_(c) {}

    void count()
    {
        if (++acquired >= count_) p.set_value();
    }

    future<void> get_future() { return p.get_future(); }
};

template <typename... Futures>
struct async_all_waiter;

template <typename F>
struct async_all_waiter<F>
{
    static void setup(std::shared_ptr<async_all_state> state, F& f)
    {
        f.state_->add_external_waiter([state]() { state->count(); });
    }
};

template <typename F, typename... Futures>
struct async_all_waiter<F, Futures...>
{
    static void setup(std::shared_ptr<async_all_state> state, F& f, Futures&... fs)
    {
        async_all_waiter<F>::setup(state, f);
        async_all_waiter<Futures...>::setup(state, fs...);
    }
};

template <typename... Futures, std::size_t... Indices>
future<void> async_wait_for_all2(std::tuple<Futures&...>& futures,
                                 utility::tuple_indices<Indices...>)
{
    async_all_state::ptr state(new async_all_state(sizeof...(Futures)));
    detail::async_all_waiter<Futures...>::setup(state, std::get<Indices>(futures)...);
    return state->get_future();
}

template <typename... Futures, std::size_t... Indices>
future<void> async_wait_for_all2(std::tuple<Futures&...>&& futures,
                                 utility::tuple_indices<Indices...>)
{
    async_all_state::ptr state(new async_all_state(sizeof...(Futures)));
    detail::async_all_waiter<Futures...>::setup(state, std::get<Indices>(futures)...);
    return state->get_future();
}

} // End of namespace detail

template <typename R>
template <typename F>
inline future<typename std::result_of<F(future<R>&)>::type> future<R>::then(F&& func)
{
    typedef typename std::result_of<F(future<R>&)>::type result_type;
    struct waiter
    {
        ptr_t src_;
        F f_;
        promise<result_type> p_;

        waiter(ptr_t src, F&& f) : src_(src), f_(std::forward<F>(f)) {}

        void invoke()
        {
            future<R> f(src_);
            detail::set_promise_value(p_, std::forward<F>(f_), f);
        }
    };
    std::shared_ptr<waiter> w = std::make_shared<waiter>(state_, std::forward<F>(func));
    state_->add_external_waiter(std::bind(&waiter::invoke, w));
    return w->p_.get_future();
}

template <typename R>
template <typename F>
inline future<typename std::result_of<F(future<R&>&)>::type> future<R&>::then(F&& func)
{
    typedef typename std::result_of<F(future<R&>&)>::type result_type;

    struct waiter
    {
        ptr_t src_;
        F f_;
        promise<result_type> p_;

        waiter(ptr_t src, F&& f) : src_(src), f_(std::forward<F>(f)) {}

        void invoke()
        {
            future<R&> f(src_);
            detail::set_promise_value(p_, std::forward<F>(f_), f);
        }
    };

    std::shared_ptr<waiter> w = std::make_shared<waiter>(state_, std::forward<F>(func));
    state_->add_external_waiter(std::bind(&waiter::invoke, w));
    return w->p_.get_future();
}

template <typename F>
inline future<typename std::result_of<F(future<void>&)>::type> future<void>::then(F&& func)
{
    typedef typename std::result_of<F(future<void>&)>::type result_type;
    struct waiter
    {
        ptr_t src_;
        F f_;
        promise<result_type> p_;

        waiter(ptr_t src, F&& f) : src_(src), f_(std::forward<F>(f)) {}

        void invoke()
        {
            future<void> f(src_);
            detail::set_promise_value(p_, std::forward<F>(f_), f);
        }
    };
    std::shared_ptr<waiter> w = std::make_shared<waiter>(state_, std::forward<F>(func));
    state_->add_external_waiter(std::bind(&waiter::invoke, w));
    return w->p_.get_future();
}

template <typename R>
template <typename F>
inline future<typename std::result_of<F(shared_future<R>&)>::type> shared_future<R>::then(F&& func)
{
    typedef typename std::result_of<F(shared_future<R>&)>::type result_type;
    struct waiter
    {
        ptr_t src_;
        F f_;
        promise<result_type> p_;

        waiter(ptr_t src, F&& f) : src_(src), f_(std::forward<F>(f)) {}

        void invoke()
        {
            shared_future<R> f(src_);
            detail::set_promise_value(p_, std::forward<F>(f_), f);
        }
    };
    std::shared_ptr<waiter> w = std::make_shared<waiter>(state_, std::forward<F>(func));
    state_->add_external_waiter(std::bind(&waiter::invoke, w));
    return w->p_.get_future();
}

template <typename R>
template <typename F>
inline future<typename std::result_of<F(shared_future<R&>&)>::type>
shared_future<R&>::then(F&& func)
{
    typedef typename std::result_of<F(shared_future<R&>&)>::type result_type;

    struct waiter
    {
        ptr_t src_;
        F f_;
        promise<result_type> p_;

        waiter(ptr_t src, F&& f) : src_(src), f_(std::forward<F>(f)) {}

        void invoke()
        {
            shared_future<R&> f(src_);
            detail::set_promise_value(p_, std::forward<F>(f_), f);
        }
    };

    std::shared_ptr<waiter> w = std::make_shared<waiter>(state_, std::forward<F>(func));
    state_->add_external_waiter(std::bind(&waiter::invoke, w));
    return w->p_.get_future();
}

template <typename F>
inline future<typename std::result_of<F(shared_future<void>&)>::type>
shared_future<void>::then(F&& func)
{
    typedef typename std::result_of<F(shared_future<void>&)>::type result_type;
    struct waiter
    {
        ptr_t src_;
        F f_;
        promise<result_type> p_;

        waiter(ptr_t src, F&& f) : src_(src), f_(std::forward<F>(f)) {}

        void invoke()
        {
            shared_future<void> f(src_);
            detail::set_promise_value(p_, std::forward<F>(f_), f);
        }
    };
    std::shared_ptr<waiter> w = std::make_shared<waiter>(state_, std::forward<F>(func));
    state_->add_external_waiter(std::bind(&waiter::invoke, w));
    return w->p_.get_future();
}

template <typename... Futures>
future<std::size_t> async_wait_for_any(Futures&... futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    auto sp = std::make_shared<detail::async_any_state<size_t>>();
    detail::async_any_waiter<0, Futures...>::setup(sp, futures...);
    return sp->get_future();
}

template <typename Iterator>
auto async_wait_for_any(Iterator begin, Iterator end) ->
    typename std::enable_if<!detail::is_future<Iterator>::value, future<Iterator>>::type
{
    static_assert(detail::is_future<typename std::iterator_traits<Iterator>::value_type>::value,
                  "Iterator must refer to future type");
    auto sp = std::make_shared<detail::async_any_state<Iterator>>();
    for (Iterator i = begin; i != end; ++i) {
        i->state_->add_external_waiter([sp, i]() { sp->set_value(i); });
    }
    return sp->get_future();
}

template <typename... Futures>
future<std::size_t> async_wait_for_any(std::tuple<Futures&...>&& futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    typedef typename utility::make_tuple_indices<sizeof...(Futures)>::type index_type;
    return detail::async_wait_for_any2(std::forward<std::tuple<Futures&...>>(futures),
                                       index_type());
}

template <typename... Futures>
future<std::size_t> async_wait_for_any(std::tuple<Futures&...>& futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    typedef typename utility::make_tuple_indices<sizeof...(Futures)>::type index_type;
    return detail::async_wait_for_any2(futures, index_type());
}

template <typename... Futures>
future<void> async_wait_for_all(Futures&... futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    detail::async_all_state::ptr state(new detail::async_all_state(sizeof...(Futures)));
    detail::async_all_waiter<Futures...>::setup(state, futures...);
    return state->get_future();
}

template <typename... Futures>
future<void> async_wait_for_all(std::tuple<Futures&...>& futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    typedef typename utility::make_tuple_indices<sizeof...(Futures)>::type index_type;
    return detail::async_wait_for_all2(futures, index_type());
}

template <typename... Futures>
future<void> async_wait_for_all(std::tuple<Futures&...>&& futures)
{
    static_assert(utility::and_<detail::is_future<Futures>::value...>::value,
                  "Only futures can be waited");
    typedef typename utility::make_tuple_indices<sizeof...(Futures)>::type index_type;
    return detail::async_wait_for_all2(std::forward<std::tuple<Futures&...>>(futures),
                                       index_type());
}

template <typename Iterator>
auto async_wait_for_all(Iterator begin, Iterator end) ->
    typename std::enable_if<!detail::is_future<Iterator>::value, future<void>>::type
{
    static_assert(detail::is_future<typename std::iterator_traits<Iterator>::value_type>::value,
                  "Iterator must refer to future type");
    // TODO: Require random-access iterator here
    detail::async_all_state::ptr state(new detail::async_all_state(end - begin));
    for (Iterator i = begin; i != end; ++i) {
        i->state_->add_external_waiter([state]() { state->count(); });
    }
    return state->get_future();
}

} // End of namespace fibers

using fibers::promise;
using fibers::make_ready_future;

} // End of namespace fibio

#endif
