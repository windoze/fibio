//
//  tcp.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-5.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <asio/ip/tcp.hpp>
#include "../fiber/fiber_object.hpp"

namespace fibio { namespace fibers { namespace this_fiber { namespace detail {
    asio::io_service &get_io_service();
}}}}    // End of namespace fibio::fibers::this_fiber::detail

namespace fibio { namespace io {
    
    // Throw system_error(error_code)
    
    asio::ip::tcp::endpoint resolve(const asio::ip::tcp::resolver::query &q) {
        asio::ip::tcp::endpoint ep;
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        asio::ip::tcp::resolver resolver(this_fiber->io_service_);
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            (*(this_fiber->caller_))([this_fiber, &q, &ep, &resolver](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                resolver.async_resolve(q, (this_fiber->fiber_strand_.wrap([this_fiber, &ep](const std::error_code &ec, asio::ip::tcp::resolver::iterator iterator){
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

    asio::ip::tcp::socket connect(const asio::ip::tcp::endpoint &remote_ep,
                                         const asio::ip::tcp::endpoint &local_ep,
                                         uint64_t timeout) {
        asio::ip::tcp::socket s(fibers::detail::fiber_object::current_fiber_->io_service_);
        if(local_ep!=asio::ip::tcp::endpoint())
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
    
    asio::ip::tcp::acceptor listen(const asio::ip::tcp::endpoint &ep, bool reuse_addr) {
        asio::ip::tcp::acceptor a(fibers::this_fiber::detail::get_io_service(), ep, reuse_addr);
        return a;
    }
    
    asio::ip::tcp::socket accept(asio::ip::tcp::acceptor &a, uint64_t timeout) {
        asio::ip::tcp::socket s(fibers::detail::fiber_object::current_fiber_->io_service_);
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
                            a.cancel();
                        }
                    }
                }));
                a.async_accept(s, (this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec){
                    async_op_triggered=true;
                    // Operation completed, cancel timer
                    sleep_timer.cancel();
                    if(ec==asio::error::operation_aborted)
                        ec=asio::error::timed_out;
                    this_fiber->last_error_=ec;
                    if(timeout==0 || timer_triggered) {
                        // Both callback are called, resume fiber
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

    size_t read_some(asio::ip::tcp::socket &s,
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
    
    size_t write_some(asio::ip::tcp::socket &s,
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
    
    // Return error_code
    
    asio::ip::tcp::endpoint resolve(const asio::ip::tcp::resolver::query &q, std::error_code &ec) {
        asio::ip::tcp::endpoint ep;
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        asio::ip::tcp::resolver resolver(this_fiber->io_service_);
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            (*(this_fiber->caller_))([this_fiber, &q, &ep, &resolver](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                resolver.async_resolve(q, (this_fiber->fiber_strand_.wrap([this_fiber, &ep](const std::error_code &ec, asio::ip::tcp::resolver::iterator iterator){
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
    
    std::error_code connect(asio::ip::tcp::socket &s,
                                      const asio::ip::tcp::endpoint &remote_ep,
                                      const asio::ip::tcp::endpoint &local_ep,
                                      uint64_t timeout) {
        std::error_code ec;
        if(local_ep!=asio::ip::tcp::endpoint())
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
            ec=std::make_error_code(static_cast<std::errc>(this_fiber->last_error_.value()));
            this_fiber->last_error_.clear();
        }
        return ec;
    }
    
    asio::ip::tcp::socket accept(asio::ip::tcp::acceptor &a,
                                 uint64_t timeout,
                                 std::error_code &ec) {
        asio::ip::tcp::socket s(fibers::detail::fiber_object::current_fiber_->io_service_);
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
                            a.cancel();
                        }
                    }
                }));
                a.async_accept(s, (this_fiber->fiber_strand_.wrap([&, this_fiber](std::error_code ec){
                    async_op_triggered=true;
                    // Operation completed, cancel timer
                    sleep_timer.cancel();
                    if(ec==asio::error::operation_aborted)
                        ec=asio::error::timed_out;
                    this_fiber->last_error_=ec;
                    if(timeout==0 || timer_triggered) {
                        // Both callback are called, resume fiber
                        this_fiber->state_=fibers::detail::fiber_object::RUNNING;
                        this_fiber->one_step();
                    }
                })));
            });
        } else {
            // TODO: Error
        }
        if (this_fiber->last_error_) {
            ec=std::make_error_code(static_cast<std::errc>(this_fiber->last_error_.value()));
            this_fiber->last_error_.clear();
        }
        return s;
    }
    
    size_t read_some(asio::ip::tcp::socket &s,
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
            ec=std::make_error_code(static_cast<std::errc>(this_fiber->last_error_.value()));
            this_fiber->last_error_.clear();
        }
        return ret;
    }
    
    size_t write_some(asio::ip::tcp::socket &s,
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
            ec=std::make_error_code(static_cast<std::errc>(this_fiber->last_error_.value()));
            this_fiber->last_error_.clear();
        }
        return ret;
    }
}}   // End of namespace fibio::io
