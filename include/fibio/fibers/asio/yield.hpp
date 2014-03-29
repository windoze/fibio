//
//  yield.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_asio_yield_hpp
#define fibio_fibers_asio_yield_hpp

#include <memory>
#include <boost/system/error_code.hpp>

namespace fibio { namespace fibers { namespace asio {
    class yield_t {
    public:
        constexpr yield_t() : ec_(0) {}
        
        yield_t operator[](boost::system::error_code &ec) const {
            yield_t tmp;
            tmp.ec_ = &ec;
            return tmp;
        }
        
        //private:
        boost::system::error_code *ec_;
    };
    
    constexpr yield_t yield;
}}} // End of namespace fibio::fibers::asio

#include <fibio/fibers/asio/detail/yield.hpp>

namespace fibio { namespace asio {
    using fibers::asio::yield;
}}

#endif
