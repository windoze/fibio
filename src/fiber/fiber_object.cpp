//
//  fibers.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-1.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <memory>
#include <algorithm>
#include <system_error>
#include <fibio/fibers/fiber.hpp>
#include <fibio/fibers/fss.hpp>
#include <fibio/fibers/mutex.hpp>
#include <fibio/fibers/condition_variable.hpp>
#include <fibio/stream/iostream.hpp>
#include <fibio/fibers/fiberize.hpp>

#include "fiber_object.hpp"
#include "scheduler_object.hpp"

namespace fibio { namespace fibers { namespace detail {
    __thread fiber_object *fiber_object::current_fiber_=0;
    
    fiber_object::fiber_object(scheduler_ptr_t sched, entry_t &&entry)
    : sched_(sched)
    , io_service_(sched->io_service_)
    , fiber_strand_(io_service_)
    , state_(READY)
    , entry_(std::move(entry))
    , runner_(([this](caller_t &c){ runner_wrapper(c); }))
    , caller_(0)
    {
        // std::cout << "Create" << reinterpret_cast<unsigned long>(this) << std::endl;
    }
    
    fiber_object::~fiber_object()
    {
        //printf("Destroy fiber 0x%lx \n", reinterpret_cast<unsigned long>(this));
    }
    
    void fiber_object::runner_wrapper(caller_t &c) {
        // Need this to complete constructor without running entry_
        c([this](){
            state_=READY;
        });
        
        // Now we're out of constructor
        caller_=&c;
        entry_();
        // Fiber function exits, set state to STOPPED
        c([this](){
            state_=STOPPED;
        });
        caller_=0;
    }
    
    void fiber_object::schedule() {
        //std::lock_guard<std::mutex> lock(fiber_mutex_);
        state_=READY;
        fiber_ptr_t pthis(shared_from_this());
        fiber_strand_.post([pthis](){
            pthis->one_step();
        });
    }
    
    void fiber_object::one_step() {
        struct tls_guard {
            tls_guard(fiber_object *pthis) {
                fiber_object::current_fiber_=pthis;
            }
            
            ~tls_guard() {
                fiber_object::current_fiber_=0;
            }
        };
        //assert(state_!=BLOCKED);
        if (state_==READY) {
            state_=RUNNING;
        }
        while (state_==RUNNING) {
            tls_guard guard(this);
            if (runner_) {
                after_step_handler_t h=runner_().get();
                // Change state to READY, schedule for next round unless after_step_handler explicitly change it back
                state_=READY;
                h();
            } else {
                // ERROR: Should not happen
                state_=STOPPED;
            }
            // Keep running if the state remains RUNNING
        }
        state_t s= state_;
        if (s==READY) {
            // Post this fiber to the scheduler
            schedule();
        } else if (s==BLOCKED) {
            // Do nothing
            // Must make sure this fiber will be posted elsewhere later, otherwise it will hold forever
        } else if (s==STOPPED) {
            // Fiber ended, clean up join queue
            std::lock_guard<std::mutex> lock(fiber_mutex_);
            for (std::function<void()> f: cleanup_queue_) {
                f();
            }
            cleanup_queue_.clear();
            scheduler_ptr_t s=sched_;
            io_service_.post([s](){
                s->on_fiber_exit();
            });
        }
    }
    
    void fiber_object::throw_on_error() {
        if (last_error_) {
            std::error_code ec=last_error_;
            last_error_.clear();
            throw std::system_error(ec);
        }
    }
    
    // Following functions can only be called inside coroutine
    void fiber_object::yield() {
        CHECK_CALLER(this);
        if (caller_) {
            fiber_ptr_t pthis(shared_from_this());
            (*caller_)([pthis](){
                pthis->state_=READY;
            });
        } else {
            // TODO: Error
        }
        
        throw_on_error();
    }
    
