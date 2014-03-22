//
//  udp.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-6.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <asio/ip/udp.hpp>
#include "../fiber/fiber_object.hpp"

namespace fibio { namespace fibers { namespace this_fiber { namespace detail {
    asio::io_service &get_io_service();
}}}}    // End of namespace fibio::fibers::this_fiber::detail

namespace fibio { namespace io {
    //using namespace udp;
    asio::ip::udp::endpoint resolve(const asio::ip::udp::resolver::query &q) {
        asio::ip::udp::endpoint ep;
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        asio::ip::udp::resolver resolver(this_fiber->io_service_);
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            (*(this_fiber->caller_))([this_fiber, &q, &ep, &resolver](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                resolver.async_resolve(q, (this_fiber->fiber_strand_.wrap([this_fiber, &ep](const std::error_code &ec, asio::ip::udp::resolver::iterator iterator){
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
    
    asio::ip::udp::endpoint resolve(const asio::ip::udp::resolver::query &q,
                                           std::error_code &ec) {
        asio::ip::udp::endpoint ep;
        fibers::detail::fiber_ptr_t this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this());
        asio::ip::udp::resolver resolver(this_fiber->io_service_);
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            (*(this_fiber->caller_))([this_fiber, &q, &ep, &resolver](){
                this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
                resolver.async_resolve(q, (this_fiber->fiber_strand_.wrap([this_fiber, &ep](const std::error_code &ec, asio::ip::udp::resolver::iterator iterator){
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
    
    std::error_code bind(asio::ip::udp::socket &s, const asio::ip::udp::endpoint &ep) {
        std::error_code ec;
        
        s.bind(ep, ec);
        
        // HACK
        ec=std::make_error_code(static_cast<std::errc>(ec.value()));

        return ec;
    }
    
    asio::ip::udp::socket connect(const asio::ip::udp::endpoint &remote_ep, uint64_t timeout) {
        asio::ip::udp::socket s(fibers::detail::fiber_object::current_fiber_->io_service_);
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

    std::error_code connect(asio::ip::udp::socket &s, const asio::ip::udp::endpoint &remote_ep, uint64_t timeout) {
        std::error_code ec;
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
}}  // End of namespace fibio::io
