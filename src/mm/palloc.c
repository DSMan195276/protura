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
#include <protura/bits.h>
#include <protura/list.h>
#include <protura/spinlock.h>
#include <protura/scheduler.h>
#include <protura/wait.h>
#include <protura/mm/kmalloc.h>
#include <protura/fs/inode.h>
#include <protura/block/bcache.h>
#include <protura/backtrace.h>
#include <protura/mm/bootmem.h>

#include <protura/mm/palloc.h>

extern char kern_end, kern_start;

struct page_buddy_map {
    list_head_t free_pages;
    int free_count;
    struct wait_queue wait_for_free;
};

struct page_buddy_alloc {
    spinlock_t lock;

    struct page *pages;
    int page_count;

    struct page_buddy_map *maps;
    int map_count;

    int free_pages;
};

#define PALLOC_MAPS 6

static struct page_buddy_map buddy_maps[PALLOC_MAPS];

static struct page_buddy_alloc buddy_allocator = {
    .lock = SPINLOCK_INIT(),
    .pages = NULL,
    .page_count = 0,

    .maps = buddy_maps,
    .map_count = PALLOC_MAPS,

    .free_pages = 0,
};

/* Called of palloc runs out of memory to hand out. Call the 'oom' routines,
 * which attempt to free memory being used by various caches.
 *
 * Note that a call to __oom() doesn't necessarially mean we're completely out
 * of memory. Reserve pages may still be aviliable. The 'oom' routines will
 * make use of these pages if they need to do allocation (Which is not
 * impossible). */
void __oom(void)
{
    kmalloc_oom();
    inode_oom();
    bcache_oom();
}

struct page *page_from_pn(pn_t page_num)
{
    return buddy_allocator.pages + page_num;
}

/* We swap the bit in the 'order' position to get this pages buddy.
 * This works because the buddy is always '2 ^ order' pages away. */
static inline pn_t get_buddy_pn(pn_t pn, int order)
{
    return pn ^ (1 << order);
}


void __pfree_add_pages(struct page_buddy_alloc *alloc, pn_t cur_page, int order)
{
    int original_order = order;
    struct page *p, *buddy;

    while (order < PALLOC_MAPS - 1) {
        buddy = page_from_pn(get_buddy_pn(cur_page, order));

        if (buddy->order != order || bit_test(&buddy->flags, PG_INVALID))
            break;

        /* Remove our buddy from it's current free list, then use the lower
         * of our two pages as our new higher-order page, and clear the
         * 'order' value of the other page */
        list_del(&buddy->page_list_node);
        alloc->maps[order].free_count--;

        cur_page &= ~(1 << order);

        p = page_from_pn(get_buddy_pn(cur_page, order));
        p->order = -1;

        order++;
    }

    p = page_from_pn(cur_page);
    p->order = order;
    list_add(&alloc->maps[order].free_pages, &p->page_list_node);
    alloc->maps[order].free_count++;

    alloc->free_pages += 1 << original_order;
}

void __mark_page_free(pa_t pa)
{
    if (pa >= V2P(&kern_start) && pa < V2P(&kern_end)) {
        kp(KP_ERROR, "Marking a page free that's part of kernel memory!\n");
        return;
    }

    __pfree_add_pages(&buddy_allocator, __PA_TO_PN(pa), 0);
}

void pfree(struct page *p, int order)
{
    int i;

    if (!p) {
        kp(KP_ERROR, "ERROR: pfree: %p\n", p);
        return;
    }

    pa_t pa = page_to_pa(p);
    if (pa >= V2P(&kern_start) && pa < V2P(&kern_end)) {
        kp(KP_ERROR, "pfree() called on page that's part of the kernel! Page was: %p!\n", p->virt);
        dump_stack(KP_ERROR);
        return;
    }

    if (!atomic_dec_and_test(&p->use_count))
        return;

    using_spinlock(&buddy_allocator.lock) {
        __pfree_add_pages(&buddy_allocator, p->page_number, order);

        for (i = 0; i <= order; i++)
            wait_queue_wake(&buddy_allocator.maps[i].wait_for_free);
    }
}

void pfree_unordered(list_head_t *head)
{
    struct page *p;
    list_foreach_take_entry(head, p, page_list_node)
        pfree(p, 0);
}