    void fiber_object::join(fiber_ptr_t f) {
        CHECK_CALLER(this);
        if (caller_) {
            fiber_ptr_t pthis(shared_from_this());
            (*caller_)([pthis, f](){
                std::lock_guard<std::mutex> lock(f->fiber_mutex_);
                if (f==pthis) {
                    // The fiber is joining itself
                    pthis->last_error_=std::make_error_code(std::errc::resource_deadlock_would_occur) ;
                } else if (f->state_==STOPPED) {
                    // f is already stopped
                    pthis->state_=RUNNING;
                } else {
                    // std::cout << "fiber(pthis) blocked" << std::endl;
                    pthis->state_=BLOCKED;
                    f->cleanup_queue_.push_back([pthis](){
                        // std::cout << "fiber(pthis) resumed" << std::endl;
                        pthis->state_=READY;
                        pthis->one_step();
                    });
                }
            });
        } else {
            // TODO: Error
        }
        
        throw_on_error();
    }
    
    void fiber_object::sleep_usec(uint64_t usec) {
        // Shortcut
        if (usec==0) {
            return;
        }
        CHECK_CALLER(this);
        if (caller_) {
            fiber_ptr_t pthis(shared_from_this());
            (*caller_)([pthis, usec](){
                pthis->state_=BLOCKED;
                timer_ptr_t sleep_timer(std::make_shared<timer_t>(pthis->io_service_));
                sleep_timer->expires_from_now(std::chrono::microseconds(usec));
                sleep_timer->async_wait(pthis->fiber_strand_.wrap([pthis, sleep_timer](std::error_code ec){
                    if (ec) {
                        pthis->last_error_=ec;
                    }
                    pthis->state_=RUNNING;
                    pthis->one_step();
                }));
            });
        } else {
            // TODO: Error
        }
        
        throw_on_error();
    }
    
    void fiber_object::add_cleanup_function(std::function<void()> &&f) {
        std::lock_guard<std::mutex> lock(fiber_mutex_);
        cleanup_queue_.push_back(std::move(f));
    }
    
    void set_fss_data(void const* key,std::shared_ptr<fss_cleanup_function> func,void* fss_data,bool cleanup_existing) {
        if (fiber_object::current_fiber_) {
            if (!func && !fss_data) {
                // Remove fss if both func and data are NULL
                fss_map_t::iterator i=fiber_object::current_fiber_->fss_.find(key);
                if (i!=fiber_object::current_fiber_->fss_.end() ) {
                    // Clean up existing if it has a cleanup func
                    if(i->second.first)
                        (*(i->second.first.get()))(i->second.second);
                    fiber_object::current_fiber_->fss_.erase(i);
                }
            } else {
                // Clean existing if needed
                if (cleanup_existing) {
                    fss_map_t::iterator i=fiber_object::current_fiber_->fss_.find(key);
                    if (i!=fiber_object::current_fiber_->fss_.end() && (i->second.first)) {
                        // Clean up existing if it has a cleanup func
                        (*(i->second.first.get()))(i->second.second);
                    }
                }
                // Insert/update the key
                fiber_object::current_fiber_->fss_[key]={func, fss_data};
            }
        }
    }
    
