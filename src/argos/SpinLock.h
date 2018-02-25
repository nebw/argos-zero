#pragma once

#include <atomic>

/* http://stackoverflow.com/a/29195378 */
class SpinLock {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
    inline void lock() {
        while (locked.test_and_set(std::memory_order_acquire)) {
            asm volatile("pause\n" : : : "memory");
        }
    }
    inline bool try_lock() { return !locked.test_and_set(std::memory_order_acquire); }
    inline void unlock() { locked.clear(std::memory_order_release); }
};
