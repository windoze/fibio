//
//  exceptions.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-28.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_exceptions_hpp
#define fibio_fibers_exceptions_hpp

#include <stdexcept>
#include <string>
#include <boost/config.hpp>
#include <boost/type_traits.hpp>
#include <boost/detail/scoped_enum_emulation.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace fibio { namespace fibers {
    class fiber_exception : public boost::system::system_error {
    public:
        fiber_exception()
        : boost::system::system_error(0, boost::system::system_category())
        {}
        
        fiber_exception(int sys_error_code)
        : boost::system::system_error(sys_error_code, boost::system::system_category())
        {}
        
        fiber_exception(int ev, const char *what_arg)
        : boost::system::system_error(boost::system::error_code(ev, boost::system::system_category()), what_arg)
        {}
        
        fiber_exception(int ev, const std::string &what_arg)
        : boost::system::system_error(boost::system::error_code(ev, boost::system::system_category()), what_arg)
        {}
        
        ~fiber_exception() noexcept
        {}
        
        int native_error() const
        { return code().value(); }
    };
    
    class condition_error : public fiber_exception {
    public:
        condition_error()
        : fiber_exception(0, "fibio::condition_error")
        {}
        
        condition_error(int ev)
        : fiber_exception(ev, "fibio::condition_error")
        {}
        
        condition_error(int ev, const char *what_arg)
        : fiber_exception(ev, what_arg)
        {}
        
        condition_error(int ev, const std::string &what_arg)
        : fiber_exception(ev, what_arg)
        {}
    };
    
    class lock_error : public fiber_exception {
    public:
        lock_error()
        : fiber_exception(0, "fibio::lock_error")
        {}
        
        lock_error(int ev)
        : fiber_exception(ev, "fibio::lock_error")
        {}
        
        lock_error(int ev, const char *what_arg)
        : fiber_exception(ev, what_arg)
        {}
        
        lock_error(int ev, const std::string &what_arg)
        : fiber_exception(ev, what_arg)
        {}
        
        ~lock_error() noexcept
        {}
    };
    
    class fiber_resource_error : public fiber_exception {
    public:
        fiber_resource_error()
        : fiber_exception(boost::system::errc::resource_unavailable_try_again,
                          "fibio::fibers::fiber_resource_error")
        {}
        
        fiber_resource_error(int ev)
        : fiber_exception(ev, "fibio::fibers::fiber_resource_error")
        {}
        
        fiber_resource_error(int ev, const char *what_arg)
        : fiber_exception(ev, what_arg)
        {}
        
        fiber_resource_error(int ev, const std::string &what_arg)
        : fiber_exception(ev, what_arg)
        {}
        
        ~fiber_resource_error() noexcept
        {}
    };
    
    class invalid_argument : public fiber_exception {
    public:
        invalid_argument()
        : fiber_exception(boost::system::errc::invalid_argument,
                          "fibio::fibers::invalid_argument")
        {}
        
        invalid_argument(int ev)
        : fiber_exception(ev, "fibio::fibers::invalid_argument")
        {}
        
        invalid_argument(int ev, const char *what_arg)
        : fiber_exception(ev, what_arg)
        {}
        
        invalid_argument(int ev, const std::string &what_arg)
        : fiber_exception(ev, what_arg)
        {}
    };
    
    class fiber_interrupted : public fiber_exception {
    public:
        fiber_interrupted()
        : fiber_exception(boost::system::errc::interrupted, "fibio::fibers::fiber_interrupted")
        {}
    };
    
    enum class future_errc
    {
        broken_promise = 1,
        future_already_retrieved,
        promise_already_satisfied,
        no_state
    };
    
    boost::system::error_category const &future_category() noexcept;
}}  // End of namespace fibio::fibers

namespace boost { namespace system {
    template<>
    struct is_error_code_enum<fibio::fibers::future_errc> : public true_type
    {};
    
    inline error_code make_error_code(fibio::fibers::future_errc e) //noexcept
    {
        return error_code(underlying_cast<int>(e), fibio::fibers::future_category());
    }
    
    inline error_condition make_error_condition(fibio::fibers::future_errc e) //noexcept
    {
        return error_condition(underlying_cast<int>(e), fibio::fibers::future_category());
    }
}}  // End of namespace boost::system

namespace fibio { namespace fibers {
    class future_error : public std::logic_error {
    private:
        boost::system::error_code ec_;
        
    public:
        future_error(boost::system::error_code ec)
        : logic_error(ec.message())
        , ec_(ec)
        {}
        
        boost::system::error_code const &code() const noexcept
        { return ec_; }
        
        const char *what() const noexcept
        { return code().message().c_str(); }
    };
    
    class future_uninitialized : public future_error {
    public:
        future_uninitialized()
        : future_error(boost::system::make_error_code(future_errc::no_state))
        {}
    };
    
    class future_already_retrieved : public future_error {
    public:
        future_already_retrieved()
        : future_error(boost::system::make_error_code(future_errc::future_already_retrieved))
        {}
    };
    
    class broken_promise : public future_error {
    public:
        broken_promise()
        : future_error(boost::system::make_error_code(future_errc::broken_promise))
        {}
    };
    
    class promise_already_satisfied : public future_error {
    public:
        promise_already_satisfied()
        : future_error(boost::system::make_error_code(future_errc::promise_already_satisfied))
        {}
    };
    
    class promise_uninitialized : public future_error {
    public:
        promise_uninitialized()
        : future_error(boost::system::make_error_code(future_errc::no_state))
        {}
    };
    
    class packaged_task_uninitialized : public future_error {
    public:
        packaged_task_uninitialized()
        : future_error(boost::system::make_error_code(future_errc::no_state))
        {}
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::fiber_exception;
    using fibers::condition_error;
    using fibers::lock_error;
    using fibers::fiber_resource_error;
    using fibers::invalid_argument;
    using fibers::fiber_interrupted;
    using fibers::future_errc;
    using fibers::future_category;
    using fibers::future_error;
    using fibers::future_uninitialized;
    using fibers::future_already_retrieved;
    using fibers::broken_promise;
    using fibers::promise_already_satisfied;
    using fibers::promise_uninitialized;
    using fibers::packaged_task_uninitialized;
}   // End of namespace fibio

#endif
