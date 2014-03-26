//
//  forward.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_forward_hpp
#define fibio_forward_hpp

#include <functional>
#include <chrono>
#include <memory>
#include <asio/io_service.hpp>
#include <asio/basic_waitable_timer.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>
#include <asio/ip/icmp.hpp>

namespace fibio { namespace fibers { namespace detail {
    struct scheduler_object;
    struct fiber_object;
    struct mutex_object;
    struct recursive_mutex_object;
    struct timed_mutex_object;
    struct timed_recursive_mutex_object;
    struct condition_variable_object;

    struct fiber_async_handler {
        fiber_async_handler();
        void start_timer_with_cancelation(uint64_t timeout, std::function<void()> &&c);
        void run_in_scheduler_context(std::function<void()> f);
        void throw_or_return(bool throw_error, std::error_code &ec);
        std::function<void(std::error_code, size_t)> get_io_handler();
        std::function<void(std::error_code)> get_async_op_handler();
        
        std::function<void(std::error_code, asio::ip::tcp::resolver::iterator)> get_resolve_handler(asio::ip::tcp *);
        std::function<void(std::error_code, asio::ip::udp::resolver::iterator)> get_resolve_handler(asio::ip::udp *);
        std::function<void(std::error_code, asio::ip::icmp::resolver::iterator)> get_resolve_handler(asio::ip::icmp *);
        
        size_t io_result() const { return io_ret_; }
        asio::ip::tcp::resolver::iterator resolve_result(asio::ip::tcp *) const { return tcp_resolve_ret_; }
        asio::ip::udp::resolver::iterator resolve_result(asio::ip::udp *) const { return udp_resolve_ret_; }
        asio::ip::icmp::resolver::iterator resolve_result(asio::ip::icmp *) const { return icmp_resolve_ret_; }
        
    private:
        void on_timeout(std::error_code ec);
        void on_async_op_complete(std::error_code ec);
        void on_io_complete(std::error_code ec, size_t sz);
        void on_tcp_resolve_complete(std::error_code error, asio::ip::tcp::resolver::iterator iterator);
        void on_udp_resolve_complete(std::error_code error, asio::ip::udp::resolver::iterator iterator);
        void on_icmp_resolve_complete(std::error_code error, asio::ip::icmp::resolver::iterator iterator);
        
        std::shared_ptr<detail::fiber_object> this_fiber;
        asio::basic_waitable_timer<std::chrono::steady_clock> sleep_timer;
        uint64_t timeout_=0;
        bool timer_triggered=false;
        bool async_op_triggered=false;
        std::function<void()> cancelation_;
        size_t io_ret_;
        asio::ip::tcp::resolver::iterator tcp_resolve_ret_;
        asio::ip::udp::resolver::iterator udp_resolve_ret_;
        asio::ip::icmp::resolver::iterator icmp_resolve_ret_;
    };
}}} // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers { namespace this_fiber { namespace detail {
    asio::io_service &get_io_service();
}}}} // End of namespace fibio::fibers::this_fiber::detail

#endif
