//
//  io_ops.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-7.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_ops_hpp
#define fibio_io_ops_hpp

// Clang version Apple LLVM version 5.1 (clang-503.0.38) (based on LLVM 3.4svn) failed to compile follow code
#if (__clang_major__==5) && (__clang_minor__==1) && (__clang_patchlevel__==0) && (__apple_build_version__==5030038)
#define FIBIO_CLANG_BUG
#endif

#ifndef FIBIO_CLANG_BUG
#include "../fiber/fiber_object.hpp"

namespace fibio { namespace io { namespace detail {
    template<typename Protocol>
    typename Protocol::endpoint resolve(const typename Protocol::resolver::query &q) {
        typename Protocol::endpoint ep;
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        typename Protocol::resolver resolver(this_fiber->io_service_);
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            (*(this_fiber->caller_))([this_fiber, &q, &ep, &resolver](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                resolver.async_resolve(q, (this_fiber->fiber_strand_.wrap([this_fiber, &ep](const std::error_code &ec, typename Protocol::resolver::iterator iterator){
                    this_fiber->last_error_=ec;
                    if (!ec) {
                        ep=*iterator;
                    }
                    this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                    this_fiber->one_step();
                })));
            });
        } else {
            // TODO: Error
        }
        this_fiber->throw_on_error();
        return ep;
    }

    template<typename Protocol>
    typename Protocol::endpoint resolve(const typename Protocol::resolver::query &q, std::error_code &ec) {
        typename Protocol::endpoint ep;
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        typename Protocol::resolver resolver(this_fiber->io_service_);
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            (*(this_fiber->caller_))([this_fiber, &q, &ep, &resolver](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                resolver.async_resolve(q, (this_fiber->fiber_strand_.wrap([this_fiber, &ep](const std::error_code &ec, typename Protocol::resolver::iterator iterator){
                    this_fiber->last_error_=ec;
                    if (!ec) {
                        ep=*iterator;
                    }
                    this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                    this_fiber->one_step();
                })));
            });
        } else {
            // TODO: Error
        }
        if (this_fiber->last_error_) {
            ec=this_fiber->last_error_;
            this_fiber->last_error_.clear();
        }
        return ep;
    }
    
    template<typename Protocol>
    typename Protocol::socket connect(const typename Protocol::endpoint &remote_ep,
                                      const typename Protocol::endpoint &local_ep,
                                      uint64_t timeout) {
        typename Protocol::socket s(fibers::detail::fiber_object::current_fiber_->io_service_);
        if(local_ep!=typename Protocol::endpoint())
            s.bind(local_ep);
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
                s.async_connect(remote_ep, (this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec){
                    async_op_triggered=true;
                    // Operation completed, cancel timer
                    sleep_timer.cancel();
                    if(ec==asio::error::operation_aborted)
                        ec=asio::error::timed_out;
                    this_fiber->last_error_=ec;
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
        return s;
    }

    template<typename Protocol>
    std::error_code connect(typename Protocol::socket &s,
                                      const typename Protocol::endpoint &remote_ep,
                                      const typename Protocol::endpoint &local_ep,
                                      uint64_t timeout)
    {
        std::error_code ec;
        if(local_ep!=typename Protocol::endpoint())
            s.bind(local_ep);
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
                s.async_connect(remote_ep, (this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec){
                    async_op_triggered=true;
                    // Operation completed, cancel timer
                    sleep_timer.cancel();
                    if(ec==asio::error::operation_aborted)
                        ec=asio::error::timed_out;
                    this_fiber->last_error_=ec;
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
        return ec;
    }
    
    template<typename Protocol>
    typename Protocol::socket accept(typename Protocol::acceptor &a) {
        typename Protocol::socket s(fibers::detail::fiber_object::current_fiber_->io_service_);
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            (*(this_fiber->caller_))([&, this_fiber](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                a.async_accept(s, (this_fiber->fiber_strand_.wrap([this_fiber, &s](std::error_code ec){
                    this_fiber->last_error_=ec;
                    this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                    this_fiber->one_step();
                })));
            });
        } else {
            // TODO: Error
        }
        this_fiber->throw_on_error();
        return s;
    }

    template<typename Protocol>
    typename Protocol::socket accept(typename Protocol::acceptor &a, std::error_code &ec) {
        typename Protocol::socket s(fibers::detail::fiber_object::current_fiber_->io_service_);
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            (*(this_fiber->caller_))([&, this_fiber](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                a.async_accept(s, (this_fiber->fiber_strand_.wrap([this_fiber, &s](std::error_code ec){
                    this_fiber->last_error_=ec;
                    this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                    this_fiber->one_step();
                })));
            });
        } else {
            // TODO: Error
        }
        if (this_fiber->last_error_) {
            ec=this_fiber->last_error_;
            this_fiber->last_error_.clear();
        }
        return s;
    }
    
    template<typename StreamDescriptor>
    size_t read_some(StreamDescriptor &s, char *buffer, size_t sz, uint64_t timeout) {
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

    template<typename StreamDescriptor>
    size_t read_some(StreamDescriptor &s, char *buffer, size_t sz, uint64_t timeout, std::error_code &ec) {
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

    template<typename StreamDescriptor>
    size_t write_some(StreamDescriptor &s, const char *buffer, size_t sz, uint64_t timeout) {
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

    template<typename StreamDescriptor>
    size_t write_some(StreamDescriptor &s, const char *buffer, size_t sz, uint64_t timeout, std::error_code &ec) {
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
}}} // End of namespace fibio::io::detail

namespace fibio { namespace fibers { namespace this_fiber { namespace detail {
    asio::io_service &get_io_service();
}}}}    // End of namespace fibio::fibers::this_fiber::detail

#endif  // !defined(FIBIO_CLANG_BUG)

#endif
