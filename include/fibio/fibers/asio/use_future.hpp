//
//  use_future.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-31.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_asio_use_future_hpp
#define fibio_asio_use_future_hpp

#include <memory>

#include <boost/config.hpp>
#include <boost/asio/detail/config.hpp>

namespace fibio { namespace fibers { namespace asio {
    template<typename Allocator = std::allocator<void>>
    class use_future_t
    {
    public:
        typedef Allocator allocator_type;
        
        /// Construct using default-constructed allocator.
        BOOST_CONSTEXPR use_future_t()
        {}
        
        /// Construct using specified allocator.
        explicit use_future_t(Allocator const& allocator)
        : allocator_(allocator)
        {}
        
        /// Specify an alternate allocator.
        template<typename OtherAllocator>
        use_future_t<OtherAllocator> operator[](OtherAllocator const& allocator) const
        { return use_future_t<OtherAllocator>(allocator); }
        
        /// Obtain allocator.
        allocator_type get_allocator() const
        { return allocator_; }
        
    private:
        Allocator allocator_;
    };
    
    BOOST_CONSTEXPR_OR_CONST use_future_t<> use_future;
}}} // End of namespace fibio::fibers::asio

namespace fibio { namespace asio {
    using fibers::asio::use_future;
}}

#include <fibio/fibers/asio/detail/use_future.hpp>

#endif
