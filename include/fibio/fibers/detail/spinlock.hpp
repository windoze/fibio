//
//  spinlock.hpp
//  fibio
//
//  Created by Chen Xu on 14-6-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_detail_spinlock_hpp
#define fibio_fibers_detail_spinlock_hpp

#include <atomic>
 
namespace fibio { namespace fibers { namespace detail {
    /**
     * class spinlock
     *
     * A spinlock meets C++11 BasicLockable concept
     */
    class spinlock {
    private:
        typedef enum {Locked, Unlocked} LockState;
        std::atomic<LockState> state_;

    public:
        /// Constructor
        spinlock() noexcept : state_(Unlocked) {}
  
        /// Blocks until a lock can be obtained for the current execution agent.
        void lock() noexcept {
            while (state_.exchange(Locked, std::memory_order_acquire) == Locked) {
                /* busy-wait */
            }
        }

        /// Releases the lock held by the execution agent.
        void unlock() noexcept {
            state_.store(Unlocked, std::memory_order_release);
        }
    };
}}} // End of namespace fibio::fibers::detail

#endif