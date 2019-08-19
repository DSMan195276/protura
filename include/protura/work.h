/*
 * Copyright (C) 2017 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_PROTURA_WORK_H
#define INCLUDE_PROTURA_WORK_H

#include <protura/types.h>
#include <protura/list.h>
#include <protura/bits.h>
#include <protura/ktimer.h>
#include <protura/spinlock.h>

struct task;

struct workqueue {
    list_head_t work_list;
    spinlock_t lock;
    struct task *work_thread;
};

enum work_type {
    WORK_NONE,
    WORK_TASK,
    WORK_CALLBACK,
    WORK_KWORK,
};

struct work {
    /* Entry into the queue of work to be run */
    list_node_t work_entry;

    /* Entry in a wakeup queue */
    list_node_t wakeup_entry;

    enum work_type type;

    void (*callback) (struct work *);
    struct task *task;
};

struct delay_work {
    struct work work;
    struct ktimer timer;
};

#define WORKQUEUE_INIT(queue) \
    { \
        .work_list = LIST_HEAD_INIT((queue).work_list), \
        .lock = SPINLOCK_INIT("workqueue-lock"), \
    }

#define WORK_INIT(work) \
    { \
        .work_entry = LIST_NODE_INIT((work).work_entry), \
        .wakeup_entry = LIST_NODE_INIT((work).wakeup_entry), \
        .type = WORK_NONE, \
    }

#define WORK_INIT_KWORK(work, func) \
    { \
        .work_entry = LIST_NODE_INIT((work).work_entry), \
        .wakeup_entry = LIST_NODE_INIT((work).wakeup_entry), \
        .type = WORK_KWORK, \
        .callback = func, \
    }

#define WORK_INIT_CALLBACK(work, func) \
    { \
        .work_entry = LIST_NODE_INIT((work).work_entry), \
        .wakeup_entry = LIST_NODE_INIT((work).wakeup_entry), \
        .type = WORK_CALLBACK, \
        .callback = func, \
    }

#define WORK_INIT_TASK(work, t) \
    { \
        .work_entry = LIST_NODE_INIT((work).work_entry), \
        .wakeup_entry = LIST_NODE_INIT((work).wakeup_entry), \
        .type = WORK_TASK, \
        .task = t, \
    }

#define DELAY_WORK_INIT_KWORK(w, func) \
    { \
        .work = WORK_INIT_KWORK((w).work, func), \
        .timer = KTIMER_INIT((w).timer), \
    }

static inline void workqueue_init(struct workqueue *queue)
{
    *queue = (struct workqueue)WORKQUEUE_INIT(*queue);
}

static inline void work_init(struct work *work)
{
    *work = (struct work)WORK_INIT(*work);
}

static inline void work_init_kwork(struct work *work, void (*callback) (struct work *))
{
    *work = (struct work)WORK_INIT_KWORK(*work, callback);
}

static inline void work_init_callback(struct work *work, void (*callback) (struct work *))
{
    *work = (struct work)WORK_INIT_CALLBACK(*work, callback);
}

static inline void work_init_task(struct work *work, struct task *task)
{
    *work = (struct work)WORK_INIT_TASK(*work, task);
}

static inline void delay_work_init_kwork(struct delay_work *work, void (*callback) (struct work *))
{
    *work = (struct delay_work)DELAY_WORK_INIT_KWORK(*work, callback);
}

void workqueue_start(struct workqueue *, const char *thread_name);
void workqueue_stop(struct workqueue *);

void workqueue_add_work(struct workqueue *, struct work *);

void work_schedule(struct work *);

void kwork_delay_schedule(struct delay_work *, int delay_ms);
int kwork_delay_unschedule(struct delay_work *work);

/*
 * Convinence methods for setting the callback and scheduling in one go.
 */
static inline void kwork_schedule_callback(struct work *work, void (*callback) (struct work *))
{
    work->callback = callback;
    work->type = WORK_KWORK;
    work_schedule(work);
}

static inline void kwork_delay_schedule_callback(struct delay_work *work, int delay_ms, void (*callback) (struct work *))
{
    work->work.callback = callback;
    kwork_delay_schedule(work, delay_ms);
}

void kwork_init(void);

#endif
