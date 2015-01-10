//
//  shared_state.hpp
//  fibio
//
//  Base on Boost.Fiber at https://github.com/olk/boost-fiber
//
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef fibio_fibers_future_detail_shared_state_hpp
#define fibio_fibers_future_detail_shared_state_hpp

#include <chrono>
#include <boost/assert.hpp>
#include <boost/atomic.hpp>
#include <boost/config.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/move/move.hpp>
#include <boost/optional.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/utility.hpp>

#include <fibio/fibers/future/future_status.hpp>
#include <fibio/fibers/condition_variable.hpp>
#include <fibio/fibers/exceptions.hpp>
#include <fibio/fibers/mutex.hpp>

namespace fibio { namespace fibers {
    typedef std::chrono::steady_clock clock_type;
}}

namespace fibio { namespace fibers { namespace detail {
    template<typename Lockable>
    struct relock_guard {
        inline relock_guard(Lockable &mtx)
        : mtx_(mtx)
        { mtx_.unlock(); }
        
        inline ~relock_guard()
        { mtx_.lock(); }
        
        Lockable &mtx_;
    };

    template< typename R >
    class shared_state : public boost::noncopyable
    {
    private:
        std::atomic< std::size_t >   use_count_;
        mutable mutex                  mtx_;
        mutable condition_variable     waiters_;
        std::atomic<bool>            ready_;
        boost::optional< R >           value_;
        std::exception_ptr           except_;
        typedef std::function<void()>  external_waiter;
        std::vector<external_waiter>   ext_waiters_;
        
        void mark_ready_and_notify_()
        {
            ready_ = true;
            waiters_.notify_all();
            for(auto &w: ext_waiters_) {
                relock_guard<mutex> g(mtx_);
                w();
            }
        }
        
        void owner_destroyed_()
        {
            //TODO: set broken_exception if future was not already done
            //      notify all waiters
            if (!ready_)
                set_exception_( utility::copy_exception( broken_promise() ) );
        }
        
        void set_value_( R const& value)
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                BOOST_THROW_EXCEPTION(promise_already_satisfied() );
            value_ = value;
            mark_ready_and_notify_();
        }
        
        void set_value_( R && value)
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                BOOST_THROW_EXCEPTION(promise_already_satisfied() );
            value_ = std::move( value);
            mark_ready_and_notify_();
        }
        
        void set_exception_( std::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      done = true, notify all waiters
            if (ready_)
                BOOST_THROW_EXCEPTION(promise_already_satisfied());
            except_ = except;
            mark_ready_and_notify_();
        }
        
        const R& get_( unique_lock< mutex > & lk)
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait_() in order to wait for the result
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            wait_(lk);
            if (except_)
                std::rethrow_exception( except_);
            return value_.get();
        }
        
