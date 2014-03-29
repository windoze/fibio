//
//  shared_mutex.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-20. Copied and modified from Boost.Thread
//
//  (C) Copyright 2006-8 Anthony Williams
//  (C) Copyright 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef fibio_shared_mutex_hpp
#define fibio_shared_mutex_hpp

#include <fibio/fibers/exceptions.hpp>
#include <fibio/fibers/mutex.hpp>
#include <fibio/fibers/condition_variable.hpp>

namespace fibio { namespace fibers {
    class shared_mutex {
    private:
        class state_data {
        public:
            state_data () :
            shared_count(0),
            exclusive(false),
            upgrade(false),
            exclusive_waiting_blocked(false)
            {}
            
            void assert_free() const {
                assert( ! exclusive );
                assert( ! upgrade );
                assert( shared_count==0 );
            }
            
            void assert_locked() const {
                assert( exclusive );
                assert( shared_count==0 );
                assert( ! upgrade );
            }
            
            void assert_lock_shared () const {
                assert( ! exclusive );
                assert( shared_count>0 );
                //BOOST_ASSERT( (! upgrade) || (shared_count>1));
                // if upgraded there are at least 2 threads sharing the mutex,
                // except when unlock_upgrade_and_lock has decreased the number of readers but has not taken yet exclusive ownership.
            }
            
            void assert_lock_upgraded () const {
                assert( ! exclusive );
                assert(  upgrade );
                assert(  shared_count>0 );
            }
            
            void assert_lock_not_upgraded () const {
                assert(  ! upgrade );
            }
            
            bool can_lock () const {
                return ! (shared_count || exclusive);
            }
            
            void exclusive_blocked (bool blocked) {
                exclusive_waiting_blocked = blocked;
            }
            
            void lock () {
                exclusive = true;
            }
            
            void unlock () {
                exclusive = false;
                exclusive_waiting_blocked = false;
            }
            
            bool can_lock_shared () const {
                return ! (exclusive || exclusive_waiting_blocked);
            }
            
            bool more_shared () const {
                return shared_count > 0 ;
            }
            
            unsigned get_shared_count () const {
                return shared_count ;
            }
            
            unsigned  lock_shared () {
                return ++shared_count;
            }
            
            
            void unlock_shared () {
                --shared_count;
            }
            
            bool unlock_shared_downgrades() {
                if (upgrade) {
                    upgrade=false;
                    exclusive=true;
                    return true;
                } else {
                    exclusive_waiting_blocked=false;
                    return false;
                }
            }
            
            void lock_upgrade () {
                ++shared_count;
                upgrade=true;
            }
            
            bool can_lock_upgrade () const {
                return ! (exclusive || exclusive_waiting_blocked || upgrade);
            }
            
            void unlock_upgrade () {
                upgrade=false;
                --shared_count;
            }
            
            //private:
            unsigned shared_count;
            bool exclusive;
            bool upgrade;
            bool exclusive_waiting_blocked;
        };
        
        state_data state;
        mutex state_change;
        condition_variable shared_cond;
        condition_variable exclusive_cond;
        condition_variable upgrade_cond;
        
        void release_waiters() {
            exclusive_cond.notify_one();
            shared_cond.notify_all();
        }
        
    public:
        shared_mutex(const shared_mutex &)=delete;
        shared_mutex(){}
        ~shared_mutex(){}
        
        void lock_shared() {
            unique_lock<mutex> lk(state_change);
            while(!state.can_lock_shared()) {
                shared_cond.wait(lk);
            }
            state.lock_shared();
        }
        
        bool try_lock_shared() {
            unique_lock<mutex> lk(state_change);
            if(!state.can_lock_shared()) {
                return false;
            }
            state.lock_shared();
            return true;
        }
        
        template <class Rep, class Period>
        bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& rel_time) {
            return try_lock_shared_until(std::chrono::steady_clock::now() + rel_time);
        }
        
        template <class Clock, class Duration>
        bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            unique_lock<mutex> lk(state_change);
            while(!state.can_lock_shared()) {
                if(cv_status::timeout==shared_cond.wait_until(lk,abs_time)) {
                    return false;
                }
            }
            state.lock_shared();
            return true;
        }

