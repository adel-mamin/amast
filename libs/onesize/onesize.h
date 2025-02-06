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
#include "common/types.h"
#include "slist/slist.h"

#define AM_POOL_BLOCK_SIZEOF(t) AM_MAX(sizeof(struct am_slist), sizeof(t))
#define AM_POOL_BLOCK_ALIGNMENT(a) AM_MAX(AM_ALIGNOF_SLIST, a)

/** onesize memory allocator descriptor */
struct am_onesize {
    struct am_blk pool; /**< the pool */
    int block_size;     /**< maximum size of allocated block [bytes] */
    struct am_slist fl; /**< list of non-allocated memory blocks (free list) */
    int nfree;          /**< current number of blocks in free list */
    int ntotal;         /**< total number of blocks */
    int nfree_min;      /**< minimum number of blocks in free list */
    /** Enter critical section */
    void (*crit_enter)(void);
    /** Exit critical section */
    void (*crit_exit)(void);
};

/** Onesize configuration. */
struct am_onesize_cfg {
    /** the memory pool */
    struct am_blk pool;
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Construct a new onesize allocator.
 *
 * Allocation requests up to block_size bytes are
 * rounded up to block_size bytes and served from a singly-linked
 * list of buffers. Due to the simplicity of onesize allocator
 * management, allocations from it are fast.
 *
 * @param hnd  the allocator
 * @param cfg  configuration
 */
void am_onesize_ctor(struct am_onesize *hnd, const struct am_onesize_cfg *cfg);

/**
 * Allocate memory block of struct of block_size (eXtended version).
 *
 * Checks if there are more free memory blocks available than \p margin.
 * If not, then returns NULL. Otherwise allocates memory block and returns it.o
 *
 * @param hnd     the allocator
 * @param margin  free memory blocks to remain available after the allocation
 *
 * @return the allocated memory or NULL, if allocation failed
 */
void *am_onesize_allocate_x(struct am_onesize *hnd, int margin);

/**
 * Allocate one memory block of struct am_onesize_cfg::block_size
 *
 * Asserts if no free memory block is available.
 *
 * @param hnd  the allocator
 *
 * @return the allocated memory
 */
void *am_onesize_allocate(struct am_onesize *hnd);

/**
 * Free a memory block.
 *
 * Inserts the block at the front of the free list.
 *
 * @param hnd  the allocator
 * @param ptr  memory block to free
 */
void am_onesize_free(struct am_onesize *hnd, const void *ptr);

/**
 * Reclaim all memory allocated so far.
 *
 * @param hnd  the allocator
 */
void am_onesize_free_all(struct am_onesize *hnd);

/**
 * The type of callback to be used with am_onesize_iterate_over_allocated()
 *
 * @param ctx    the caller's context
 * @param index  the buffer index
 * @param buf    the buffer pointer
 * @param size   the buffer size [bytes]
 */
typedef void (*am_onesize_iterate_fn)(
    void *ctx, int index, const char *buf, int size
);

/**
 * Iterate over allocated memory blocks with a provided callback function.
 *
 * Could be used for inspection of allocated memory for debugging.
 *
 * @param hnd  the allocator
 * @param num  the number of allocated blocks to iterate over
 * @param cb   the callback to call for each allocated memory block
 * @param ctx  the caller's specific context to be used with the callback
 */
void am_onesize_iterate_over_allocated(
    struct am_onesize *hnd, int num, am_onesize_iterate_fn cb, void *ctx
);

/**
 * Returns the number of free blocks available for allocation.
 *
 * @param hnd  the allocator
 *
 * @return the number of free blocks
 */
int am_onesize_get_nfree(const struct am_onesize *hnd);

/**
 * The minimum number of free memory blocks of size block_size available so far.
 *
 * Could be used to assess the usage of the underlying memory pool.
 *
 * @param hnd  the allocator
 *
 * @return the minimum number of blocks of size block_size available so far
 */
int am_onesize_get_nfree_min(const struct am_onesize *hnd);

/**
 * Returns the memory block size.
 *
 * @param hnd  the allocator
 *
 * @return the block size [bytes]
 */
int am_onesize_get_block_size(const struct am_onesize *hnd);

/**
 * Get total number of memory blocks - the total capacity of the allocator.
 *
 * @param hnd  the allocator
 *
 * @return the total number of blocks
 */
int am_onesize_get_nblocks(const struct am_onesize *hnd);

#ifdef __cplusplus
}
#endif

#endif /* AM_ONESIZE_H_INCLUDED */