/* Breaks apart a page of 'order' size, into to two pages of 'order - 1' size */
static void break_page(struct page_buddy_alloc *alloc, int order, unsigned int flags)
{
    struct page *p, *buddy;

    if (order >= PALLOC_MAPS || order < 0) {
        kp(KP_ERROR, "palloc: break_page failed!");
        return;
    }

    if (alloc->maps[order].free_count == 0) {
        break_page(alloc, order + 1, flags);

        /* It's possible 'break_page()' failed */
        if (alloc->maps[order].free_count == 0)
            return;
    }

    p = list_take_last(&alloc->maps[order].free_pages, struct page, page_list_node);
    alloc->maps[order].free_count--;

    order--;

    buddy = page_from_pn(get_buddy_pn(p->page_number, order));

    p->order = order;
    buddy->order = order;

    list_add(&alloc->maps[order].free_pages, &p->page_list_node);
    list_add(&alloc->maps[order].free_pages, &buddy->page_list_node);
    alloc->maps[order].free_count += 2;
}

static void __palloc_sleep_for_enough_pages(struct page_buddy_alloc *alloc, int order, unsigned int flags)
{
    if (alloc->free_pages < (1 << order)) {
        kp(KP_WARNING, "Out of memory! Attempting to free some up...\n");
        __oom();
    }

    wait_queue_event_spinlock(&alloc->maps[order].wait_for_free, alloc->free_pages >= (1 << order), &alloc->lock);
}

static struct page *__palloc_phys_multiple(struct page_buddy_alloc *alloc, int order, unsigned int flags)
{
    struct page *p;

    if (!(flags & __PAL_NOWAIT))
        __palloc_sleep_for_enough_pages(&buddy_allocator, order, flags);

    if (alloc->maps[order].free_count == 0) {
        break_page(alloc, order + 1, flags);
        if (alloc->maps[order].free_count == 0) {
            p = NULL;
            goto return_page;
        }
    }

    p = list_take_last(&alloc->maps[order].free_pages, struct page, page_list_node);
    alloc->maps[order].free_count--;

    /* Sanity check - if somehow the page array was corrupted, this could catch
     * it and fail us early */
    if (p->page_number != (p - alloc->pages))
        panic("SOMETHING IS WRONG!!!! page=%p, %d, %p\n", p, p->page_number, p->virt);

    p->order = -1;

    buddy_allocator.free_pages -= 1 << order;

  return_page:
    if (p)
        atomic_inc(&p->use_count);

    return p;
}

struct page *palloc(int order, unsigned int flags)
{
    struct page *p;

    using_spinlock(&buddy_allocator.lock)
        p = __palloc_phys_multiple(&buddy_allocator, order, flags);

    pa_t pa = page_to_pa(p);
    if (pa >= V2P(&kern_start) && pa < V2P(&kern_end)) {
        kp(KP_ERROR, "palloc() is returning a page that's part of the kernel!!!\n");
        dump_stack(KP_ERROR);
    }


    return p;
}

int palloc_unordered(list_head_t *head, int count, unsigned int flags)
{
    using_spinlock(&buddy_allocator.lock) {
        struct page *p;
        int i;

        for (i = 0; i < count; i++) {
            p = __palloc_phys_multiple(&buddy_allocator, 0, flags);

            list_add_tail(head, &p->page_list_node);

            if (!p)
                panic("OOM!!!\n");
        }
    }

    return 0;
}

int palloc_free_page_count(void)
{
    return buddy_allocator.free_pages;
}

void palloc_init(int pages)
{
    struct page *p;
    int i;

    kp(KP_DEBUG, "Initalizing buddy allocator\n");

    buddy_allocator.page_count = pages;
    buddy_allocator.pages = bootmem_alloc(pages * sizeof(struct page), PG_SIZE);

    kp(KP_DEBUG, "Pages: %d, array: %p\n", pages, buddy_allocator.pages);

    memset(buddy_allocator.pages, 0, pages * sizeof(struct page));

    /* All pages start as INVALID. As the arch init code goes, it will call
     * 'free' on any pages which are valid to use. */
    p = buddy_allocator.pages + pages;
    while (p-- >= buddy_allocator.pages) {
        p->order = -1;
        p->page_number = (int)(p - buddy_allocator.pages);
        list_node_init(&p->page_list_node);
        bit_set(&p->flags, PG_INVALID);
        p->virt = P2V((p->page_number) << PG_SHIFT);
    }

    for (i = 0; i < PALLOC_MAPS; i++) {
        list_head_init(&buddy_allocator.maps[i].free_pages);
        wait_queue_init(&buddy_allocator.maps[i].wait_for_free);
        buddy_allocator.maps[i].free_count = 0;
    }
}

