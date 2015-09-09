//
//  async.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_future_async_hpp
#define fibio_fibers_future_async_hpp

#include <algorithm>
#include <memory>
#include <tuple>
#include <fibio/utility.hpp>
#include <fibio/fibers/fiber_group.hpp>
#include <fibio/fibers/future/future.hpp>
#include <fibio/fibers/future/packaged_task.hpp>
#include <fibio/concurrent/concurrent_queue.hpp>

namespace fibio {
namespace fibers {
namespace detail {

template <typename Fn, class... Args>
struct task_data
{
    typedef std::tuple<typename std::decay<Fn>::type, typename std::decay<Args>::type...> data_type;
    typedef typename std::result_of<Fn(Args...)>::type result_type;
    typedef future<result_type> future_type;
    typedef packaged_task<result_type()> task_type;

    task_data(Fn&& fn, Args&&... args)
    : fn_(new data_type(std::forward<Fn>(fn), std::forward<Args>(args)...))
    {
    }

    template <std::size_t... Indices>
    result_type run2(utility::tuple_indices<Indices...>)
    {
        return utility::invoke(std::move(std::get<0>(*fn_)), std::move(std::get<Indices>(*fn_))...);
    }

    result_type operator()()
    {
        typedef
            typename utility::make_tuple_indices<std::tuple_size<std::tuple<Fn, Args...>>::value,
                                                 1>::type index_type;
        return run2(index_type());
    }

    std::unique_ptr<data_type> fn_;
};

} // End of namespace detail

/**
 * Run function asynchronously, returns a future, which will be ready when function completes
 */
template <typename Fn, typename... Args>
typename detail::task_data<Fn, Args...>::future_type async(Fn&& fn, Args&&... args)
{
    typedef detail::task_data<Fn, Args...> data_type;
    typename data_type::task_type task(
        data_type(std::forward<Fn>(fn), std::forward<Args>(args)...));
    typename data_type::future_type ret(task.get_future());
    fiber(std::move(task)).detach();
    return std::move(ret);
}

/**
 * Run function asynchronously in a fiber pool, returns a future
 */
template <typename T>
struct async_executor
{
    typedef typename std::decay<T>::type result_type;

    async_executor(size_t pool_size = 0)
    {
        if (pool_size == 0)
            pool_size = std::min(this_fiber::get_scheduler().worker_pool_size(),
                                 size_t(std::thread::hardware_concurrency()));
        for (size_t i = 0; i < pool_size; i++)
            fibers_.create_fiber([this]() {
                for (auto& i : queue_) i();
            });
    }

    ~async_executor()
    {
        queue_.close();
        fibers_.join_all();
    }

    template <typename Fn, typename... Args>
    future<result_type> operator()(Fn&& fn, Args&&... args)
    {
        typedef detail::task_data<Fn, Args...> task_type;
        static_assert(std::is_convertible<typename task_type::result_type, T>::value,
                      "function must return compatible type");
        packaged_task<result_type()> task(
            task_type(std::forward<Fn>(fn), std::forward<Args>(args)...));
        future<result_type> ret = task.get_future();
        queue_.push(std::move(task));
        return ret;
    }

private:
    concurrent::concurrent_queue<packaged_task<result_type()>> queue_;
    fiber_group fibers_;
};

/**
 *  Asynchronous wrapper for the function, accepts same arguments as the original function and
 * returns a future
 */
template <typename Fn>
struct async_function
{
    typedef utility::function_traits<Fn> traits_type;
    typedef typename traits_type::result_type result_type;
    typedef typename traits_type::arguments_tuple arguments_tuple;

    async_function(Fn&& fn)
    : fn_(std::forward<Fn>(fn))
    , queue_(new concurrent::concurrent_queue<queue_element>)
    , fiber_(&async_function::execute, this)
    {
    }

    async_function(async_function&&) = default;

    ~async_function()
    {
        queue_->close();
        fiber_.join();
    }

    template <typename... Args>
    future<result_type> operator()(Args&&... args)
    {
        return async_call(std::make_tuple(std::forward<Args>(args)...));
    }

    future<result_type> apply(arguments_tuple&& args)
    {
        return async_call(std::forward<arguments_tuple>(args));
    }

private:
    struct async_function_args
    {
        arguments_tuple args;
        promise<result_type> ret;
    };
    typedef std::unique_ptr<async_function_args> queue_element;

    future<result_type> async_call(arguments_tuple&& args)
    {
        queue_element e(
            new async_function_args{std::forward<arguments_tuple>(args), promise<result_type>()});
        future<result_type> ret(e->ret.get_future());
        queue_->push(std::move(e));
        return ret;
    }

    template <std::size_t... Indices>
    result_type call2(arguments_tuple&& args, utility::tuple_indices<Indices...>)
    {
        return utility::invoke(fn_, std::move(std::get<Indices>(args))...);
    }

    void call(queue_element&& args)
    {
        typedef typename utility::make_tuple_indices<traits_type::arity>::type index_type;
        args->ret.set_value(call2(std::move(args->args), index_type()));
    }

    void execute()
    {
        for (auto& e : *queue_) call(std::move(e));
    }

    Fn fn_;
    std::unique_ptr<concurrent::concurrent_queue<queue_element>> queue_;
    fiber fiber_;
};

/**
 *  Create an asynchronous wrapper for the function
 */
template <typename Fn>
async_function<Fn> make_async(Fn&& fn)
{
    return async_function<Fn>(std::forward<Fn>(fn));
}

/**
 *  Foreign thread pool
 */
struct foreign_thread_pool
{
    foreign_thread_pool(size_t pool_size = 1)
    {
        for (size_t i = 0; i < pool_size; i++) {
            threads_.emplace_back([this]() {
                for (auto& i : queue_) i();
            });
        }
    }

    ~foreign_thread_pool()
    {
        queue_.close();
        for (auto& t : threads_) {
            t.join();
        }
    }

    template <typename Fn, typename... Args>
    auto async_call(Fn&& fn, Args&&... args)
        -> future<typename detail::task_data<Fn, Args...>::result_type>
    {
        typedef detail::task_data<Fn, Args...> task_data_type;
        typedef typename task_data_type::result_type result_type;
        std::shared_ptr<packaged_task<result_type()>> task(new packaged_task<result_type()>(
            task_data_type(std::forward<Fn>(fn), std::forward<Args>(args)...)));
        future<result_type> ret = task->get_future();
        struct thr_task
        {
            void operator()() { (*pt_)(); }

            std::shared_ptr<packaged_task<result_type()>> pt_;
        };
        queue_.push(thr_task{task});
        return ret;
    }

    template <typename Fn, typename... Args>
    auto operator()(Fn&& fn, Args&&... args) -> typename detail::task_data<Fn, Args...>::result_type
    {
        return async_call(std::forward<Fn>(fn), std::forward<Args>(args)...).get();
    }

private:
    typedef std::function<void()> task_type;
    typedef concurrent::basic_concurrent_queue<task_type,
                                               std::unique_lock<std::mutex>,
                                               std::condition_variable> queue_type;

    queue_type queue_;
    std::vector<std::thread> threads_;
};

} // End of namespace fibers

using fibers::async;
using fibers::async_executor;
using fibers::async_function;
using fibers::make_async;
using fibers::foreign_thread_pool;

} // End of namespace fibio

#endif
