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
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <fibio/fibers/detail/fiber_base.hpp>
#include <fibio/fibers/fiber.hpp>

namespace fibio { namespace fibers { namespace asio { namespace detail {
    typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> yield_timer_t;
    typedef std::shared_ptr<boost::asio::basic_waitable_timer<std::chrono::steady_clock>> timer_ptr_t;
    
    struct async_activator {
        async_activator(fibio::fibers::detail::fiber_base::ptr_t this_fiber,
                        uint64_t timeout,
                        std::function<void()> &&cancelation)
        : fiber_(this_fiber)
        , timeout_(timeout)
        , cancelation_(std::move(cancelation))
        , timer_(std::make_shared<yield_timer_t>(::fibio::asio::get_io_service()))
        , timer_triggered(false)
        , async_op_triggered(false)
        {}
        
        void activate() {
            async_op_triggered=true;
            // Operation completed, cancel timer
            if(timeout_>0 && !timer_triggered) timer_->cancel();
            if(timeout_==0 || timer_triggered) {
                // Both callback are called, resume fiber
                fiber_->activate();
            }
        }

        void start_timer_with_cancelation() {
            if (timeout_>0) {
                timer_->expires_from_now(std::chrono::microseconds(timeout_));
                timer_->async_wait(fiber_->get_fiber_strand().wrap(std::bind(&async_activator::on_timeout, this, std::placeholders::_1)));
            }
        }
        
        void on_timeout(const boost::system::error_code &ec) {
            timer_triggered=true;
            if(async_op_triggered) {
                // Both callback are called, resume fiber
                fiber_->activate();
            } else {
                if(cancelation_) cancelation_();
            }
        }
        
        // private:
        fibio::fibers::detail::fiber_base::ptr_t fiber_;
        uint64_t timeout_;
        std::function<void()> cancelation_;
        timer_ptr_t timer_;
        bool timer_triggered;
        bool async_op_triggered;
    };
    
    typedef std::shared_ptr<async_activator> activator_ptr_t;
    
    template<typename T>
    class yield_handler
    {
    public:
        yield_handler(const yield_t &y)
        : ec_(y.ec_)
        , value_(0)
        , activator_(std::make_shared<async_activator>(fibio::fibers::detail::get_current_fiber_ptr(),
                                                       y.timeout_,
                                                       std::move(y.cancelation_)))
        {}
        
        void operator()(T t)
        {
            // Async op completed, resume waiting fiber
            *ec_ = boost::system::error_code();
            *value_ = t;
            activator_->activate();
        }
        
        void operator()(const boost::system::error_code &ec, T t)
        {
            // Async op completed, resume waiting fiber
            *ec_ = ec;
            *value_ = t;
            activator_->activate();
        }
        
        void start_timer_with_cancelation() {
            activator_->start_timer_with_cancelation();
        }
        
        //private:
        boost::system::error_code *ec_;
        T *value_;
        activator_ptr_t activator_;
    };
    
    // Completion handler to adapt a void promise as a completion handler.
    template<>
    class yield_handler<void>
    {
    public:
        yield_handler(const yield_t &y)
        : ec_(y.ec_)
        , activator_(std::make_shared<async_activator>(fibio::fibers::detail::get_current_fiber_ptr(),
                                                       y.timeout_,
                                                       std::move(y.cancelation_)))
        {}
        
        void operator()()
        {
            // Async op completed, resume waiting fiber
            *ec_ = boost::system::error_code();
            activator_->activate();
        }
        
        void operator()(boost::system::error_code const& ec)
        {
            // Async op completed, resume waiting fiber
            *ec_ = ec;
            activator_->activate();
        }
        
        void start_timer_with_cancelation() {
            activator_->start_timer_with_cancelation();
        }
        
        //private:
        boost::system::error_code *ec_;
        activator_ptr_t activator_;
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
            h.start_timer_with_cancelation();
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
            h.start_timer_with_cancelation();
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
