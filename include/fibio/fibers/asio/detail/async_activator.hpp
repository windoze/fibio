//
//  async_activator.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-30.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_asio_detail_async_activator_hpp
#define fibio_fibers_asio_detail_async_activator_hpp

#include <chrono>
#include <memory>
#include <functional>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/system/error_code.hpp>
#include <fibio/fibers/detail/fiber_base.hpp>

namespace fibio { namespace fibers { namespace asio { namespace detail {
    struct async_activator : std::enable_shared_from_this<async_activator> {
        typedef std::shared_ptr<async_activator> ptr_t;

        async_activator(fibio::fibers::detail::fiber_base::ptr_t this_fiber,
                        uint64_t timeout,
                        std::function<void()> &&cancelation)
        : fiber_(this_fiber)
        , timeout_(timeout)
        , cancelation_(std::move(cancelation))
        , timer_(std::make_shared<yield_timer_t>(fiber_->get_fiber_strand().get_io_service()))
        , timer_triggered(false)
        , async_op_triggered(false)
        {}
        
        async_activator(const async_activator&)=delete;
        async_activator(async_activator&&)=delete;
        
        void activate() {
            async_op_triggered=true;
            // Operation completed, cancel timer
            if(timeout_>0 && !timer_triggered) timer_->cancel();
            if(timeout_==0 || timer_triggered) {
                // Both callback are called, resume fiber
                //fiber_->activate();
                ptr_t pthis(shared_from_this());
                fiber_->get_fiber_strand().dispatch([pthis](){ pthis->fiber_->activate(); });
            }
        }
        
        void start_timer_with_cancelation() {
            if (timeout_>0) {
                timer_->expires_from_now(std::chrono::microseconds(timeout_));
                ptr_t pthis(shared_from_this());
                timer_->async_wait(fiber_->get_fiber_strand().wrap(std::bind(&async_activator::on_timeout, pthis, std::placeholders::_1)));
            }
        }
        
        // private:
        void on_timeout(const boost::system::error_code &ec) {
            timer_triggered=true;
            if(async_op_triggered) {
                // Both callback are called, resume fiber
                //fiber_->activate();
                ptr_t pthis(shared_from_this());
                fiber_->get_fiber_strand().dispatch([pthis](){ pthis->fiber_->activate(); });
            } else {
                if(cancelation_) cancelation_();
            }
        }
        
        typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> yield_timer_t;
        typedef std::shared_ptr<boost::asio::basic_waitable_timer<std::chrono::steady_clock>> timer_ptr_t;
        
        fibio::fibers::detail::fiber_base::ptr_t fiber_;
        uint64_t timeout_;
        std::function<void()> cancelation_;
        timer_ptr_t timer_;
        bool timer_triggered;
        bool async_op_triggered;
    };
}}}}    // End of namespace fibio::fibers::asio::detail

#endif