        void wait_( unique_lock< mutex > & lk) const
        {
            //TODO: blocks until the result becomes available
            while (!ready_)
                waiters_.wait(lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for_( unique_lock< mutex > & lk,
                                std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_for( lk, timeout_duration) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
        future_status wait_until_( unique_lock< mutex > & lk,
                                  clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_until( lk, timeout_time) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
    protected:
        virtual void deallocate_future() = 0;
        
    public:
        typedef boost::intrusive_ptr< shared_state >    ptr_t;
        
        shared_state() :
        use_count_( 0), mtx_(), ready_( false),
        value_(), except_()
        {}
        
        virtual ~shared_state() {}
        
        template<typename Fn>
        void add_external_waiter(Fn &&fn)
        {
            unique_lock< mutex > lk( mtx_);
            if (ready_) {
                relock_guard<mutex> g(mtx_);
                fn();
            } else {
                ext_waiters_.emplace_back(std::forward<Fn>(fn));
            }
        }
        
        void owner_destroyed()
        {
            //TODO: lock mutex
            //      set broken_exception if future was not already done
            //      done = true, notify all waiters
            unique_lock< mutex > lk( mtx_);
            owner_destroyed_();
        }
        
        void set_value( R const& value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            unique_lock< mutex > lk( mtx_);
            set_value_( value);
        }
        
        void set_value( R && value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      rhe operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            unique_lock< mutex > lk( mtx_);
            set_value_( std::move( value) );
        }
        
        void set_exception( std::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            unique_lock< mutex > lk( mtx_);
            set_exception_( except);
        }
        
        const R& get()
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait() in order to wait for the result
            //      the value stored in the shared state
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            //      valid() == false after a call to this method.
            //      detect the case when valid == false before the call and throw a
            //      future_error with an error condition of future_errc::no_state
            unique_lock< mutex > lk( mtx_);
            return get_( lk);
        }
        
        void wait() const
        {
            //TODO: blocks until the result becomes available
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            wait_( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_for_( lk, timeout_duration);
        }
        
        future_status wait_until( clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_until_( lk, timeout_time);
        }
        
        void reset()
        { ready_ = false; }
        
        friend inline void intrusive_ptr_add_ref( shared_state * p) BOOST_NOEXCEPT
        { ++p->use_count_; }
        
        friend inline void intrusive_ptr_release( shared_state * p)
        {
            if ( 0 == --p->use_count_)
                p->deallocate_future();
        }
    };
    
    template< typename R >
    class shared_state< R & > : public boost::noncopyable
    {
    private:
        std::atomic< std::size_t >   use_count_;
        mutable mutex           mtx_;
        mutable condition_variable       waiters_;
        std::atomic<bool>              ready_;
        R                   *   value_;
        std::exception_ptr           except_;
        typedef std::function<void()>  external_waiter;
        std::vector<external_waiter>   ext_waiters_;
        
        void mark_ready_and_notify_()
        {
            ready_ = true;
            waiters_.notify_all();
            for(auto &w: ext_waiters_) {
                relock_guard<mutex> g(mtx_);
                w();
            }
        }
        
        void owner_destroyed_()
        {
            //TODO: set broken_exception if future was not already done
            //      notify all waiters
            if (!ready_) {
                set_exception_( utility::copy_exception( broken_promise() ) );
            }
        }
        
        void set_value_( R & value)
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                BOOST_THROW_EXCEPTION(promise_already_satisfied() );
            value_ = & value;
            mark_ready_and_notify_();
        }
        
        void set_exception_( std::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      done = true, notify all waiters
            if (ready_)
                BOOST_THROW_EXCEPTION(promise_already_satisfied() );
            except_ = except;
            mark_ready_and_notify_();
        }
        
        R & get_( unique_lock< mutex > & lk)
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait_() in order to wait for the result
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            wait_(lk);
            if (except_)
                std::rethrow_exception(except_);
            return * value_;
        }
        
        void wait_( unique_lock< mutex > & lk) const
        {
            //TODO: blocks until the result becomes available
            while ( ! ready_)
                waiters_.wait( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for_( unique_lock< mutex > & lk,
                                std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_for( lk, timeout_duration) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
        future_status wait_until_( unique_lock< mutex > & lk,
                                  clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_until( lk, timeout_time) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
    protected:
        virtual void deallocate_future() = 0;
        
    public:
        typedef boost::intrusive_ptr< shared_state >    ptr_t;
        
        shared_state() :
        use_count_( 0), mtx_(), ready_( false),
        value_( 0), except_()
        {}
        
        virtual ~shared_state() {}
        
        template<typename Fn>
        void add_external_waiter(Fn &&fn)
        {
            unique_lock< mutex > lk( mtx_);
            if (ready_) {
                relock_guard<mutex> g(mtx_);
                fn();
            } else {
                ext_waiters_.emplace_back(std::forward<Fn>(fn));
            }
        }
        
        void owner_destroyed()
        {
            //TODO: lock mutex
            //      set broken_exception if future was not already done
            //      done = true, notify all waiters
            unique_lock< mutex > lk( mtx_);
            owner_destroyed_();
        }
        
        void set_value( R & value)
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            unique_lock< mutex > lk( mtx_);
            set_value_( value);
        }
        
        void set_exception( std::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            unique_lock< mutex > lk( mtx_);
            set_exception_( except);
        }
        
        R & get()
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait() in order to wait for the result
            //      the value stored in the shared state
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            //      valid() == false after a call to this method.
            //      detect the case when valid == false before the call and throw a
            //      future_error with an error condition of future_errc::no_state
            unique_lock< mutex > lk( mtx_);
            return get_( lk);
        }
        
        void wait() const
        {
            //TODO: blocks until the result becomes available
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            wait_( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_for_( lk, timeout_duration);
        }
        
        future_status wait_until( clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_until_( lk, timeout_time);
        }
        
        void reset()
        { ready_ = false; }
        
        friend inline void intrusive_ptr_add_ref( shared_state * p) BOOST_NOEXCEPT
        { ++p->use_count_; }
        
        friend inline void intrusive_ptr_release( shared_state * p)
        {
            if ( 0 == --p->use_count_)
                p->deallocate_future();
        }
    };
    
    template<>
    class shared_state< void > : public boost::noncopyable
    {
    private:
        std::atomic< std::size_t >   use_count_;
        mutable mutex           mtx_;
        mutable condition_variable       waiters_;
        std::atomic<bool>              ready_;
        std::exception_ptr           except_;
        typedef std::function<void()>  external_waiter;
        std::vector<external_waiter>   ext_waiters_;
        
        void mark_ready_and_notify_()
        {
            ready_ = true;
            waiters_.notify_all();
            for(auto &w: ext_waiters_) {
                relock_guard<mutex> g(mtx_);
                w();
            }
        }
        
        void owner_destroyed_()
        {
            //TODO: set broken_exception if future was not already done
            //      notify all waiters
            if (!ready_) {
                set_exception_(utility::copy_exception( broken_promise() ) );
            }
        }
        
        void set_value_()
        {
            //TODO: store the value and make the future ready
            //      notify all waiters
            if (ready_)
                BOOST_THROW_EXCEPTION(promise_already_satisfied() );
            mark_ready_and_notify_();
        }
        
        void set_exception_( std::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      done = true, notify all waiters
            if (ready_)
                BOOST_THROW_EXCEPTION(promise_already_satisfied() );
            except_ = except;
            mark_ready_and_notify_();
        }
        
        void get_( unique_lock< mutex > & lk)
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait_() in order to wait for the result
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            wait_(lk);
            if (except_)
                std::rethrow_exception( except_);
        }
        
        void wait_(unique_lock< mutex > & lk) const
        {
            //TODO: blocks until the result becomes available
            while ( ! ready_)
                waiters_.wait( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for_( unique_lock< mutex > & lk,
                                std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_for( lk, timeout_duration) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
        future_status wait_until_( unique_lock< mutex > & lk,
                                  clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            while ( ! ready_)
            {
                cv_status st( waiters_.wait_until( lk, timeout_time) );
                if ( cv_status::timeout == st && ! ready_)
                    return future_status::timeout;
            }
            return future_status::ready;
        }
        
    protected:
        virtual void deallocate_future() = 0;
        
    public:
        typedef boost::intrusive_ptr< shared_state >    ptr_t;
        
        shared_state() :
        use_count_( 0), mtx_(), ready_( false), except_()
        {}
        
        virtual ~shared_state() {}
        
        template<typename Fn>
        void add_external_waiter(Fn &&fn)
        {
            unique_lock< mutex > lk( mtx_);
            if (ready_) {
                relock_guard<mutex> g(mtx_);
                fn();
            } else {
                ext_waiters_.emplace_back(std::forward<Fn>(fn));
            }
        }
        
        void owner_destroyed()
        {
            //TODO: lock mutex
            //      set broken_exception if future was not already done
            //      done = true, notify all waiters
            unique_lock< mutex > lk( mtx_);
            owner_destroyed_();
        }
        
        void set_value()
        {
            //TODO: store the value into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            unique_lock< mutex > lk( mtx_);
            set_value_();
        }
        
        void set_exception( std::exception_ptr except)
        {
            //TODO: store the exception pointer p into the shared state and make the state ready
            //      the operation is atomic, i.e. it behaves as though they acquire a single mutex
            //      associated with the promise object while updating the promise object
            //      an exception is thrown if there is no shared state or the shared state already
            //      stores a value or exception
            unique_lock< mutex > lk( mtx_);
            set_exception_( except);
        }
        
        void get()
        {
            //TODO: the get method waits until the future has a valid result and
            //      (depending on which template is used) retrieves it
            //      it effectively calls wait() in order to wait for the result
            //      the value stored in the shared state
            //      if it satisfies the requirements of MoveAssignable, the value is moved,
            //      otherwise it is copied
            //      valid() == false after a call to this method.  
            //      detect the case when valid == false before the call and throw a
            //      future_error with an error condition of future_errc::no_state
            unique_lock< mutex > lk( mtx_);
            get_( lk);
        }
        
        void wait() const
        {
            //TODO: blocks until the result becomes available
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            wait_( lk);
        }
        
        template< class Rep, class Period >
        future_status wait_for( std::chrono::duration< Rep, Period > const& timeout_duration) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_for_( lk, timeout_duration);
        }
        
        future_status wait_until( clock_type::time_point const& timeout_time) const
        {
            //TODO: blocks until the result becomes available or timeout
            //      valid() == true after the call
            unique_lock< mutex > lk( mtx_);
            return wait_until_( lk, timeout_time);
        }
        
        void reset()
        { ready_ = false; }
        
        friend inline void intrusive_ptr_add_ref( shared_state * p) BOOST_NOEXCEPT
        { ++p->use_count_; }
        
        friend inline void intrusive_ptr_release( shared_state * p)
        {
            if ( 0 == --p->use_count_)
                p->deallocate_future();
        }
    };
}}} // End of namespace fibio::fibers::detail

#endif
