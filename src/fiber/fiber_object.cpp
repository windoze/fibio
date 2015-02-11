//
//  fibers.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-1.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifdef __WIN32
//  <winsock2.h> must be included before <windows.h>
#   include <winsock2.h>
#endif

#include <memory>
#include <algorithm>
#if defined(HAVE_VALGRIND_H)
#   if BOOST_VERSION>=105700
//      For boost 1.57 and up, enable built-in valgrind support
#       define BOOST_USE_VALGRIND
#   else
//      For boost 1.56 and earlier
#       include "valgrind.h"
#       include <unordered_map>
#   endif
#endif  // defined(HAVE_VALGRIND_H)
#ifdef __WIN32
#   include <boost/coroutine/standard_stack_allocator.hpp>
#else
#   include <boost/coroutine/protected_stack_allocator.hpp>
#endif
#include <boost/coroutine/stack_context.hpp>

#include <fibio/fibers/fiber.hpp>
#include <fibio/fibers/fss.hpp>
#include <fibio/fibers/mutex.hpp>
#include <fibio/fibers/condition_variable.hpp>

#include "fiber_object.hpp"
#include "scheduler_object.hpp"

static const auto NOT_A_FIBER=fibio::fiber_exception(boost::system::errc::no_such_process);
static const auto DEADLOCK=fibio::fiber_exception(boost::system::errc::resource_deadlock_would_occur);

namespace fibio { namespace fibers { namespace detail {
#ifdef BOOST_USE_SEGMENTED_STACKS
#   define BOOST_COROUTINE_STACK_ALLOCATOR boost::coroutines::basic_segmented_stack_allocator
#else
#   ifdef __WIN32
#       define BOOST_COROUTINE_STACK_ALLOCATOR boost::coroutines::basic_standard_stack_allocator
#   else
#       define BOOST_COROUTINE_STACK_ALLOCATOR boost::coroutines::basic_protected_stack_allocator
#   endif
#endif

    // Define a fibio_stack_allocator, use valgrind_stack_allocator when building
    //  with valgrind support
    // Valgrind support is built-in since Boost 1.57
#if defined(HAVE_VALGRIND_H) && (BOOST_VERSION<105700)
    // Wraps boost::coroutine::stack_allocator, and if Valgrind is installed
    // will register stacks, so that Valgrind is not confused.
    template<typename traitsT>
    class valgrind_stack_allocator {
        typedef traitsT traits_type;
        BOOST_COROUTINE_STACK_ALLOCATOR<traitsT> allocator;
        std::unordered_map<void*, unsigned> stack_ids;
        
    public:
        void inline allocate( boost::coroutines::stack_context &sc, std::size_t size) {
            allocator.allocate(sc, size);
            auto res = stack_ids.insert({sc.sp, VALGRIND_STACK_REGISTER(sc.sp, (((char*)sc.sp) - sc.size))});
            (void)res;
            assert(res.second);
        }
        
        void inline deallocate( boost::coroutines::stack_context & sc) {
            auto id = stack_ids.find(sc.sp);
            assert(id != stack_ids.end());
            VALGRIND_STACK_DEREGISTER(id->second);
            stack_ids.erase(id);
            allocator.deallocate(sc);
        }
    };
    typedef valgrind_stack_allocator<boost::coroutines::stack_traits> fibio_stack_allocator;
#else  // defined(HAVE_VALGRIND_H)
    // Use default stack allocator when building w/o valgrind support
    typedef BOOST_COROUTINE_STACK_ALLOCATOR<boost::coroutines::stack_traits> fibio_stack_allocator;
#endif  // !defined(HAVE_VALGRIND_H)
    
    THREAD_LOCAL fiber_object *fiber_object::current_fiber_=0;
    
    fiber_object::fiber_object(scheduler_ptr_t sched, fiber_data_base *entry)
    : sched_(sched)
    , fiber_strand_(std::make_shared<boost::asio::strand>(sched_->io_service_))
    , state_(READY)
    , entry_(entry)
    , runner_(std::bind(&fiber_object::runner_wrapper, this, std::placeholders::_1),
              boost::coroutines::attributes(),
              fibio_stack_allocator() )
    , caller_(0)
    {}
    
