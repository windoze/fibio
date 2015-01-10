//
//  fiber_group.hpp
//  fibio
//
//  Base on Boost.Thread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-9 Anthony Williams
//

#ifndef fibio_fiber_group_hpp
#define fibio_fiber_group_hpp

#include <list>
#include <algorithm>
#include <fibio/fibers/fiber.hpp>
#include <fibio/fibers/shared_mutex.hpp>

namespace fibio { namespace fibers {
    /// fiber_group
    class fiber_group {
    private:
        /// fiber_group is non-copyable
        fiber_group(fiber_group const&)=delete;
        fiber_group& operator=(fiber_group const&)=delete;

    public:
        /// constructor
        fiber_group() {}
        
        /// destructor
        ~fiber_group() {
            join_all();
            for(std::list<fiber*>::iterator it=fibers_.begin(), end=fibers_.end(); it!=end; ++it) {
                delete *it;
            }
        }
        
        /**
         * check if the current fiber is in the fiber group
         */
        bool is_this_fiber_in() {
            fiber::id id = this_fiber::get_id();
            shared_lock<shared_timed_mutex> guard(m_);
            for(std::list<fiber*>::iterator it=fibers_.begin(),end=fibers_.end(); it!=end; ++it) {
                if ((*it)->get_id() == id)
                    return true;
            }
            return false;
        }
        
        /**
         * check if the given fiber is in the fiber group
         */
        bool is_fiber_in(fiber* thrd) {
            if(thrd) {
                fiber::id id = thrd->get_id();
                shared_lock<shared_timed_mutex> guard(m_);
                for(std::list<fiber*>::iterator it=fibers_.begin(),end=fibers_.end(); it!=end; ++it) {
                    if ((*it)->get_id() == id)
                        return true;
                }
                return false;
            } else {
                return false;
            }
        }
        
        /**
         * create a new fiber in the fiber group
         */
        template<typename Fn, typename... Args>
        fiber* create_fiber(Fn &&fn, Args&&... args)
        {
            lock_guard<shared_timed_mutex> guard(m_);
            std::unique_ptr<fiber> new_fiber(new fiber(std::forward<Fn>(fn), std::forward<Args>(args)...));
            fibers_.push_back(new_fiber.get());
            return new_fiber.release();
        }
        
        /**
         * add an existing fiber into the fiber group
         */
        void add_fiber(fiber* thrd) {
            if(thrd) {
                lock_guard<shared_timed_mutex> guard(m_);
                fibers_.push_back(thrd);
            }
        }
        
        /**
         * remove a fiber from the fiber group
         */
        void remove_fiber(fiber* thrd) {
            lock_guard<shared_timed_mutex> guard(m_);
            std::list<fiber*>::iterator const it=std::find(fibers_.begin(),fibers_.end(),thrd);
            if(it!=fibers_.end()) {
                fibers_.erase(it);
            }
        }
        
        /**
         * wait until all fibers exit
         */
        void join_all() {
            shared_lock<shared_timed_mutex> guard(m_);
            
            for(std::list<fiber*>::iterator it=fibers_.begin(),end=fibers_.end(); it!=end; ++it) {
                if ((*it)->joinable())
                    (*it)->join();
            }
        }
        
        /**
         * returns the number of fibers in the group
         */
        size_t size() const {
            shared_lock<shared_timed_mutex> guard(m_);
            return fibers_.size();
        }
        
    private:
        std::list<fiber*> fibers_;
        mutable shared_timed_mutex m_;
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::fiber_group;
}   // End of namespace fibio

#endif
