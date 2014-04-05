//
//  use_future.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-31.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_asio_detail_use_future_hpp
#define fibio_asio_detail_use_future_hpp

#include <memory>

#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/make_shared.hpp>
#include <boost/move/move.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/detail/memory.hpp>
#include <boost/throw_exception.hpp>

#include <fibio/fibers/future/future.hpp>
#include <fibio/fibers/future/promise.hpp>

namespace fibio { namespace fibers { namespace asio { namespace detail {
    // Completion handler to adapt a promise as a completion handler.
    template<typename T>
    class promise_handler
    {
    public:
        // Construct from use_future special value.
        template<typename Allocator>
        promise_handler(asio::use_future_t<Allocator> uf)
        : promise_(std::allocate_shared<promise<T>>(uf.get_allocator(),
                                                    boost::allocator_arg,
                                                    uf.get_allocator()))
        , fiber_(fibio::fibers::detail::get_current_fiber_ptr())
        {}
        
        void operator()(T t)
        {
            promise_->set_value(t);
        }
        
        void operator()(const boost::system::error_code &ec, T t)
        {
            if (ec)
                promise_->set_exception(boost::copy_exception(boost::system::system_error(ec)));
            else
                promise_->set_value();
        }
        
        //private:
        std::shared_ptr<promise<T>> promise_;
        fibio::fibers::detail::fiber_base::ptr_t fiber_;
    };
    
    template<>
    class promise_handler<void>
    {
    public:
        // Construct from use_future special value. Used during rebinding.
        template<typename Allocator>
        promise_handler(use_future_t<Allocator> uf)
        : promise_(std::allocate_shared<promise<void>>(uf.get_allocator(),
                                                       boost::allocator_arg,
                                                       uf.get_allocator()))
        , fiber_(fibio::fibers::detail::get_current_fiber_ptr())
        {}
        
        void operator()()
        {
            promise_->set_value();
        }
        
        void operator()(const boost::system::error_code &ec)
        {
            if (ec)
                promise_->set_exception(boost::copy_exception(boost::system::system_error(ec)));
            else
                promise_->set_value();
        }
        
        //private:
        std::shared_ptr<promise<void>> promise_;
        fibio::fibers::detail::fiber_base::ptr_t fiber_;
    };

    // Ensure any exceptions thrown from the handler are propagated back to the
    // caller via the future.
    template<typename Function, typename T>
    void fibio_do_handler_invoke(Function f, fibio::fibers::asio::detail::promise_handler<T> h)
    {
        std::shared_ptr<fibio::fibers::promise<T>> p(h.promise_);
        try
        { f(); }
        catch (...)
        { p->set_exception(boost::current_exception()); }
    }
}}}}    // End of namespace fibio::fibers::asio::detail

namespace boost { namespace asio {
    namespace detail {
        template<typename Function, typename T>
        void asio_handler_invoke(Function f, fibio::fibers::asio::detail::promise_handler<T> *h)
        {
            fibio::fiber(fibio::fibers::asio::detail::fibio_do_handler_invoke<Function, T>,
                         f,
                         fibio::fibers::asio::detail::promise_handler<T>(std::move(*h))).detach();
        }
    }   // End of namespace boost::asio::detail
    
    // Handler traits specialisation for promise_handler.
    template<typename T>
    class async_result<fibio::fibers::asio::detail::promise_handler<T>>
    {
    public:
        // The initiating function will return a future.
        typedef fibio::fibers::future<T> type;
        
        // Constructor creates a new promise for the async operation, and obtains the
        // corresponding future.
        explicit async_result(fibio::fibers::asio::detail::promise_handler<T> &h)
        { value_ = h.promise_->get_future(); }
        
        // Obtain the future to be returned from the initiating function.
        type get()
        { return std::move(value_); }
        
    private:
        type value_;
    };
    
    // Handler type specialisation for use_future.
    template<typename Allocator, typename ReturnType>
    struct handler_type<fibio::fibers::asio::use_future_t<Allocator>, ReturnType()>
    { typedef fibio::fibers::asio::detail::promise_handler<void> type; };
    
    // Handler type specialisation for use_future.
    template<typename Allocator, typename ReturnType, typename Arg1>
    struct handler_type<fibio::fibers::asio::use_future_t<Allocator>, ReturnType(Arg1)>
    { typedef fibio::fibers::asio::detail::promise_handler<Arg1> type; };
    
    // Handler type specialisation for use_future.
    template<typename Allocator, typename ReturnType>
    struct handler_type<fibio::fibers::asio::use_future_t<Allocator>, ReturnType(boost::system::error_code)>
    { typedef fibio::fibers::asio::detail::promise_handler<void> type; };
    
    // Handler type specialisation for use_future.
    template<typename Allocator, typename ReturnType, typename Arg2>
    struct handler_type<fibio::fibers::asio::use_future_t<Allocator>, ReturnType(boost::system::error_code, Arg2)>
    { typedef fibio::fibers::asio::detail::promise_handler<Arg2> type; };
}}  // End of namespace boost::asio

#endif
