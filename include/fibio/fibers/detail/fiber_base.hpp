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

namespace fibio { namespace fibers { namespace detail {
    struct fiber_base {
        typedef std::shared_ptr<fiber_base> ptr_t;
        virtual ~fiber_base(){};
        virtual void pause()=0;
        virtual void activate()=0;
    };
    
    fiber_base::ptr_t get_current_fiber_ptr();
}}}

#endif