        void unlock_shared() {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_shared();
            state.unlock_shared();
            if (! state.more_shared()) {
                if (state.upgrade) {
                    // As there is a thread doing a unlock_upgrade_and_lock that is waiting for ! state.more_shared()
                    // avoid other threads to lock, lock_upgrade or lock_shared, so only this thread is notified.
                    state.upgrade=false;
                    state.exclusive=true;
                    lk.unlock();
                    upgrade_cond.notify_one();
                } else {
                    state.exclusive_waiting_blocked=false;
                    lk.unlock();
                }
                release_waiters();
            }
        }
        
        void lock() {
            unique_lock<mutex> lk(state_change);
            while (state.shared_count || state.exclusive) {
                state.exclusive_waiting_blocked=true;
                exclusive_cond.wait(lk);
            }
            state.exclusive=true;
        }
        
        template <class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time) {
            return try_lock_until(std::chrono::steady_clock::now() + rel_time);
        }
        
        template <class Clock, class Duration>
        bool try_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            unique_lock<mutex> lk(state_change);
            while(state.shared_count || state.exclusive) {
                state.exclusive_waiting_blocked=true;
                if(cv_status::timeout == exclusive_cond.wait_until(lk,abs_time)) {
                    if(state.shared_count || state.exclusive) {
                        state.exclusive_waiting_blocked=false;
                        release_waiters();
                        return false;
                    }
                    break;
                }
            }
            state.exclusive=true;
            return true;
        }
        
        bool try_lock() {
            unique_lock<mutex> lk(state_change);
            if(state.shared_count || state.exclusive) {
                return false;
            } else {
                state.exclusive=true;
                return true;
            }
            
        }
        
        void unlock() {
            unique_lock<mutex> lk(state_change);
            state.assert_locked();
            state.exclusive=false;
            state.exclusive_waiting_blocked=false;
            state.assert_free();
            release_waiters();
        }
        
        void lock_upgrade() {
            unique_lock<mutex> lk(state_change);
            while(state.exclusive || state.exclusive_waiting_blocked || state.upgrade) {
                shared_cond.wait(lk);
            }
            state.lock_shared();
            state.upgrade=true;
        }
        
        template <class Rep, class Period>
        bool try_lock_upgrade_for(const std::chrono::duration<Rep, Period>& rel_time) {
            return try_lock_upgrade_until(std::chrono::steady_clock::now() + rel_time);
        }
        
        template <class Clock, class Duration>
        bool try_lock_upgrade_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            unique_lock<mutex> lk(state_change);
            while(state.exclusive || state.exclusive_waiting_blocked || state.upgrade) {
                if(cv_status::timeout == shared_cond.wait_until(lk,abs_time)) {
                    if(state.exclusive || state.exclusive_waiting_blocked || state.upgrade) {
                        return false;
                    }
                    break;
                }
            }
            state.lock_shared();
            state.upgrade=true;
            return true;
        }

        bool try_lock_upgrade() {
            unique_lock<mutex> lk(state_change);
            if(state.exclusive || state.exclusive_waiting_blocked || state.upgrade) {
                return false;
            } else {
                state.lock_shared();
                state.upgrade=true;
                state.assert_lock_upgraded();
                return true;
            }
        }
        
        void unlock_upgrade() {
            unique_lock<mutex> lk(state_change);
            //state.upgrade=false;
            state.unlock_upgrade();
            if(! state.more_shared() ) {
                state.exclusive_waiting_blocked=false;
                release_waiters();
            } else {
                shared_cond.notify_all();
            }
        }
        
        // Upgrade <-> Exclusive
        void unlock_upgrade_and_lock() {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_upgraded();
            state.unlock_shared();
            while (state.more_shared()) {
                upgrade_cond.wait(lk);
            }
            state.upgrade=false;
            state.exclusive=true;
            state.assert_locked();
        }
        
        void unlock_and_lock_upgrade() {
            unique_lock<mutex> lk(state_change);
            state.assert_locked();
            state.exclusive=false;
            state.upgrade=true;
            state.lock_shared();
            state.exclusive_waiting_blocked=false;
            state.assert_lock_upgraded();
            release_waiters();
        }
        
        bool try_unlock_upgrade_and_lock() {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_upgraded();
            if(   !state.exclusive
               && !state.exclusive_waiting_blocked
               && state.upgrade
               && state.shared_count==1)
            {
                state.shared_count=0;
                state.exclusive=true;
                state.upgrade=false;
                state.assert_locked();
                return true;
            }
            return false;
        }

        template <class Rep, class Period>
        bool try_unlock_upgrade_and_lock_for(const std::chrono::duration<Rep, Period>& rel_time) {
            return try_unlock_upgrade_and_lock_until(std::chrono::steady_clock::now() + rel_time);
        }
        
        template <class Clock, class Duration>
        bool try_unlock_upgrade_and_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_upgraded();
            if (state.shared_count != 1) {
                for (;;) {
                    cv_status status = shared_cond.wait_until(lk,abs_time);
                    if (state.shared_count == 1)
                        break;
                    if(status == cv_status::timeout)
                        return false;
                }
            }
            state.upgrade=false;
            state.exclusive=true;
            state.exclusive_waiting_blocked=false;
            state.shared_count=0;
            return true;
        }
        
        // Shared <-> Exclusive
        void unlock_and_lock_shared() {
            unique_lock<mutex> lk(state_change);
            state.assert_locked();
            state.exclusive=false;
            state.lock_shared();
            state.exclusive_waiting_blocked=false;
            release_waiters();
        }
        
        bool try_unlock_shared_and_lock() {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_shared();
            if(   !state.exclusive
               && !state.exclusive_waiting_blocked
               && !state.upgrade
               && state.shared_count==1)
            {
                state.shared_count=0;
                state.exclusive=true;
                return true;
            }
            return false;
        }

        template <class Rep, class Period>
        bool try_unlock_shared_and_lock_for(const std::chrono::duration<Rep, Period>& rel_time) {
            return try_unlock_shared_and_lock_until(std::chrono::steady_clock::now() + rel_time);
        }
        
        template <class Clock, class Duration>
        bool try_unlock_shared_and_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_shared();
            if (state.shared_count != 1) {
                for (;;) {
                    cv_status status = shared_cond.wait_until(lk,abs_time);
                    if (state.shared_count == 1)
                        break;
                    if(status == cv_status::timeout)
                        return false;
                }
            }
            state.upgrade=false;
            state.exclusive=true;
            state.exclusive_waiting_blocked=false;
            state.shared_count=0;
            return true;
        }
        
        // Shared <-> Upgrade
        void unlock_upgrade_and_lock_shared() {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_upgraded();
            state.upgrade=false;
            state.exclusive_waiting_blocked=false;
            release_waiters();
        }
        
        bool try_unlock_shared_and_lock_upgrade() {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_shared();
            if(   !state.exclusive
               && !state.exclusive_waiting_blocked
               && !state.upgrade
               )
            {
                state.upgrade=true;
                return true;
            }
            return false;
        }

        template <class Rep, class Period>
        bool try_unlock_shared_and_lock_upgrade_for(const std::chrono::duration<Rep, Period>& rel_time) {
            return try_unlock_shared_and_lock_upgrade_until(std::chrono::steady_clock::now() + rel_time);
        }
        
        template <class Clock, class Duration>
        bool try_unlock_shared_and_lock_upgrade_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            unique_lock<mutex> lk(state_change);
            state.assert_lock_shared();
            if(   state.exclusive
               || state.exclusive_waiting_blocked
               || state.upgrade
               )
            {
                for (;;) {
                    cv_status status = exclusive_cond.wait_until(lk,abs_time);
                    if(   ! state.exclusive
                       && ! state.exclusive_waiting_blocked
                       && ! state.upgrade
                       )
                        break;
                    if(status == cv_status::timeout)
                        return false;
                }
            }
            state.upgrade=true;
            return true;
        }
    };
    
    template<typename Mutex>
    class shared_lock {
    protected:
        Mutex* m;
        bool is_locked;
        
    public:
        typedef Mutex mutex_type;
        shared_lock(const shared_lock &)=delete;
        
        shared_lock() noexcept
        : m(0)
        , is_locked(false)
        {}
        
        explicit shared_lock(Mutex& m_)
        : m(&m_)
        , is_locked(false)
        {
            lock();
        }
        
        shared_lock(Mutex& m_, std::adopt_lock_t)
        : m(&m_)
        , is_locked(true)
        {}
        
        shared_lock(Mutex& m_, std::defer_lock_t) noexcept
        : m(&m_)
        , is_locked(false)
        {}
        
        shared_lock(Mutex& m_, std::try_to_lock_t)
        : m(&m_)
        , is_locked(false)
        {
            try_lock();
        }

        template <class Clock, class Duration>
        shared_lock(Mutex& mtx, const std::chrono::time_point<Clock, Duration>& t)
        : m(&mtx)
        , is_locked(mtx.try_lock_shared_until(t))
        {}
        
        template <class Rep, class Period>
        shared_lock(Mutex& mtx, const std::chrono::duration<Rep, Period>& d)
        : m(&mtx)
        , is_locked(mtx.try_lock_shared_for(d))
        {}
        
        shared_lock(shared_lock<Mutex> &&other) noexcept
        : m(other.m), is_locked(other.is_locked)
        {
            other.is_locked=false;
            other.m=0;
        }
        
        //std-2104 unique_lock move-assignment should not be noexcept
        shared_lock& operator=(shared_lock<Mutex> &&other) {
            shared_lock temp(::std::move(other));
            swap(temp);
            return *this;
        }
        
        void swap(shared_lock& other) noexcept {
            std::swap(m,other.m);
            std::swap(is_locked,other.is_locked);
        }
        
        Mutex* mutex() const noexcept {
            return m;
        }
        
        Mutex* release() noexcept {
            Mutex* const res=m;
            m=0;
            is_locked=false;
            return res;
        }
        
        ~shared_lock() {
            if(owns_lock()) {
                m->unlock_shared();
            }
        }
        
        void lock() {
            if(m==0) {
                throw fiber_exception(boost::system::errc::operation_not_permitted, "shared_lock has no mutex");
            }
            if(owns_lock()) {
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur, "shared_lock owns already the mutex");
            }
            m->lock_shared();
            is_locked=true;
        }
        
        bool try_lock() {
            if(m==0) {
                throw fiber_exception(boost::system::errc::operation_not_permitted, "shared_lock has no mutex");
            }
            if(owns_lock()) {
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur, "shared_lock owns already the mutex");
            }
            is_locked=m->try_lock_shared();
            return is_locked;
        }

        template <class Rep, class Period>
        bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time) {
            if(m==0) {
                throw fiber_exception(boost::system::errc::operation_not_permitted, "shared_lock has no mutex");
            }
            if(owns_lock()) {
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur, "shared_lock owns already the mutex");
            }
            is_locked=m->try_lock_shared_for(rel_time);
            return is_locked;
        }
        
        template <class Clock, class Duration>
        bool try_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
            if(m==0) {
                throw fiber_exception(boost::system::errc::operation_not_permitted, "shared_lock has no mutex");
            }
            if(owns_lock()) {
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur, "shared_lock owns already the mutex");
            }
            is_locked=m->try_lock_shared_until(abs_time);
            return is_locked;
        }

        void unlock() {
            if(m==0) {
                throw fiber_exception(boost::system::errc::operation_not_permitted, "shared_lock has no mutex");
            }
            if(!owns_lock()) {
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur, "shared_lock doesn't own the mutex");
            }
            m->unlock_shared();
            is_locked=false;
        }
        
        explicit operator bool() const noexcept {
            return owns_lock();
        }

        bool owns_lock() const noexcept {
            return is_locked;
        }
    };
}}  // End of namespace fibio::fibers

namespace std {
    template<typename Mutex>
    void swap(fibio::fibers::shared_lock<Mutex>& lhs, fibio::fibers::shared_lock<Mutex>& rhs) noexcept {
        lhs.swap(rhs);
    }
}

namespace fibio {
    using fibers::shared_mutex;
    using fibers::shared_lock;
}   // End of namespace fibio

#endif
