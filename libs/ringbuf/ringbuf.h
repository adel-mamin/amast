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
 * Ring buffer API declaration.
 */

#ifndef AM_RINGBUF_H_INCLUDED
#define AM_RINGBUF_H_INCLUDED

struct am_ringbuf_desc {
    unsigned status;
    int read_offset;
    int read_skip;
    int write_offset;

    unsigned char *buf;
    int buf_size;
};

#ifdef __cplusplus
extern "C" {
#endif

void am_ringbuf_ctor(struct am_ringbuf_desc *desc, void *buf, int buf_size);

int am_ringbuf_get_read_ptr(const struct am_ringbuf_desc *desc, unsigned char **ptr);
int am_ringbuf_get_write_ptr(struct am_ringbuf_desc *desc, unsigned char **ptr, int size);
int am_ringbuf_flush(const struct am_ringbuf_desc *desc, int offset);
int am_ringbuf_seek(const struct am_ringbuf_desc *desc, int offset);

int am_ringbuf_get_data_size(const struct am_ringbuf_desc *desc);
int am_ringbuf_get_free_size(const struct am_ringbuf_desc *desc);

void am_ringbuf_add_dropped(const struct am_ringbuf_desc *desc, int dropped);
int am_ringbuf_get_dropped(const struct am_ringbuf_desc *desc);
void am_ringbuf_clear_dropped(const struct am_ringbuf_desc *desc);

#ifdef __cplusplus
}
#endif

#endif /* AM_RINGBUF_H_INCLUDED */
