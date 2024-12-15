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
 *
 * Ring buffer API implementation.
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "common/macros.h"
#include "ringbuf/ringbuf.h"

void am_ringbuf_ctor(struct am_ringbuf_desc *desc, void *buf, int buf_size) {
    AM_ASSERT(desc);
    memset(desc, 0, sizeof(*desc));
    desc->buf = buf;
    desc->buf_size = buf_size;
}

int am_ringbuf_get_read_ptr(const struct am_ringbuf_desc *desc, uint8_t **ptr) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */
    AM_ASSERT(ptr);

    int rd = desc->read_offset;
    int rds = AM_ATOMIC_LOAD_N(&desc->read_skip);
    AM_ASSERT((rd + rds) <= desc->buf_size);
    int wr = AM_ATOMIC_LOAD_N(&desc->write_offset);

    if (rd == wr) {
        *ptr = NULL;
        return 0;
    }
    *ptr = &desc->buf[rd];
    if (rd > wr) {
        return desc->buf_size - rd - rds;
    }
    return wr - rd;
}

int am_ringbuf_get_write_ptr(struct am_ringbuf_desc *desc, uint8_t **ptr, int size) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */
    AM_ASSERT(ptr);
    AM_ASSERT(size <= (desc->buf_size - 1));

    int rd = AM_ATOMIC_LOAD_N(&desc->read_offset);
    int wr = desc->write_offset;
    if (wr >= rd) {
        int avail = (0 == wr) ? desc->buf_size - 1 : desc->buf_size - wr;
        if (avail >= size) {
            *ptr = &desc->buf[wr];
            AM_ATOMIC_STORE_N(&desc->read_skip, 0);
            return avail;
        }
        AM_ATOMIC_STORE_N(&desc->read_skip, avail);
        AM_ATOMIC_STORE_N(&desc->write_offset, 0);
        wr = 0;
    }
    AM_ASSERT(wr < rd);
    int avail = rd - wr - 1;
    if (avail >= size) {
        *ptr = &desc->buf[wr];
        return avail;
    }
    *ptr = NULL;
    return 0;
}

void am_ringbuf_flush(struct am_ringbuf_desc *desc, int offset) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */

    int rd = AM_ATOMIC_LOAD_N(&desc->read_offset);
    int wr = desc->write_offset;
    if (wr >= rd) {
        int avail = (0 == wr) ? desc->buf_size - 1 : desc->buf_size - wr;
        AM_ASSERT(offset <= avail);
        wr = (wr + offset) % desc->buf_size;
    } else {
        int avail = rd - wr - 1;
        AM_ASSERT(offset <= avail);
        wr += offset;
    }
    AM_ATOMIC_STORE_N(&desc->write_offset, wr);
}

int am_ringbuf_seek(const struct am_ringbuf_desc *desc, int offset) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */
    (void)offset;
    return 0;
}

int am_ringbuf_get_data_size(const struct am_ringbuf_desc *desc) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */
    return 0;
}

int am_ringbuf_get_free_size(const struct am_ringbuf_desc *desc) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */
    return 0;
}

void am_ringbuf_add_dropped(const struct am_ringbuf_desc *desc, int dropped) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */
    (void)dropped;
}

int am_ringbuf_get_dropped(const struct am_ringbuf_desc *desc) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */
    return 0;
}

void am_ringbuf_clear_dropped(const struct am_ringbuf_desc *desc) {
    AM_ASSERT(desc);
    AM_ASSERT(desc->buf); /* was am_ringbuf_ctor() called? */
}

