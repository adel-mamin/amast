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
 * Event pools API implementation.
 */

#include <string.h>

#include "common/alignment.h"
#include "common/macros.h"
#include "onesize/onesize.h"
#include "event_common.h"
#include "event_pool.h"

void am_event_alloc_init(struct am_event_alloc* alloc) {
    memset(alloc, 0, sizeof(*alloc));
}

void am_event_alloc_add_pool(
    struct am_event_alloc* alloc,
    void* pool,
    int size,
    int block_size,
    int alignment
) {
    AM_ASSERT(alloc->npools < AM_EVENT_POOLS_NUM_MAX);
    if (alloc->npools > 0) {
        int prev_size =
            am_onesize_get_block_size(&alloc->pools[alloc->npools - 1]);
        AM_ASSERT(block_size > prev_size);
    }

    struct am_onesize_cfg cfg = {
        .pool = {.ptr = pool, .size = size},
        .block_size = block_size,
        .alignment = alignment
    };
    am_onesize_ctor(&alloc->pools[alloc->npools], &cfg);

    ++alloc->npools;
}

int am_event_alloc_get_nfree_min(struct am_event_alloc* alloc, int index) {
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < alloc->npools);

    am_event_crit_enter();
    int nfree = am_onesize_get_nfree_min(&alloc->pools[index]);
    am_event_crit_exit();

    return nfree;
}

int am_event_alloc_get_nfree(struct am_event_alloc* alloc, int index) {
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < alloc->npools);

    am_event_crit_enter();
    int nfree = am_onesize_get_nfree(&alloc->pools[index]);
    am_event_crit_exit();

    return nfree;
}

int am_event_alloc_get_nblocks(struct am_event_alloc* alloc, int index) {
    AM_ASSERT(index >= 0);
    AM_ASSERT(index < alloc->npools);

    return am_onesize_get_nblocks(&alloc->pools[index]);
}

int am_event_alloc_get_num(const struct am_event_alloc* alloc) {
    return alloc->npools;
}

/**
 * A helper callback of type \ref am_onesize_iterate_fn.
 *
 * Expected to be used together with am_onesize_iterate_over_allocated()
 *
 * @param ctx    an instance of struct am_event_log_ctx
 * @param index  the index of event buffer to log
 * @param buf    the event buffer to log
 * @param size   the size of event buffer to log [bytes]
 */
static void am_event_log_cb(void* ctx, int index, const char* buf, int size) {
    AM_ASSERT(ctx);
    AM_ASSERT(buf);
    AM_ASSERT(AM_ALIGNOF_PTR(buf) >= AM_ALIGNOF(am_event_t));
    AM_ASSERT(size >= (int)sizeof(struct am_event));

    struct am_event_log_ctx* log = (struct am_event_log_ctx*)ctx;
    AM_ASSERT(log->cb);
    AM_ASSERT(log->pool_ind >= 0);

    const struct am_event* event = AM_CAST(const struct am_event*, buf);
    log->cb(log->pool_ind, index, event, size);
}

void am_event_alloc_log_unsafe(
    struct am_event_alloc* alloc, int num, am_event_log_fn cb
) {
    AM_ASSERT(num != 0);
    AM_ASSERT(cb);

    struct am_event_log_ctx ctx = {.cb = cb};
    for (int i = 0; i < alloc->npools; ++i) {
        ctx.pool_ind = i;
        am_onesize_iterate_over_allocated_unsafe(
            &alloc->pools[i], num, am_event_log_cb, &ctx
        );
    }
}
