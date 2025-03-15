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
 * Ring buffer API implementation.
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "ringbuf/ringbuf.h"

void am_ringbuf_ctor(struct am_ringbuf *rb, void *buf, int buf_size) {
    AM_ASSERT(rb);
    AM_ASSERT(buf);
    AM_ASSERT(buf_size > 0);

    memset(rb, 0, sizeof(*rb));
    rb->buf = buf;
    rb->buf_size = buf_size;
}

int am_ringbuf_get_read_ptr(struct am_ringbuf *rb, uint8_t **ptr) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);
    AM_ASSERT(ptr);

    int rd = AM_ATOMIC_LOAD_N(&rb->read_offset);
    int wr = AM_ATOMIC_LOAD_N(&rb->write_offset);
    if (rd == wr) {
        *ptr = NULL;
        return 0;
    }
    *ptr = &rb->buf[rd];
    if (rd <= wr) {
        return wr - rd;
    }
    int rds = AM_ATOMIC_LOAD_N(&rb->read_skip);
    int avail = rb->buf_size - rd - rds;
    AM_ASSERT(avail >= 0);
    if (avail) {
        return avail;
    }
    AM_ATOMIC_STORE_N(&rb->read_offset, 0);
    rd = 0;
    if (rd == wr) {
        *ptr = NULL;
        return 0;
    }
    *ptr = &rb->buf[0];
    return wr;
}

int am_ringbuf_get_write_ptr(struct am_ringbuf *rb, uint8_t **ptr, int size) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);
    AM_ASSERT(ptr);
    AM_ASSERT(size >= 0);
    AM_ASSERT(size < rb->buf_size);

    int rd = AM_ATOMIC_LOAD_N(&rb->read_offset);
    int wr = AM_ATOMIC_LOAD_N(&rb->write_offset);
    if (wr >= rd) {
        int avail = (0 == rd) ? rb->buf_size - 1 - wr : rb->buf_size - wr;
        if (avail >= size) {
            *ptr = &rb->buf[wr];
            AM_ATOMIC_STORE_N(&rb->read_skip, 0);
            return avail;
        }
        if (rd <= size) {
            *ptr = NULL;
            return 0;
        }
        AM_ATOMIC_STORE_N(&rb->read_skip, avail);
        AM_ATOMIC_STORE_N(&rb->write_offset, 0);
        wr = 0;
    }
    AM_ASSERT(wr < rd);
    int avail = rd - wr - 1;
    if (avail >= size) {
        *ptr = &rb->buf[wr];
        return avail;
    }
    *ptr = NULL;
    return 0;
}

void am_ringbuf_flush(struct am_ringbuf *rb, int offset) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);
    AM_ASSERT(offset >= 0);

    int rd = AM_ATOMIC_LOAD_N(&rb->read_offset);
    int wr = AM_ATOMIC_LOAD_N(&rb->write_offset);
    if (wr >= rd) {
        int avail = (0 == rd) ? rb->buf_size - 1 - wr : rb->buf_size - wr;
        AM_ASSERT(offset <= avail);
        wr = (wr + offset) % rb->buf_size;
    } else {
        int avail = rd - wr;
        AM_ASSERT(offset < avail);
        wr += offset;
    }
    AM_ATOMIC_STORE_N(&rb->write_offset, wr);
}

void am_ringbuf_seek(struct am_ringbuf *rb, int offset) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);
    AM_ASSERT(offset >= 0);

    if (0 == offset) {
        return;
    }

    int rd = AM_ATOMIC_LOAD_N(&rb->read_offset);
    int wr = AM_ATOMIC_LOAD_N(&rb->write_offset);

    AM_ASSERT(rd != wr);

    if (rd > wr) {
        int rds = AM_ATOMIC_LOAD_N(&rb->read_skip);
        AM_ASSERT((rd + rds) <= rb->buf_size);
        int avail = rb->buf_size - rd - rds;
        AM_ASSERT(offset <= avail);
        rd += AM_MIN(offset, avail);
        rd %= rb->buf_size - rds;
    } else {
        int avail = wr - rd;
        AM_ASSERT(offset <= avail);
        rd += AM_MIN(offset, avail);
    }
    AM_ATOMIC_STORE_N(&rb->read_offset, rd);
}

int am_ringbuf_get_data_size(const struct am_ringbuf *rb) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);

    int rd = AM_ATOMIC_LOAD_N(&rb->read_offset);
    int wr = AM_ATOMIC_LOAD_N(&rb->write_offset);

    if (rd <= wr) {
        return wr - rd;
    }
    int rds = AM_ATOMIC_LOAD_N(&rb->read_skip);
    AM_ASSERT((rd + rds) <= rb->buf_size);
    return rb->buf_size - rd - rds + wr;
}

int am_ringbuf_get_free_size(const struct am_ringbuf *rb) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);

    int rd = AM_ATOMIC_LOAD_N(&rb->read_offset);
    int wr = AM_ATOMIC_LOAD_N(&rb->write_offset);
    if (wr >= rd) {
        return rb->buf_size - 1 - wr + rd;
    }
    return rd - wr - 1;
}

void am_ringbuf_add_dropped(struct am_ringbuf *rb, int dropped) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);
    AM_ASSERT(dropped >= 0);

    unsigned d = AM_ATOMIC_LOAD_N(&rb->dropped);
    d += (unsigned)dropped;
    AM_ATOMIC_STORE_N(&rb->dropped, d);
}

unsigned am_ringbuf_get_dropped(const struct am_ringbuf *rb) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);

    return AM_ATOMIC_LOAD_N(&rb->dropped);
}

void am_ringbuf_clear_dropped(struct am_ringbuf *rb) {
    AM_ASSERT(rb);
    AM_ASSERT(rb->buf);

    AM_ATOMIC_STORE_N(&rb->dropped, 0);
}
