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
#include <functional>
#include <boost/system/error_code.hpp>

namespace fibio { namespace fibers { namespace asio {
    /**
     * class yield_t
     *
     * Instance of the class can be used as completion handler for Boost.ASIO async_* call,
     * makes calls block current fiber until completion.
     */
    class yield_t {
    public:
        /// constructor
        constexpr yield_t()
        : ec_(0)
        {}

        /**
         * Return a yield context that sets the specified error_code.
         *
         * @param ec a reference of error_code to store returned errors
         */
        yield_t operator[](boost::system::error_code &ec) const {
            yield_t tmp;
            tmp.ec_ = &ec;
            return tmp;
        }
        
        //private:
        boost::system::error_code *ec_;
    };
    
    /// predefined instance of yield_t can be used directly
    constexpr yield_t yield;
}}} // End of namespace fibio::fibers::asio

namespace fibio { namespace asio {
    using fibers::asio::yield;
}}

#include <fibio/fibers/asio/detail/yield.hpp>

#endif
