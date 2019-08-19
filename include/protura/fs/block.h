/*
 * Copyright (C) 2015 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_FS_BLOCK_H
#define INCLUDE_FS_BLOCK_H

#include <protura/types.h>
#include <protura/stddef.h>
#include <protura/debug.h>
#include <protura/scheduler.h>
#include <protura/wait.h>
#include <protura/list.h>
#include <protura/hlist.h>
#include <protura/mutex.h>
#include <protura/atomic.h>
#include <protura/dev.h>
#include <protura/crc.h>

struct block_device;

enum {
    /* If this is set, then the contents of this block has been modified and
     * doesn't match the contents of the disk. */
    BLOCK_DIRTY,
    /* If this isn't set, then that means the contents of this block aren't
     * correct, and need to be read from the disk. */
    BLOCK_VALID,
};

struct block {
    /* The actual data for this block */
    char *data;

    /* 'block_size' is the size of the data for this block. */
    size_t block_size;

    /* Location of the first block on the device, and the actual device it represents. */
    sector_t sector;
    dev_t dev;

    struct block_device *bdev;
    list_node_t bdev_blocks_entry;

    flags_t flags;

    /* To be able to modify this block, you have to acquire this lock */
    mutex_t block_mutex;

#ifdef CONFIG_BLOCK_CHANGE_CRC_CHECK
    /* Used to catch b->dirty not set errors */
    uint16_t crc;
#endif

    struct wait_queue queue;

    list_node_t block_list_node;
    struct hlist_node cache;
    list_node_t block_lru_node;
};

static inline void block_mark_dirty(struct block *b)
{
    flag_set(&b->flags, BLOCK_DIRTY);
}

static inline void block_mark_clean(struct block *b)
{
    flag_clear(&b->flags, BLOCK_DIRTY);
}

static inline int block_waiting(struct block *b)
{
    return mutex_waiting(&b->block_mutex);
}

struct block *bread(dev_t, sector_t);
void brelease(struct block *);

#define using_block(dev, sector, block) \
    using_nocheck(((block) = bread(dev, sector)), (brelease(block)))

void block_cache_init(void);

struct block_device;
struct file_ops;

struct block_device_ops {
    void (*sync_block) (struct block_device *, struct block *b);
};

struct block_device {
    const char *name;
    int major;

    list_head_t blocks;

    size_t block_size;

    struct block_device_ops *ops;
    struct file_ops *fops;
};


extern struct file_ops block_dev_file_ops_generic;

int block_dev_file_open_generic(struct inode *dev, struct file *filp);
int block_dev_file_close_generic(struct file *);

enum {
    BLOCK_DEV_NONE = 0,
    BLOCK_DEV_IDE_MASTER = 1,
    BLOCK_DEV_IDE_SLAVE = 2,
    BLOCK_DEV_ANON = 3,
};

void block_dev_init(void);

struct block_device *block_dev_get(dev_t device);
int block_dev_set_block_size(dev_t device, size_t size);
void block_dev_clear(dev_t dev);

#define BLOCK_CRC_POLY CRC_ANSI_POLY

#ifdef CONFIG_BLOCK_CHANGE_CRC_CHECK
static inline void block_set_crc(struct block *b)
{
    b->crc = crc16(b->data, b->block_size, BLOCK_CRC_POLY);
}

static inline void block_check_crc(struct block *b)
{
    if (crc16(b->data, b->block_size, BLOCK_CRC_POLY) != b->crc)
        panic("Block %d:%d: CRC check failed, b->dirty should be set! b->dirty=%d\n", b->dev, b->sector, flag_test(&b->flags, BLOCK_DIRTY));
}
#else
static inline void block_set_crc(struct block *b) { }
static inline void block_check_crc(struct block *b) { }
#endif

static inline void block_dev_sync_block(struct block_device *dev, struct block *b)
{
    if (!flag_test(&b->flags, BLOCK_DIRTY) && flag_test(&b->flags, BLOCK_VALID))
        block_check_crc(b);

    if (!flag_test(&b->flags, BLOCK_VALID) || flag_test(&b->flags, BLOCK_DIRTY)) {
        (dev->ops->sync_block) (dev, b);
        block_set_crc(b);
    }
}

static inline void block_lock(struct block *b)
{
    kp(KP_LOCK, "block %d:%d: Locking\n", b->dev, b->sector);
    mutex_lock(&b->block_mutex);
    block_dev_sync_block(b->bdev, b);
    kp(KP_LOCK, "block %d:%d: Locked\n", b->dev, b->sector);
}

static inline int block_try_lock(struct block *b)
{
    kp(KP_LOCK, "block %d:%d: Locking\n", b->dev, b->sector);
    if (mutex_try_lock(&b->block_mutex)) {
        block_dev_sync_block(b->bdev, b);
        kp(KP_LOCK, "block %d:%d: Locked\n", b->dev, b->sector);
        return SUCCESS;
    }

    return 1;
}

static inline void block_unlock(struct block *b)
{
    kp(KP_LOCK, "block %d:%d: Unlocking\n", b->dev, b->sector);
    block_dev_sync_block(b->bdev, b);
    mutex_unlock(&b->block_mutex);
    kp(KP_LOCK, "block %d:%d: Unlocked\n", b->dev, b->sector);
}

dev_t block_dev_anon_get(void);
void block_dev_anon_put(dev_t);

#endif
