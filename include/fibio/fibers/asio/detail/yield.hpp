//
//  yield.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_asio_detail_yield_hpp
#define fibio_fibers_asio_detail_yield_hpp

#include <chrono>
#include <memory>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/throw_exception.hpp>
#include <fibio/fibers/detail/fiber_base.hpp>

namespace fibio { namespace fibers { namespace asio { namespace detail {
    template<typename T>
    class yield_handler
    {
    public:
        yield_handler(const yield_t &y)
        : ec_(y.ec_)
        , value_(0)
        , fiber_(fibio::fibers::detail::get_current_fiber_ptr())
        {}
        
        void operator()(T t)
        {
            // Async op completed, resume waiting fiber
            *ec_ = boost::system::error_code();
            *value_ = t;
            fiber_->get_fiber_strand().dispatch( std::bind(&fibio::fibers::detail::fiber_base::activate, fiber_) );
        }
        
        void operator()(const boost::system::error_code &ec, T t)
        {
            // Async op completed, resume waiting fiber
            *ec_ = ec;
            *value_ = t;
            fiber_->get_fiber_strand().dispatch( std::bind(&fibio::fibers::detail::fiber_base::activate, fiber_) );
        }
        
        //private:
        boost::system::error_code *ec_;
        T *value_;
        fibio::fibers::detail::fiber_base::ptr_t fiber_;
    };
    
    // Completion handler to adapt a void promise as a completion handler.
    template<>
    class yield_handler<void>
    {
    public:
        yield_handler(const yield_t &y)
        : ec_(y.ec_)
        , fiber_(fibio::fibers::detail::get_current_fiber_ptr())
        {}
        
        void operator()()
        {
            // Async op completed, resume waiting fiber
            *ec_ = boost::system::error_code();
            fiber_->get_fiber_strand().dispatch( std::bind(&fibio::fibers::detail::fiber_base::activate, fiber_) );
        }
        
        void operator()(boost::system::error_code const& ec)
        {
            // Async op completed, resume waiting fiber
            *ec_ = ec;
            fiber_->get_fiber_strand().dispatch( std::bind(&fibio::fibers::detail::fiber_base::activate, fiber_) );
        }
        
        //private:
        boost::system::error_code *ec_;
        fibio::fibers::detail::fiber_base::ptr_t fiber_;
    };
}}}}    // End of namespace fibio::fibers::asio::detail

namespace boost { namespace asio {
    template<typename T>
    class async_result<fibio::fibers::asio::detail::yield_handler<T>>
    {
    public:
        typedef T type;
        
        explicit async_result(fibio::fibers::asio::detail::yield_handler<T> &h)
        {
            out_ec_ = h.ec_;
            if (!out_ec_) h.ec_ = & ec_;
            h.value_ = & value_;
        }
        
        type get()
        {
            // Wait until async op completed
            fibio::fibers::detail::get_current_fiber_ptr()->pause();
            if (!out_ec_ && ec_)
                throw_exception(boost::system::system_error(ec_));
            return value_;
        }
        
    private:
        
        boost::system::error_code *out_ec_;
        boost::system::error_code ec_;
        type value_;
    };

    template<>
    class async_result<fibio::fibers::asio::detail::yield_handler<void>>
    {
    public:
        typedef void  type;
        
        explicit async_result(fibio::fibers::asio::detail::yield_handler<void> &h)
        {
            out_ec_ = h.ec_;
            if (!out_ec_) h.ec_ = & ec_;
        }
        
        void get()
        {
            // Wait until async op completed
            fibio::fibers::detail::get_current_fiber_ptr()->pause();
            if (!out_ec_ && ec_)
                throw_exception(boost::system::system_error(ec_));
        }
        
    private:
        boost::system::error_code *out_ec_;
        boost::system::error_code ec_;
    };
    
    // Handler type specialisation for yield_t.
    template<typename ReturnType>
    struct handler_type<fibio::fibers::asio::yield_t, ReturnType()>
    { typedef fibio::fibers::asio::detail::yield_handler<void> type; };
    
    // Handler type specialisation for yield_t.
    template<typename ReturnType, typename Arg1>
    struct handler_type<fibio::fibers::asio::yield_t, ReturnType(Arg1)>
    { typedef fibio::fibers::asio::detail::yield_handler<Arg1> type; };
    
    // Handler type specialisation for yield_t.
    template<typename ReturnType>
    struct handler_type<fibio::fibers::asio::yield_t, ReturnType(boost::system::error_code)>
    { typedef fibio::fibers::asio::detail::yield_handler<void> type; };
    
    // Handler type specialisation for yield_t.
    template<typename ReturnType, typename Arg2>
    struct handler_type<fibio::fibers::asio::yield_t, ReturnType( boost::system::error_code, Arg2)>
    { typedef fibio::fibers::asio::detail::yield_handler<Arg2> type; };
}}  // End of namespace boost::asio
#endif
