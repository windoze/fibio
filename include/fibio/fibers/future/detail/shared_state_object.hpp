//
//  shared_state_object.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_future_detail_shared_state_object_hpp
#define fibio_fibers_future_detail_shared_state_object_hpp

#include <boost/config.hpp>
#include <fibio/fibers/future/detail/shared_state.hpp>

namespace fibio { namespace fibers { namespace detail {
    template< typename R, typename Allocator >
    class shared_state_object : public shared_state< R >
    {
    public:
        typedef typename Allocator::template rebind<
        shared_state_object< R, Allocator >
        >::other                                      allocator_t;
        
        shared_state_object( allocator_t const& alloc) :
        shared_state< R >(), alloc_( alloc)
        {}
        
    protected:
        void deallocate_future()
        { destroy_( alloc_, this); }
        
    private:
        allocator_t             alloc_;
        
        static void destroy_( allocator_t & alloc, shared_state_object * p)
        {
            alloc.destroy( p);
            alloc.deallocate( p, 1);
        }
    };
}}} // End of namespace fibio::fibers::detail

#endif
