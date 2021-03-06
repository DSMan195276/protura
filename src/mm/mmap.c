/*
 * Copyright (C) 2015 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */

#include <protura/types.h>
#include <protura/debug.h>
#include <protura/string.h>
#include <protura/list.h>
#include <protura/snprintf.h>
#include <protura/task.h>
#include <protura/mm/palloc.h>
#include <protura/mm/kmalloc.h>
#include <protura/mm/memlayout.h>
#include <protura/mm/vm.h>
#include <protura/fs/vfs.h>

static int mmap_file_fill_page(struct vm_map *map, va_t address)
{
    struct page *p = palloc(0, PAL_KERNEL);
    if (!p)
        return -ENOSPC;

    address = PG_ALIGN_DOWN(address);

    off_t memoffset = address - map->addr.start;
    off_t offset = memoffset + map->file_page_offset;

    struct user_buffer read_buf = make_kernel_buffer(p->virt);

    int err = vfs_pread(map->filp, read_buf, PG_SIZE, offset);

    if (err < 0)
        return err;

    page_table_map_entry(map->owner->page_dir, address, page_to_pa(p), map->flags, PCM_CACHED);
    return 0;
}

const struct vm_map_ops mmap_file_ops = {
    .fill_page = mmap_file_fill_page,
};

