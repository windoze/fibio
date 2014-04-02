//
//  fiber_base.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_detail_fiber_base_hpp
#define fibio_fibers_detail_fiber_base_hpp

#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>

namespace fibio { namespace fibers { namespace detail {
    struct fiber_base {
        typedef std::shared_ptr<fiber_base> ptr_t;
        virtual ~fiber_base(){};
        virtual void pause(ptr_t switch_to=ptr_t())=0;
        virtual void activate()=0;
        virtual boost::asio::strand &get_fiber_strand()=0;
        inline boost::asio::io_service &get_io_service() {
            return get_fiber_strand().get_io_service();
        }
    };
    
    fiber_base::ptr_t get_current_fiber_ptr();
}}}

#endif
