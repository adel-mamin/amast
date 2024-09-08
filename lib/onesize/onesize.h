/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
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

#ifndef ONESIZE_H_INCLUDED
#define ONESIZE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** onesize memory allocator descriptor */
struct onesize {
    struct blk pool;   /**< the pool */
    int block_size;    /**< maximum size of allocated block [bytes] */
    struct a1slist fl; /**< list of non-allocated memory blocks (free list) */
    int nfree;         /**< current number of blocks in free list */
    int ntotal;        /**< total number of blocks */
    int minfree;       /**< minimum number of blocks in free list */
};

/**
 * Initializes a new onesize allocator.
 * Allocation requests up to block_size bytes are
 * rounded up to block_size bytes and served from a singly-linked
 * list of buffers. Due to the simplicity of onesize allocator
 * management, allocations from it are fast.
 *
 * @param hnd         the allocator
 * @param pool        the memory pool
 * @param block_size  the maximum size of the memory block the allocator
 *                    can allocate [bytes]. The allocation of memory blocks
 *                    bigger that this size will fail.
 * @param alignment   the alignment of allocated memory blocks [bytes]
 */
void onesize_init(
    struct onesize *hnd, struct blk *pool, int block_size, int alignment
);

/**
 * Allocate memory if \p size is <= block_size. The block at the front
 * of the free list is removed from the list and returned.
 * Otherwise NULL is returned.
 *
 * @param hnd   the allocator
 * @param size  amount of memory to allocate [bytes]
 * @return the allocated memory or NULL, if allocation failed
 */
void *onesize_allocate(struct onesize *hnd, int size);

/**
 * Free a memory block.
 * Insert the block at the front of the free list.
 *
 * @param hnd  the allocator
 * @param ptr  memory block to free
 */
void onesize_free(struct onesize *hnd, const void *ptr);

/**
 * Reclaim all memory allocated so far.
 * @param hnd  the allocator
 */
void onesize_free_all(struct onesize *hnd);

/**
 * The type of callback to be used with onesize_iterate_over_allocated()
 * @param ctx    the caller's context
 * @param index  the buffer index
 * @param buf    the buffer pointer
 * @param size   the buffer size [bytes]
 */
typedef void (*onesize_iterate_func)(
    void *ctx, int index, const char *buf, int size
);

/**
 * Iterate over allocated memory blocks with a provided callback function.
 *
 * Could be used for inspection of allocated memory for debugging.
 * @param hnd  the allocator
 * @param num  the number of allocated blocks to interate over
 * @param ctx  the caller's specific context to be used with the callback
 * @param cb   the callback to call for each allocated memory block
 */
void onesize_iterate_over_allocated(
    struct onesize *hnd, int num, void *ctx, onesize_iterate_func cb
);

/**
 * Returns the number of free blocks available for allocation.
 * @param hnd  the allocator
 * @return the number of free blocks
 */
int onesize_get_nfree(struct onesize *hnd);

/**
 * The minimum number of free memory blocks of size block_size available so far.
 * Could be used to assess the usage of the underlying memory pool.
 * @param hnd  the allocator
 * @return the minimum number of blocks of size block_size available so far
 */
int onesize_get_min_nfree(struct onesize *hnd);

/**
 * Returns the memory block size.
 * @param hnd  the allocator
 * @return the block size [bytes]
 */
int onesize_get_block_size(struct onesize *hnd);

/**
 * Get total number of memory blocks - the total capacity of the allocator.
 * @return the total number of blocks
 */
int onesize_get_nblocks(struct onesize *hnd);

#ifdef __cplusplus
}
#endif

#endif /* ONESIZE_H_INCLUDED */
