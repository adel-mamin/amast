/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 *
 * Source: https://github.com/adel-mamin/amast
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 * onesize memory allocator interface
 */

#ifndef AM_ONESIZE_H_INCLUDED
#define AM_ONESIZE_H_INCLUDED

#include "common/macros.h"
#include "blk/blk.h"
#include "slist/slist.h"

#define AM_POOL_BLOCK_SIZEOF(t) AM_MAX(sizeof(struct am_slist), sizeof(t))
#define AM_POOL_BLOCK_ALIGNMENT(a) AM_MAX(AM_ALIGNOF_SLIST, a)

#ifdef __cplusplus
extern "C" {
#endif

/** onesize memory allocator descriptor */
struct am_onesize {
    struct am_blk pool; /**< the pool */
    int block_size;     /**< maximum size of allocated block [bytes] */
    struct am_slist fl; /**< list of non-allocated memory blocks (free list) */
    int nfree;          /**< current number of blocks in free list */
    int ntotal;         /**< total number of blocks */
    int minfree;        /**< minimum number of blocks in free list */
    /** Enter critical section */
    void (*crit_enter)(void);
    /** Exit critical section */
    void (*crit_exit)(void);
};

/** Onesize configuration. */
struct am_onesize_cfg {
    /** the memory pool */
    struct am_blk *pool;
    /**
     * The maximum size of the memory block the allocator
     * can allocate [bytes]. The allocation of memory blocks
     * bigger that this size will fail.
     */
    int block_size;
    /** The alignment of allocated memory blocks [bytes] */
    int alignment;
    /** Enter critical section */
    void (*crit_enter)(void);
    /** Exit critical section */
    void (*crit_exit)(void);
};

/**
 * Construct a new onesize allocator.
 * Allocation requests up to block_size bytes are
 * rounded up to block_size bytes and served from a singly-linked
 * list of buffers. Due to the simplicity of onesize allocator
 * management, allocations from it are fast.
 *
 * @param hnd  the allocator
 * @param cfg  configuration
 */
void am_onesize_ctor(struct am_onesize *hnd, struct am_onesize_cfg *cfg);

/**
 * Allocate memory if \p size is <= block_size. The block at the front
 * of the free list is removed from the list and returned.
 * Otherwise NULL is returned.
 *
 * @param hnd   the allocator
 * @param size  amount of memory to allocate [bytes]
 * @return the allocated memory or NULL, if allocation failed
 */
void *am_onesize_allocate(struct am_onesize *hnd, int size);

/**
 * Free a memory block.
 * Insert the block at the front of the free list.
 *
 * @param hnd  the allocator
 * @param ptr  memory block to free
 */
void am_onesize_free(struct am_onesize *hnd, const void *ptr);

/**
 * Reclaim all memory allocated so far.
 * @param hnd  the allocator
 */
void am_onesize_free_all(struct am_onesize *hnd);

/**
 * The type of callback to be used with am_onesize_iterate_over_allocated()
 * @param ctx    the caller's context
 * @param index  the buffer index
 * @param buf    the buffer pointer
 * @param size   the buffer size [bytes]
 */
typedef void (*am_onesize_iterate_func)(
    void *ctx, int index, const char *buf, int size
);

/**
 * Iterate over allocated memory blocks with a provided callback function.
 *
 * Could be used for inspection of allocated memory for debugging.
 * @param hnd  the allocator
 * @param num  the number of allocated blocks to iterate over
 * @param ctx  the caller's specific context to be used with the callback
 * @param cb   the callback to call for each allocated memory block
 */
void am_onesize_iterate_over_allocated(
    struct am_onesize *hnd, int num, void *ctx, am_onesize_iterate_func cb
);

/**
 * Returns the number of free blocks available for allocation.
 * @param hnd  the allocator
 * @return the number of free blocks
 */
int am_onesize_get_nfree(const struct am_onesize *hnd);

/**
 * The minimum number of free memory blocks of size block_size available so far.
 * Could be used to assess the usage of the underlying memory pool.
 * @param hnd  the allocator
 * @return the minimum number of blocks of size block_size available so far
 */
int am_onesize_get_min_nfree(const struct am_onesize *hnd);

/**
 * Returns the memory block size.
 * @param hnd  the allocator
 * @return the block size [bytes]
 */
int am_onesize_get_block_size(const struct am_onesize *hnd);

/**
 * Get total number of memory blocks - the total capacity of the allocator.
 * @return the total number of blocks
 */
int am_onesize_get_nblocks(const struct am_onesize *hnd);

#ifdef __cplusplus
}
#endif

#endif /* AM_ONESIZE_H_INCLUDED */
