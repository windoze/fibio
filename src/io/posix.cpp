//
//  posix.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <asio/posix/stream_descriptor.hpp>
#include "../fiber/fiber_object.hpp"

namespace fibio { namespace fibers { namespace this_fiber { namespace detail {
    asio::io_service &get_io_service();
}}}}    // End of namespace fibio::fibers::this_fiber::detail

namespace fibio { namespace io {
    using namespace asio;
    using asio::posix::stream_descriptor;
    
    posix::stream_descriptor open(const stream_descriptor::native_handle_type &h) {
        return stream_descriptor(fibers::this_fiber::detail::get_io_service(), h);
    }
    
    size_t read_some(stream_descriptor &s,
                     char *buffer,
                     size_t sz,
                     uint64_t timeout) {
        size_t ret=0;
        auto buf=asio::buffer(buffer, sz);
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        fibers::detail::timer_t sleep_timer(this_fiber->io_service_);
        if(timeout>0) sleep_timer.expires_from_now(std::chrono::microseconds(timeout));
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            bool timer_triggered=false;
            bool async_op_triggered=false;
            (*(this_fiber->caller_))([&, this_fiber](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                if(timeout>0) sleep_timer.async_wait(this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec){
                    /*if (ec!=asio::error::operation_aborted)*/ {
                        timer_triggered=true;
                        // Timeout, cancel socket op if it's still pending
                        if(async_op_triggered) {
                            // Both callback are called, resume fiber
                            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                            this_fiber->one_step();
                        } else {
                            s.cancel();
                        }
                    }
                }));
                s.async_read_some(buf, (this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec, size_t sz){
                    async_op_triggered=true;
                    // Operation completed, cancel timer
                    sleep_timer.cancel();
                    if(ec==asio::error::operation_aborted)
                        ec=asio::error::timed_out;
                    this_fiber->last_error_=ec;
                    ret=sz;
                    if(timeout==0 || timer_triggered) {
                        // Both callback are called, resume fiber
                        // this_fiber->schedule();
                        this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                        this_fiber->one_step();
                    }
                })));
            });
        } else {
            // TODO: Error
        }
        this_fiber->throw_on_error();
        return ret;
    }
    
    size_t write_some(stream_descriptor &s,
                      const char *buffer,
                      size_t sz,
                      uint64_t timeout) {
        size_t ret=0;
        auto buf=asio::buffer(buffer, sz);
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        fibers::detail::timer_t sleep_timer(this_fiber->io_service_);
        if(timeout>0) sleep_timer.expires_from_now(std::chrono::microseconds(timeout));
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            bool timer_triggered=false;
            bool async_op_triggered=false;
            (*(this_fiber->caller_))([&, this_fiber](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                if(timeout>0) sleep_timer.async_wait(this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec){
                    /*if (ec!=asio::error::operation_aborted)*/ {
                        timer_triggered=true;
                        // Timeout, cancel socket op if it's still pending
                        if(async_op_triggered) {
                            // Both callback are called, resume fiber
                            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                            this_fiber->one_step();
                        } else {
                            s.cancel();
                        }
                    }
                }));
                s.async_write_some(buf, (this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec, size_t sz){
                    async_op_triggered=true;
                    // Operation completed, cancel timer
                    sleep_timer.cancel();
                    if(ec==asio::error::operation_aborted)
                        ec=asio::error::timed_out;
                    this_fiber->last_error_=ec;
                    ret=sz;
                    if(timeout==0 || timer_triggered) {
                        // Both callback are called, resume fiber
                        // this_fiber->schedule();
                        this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                        this_fiber->one_step();
                    }
                })));
            });
        } else {
            // TODO: Error
        }
        this_fiber->throw_on_error();
        return ret;
    }
    
    std::error_code open(stream_descriptor &s, const stream_descriptor::native_handle_type &h) {
        std::error_code ec;
        s.assign(h, ec);
        return ec;
    }
    
    size_t read_some(stream_descriptor &s,
                     char *buffer,
                     size_t sz,
                     uint64_t timeout,
                     std::error_code &ec) {
        size_t ret=0;
        auto buf=asio::buffer(buffer, sz);
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        fibers::detail::timer_t sleep_timer(this_fiber->io_service_);
        if(timeout>0) sleep_timer.expires_from_now(std::chrono::microseconds(timeout));
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            bool timer_triggered=false;
            bool async_op_triggered=false;
            (*(this_fiber->caller_))([&, this_fiber](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                if(timeout>0) sleep_timer.async_wait(this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec){
                    /*if (ec!=asio::error::operation_aborted)*/ {
                        timer_triggered=true;
                        // Timeout, cancel socket op if it's still pending
                        if(async_op_triggered) {
                            // Both callback are called, resume fiber
                            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                            this_fiber->one_step();
                        } else {
                            s.cancel();
                        }
                    }
                }));
                s.async_read_some(buf, (this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec, size_t sz){
                    async_op_triggered=true;
                    // Operation completed, cancel timer
                    sleep_timer.cancel();
                    if(ec==asio::error::operation_aborted)
                        ec=asio::error::timed_out;
                    this_fiber->last_error_=ec;
                    ret=sz;
                    if(timeout==0 || timer_triggered) {
                        // Both callback are called, resume fiber
                        // this_fiber->schedule();
                        this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                        this_fiber->one_step();
                    }
                })));
            });
        } else {
            // TODO: Error
        }
        if (this_fiber->last_error_) {
            ec=this_fiber->last_error_;
            this_fiber->last_error_.clear();
        }
        return ret;
    }
    
    size_t write_some(stream_descriptor &s,
                      const char *buffer,
                      size_t sz,
                      uint64_t timeout,
                      std::error_code &ec) {
        size_t ret=0;
        auto buf=asio::buffer(buffer, sz);
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        fibers::detail::timer_t sleep_timer(this_fiber->io_service_);
        if(timeout>0) sleep_timer.expires_from_now(std::chrono::microseconds(timeout));
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            bool timer_triggered=false;
            bool async_op_triggered=false;
            (*(this_fiber->caller_))([&, this_fiber](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                if(timeout>0) sleep_timer.async_wait(this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec){
                    /*if (ec!=asio::error::operation_aborted)*/ {
                        timer_triggered=true;
                        // Timeout, cancel socket op if it's still pending
                        if(async_op_triggered) {
                            // Both callback are called, resume fiber
                            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                            this_fiber->one_step();
                        } else {
                            s.cancel();
                        }
                    }
                }));
                s.async_write_some(buf, (this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec, size_t sz){
                    async_op_triggered=true;
                    // Operation completed, cancel timer
                    sleep_timer.cancel();
                    if(ec==asio::error::operation_aborted)
                        ec=asio::error::timed_out;
                    this_fiber->last_error_=ec;
                    ret=sz;
                    if(timeout==0 || timer_triggered) {
                        // Both callback are called, resume fiber
                        // this_fiber->schedule();
                        this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                        this_fiber->one_step();
                    }
                })));
            });
        } else {
            // TODO: Error
        }
        if (this_fiber->last_error_) {
            ec=this_fiber->last_error_;
            this_fiber->last_error_.clear();
        }
        return ret;
    }
}}  // End of namespace fibio::io
