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
#include <chrono>
#include <boost/system/error_code.hpp>

namespace fibio { namespace fibers { namespace asio {
    class yield_t {
    public:
        yield_t()
        : ec_(0)
        , timeout_(0)
        , cancelation_()
        {}

        template<class Rep, class Period>
        yield_t operator()(boost::system::error_code &ec, const std::chrono::duration<Rep,Period>& timeout_duration, std::function<void()> &&fn) const {
            yield_t tmp;
            tmp.ec_ = &ec;
            tmp.timeout_ = std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count();
            tmp.cancelation_ = std::move(fn);
            return tmp;
        }
        
        yield_t operator[](boost::system::error_code &ec) const {
            yield_t tmp;
            tmp.ec_ = &ec;
            tmp.timeout_ = 0;
            return tmp;
        }
        
        //private:
        boost::system::error_code *ec_;
        uint64_t timeout_;
        mutable std::function<void()> cancelation_;
    };
    
    const yield_t yield;
}}} // End of namespace fibio::fibers::asio

#include <fibio/fibers/asio/detail/yield.hpp>

namespace fibio { namespace asio {
    using fibers::asio::yield;
}}

#endif