    fiber_object::fiber_object(scheduler_ptr_t sched, strand_ptr_t strand, fiber_data_base *entry)
    : sched_(sched)
    , fiber_strand_(strand)
    , state_(READY)
    , entry_(entry)
    , runner_(std::bind(&fiber_object::runner_wrapper, this, std::placeholders::_1),
              boost::coroutines::attributes(),
              fibio_stack_allocator() )
    , caller_(0)
    {}
    
    fiber_object::~fiber_object() {
        if (state_!=STOPPED) {
            // std::thread will call std::terminate if deleting a unstopped thread
            std::terminate();
        }
        if (uncaught_exception_) {
            // There is an uncaught exception not propagated to joiner
            std::terminate();
        }
    }
    
    void fiber_object::set_name(const std::string &s) {
        std::lock_guard<spinlock> lock(mtx_);
        name_=s;
    }
    
    std::string fiber_object::get_name() {
        std::lock_guard<spinlock> lock(mtx_);
        return name_;
    }
    
    void fiber_object::runner_wrapper(caller_t &c) {
        struct cleaner {
            cleaner(spinlock &mtx,
                    cleanup_queue_t &q,
                    fss_map_t &fss)
            : mtx_(mtx)
            , q_(q)
            , fss_(fss)
            {}
            
            ~cleaner()
            try {
                // No exception should be thrown in destructor
                cleanup_queue_t temp;
                {
                    // Move cleanup queue content out
                    std::lock_guard<spinlock> lock(mtx_);
                    temp.swap(q_);
                }
                for (std::function<void()> f: temp) {
                    f();
                }
                for (auto &v: fss_) {
                    if (v.second.first && v.second.second) {
                        (*(v.second.first))(v.second.second);
                    }
                }
            } catch(...) {
                // TODO: Error
            }
            spinlock &mtx_;
            cleanup_queue_t &q_;
            fss_map_t &fss_;
        };
        // Need this to complete constructor without running entry_
        c(READY);
        
        // Now we're out of constructor
        caller_=&c;
        try {
            cleaner c(mtx_, cleanup_queue_, fss_);
            entry_->run();
        } catch(const boost::coroutines::detail::forced_unwind&) {
            // Boost.Coroutine requirement
            throw;
        } catch(...) {
            uncaught_exception_=std::current_exception();
        }
        // Clean fiber arguments before fiber destroy
        entry_.reset();
        // Fiber function exits, set state to STOPPED
        c(STOPPED);
        caller_=0;
    }
    
    void fiber_object::detach() {
        std::lock_guard<spinlock> lock(mtx_);
        if (state_!=STOPPED) {
            // Hold a reference to this, make sure detached fiber live till ends
            this_ref_=shared_from_this();
        }
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
        if (state_==READY) {
            state_=RUNNING;
        }
        // Keep running if necessary
        while (state_==RUNNING) {
            tls_guard guard(this);
            state_=runner_().get();
        }
        state_t s= state_;
        if (s==READY) {
            // Post this fiber to the scheduler
            resume();
        } else if (s==BLOCKED) {
            // Must make sure this fiber will be posted elsewhere later, otherwise it will hold forever
        } else if (s==STOPPED) {
            cleanup_queue_t temp;
            {
                // Move joining queue content out
                std::lock_guard<spinlock> lock(mtx_);
                temp.swap(join_queue_);
            }
            // Fiber ended, clean up joining queue
            for (std::function<void()> f: temp) {
                f();
            }
            // Post exit message to scheduler
            get_fiber_strand().post(std::bind(&scheduler_object::on_fiber_exit, sched_, shared_from_this()));
        }
    }
    
    boost::asio::strand &fiber_object::get_fiber_strand() {
        return *fiber_strand_;
    }
    
