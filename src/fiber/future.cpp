//
//  future.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <fibio/fibers/exceptions.hpp>

namespace fibio { namespace fibers {
    class future_error_category : public boost::system::error_category
    {
    public:
        virtual const char* name() const BOOST_NOEXCEPT
        { return "future"; }
        
        virtual std::string message( int ev) const
        {
            switch (BOOST_SCOPED_ENUM_NATIVE(future_errc)(ev))
            {
                case future_errc::broken_promise:
                    return std::string("The associated promise has been destructed prior "
                                       "to the associated state becoming ready.");
                case future_errc::future_already_retrieved:
                    return std::string("The future has already been retrieved from "
                                       "the promise or packaged_task.");
                case future_errc::promise_already_satisfied:
                    return std::string("The state of the promise has already been set.");
                case future_errc::no_state:
                    return std::string("Operation not permitted on an object without "
                                       "an associated state.");
            }
            return std::string("unspecified future_errc value\n");
        }
    };
    
    boost::system::error_category const& future_category() BOOST_NOEXCEPT
    {
        static fibers::future_error_category cat;
        return cat;
    }
}}