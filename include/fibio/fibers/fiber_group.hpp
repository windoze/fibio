//
//  fiber_group.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fiber_group_hpp
#define fibio_fiber_group_hpp

#include <list>
#include <algorithm>
#include <fibio/fibers/shared_mutex.hpp>

namespace fibio { namespace fibers {
    class fiber_group {
    private:
        fiber_group(fiber_group const&)=delete;
        fiber_group& operator=(fiber_group const&)=delete;

    public:
        fiber_group() {}
        
        ~fiber_group() {
            for(std::list<fiber*>::iterator it=fibers_.begin(), end=fibers_.end(); it!=end; ++it) {
                delete *it;
            }
        }
        
        bool is_this_fiber_in() {
            fiber::id id = this_fiber::get_id();
            shared_lock<shared_mutex> guard(m_);
            for(std::list<fiber*>::iterator it=fibers_.begin(),end=fibers_.end(); it!=end; ++it) {
                if ((*it)->get_id() == id)
                    return true;
            }
            return false;
        }
        
        bool is_fiber_in(fiber* thrd) {
            if(thrd) {
                fiber::id id = thrd->get_id();
                shared_lock<shared_mutex> guard(m_);
                for(std::list<fiber*>::iterator it=fibers_.begin(),end=fibers_.end(); it!=end; ++it) {
                    if ((*it)->get_id() == id)
                        return true;
                }
                return false;
            } else {
                return false;
            }
        }
        
        template<typename Fn, typename... Args>
        fiber* create_fiber(Fn &&fn, Args&&... args)
        {
            lock_guard<shared_mutex> guard(m_);
            std::auto_ptr<fiber> new_fiber(new fiber(std::forward<Fn>(fn), std::forward<Args>(args)...));
            fibers_.push_back(new_fiber.get());
            return new_fiber.release();
        }
        
        void add_fiber(fiber* thrd) {
            if(thrd) {
                lock_guard<shared_mutex> guard(m_);
                fibers_.push_back(thrd);
            }
        }
        
        void remove_fiber(fiber* thrd) {
            lock_guard<shared_mutex> guard(m_);
            std::list<fiber*>::iterator const it=std::find(fibers_.begin(),fibers_.end(),thrd);
            if(it!=fibers_.end()) {
                fibers_.erase(it);
            }
        }
        
        void join_all() {
            shared_lock<shared_mutex> guard(m_);
            
            for(std::list<fiber*>::iterator it=fibers_.begin(),end=fibers_.end(); it!=end; ++it) {
                if ((*it)->joinable())
                    (*it)->join();
            }
        }
        
        size_t size() const {
            shared_lock<shared_mutex> guard(m_);
            return fibers_.size();
        }
        
    private:
        std::list<fiber*> fibers_;
        mutable shared_mutex m_;
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::fiber_group;
}   // End of namespace fibio

#endif
