//
//  resolver.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_resolver_hpp
#define fibio_resolver_hpp

#include <asio/ip/basic_resolver.hpp>
#include <fibio/io/detail/wrapper_base.hpp>

namespace fibio { namespace io {
    template<typename InternetProtocol, typename ResolverService>
    struct fiberized<asio::ip::basic_resolver<InternetProtocol, ResolverService>> : public asio::ip::basic_resolver<InternetProtocol, ResolverService>
    {
        typedef asio::ip::basic_resolver<InternetProtocol, ResolverService> base_type;
        using base_type::base_type;
        
        fiberized() : base_type(fibers::this_fiber::detail::get_io_service()) {}
        
        typename base_type::iterator resolve(const typename base_type::query & q)
        {
            std::error_code ec;
            return do_resolve(q, ec, true);
        }
        
        typename base_type::iterator resolve(const typename base_type::query & q,
                                             std::error_code & ec)
        {
            return do_resolve(q, ec, false);
        }
        
        typename base_type::iterator do_resolve(const typename base_type::query & q,
                                                std::error_code & ec,
                                                bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.run_in_scheduler_context([this, &async_handler, &q](){
                async_handler.start_timer_with_cancelation(resolve_timeout_, [this, &q](){ base_type::cancel(); });
                base_type::async_resolve(q, async_handler.get_resolve_handler((typename base_type::protocol_type*)(0)));
            });
            
            async_handler.throw_or_return(throw_error, ec);
            return async_handler.resolve_result((typename base_type::protocol_type*)(0));
        }
        
        typename base_type::iterator resolve(const typename base_type::endpoint_type & e) {
            std::error_code ec;
            return do_resolve(e, ec, true);
        }
        
        typename base_type::iterator resolve(const typename base_type::endpoint_type & e,
                                             std::error_code & ec)
        {
            return do_resolve(e, ec, false);
        }
        
        typename base_type::iterator do_resolve(const typename base_type::endpoint_type & e,
                                                std::error_code & ec,
                                                bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.run_in_scheduler_context([this, &async_handler, &e](){
                async_handler.start_timer_with_cancelation(resolve_timeout_, [this, &e](){ base_type::cancel(); });
                base_type::async_resolve(e, async_handler.get_resolve_handler((typename base_type::protocol_type*)(0)));
            });
            
            async_handler.throw_or_return(throw_error, ec);
            return async_handler.resolve_result((typename base_type::protocol_type*)(0));
        }
        
        template<typename Rep, typename Period>
        void set_resolve_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { resolve_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        uint64_t resolve_timeout_=0;
    };
}}  // End of namespace fibio::io

#endif
