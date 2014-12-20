//
//  spinlock.hpp
//  fibio
//
//  Created by Chen Xu on 14-6-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_detail_spinlock_hpp
#define fibio_fibers_detail_spinlock_hpp

#include <boost/atomic.hpp>
 
namespace fibio { namespace fibers { namespace detail {
    class spinlock {
    private:
        typedef enum {Locked, Unlocked} LockState;
        boost::atomic<LockState> state_;

    public:
        spinlock() noexcept : state_(Unlocked) {}
  
        void lock() noexcept {
            while (state_.exchange(Locked, boost::memory_order_acquire) == Locked) {
                /* busy-wait */
            }
        }

        void unlock() noexcept {
            state_.store(Unlocked, boost::memory_order_release);
        }
    };
}}} // End of namespace fibio::fibers::detail

#endif