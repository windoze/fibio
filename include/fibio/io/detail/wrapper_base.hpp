//
//  wrapper_base.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_detail_wrapper_base_hpp
#define fibio_io_detail_wrapper_base_hpp

#include <fibio/fibers/detail/forward.hpp>

namespace fibio { namespace io {
    namespace detail {
        using fibio::fibers::detail::fiber_async_handler;
    }
    
    template<typename T>
    struct fiberized;
}}  // End of namespace fibio::io

// fiberized operations implementation

#define FIBIO_IMPLEMENT_EXTENDED_CONNECT \
    std::error_code connect(const std::string &host, const std::string &service) { \
        std::error_code ec; \
        typedef fiberized<asio::ip::basic_resolver<typename base_type::lowest_layer_type::protocol_type>> resolver_type; \
        typedef typename resolver_type::query query_type; \
        resolver_type r; \
        query_type q(host, service); \
        typename resolver_type::iterator i=r.resolve(q, ec); \
        if (ec) return ec; \
        return connect(i->endpoint(), ec); \
    } \
    std::error_code connect(const std::string &addr, unsigned short port) { \
        std::error_code ec; \
        asio::ip::address a=asio::ip::address::from_string(addr, ec); \
        if (ec) return ec; \
        typename base_type::lowest_layer_type::endpoint_type endpoint(a, port); \
        return connect(endpoint, ec); \
    }

