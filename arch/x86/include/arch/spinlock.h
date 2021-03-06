/*
 * Copyright (C) 2015 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_ARCH_SPINLOCK_H
#define INCLUDE_ARCH_SPINLOCK_H

#include <protura/types.h>
#include <protura/debug.h>
#include <protura/stddef.h>
#include <arch/asm.h>

struct spinlock {
    unsigned int locked;
    unsigned int eflags;
};

typedef struct spinlock spinlock_t;

#define SPINLOCK_INIT() { .locked = 0 }

static inline void spinlock_init(spinlock_t *lock)
{
    *lock = (spinlock_t)SPINLOCK_INIT();
}

static inline void spinlock_acquire(spinlock_t *lock)
{
    uint32_t tmp_flags = eflags_read();

    cli();
    while (xchg(&lock->locked, 1) != 0)
        ;

    lock->eflags = tmp_flags;
}

static inline void spinlock_release(spinlock_t *lock)
{
    uint32_t tmp_flags = lock->eflags;

    xchg(&lock->locked, 0);

    eflags_write(tmp_flags);
}

static inline void spinlock_release_cleanup(spinlock_t **spinlock)
{
    spinlock_release(*spinlock);
}

static inline void spinlock_acquire_cleanup(spinlock_t **lock)
{
    spinlock_acquire(*lock);
}

static inline int spinlock_try_acquire(spinlock_t *lock)
{
    uint32_t eflags;
    int got_lock;

    eflags = eflags_read();
    cli();

    got_lock = xchg(&lock->locked, 1);

    if (got_lock == 0)
        lock->eflags = eflags;
    else
        eflags_write(eflags);

    return got_lock == 0;
}

/* Wraps acquiring and releaseing a spinlock. Usages of 'using_spinlock' can't
 * ever leave-out a matching release for the acquire. */
#define using_spinlock(lock) scoped_using_cond(1, spinlock_acquire, spinlock_release_cleanup, lock)

/* Can be used in a 'using_spinlock' block of code to release a lock for a
 * section of code, and then acquire it back after that code is done.
 *
 * It's useful in sections of code where we may go to sleep, and we want to
 * release the lock before yielding, and then acquire the lock back when we
 * start running again. */
#define not_using_spinlock(lock) scoped_using_cond(1, spinlock_release, spinlock_acquire_cleanup, lock)

#endif
