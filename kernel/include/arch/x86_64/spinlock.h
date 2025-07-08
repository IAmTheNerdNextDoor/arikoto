#pragma once

typedef struct {
    volatile int locked;
} spinlock_t;

#define SPINLOCK_INIT { 0 }

static inline void spinlock_init(spinlock_t *lock) {
    lock->locked = 0;
}

static inline void spinlock_acquire(spinlock_t *lock) {
    while (1) {
        int expected_value = 1;
        asm volatile (
            "lock xchg %0, %1"
            : "+r" (expected_value), "+m" (lock->locked)
            :
            : "memory"
        );

        if (expected_value == 0) {
            break;
        }

        while (lock->locked != 0) {
            asm volatile ("pause" ::: "memory");
        }
    }
}

static inline void spinlock_release(spinlock_t *lock) {
    asm volatile ("" ::: "memory");
    lock->locked = 0;
}
