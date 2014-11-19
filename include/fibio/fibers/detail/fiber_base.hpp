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
    /// struct fiber_base
    /**
     * can be used to comminicate with foreign threads
     */
    struct scheduler_object;
    struct fiber_base {
        typedef std::shared_ptr<fiber_base> ptr_t;
        
        /// destructor
        virtual ~fiber_base(){};
        
        /// Pause the fiber
        /**
         * NOTE: Must be called in a fiber, also you need to make sure the fiber will be resumed later
         */
        virtual void pause()=0;

        /// Resume the fiber
        /**
         * NOTE: Must be called in the context not in this fiber, the function will return immediately
         */
        virtual void resume()=0;
        
        /// Activate the fiber
        /**
         * NOTE: Must be called in the thread that runs the io_service loop, but not in any fiber,
         * i.e. in an asynchronous completion handle
         */
        virtual void activate()=0;
        
        /**
         * Returns the strand object associated with the fiber
         */
        virtual boost::asio::strand &get_fiber_strand()=0;

        /**
         * Returns the io_service object associated with the fiber
         */
        inline boost::asio::io_service &get_io_service() {
            return get_fiber_strand().get_io_service();
        }
        
        /**
         * Returns the scheduler the fiber was created
         */
        virtual std::shared_ptr<scheduler_object> get_scheduler()=0;
    };
    
    /**
     * Returns a shared pointer to currently running fiber
     */
    fiber_base::ptr_t get_current_fiber_ptr();
}}}

#endif