#define FIBIO_IMPLEMENT_FIBERIZED_CONNECT \
    void connect(const typename base_type::lowest_layer_type::endpoint_type & peer_endpoint) { \
        std::error_code ec; \
        do_connect(peer_endpoint, ec, true); \
    } \
    std::error_code connect(const typename base_type::lowest_layer_type::endpoint_type & peer_endpoint, \
                            std::error_code & ec) \
    { return do_connect(peer_endpoint, ec, false); } \
    std::error_code do_connect(const typename base_type::lowest_layer_type::endpoint_type & peer_endpoint, \
                               std::error_code & ec, \
                               bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &peer_endpoint](){ \
            async_handler.start_timer_with_cancelation(connect_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->lowest_layer().async_connect(peer_endpoint, async_handler.get_async_op_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return ec; \
    } \
    template<typename Rep, typename Period> \
    void set_connect_timeout(const std::chrono::duration<Rep, Period>& timeout_duration) \
    { connect_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); } \
    uint64_t connect_timeout_=0;

#define FIBIO_IMPLEMENT_FIBERIZED_READ_SOME \
    template<typename MutableBufferSequence> \
    std::size_t read_some(const MutableBufferSequence & buffers) { \
        std::error_code ec; \
        return do_read_some(buffers, ec, true); \
    } \
    template<typename MutableBufferSequence> \
    std::size_t read_some(const MutableBufferSequence & buffers, \
                          std::error_code &ec) \
    { return do_read_some(buffers, ec, false); } \
    template<typename MutableBufferSequence> \
    std::size_t do_read_some(const MutableBufferSequence & buffers, \
                             std::error_code &ec, \
                             bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers](){ \
            async_handler.start_timer_with_cancelation(read_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_read_some(buffers, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename Rep, typename Period> \
    void set_read_timeout(const std::chrono::duration<Rep, Period>& timeout_duration) \
    { read_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); } \
    uint64_t read_timeout_=0;
    
#define FIBIO_IMPLEMENT_FIBERIZED_WRITE_SOME \
    template<typename ConstBufferSequence> \
    std::size_t write_some(const ConstBufferSequence & buffers) { \
        std::error_code ec; \
        return do_write_some(buffers, ec, true); \
    } \
    template<typename ConstBufferSequence> \
    std::size_t write_some(const ConstBufferSequence & buffers, \
                           std::error_code & ec) \
    { return do_write_some(buffers, ec, false); } \
    template<typename ConstBufferSequence> \
    std::size_t do_write_some(const ConstBufferSequence & buffers, \
                              std::error_code & ec, \
                              bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers](){ \
            async_handler.start_timer_with_cancelation(write_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_write_some(buffers, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename Rep, typename Period> \
    void set_write_timeout(const std::chrono::duration<Rep, Period>& timeout_duration) \
    { write_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); } \
    uint64_t write_timeout_=0;
    
#define FIBIO_IMPLEMENT_FIBERIZED_SEND \
    template<typename MutableBufferSequence> \
    std::size_t send(const MutableBufferSequence & buffers) { \
        std::error_code ec; \
        return do_send(buffers, ec, true); \
    } \
    template<typename ConstBufferSequence> \
    std::size_t send(const ConstBufferSequence & buffers, \
                     std::error_code &ec) \
    { return do_send(buffers, ec, false); } \
    template<typename ConstBufferSequence> \
    std::size_t do_send(const ConstBufferSequence & buffers, \
                        std::error_code &ec, \
                        bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers](){ \
            async_handler.start_timer_with_cancelation(send_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_send(buffers, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename ConstBufferSequence> \
    std::size_t send(const ConstBufferSequence & buffers, \
                     asio::socket_base::message_flags flags) \
    { \
        std::error_code ec; \
        return do_send(buffers, flags, ec, true); \
    } \
    template<typename ConstBufferSequence> \
    std::size_t send(const ConstBufferSequence & buffers, \
                     asio::socket_base::message_flags flags, \
                     std::error_code &ec) \
    { return do_send(buffers, flags, ec, false); } \
    template<typename ConstBufferSequence> \
    std::size_t do_send(const ConstBufferSequence & buffers, \
                        asio::socket_base::message_flags flags, \
                        std::error_code &ec, \
                        bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers, &flags](){ \
            async_handler.start_timer_with_cancelation(send_timeout_, [this](){ this->lowest_layer().cancel(); });\
            this->async_send(buffers, flags, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename Rep, typename Period> \
    void set_send_timeout(const std::chrono::duration<Rep, Period>& timeout_duration) \
    { send_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); } \
    uint64_t send_timeout_=0;
    
#define FIBIO_IMPLEMENT_FIBERIZED_RECEIVE \
    template<typename MutableBufferSequence> \
    std::size_t receive(const MutableBufferSequence & buffers) { \
        std::error_code ec; \
        return do_receive(buffers, ec, true); \
    } \
    template<typename MutableBufferSequence> \
    std::size_t receive(const MutableBufferSequence & buffers, \
                        std::error_code &ec) \
    { return do_receive(buffers, ec, false); } \
    template<typename MutableBufferSequence> \
    std::size_t do_receive(const MutableBufferSequence & buffers, \
                           std::error_code &ec, \
                           bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers](){ \
            async_handler.start_timer_with_cancelation(receive_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_receive(buffers, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename MutableBufferSequence> \
    std::size_t receive(const MutableBufferSequence & buffers, \
                        asio::socket_base::message_flags flags) \
    { \
        std::error_code ec; \
        return do_receive(buffers, flags, ec, true); \
    } \
    template<typename MutableBufferSequence> \
    std::size_t receive(const MutableBufferSequence & buffers, \
                        asio::socket_base::message_flags flags, \
                        std::error_code &ec) \
    { return do_receive(buffers, flags, ec, false); } \
    template<typename MutableBufferSequence> \
    std::size_t do_receive(const MutableBufferSequence & buffers, \
                           asio::socket_base::message_flags flags, \
                           std::error_code &ec, \
                           bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers, &flags](){ \
            async_handler.start_timer_with_cancelation(receive_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_receive(buffers, flags, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename Rep, typename Period> \
    void set_receive_timeout(const std::chrono::duration<Rep, Period>& timeout_duration) \
    { receive_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); } \
    uint64_t receive_timeout_=0;
    
#define FIBIO_IMPLEMENT_FIBERIZED_SEND_TO \
    template<typename MutableBufferSequence> \
    std::size_t send_to(const MutableBufferSequence & buffers, \
                        const typename base_type::lowest_layer_type::endpoint_type & destination) \
    { \
        std::error_code ec; \
        return do_send_to(buffers, destination, ec, true); \
    } \
    template<typename ConstBufferSequence> \
    std::size_t send_to(const ConstBufferSequence & buffers, \
                        const typename base_type::lowest_layer_type::endpoint_type & destination, \
                        std::error_code &ec) \
    { return do_send_to(buffers, destination, ec, false); } \
    template<typename ConstBufferSequence> \
    std::size_t do_send_to(const ConstBufferSequence & buffers, \
                           const typename base_type::lowest_layer_type::endpoint_type & destination, \
                           std::error_code &ec, \
                           bool throw_error) \
    {\
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers, &destination](){ \
            async_handler.start_timer_with_cancelation(send_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_send_to(buffers, destination, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename ConstBufferSequence> \
    std::size_t send_to(const ConstBufferSequence & buffers, \
                        const typename base_type::lowest_layer_type::endpoint_type & destination, \
                        asio::socket_base::message_flags flags) \
    { \
        std::error_code ec; \
        return do_send_to(buffers, destination, flags, ec, true); \
    } \
    template<typename ConstBufferSequence> \
    std::size_t send_to(const ConstBufferSequence & buffers, \
                        const typename base_type::lowest_layer_type::endpoint_type & destination, \
                        asio::socket_base::message_flags flags, \
                        std::error_code &ec) \
    { return do_send_to(buffers, destination, flags, ec, false); } \
    template<typename ConstBufferSequence> \
    std::size_t do_send_to(const ConstBufferSequence & buffers, \
                           const typename base_type::lowest_layer_type::endpoint_type & destination, \
                           asio::socket_base::message_flags flags, \
                           std::error_code &ec, \
                           bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers, &destination, &flags](){ \
            async_handler.start_timer_with_cancelation(send_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_send_to(buffers, destination, flags, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename Rep, typename Period> \
    void set_send_to_timeout(const std::chrono::duration<Rep, Period>& timeout_duration) \
    { send_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); } \
    uint64_t send_to_timeout_=0;
    
#define FIBIO_IMPLEMENT_FIBERIZED_RECEIVE_FROM \
    template<typename MutableBufferSequence> \
    std::size_t receive_from(const MutableBufferSequence & buffers, \
                             typename base_type::lowest_layer_type::endpoint_type & sender_endpoint) \
    { \
        std::error_code ec; \
        return do_receive_from(buffers, sender_endpoint, ec, true); \
    } \
    template<typename MutableBufferSequence> \
    std::size_t receive_from(const MutableBufferSequence & buffers, \
                             typename base_type::lowest_layer_type::endpoint_type & sender_endpoint, \
                             std::error_code &ec) \
    { return do_receive_from(buffers, sender_endpoint, ec, false); } \
    template<typename MutableBufferSequence> \
    std::size_t do_receive_from(const MutableBufferSequence & buffers, \
                                typename base_type::lowest_layer_type::endpoint_type & sender_endpoint, \
                                std::error_code &ec, \
                                bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers, &sender_endpoint](){ \
            async_handler.start_timer_with_cancelation(receive_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_receive_from(buffers, sender_endpoint, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename MutableBufferSequence> \
    std::size_t receive_from(const MutableBufferSequence & buffers, \
                             typename base_type::lowest_layer_type::endpoint_type & sender_endpoint, \
                             asio::socket_base::message_flags flags) \
    { \
        std::error_code ec; \
        return do_receive_from(buffers, sender_endpoint, flags, ec, true); \
    } \
    template<typename MutableBufferSequence> \
    std::size_t receive_from(const MutableBufferSequence & buffers, \
                             typename base_type::lowest_layer_type::endpoint_type & sender_endpoint, \
                             asio::socket_base::message_flags flags, \
                             std::error_code &ec) \
    { return do_receive_from(buffers, sender_endpoint, flags, ec, false); } \
    template<typename MutableBufferSequence> \
    std::size_t do_receive_from(const MutableBufferSequence & buffers, \
                                typename base_type::lowest_layer_type::endpoint_type & sender_endpoint, \
                                asio::socket_base::message_flags flags, \
                                std::error_code &ec, \
                                bool throw_error) \
    { \
        detail::fiber_async_handler async_handler; \
        async_handler.run_in_scheduler_context([this, &async_handler, &buffers, &sender_endpoint, &flags](){ \
            async_handler.start_timer_with_cancelation(receive_timeout_, [this](){ this->lowest_layer().cancel(); }); \
            this->async_receive_from(buffers, sender_endpoint, flags, async_handler.get_io_handler()); \
        }); \
        async_handler.throw_or_return(throw_error, ec); \
        return async_handler.io_result(); \
    } \
    template<typename Rep, typename Period> \
    void set_receive_from_timeout(const std::chrono::duration<Rep, Period>& timeout_duration) \
    { receive_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); } \
    uint64_t receive_from_timeout_=0;

#endif