    // Switch out of fiber context
    void fiber_object::pause() {
        // Pre-condition
        // Can only pause current running fiber
        assert(current_fiber_==this);
        
        set_state(BLOCKED);
        
        // Check interruption when resumed
        std::lock_guard<spinlock> lock(mtx_);
        if (interrupt_disable_level_==0 && interrupt_requested_) {
            BOOST_THROW_EXCEPTION(fiber_interrupted());
        }
    }
    
    inline void activate_fiber(fiber_ptr_t this_fiber) {
        // Pre-condition
        // Cannot activate current running fiber
        assert(fiber_object::current_fiber_!=this_fiber.get());
        
        this_fiber->state_=fiber_object::READY;
        this_fiber->one_step();
    }
    
    void fiber_object::activate() {
        if (fiber_object::current_fiber_
            && fiber_object::current_fiber_->sched_
            && (fiber_object::current_fiber_->sched_==sched_))
        {
            get_fiber_strand().dispatch(std::bind(activate_fiber, shared_from_this()));
        } else {
            resume();
        }
    }
    
    void fiber_object::resume() {
        get_fiber_strand().post(std::bind(activate_fiber, shared_from_this()));
    }
    
    // Following functions can only be called inside coroutine
    void fiber_object::yield(fiber_ptr_t hint) {
        // Pre-condition
        // Can only pause current running fiber
        assert(current_fiber_==this);
        assert(state_==RUNNING);

        // Do yeild when:
        //  1. there is only 1 thread in this scheduler
        //  2. or, too many fibers out there (fiber_count > thread_count*2)
        //  3. or, hint is a fiber that shares the strand with this one
        //  4. or, there is no hint (force yield)
        if ((sched_->threads_.size()==1)
            || (sched_->fiber_count_>sched_->threads_.size()*2)
            || (hint && (hint->fiber_strand_==fiber_strand_))
            || !hint
            )
        {
            set_state(READY);
        }
    }

    void fiber_object::join(fiber_ptr_t f) {
        CHECK_CALLER(this);
        std::lock_guard<spinlock> lock(f->mtx_);
        if (this==f.get()) {
            // The fiber is joining itself
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if (f->state_==STOPPED) {
            // f is already stopped, do nothing
            return;
        } else {
            f->join_queue_.push_back(std::bind(&fiber_object::activate, shared_from_this()));
        }

        { relock_guard<spinlock> relock(f->mtx_); pause(); }
    }
    
    void propagate_exception(fiber_ptr_t f) {
        std::exception_ptr e;
        std::swap(e, f->uncaught_exception_);
        // throw exception
        if (e) {
            std::rethrow_exception(e);
        }
    }
    
    void fiber_object::join_and_rethrow(fiber_ptr_t f) {
        CHECK_CALLER(this);
        std::lock_guard<spinlock> lock(f->mtx_);
        if (this==f.get()) {
            // The fiber is joining itself
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if (f->state_==STOPPED) {
            // f is already stopped
            propagate_exception(f);
            return;
        } else {
            // std::cout << "fiber(pthis) blocked" << std::endl;
            f->join_queue_.push_back(std::bind(&fiber_object::activate, shared_from_this()));
        }

        { relock_guard<spinlock> relock(f->mtx_); pause(); }

        // Joining completed, propagate exception from joinee
        propagate_exception(f);
    }

    void fiber_object::sleep_rel(duration_t d) {
        // Shortcut
        if (d==duration_t::zero()) {
            return;
        }
        CHECK_CALLER(this);
        timer_t sleep_timer(get_io_service());
        sleep_timer.expires_from_now(d);
        sleep_timer.async_wait(std::bind(&fiber_object::activate, shared_from_this()));

        pause();
    }
    
    void fiber_object::add_cleanup_function(std::function<void()> &&f) {
        std::lock_guard<spinlock> lock(mtx_);
        cleanup_queue_.push_back(std::move(f));
    }
    
    void fiber_object::interrupt() {
        std::lock_guard<spinlock> lock(mtx_);
        if (interrupt_disable_level_==0) {
            interrupt_requested_=true;
        }
    }
    
    void set_fss_data(void const* key,
                      std::shared_ptr<fss_cleanup_function> func,
                      void* fss_data,
                      bool cleanup_existing)
    {
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
    
    fiber_base::ptr_t get_current_fiber_ptr() {
        if(!fiber_object::current_fiber_) {
            // Not a fiber
            BOOST_THROW_EXCEPTION(NOT_A_FIBER);
        }
        return std::static_pointer_cast<fiber_base>(fiber_object::current_fiber_->shared_from_this());
    }
}}}   // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers {
    void fiber::start() {
        if (auto cf=current_fiber()) {
            // use current scheduler if we're in a fiber
            impl_=cf->sched_->make_fiber(data_.release());
        } else {
            // use default scheduler if we're not in a fiber
            impl_=scheduler::get_instance().impl_->make_fiber(data_.release());
        }
    }
    
