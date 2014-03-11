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
    
#if 0
    // TODO:
    socket listen(const endpoint &ep, bool reuse_addr=true) {
        
    }

    socket connect(const endpoint &remote_ep, const endpoint &local_ep=endpoint()) {
        
    }
#endif
}}  // End of namespace fibio::io
