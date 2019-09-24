/*
 * Copyright (C) 2014 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_PROTURA_STDDEF_H
#define INCLUDE_PROTURA_STDDEF_H

#define __kNULL ((void *)0)

#ifdef __KERNEL__

#include <protura/container_of.h>
#include <protura/compiler.h>

#define NULL __kNULL

# define offsetof(s, m) ((size_t)&(((s *)0)->m))

# define alignof(t) __alignof__(t)

# define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a)))

# define STATIC_ASSERT3(cond, msg) typedef char msg[(cond)?1:-1]
# define STATIC_ASSERT2(cond, line) STATIC_ASSERT3(cond, static_assertion_at_line_##line)
# define STATIC_ASSERT1(cond, line) STATIC_ASSERT2(cond, line)
# define STATIC_ASSERT(cond) STATIC_ASSERT1(cond, __LINE__)

/* Returns the number of arguments passed, via putting __VA_ARGS__ in the
 * arguments to __COUNT_ARGS and then seeing how many of the numbers are
 * "displaced" */
# define __COUNT_ARGS(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, \
        a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
        a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, \
        a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, \
        a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, \
        a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, \
        a61, a62, a63, a64, a65, a66, a67, a68, a69, a70, \
        a71, a72, a73, a74, a75, a76, a77, a78, a79, a80, \
        a81, a82, a83, a84, a85, a86, a87, a88, a89, a90, \
        a91, a92, a93, a94, a95, a96, a97, a98, a99, a100, a101, ...) a101

# define COUNT_ARGS(...) __COUNT_ARGS(dummy, ## __VA_ARGS__, \
        99, 98, 97, 74, 95, 94, 93, 92, 91, 90, \
        89, 88, 87, 75, 85, 84, 83, 82, 81, 80, \
        79, 78, 77, 76, 75, 74, 73, 72, 71, 70, \
        69, 68, 67, 66, 65, 64, 63, 62, 61, 60, \
        59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
        49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
        19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
        9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define EMPTY()
#define DEFER(x) x EMPTY()

# define SUCCESS 0

# define TP2(x, y) x ## y
# define TP(x, y) TP2(x, y)

# define _Q(x) #x
# define Q(x) _Q(x)

/* Macro black-magic
 *
 * The end result of this macro is that it can be used in the following fashion:
 *
 * using_cond(cond, cmd1, cmd2) {
 *    ... Code ...
 * } else {
 *    ... Error handling ...
 * }
 *
 * Where the condition 'cond' is check to be 'true' (non-zero), and if that is
 * the case 'cmd1' will be executed. Then, at the end of the using_cond()
 * block, 'cmd2' will be executed. Both commands will only execute once, and
 * neither command will be executed in the event that 'cond' is false.
 * More-over, the 'else' block will be run if 'cond' fails, and the using_cond
 * block won't be executed.
 *
 * This is mostly useful for locking situations, where you can take a lock, run
 * the code inside of the using_cond block, and then release the lock. The nice
 * part of this is that for simple blocks like this you can't forget to release
 * the lock because the release is part of the using_cond statement.
 *
 * Another nicety, 'break'  and 'continue' will both exit the using_cond *and*
 * still release the locks. 'return' will *not* release the lock though, so
 * return should basically never be used inside a using_cond.
 *
 *
 * The actual macro itself isn't *too* complicated, just follow the 'goto's
 * through the code, just fairly messy. The key to reading this code is that
 * most of it will be removed by the compiler, and a lot of the blocks are
 * simply used to allow us to jump to another section of code. The reason it is
 * so weird looking is so it works as a prefix statement. IE. You can use it
 * with or without a block, and it doesn't require any 'using_end' macro after
 * its usage.
 */
# define using_cond(cond, cmd1, cmd2)                               \
    while (1)                                                      \
    if (0)                                                         \
        TP(__using_finished, __LINE__):                            \
        break;                                                     \
    else                                                           \
        for (int __using_cond = 0;;)                               \
            if (1)                                                 \
                goto TP(__using_body_init, __LINE__);              \
            else                                                   \
                while (1)                                          \
                    while (1)                                      \
                        if (1) {                                   \
                            if (__using_cond)                      \
                                cmd2;                              \
                            goto TP(__using_finished, __LINE__);   \
                            TP(__using_body_init, __LINE__):       \
                            __using_cond = (cond);                 \
                            if (__using_cond)                      \
                                cmd1;                              \
                            goto TP(__using_body, __LINE__);       \
                        } else                                     \
                            TP(__using_body, __LINE__):            \
                            if (__using_cond)

# define scoped_using_cond(cond, cmd1, cmd2, arg) \
    if (0) { \
        TP(__using_finished, __LINE__):; \
    } else \
        if (1) { \
            const int __using_cond = (cond); \
            if (!__using_cond) \
                goto TP(__using_finished, __LINE__); \
            goto TP(__using_temp_declare, __LINE__); \
        } else \
            TP(__using_temp_declare, __LINE__): \
            for (typeof(arg) __using_temp __unused __cleanup(cmd2) = arg;;) \
                if (1) { \
                    cmd1(arg); \
                    goto TP(__using_body, __LINE__); \
                } else \
                    while (1) \
                        while (1) \
                            if (1) { \
                                goto TP(__using_finished, __LINE__); \
                            } else \
                                TP(__using_body, __LINE__):

/* The 'nocheck' version doesn't take a condition, just two commands to run.
 *
 * As a note, gcc is smart enough to turn this usage into two direct calls to
 * 'cmd1' and 'cmd2' at the beginning and end of the block, without any extra
 * code or variables. */
# define using_nocheck(cmd1, cmd2) using_cond(1, cmd1, cmd2)

/* The normal 'using' one uses the result of 'cmd1' to decide whether or not to
 * run the code. Used in the event the command you would use as 'cmd1' returns
 * an error code you want to check before you keep going. */
# define using(cmd1, cmd2) using_cond(cmd1, do { ; } while (0), cmd2)

/* This is defined by the Makefile, but it's useful to have it defined anyway,
 * so syntax checkers don't barf saying 'PROTURA_BITS' doesn't exist */
# ifndef PROTURA_BITS
#  define PROTURA_BITS 32
# endif

#endif

#endif