    void fiber::start(attributes attr) {
        if (auto cf=current_fiber()) {
            // use current scheduler if we're in a fiber
            switch(attr.policy) {
                case attributes::scheduling_policy::normal: {
                    // Create an isolated fiber
                    impl_=cf->sched_->make_fiber(data_.release());
                    break;
                }
                case attributes::scheduling_policy::stick_with_parent: {
                    // Create a fiber shares strand with parent
                    impl_=cf->sched_->make_fiber(current_fiber()->fiber_strand_, data_.release());
                    break;
                }
                default:
                    break;
            }
        } else {
            // use default scheduler if we're not in a fiber
            impl_=scheduler::get_instance().impl_->make_fiber(data_.release());
        }
    }
    
    void fiber::start(scheduler &sched) {
        // use supplied scheduler to start fiber
        impl_=sched.impl_->make_fiber(data_.release());
    }
    
    fiber::fiber(fiber &&other) noexcept
    : data_(std::move(other.data_))
    , impl_(std::move(other.impl_))
    {}
    
    fiber& fiber::operator=(fiber &&other) noexcept {
        if (impl_) {
            // This fiber is still active, std::thread will call std::terminate in the case
            std::terminate();
        }
        data_=std::move(other.data_);
        impl_=std::move(other.impl_);
        return *this;
    }
    
    void fiber::set_name(const std::string &s) {
        impl_->set_name(s);
    }
    
    std::string fiber::get_name() {
        return impl_->get_name();
    }
    
    bool fiber::joinable() const noexcept {
        // Return true iff this is a fiber and not the current calling fiber
        // and 2 fibers are in the same scheduler
        return (impl_ && current_fiber()!=impl_.get())
        && (impl_->sched_==current_fiber()->sched_);
    }
    
    fiber::id fiber::get_id() const noexcept {
        return reinterpret_cast<fiber::id>(impl_.get());
    }
    
    void fiber::join(bool propagate_exception) {
        if (!impl_) {
            BOOST_THROW_EXCEPTION(NOT_A_FIBER);
        }
        if (impl_.get()==current_fiber()) {
            BOOST_THROW_EXCEPTION(DEADLOCK);
        }
        if (!joinable()) {
            BOOST_THROW_EXCEPTION(invalid_argument());
        }
        if (current_fiber()) {
            if (propagate_exception) {
                current_fiber()->join_and_rethrow(impl_);
            } else {
                current_fiber()->join(impl_);
            }
        }
    }
    
    void fiber::detach() {
        if (!(impl_ && current_fiber()!=impl_.get())) {
            BOOST_THROW_EXCEPTION(NOT_A_FIBER);
        }
        detail::fiber_ptr_t this_fiber=impl_;
        impl_->get_fiber_strand().post(std::bind(&detail::fiber_object::detach, impl_));
        impl_.reset();
    }
    
    void fiber::swap(fiber &other) noexcept(true) {
        std::swap(impl_, other.impl_);
    }
    
    unsigned fiber::hardware_concurrency() noexcept {
        return std::thread::hardware_concurrency();
    }
    
    void fiber::interrupt() {
        if (!impl_) {
            BOOST_THROW_EXCEPTION(NOT_A_FIBER);
        }
        std::lock_guard<detail::spinlock> lock(impl_->mtx_);
        if (impl_->interrupt_disable_level_==0) {
            impl_->interrupt_requested_=true;
        }
    }
    