    void* get_fss_data(void const* key) {
        if (fiber_object::current_fiber_) {
            fss_map_t::iterator i=fiber_object::current_fiber_->fss_.find(key);
            if (i!=fiber_object::current_fiber_->fss_.end()) {
                return i->second.second;
            } else {
                // Create if not exist
                //fiber_object::current_fiber_->fss_.insert({key, {std::shared_ptr<fss_cleanup_function>(), 0}});
            }
        }
        return 0;
    }
}}}   // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers {
    fiber::fiber(std::function<void()> &&f)
    : m_(scheduler::get_instance().m_->make_fiber(std::move(f)))
    {}
    
    fiber::fiber(fiber &&other)
    : m_(std::move(other.m_))
    {}
    
    fiber::fiber(scheduler &s, std::function<void()> &&f)
    : m_(s.m_->make_fiber(std::move(f)))
    {}
    
    bool fiber::joinable() const {
        // Return true iff this is a fiber and not the current calling fiber
        return (m_ && detail::fiber_object::current_fiber_!=m_.get());
    }
    
    fiber::id fiber::get_id() const {
        return reinterpret_cast<fiber::id>(m_.get());
    }
    
    void fiber::join() {
        if (!m_) {
            throw std::make_error_code(std::errc::no_such_process);
        }
        if (m_.get()==detail::fiber_object::current_fiber_) {
            throw std::make_error_code(std::errc::resource_deadlock_would_occur);
        }
        if (detail::fiber_object::current_fiber_) {
            detail::fiber_object::current_fiber_->join(m_);
        }
    }
    
    void fiber::detach() {
        m_.reset();
    }
    
    void fiber::swap(fiber &other) noexcept(true) {
        std::swap(m_, other.m_);
    }
    
    unsigned fiber::hardware_concurrency() {
        if (detail::fiber_object::current_fiber_) {
            return std::min(std::thread::hardware_concurrency(),
                            unsigned(detail::fiber_object::current_fiber_->sched_->threads_.size()));
        }
        return std::min(std::thread::hardware_concurrency(),
                        unsigned(scheduler::get_instance().m_->threads_.size()));
    }
    
    namespace this_fiber {
        void yield() {
            if (::fibio::fibers::detail::fiber_object::current_fiber_) {
                ::fibio::fibers::detail::fiber_object::current_fiber_->yield();
            }
        }
        
        fiber::id get_id() {
            return reinterpret_cast<fiber::id>(::fibio::fibers::detail::fiber_object::current_fiber_);
        }
        
        bool is_a_fiber() noexcept(true) {
            return ::fibio::fibers::detail::fiber_object::current_fiber_;
        }
        
        namespace detail {
            void sleep_usec(uint64_t usec) {
                if (::fibio::fibers::detail::fiber_object::current_fiber_) {
                    ::fibio::fibers::detail::fiber_object::current_fiber_->sleep_usec(usec);
                }
            }
            
            asio::io_service &get_io_service() {
                if (::fibio::fibers::detail::fiber_object::current_fiber_) {
                    return ::fibio::fibers::detail::fiber_object::current_fiber_->io_service_;
                }
                throw std::system_error(std::make_error_code(std::errc::no_such_process));
            }
        }   // End of namespace detail
    }   // End of namespace this_fiber
    
    fiberized_std_stream_guard::fiberized_std_stream_guard()
    : old_cin_buf_(0)
    , old_cout_buf_(0)
    , old_cerr_buf_(0)
    , cin_buf_(new sbuf_t())
    , cout_buf_(new sbuf_t())
    , cerr_buf_(new sbuf_t())
    {
        old_cin_buf_=std::cin.rdbuf(cin_buf_);
        old_cout_buf_=std::cout.rdbuf(cout_buf_);
        old_cerr_buf_=std::cerr.rdbuf(cerr_buf_);
        cin_buf_->open(0);
        cout_buf_->open(1);
        cerr_buf_->open(2);
        // Set cerr to unbuffered
        std::cerr.rdbuf()->pubsetbuf(0, 0);
    }
    
    fiberized_std_stream_guard::~fiberized_std_stream_guard() {
        cin_buf_->release();
        cout_buf_->release();
        cerr_buf_->release();
        std::cin.rdbuf(old_cin_buf_);
        std::cout.rdbuf(old_cout_buf_);
        std::cerr.rdbuf(old_cerr_buf_);
        delete cin_buf_;
        delete cout_buf_;
        delete cerr_buf_;
    }
    
    int fiberize(size_t nthr, std::function<int(int, char*[])> &&entry, int argc, char *argv[]) {
        int ret=0;
        try
        {
            fibio::scheduler sched=fibio::scheduler::get_instance();
            sched.start(nthr);
            fibio::fiber f([&](){
                fiberized_std_stream_guard fg;
                ret=entry(argc, argv);
            });
            sched.join();
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        if(detail::scheduler_object::the_instance_) {
            detail::scheduler_object::the_instance_.reset();
        }
        return ret;
    }
}}  // End of namespace fibio::fibers