    namespace this_fiber {
        void yield() {
            if (auto cf=current_fiber()) {
                cf->yield();
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_FIBER);
            }
        }
        
        fiber::id get_id() {
            return reinterpret_cast<fiber::id>(current_fiber());
        }
        
        bool is_a_fiber() noexcept(true) {
            return current_fiber();
        }
        
        namespace detail {
            void sleep_rel(fibers::detail::duration_t d) {
                if (auto cf=current_fiber()) {
                    if (d<fibers::detail::duration_t::zero()) {
                        BOOST_THROW_EXCEPTION(fibio::invalid_argument());
                    }
                    cf->sleep_rel(d);
                } else {
                    BOOST_THROW_EXCEPTION(NOT_A_FIBER);
                }
            }
            
            boost::asio::io_service &get_io_service() {
                if (auto cf=current_fiber()) {
                    return cf->get_io_service();
                }
                BOOST_THROW_EXCEPTION(NOT_A_FIBER);
            }
            
            boost::asio::strand &get_strand() {
                if (auto cf=current_fiber()) {
                    return cf->get_fiber_strand();
                } else {
                    BOOST_THROW_EXCEPTION(NOT_A_FIBER);
                }
            }
        }   // End of namespace detail
        
        std::string get_name() {
            if (auto cf=current_fiber()) {
                return cf->get_name();
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_FIBER);
            }
        }
        
        void set_name(const std::string &name) {
            if (auto cf=current_fiber()) {
                cf->set_name(name);
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_FIBER);
            }
        }
        
        scheduler get_scheduler() {
            if (auto cf=current_fiber()) {
                return scheduler(cf->sched_);
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_FIBER);
            }
        }
        
        disable_interruption::disable_interruption() {
            if (auto cf=current_fiber()) {
                std::lock_guard<fibers::detail::spinlock> lock(cf->mtx_);
                cf->interrupt_disable_level_++;
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_FIBER);
            }
        }
        
        disable_interruption::~disable_interruption() {
            if (auto cf=current_fiber()) {
                std::lock_guard<fibers::detail::spinlock> lock(cf->mtx_);
                cf->interrupt_disable_level_--;
            }
        }
        
        restore_interruption::restore_interruption() {
            if (auto cf=current_fiber()) {
                std::lock_guard<fibers::detail::spinlock> lock(cf->mtx_);
                std::swap(cf->interrupt_disable_level_, level_);
            } else {
                BOOST_THROW_EXCEPTION(NOT_A_FIBER);
            }
        }
        
        restore_interruption::~restore_interruption() {
            if (auto cf=current_fiber()) {
                std::lock_guard<fibers::detail::spinlock> lock(cf->mtx_);
                std::swap(cf->interrupt_disable_level_, level_);
            }
        }
        
        bool interruption_enabled() noexcept {
            if (auto cf=current_fiber()) {
                std::lock_guard<fibers::detail::spinlock> lock(cf->mtx_);
                return cf->interrupt_disable_level_==0;
            }
            return false;
        }
        
        bool interruption_requested() noexcept {
            if (auto cf=current_fiber()) {
                std::lock_guard<fibers::detail::spinlock> lock(cf->mtx_);
                return cf->interrupt_requested_==0;
            }
            return false;
        }
        
        void interruption_point() {
            if (auto cf=current_fiber()) {
                std::lock_guard<fibers::detail::spinlock> lock(cf->mtx_);
                if (cf->interrupt_requested_) {
                    BOOST_THROW_EXCEPTION(fibers::fiber_interrupted());
                }
            }
        }
        
        void at_fiber_exit(std::function<void()> &&f) {
            if (auto cf=current_fiber()) {
                std::lock_guard<fibers::detail::spinlock> lock(cf->mtx_);
                cf->cleanup_queue_.push_back(std::forward<std::function<void()>>(f));
            }
        }
    }   // End of namespace this_fiber
}}  // End of namespace fibio::fibers

