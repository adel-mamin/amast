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
 *
 * Ring buffer API declaration.
 */

#ifndef AM_RINGBUF_H_INCLUDED
#define AM_RINGBUF_H_INCLUDED

#include <stdint.h>

/**
 * Ring buffer descriptor.
 *
 * Only exposed to allow ring buffer users to allocate (possibly static) memory for it.
 */
struct am_ringbuf {
    /** dropped bytes counter */
    unsigned dropped;
    /** reading offset [bytes] */
    int read_offset;
    /** skip this many bytes at the tail of ring buffer */
    int read_skip;
    /** writing offset [bytes] */
    int write_offset;

    /** ring buffer */
    uint8_t *buf;
    /** ring buffer size [bytes] */
    int buf_size;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Construct ring buffer.
 *
 * @param rb        the ring buffer
 * @param buf       the ring buffer's memory
 * @param buf_size  the ring buffer's memory size [bytes]
 */
void am_ringbuf_ctor(struct am_ringbuf *rb, void *buf, int buf_size);

/**
 * Return ring buffer read data pointer.
 *
 * The caller can do anything with the data behind the read pointer.
 *
 * Call am_ringbuf_seek() to inform writer that the data reading is complete.
 * Until then writer will not be able to write to the memory.
 *
 * @param rb   the ring buffer
 * @param ptr  the read data pointer is returned here. Can be NULL.
 *
 * @return the byte size of the memory pointed to by \p ptr. Can be 0.
 */
int am_ringbuf_get_read_ptr(struct am_ringbuf *rb, uint8_t **ptr);

/**
 * Return ring buffer write data pointer.
 *
 * Anything can be done with the memory pointed to by the write pointer.
 *
 * Call am_ringbuf_flush() to inform reader that data writing is complete.
 * Until then reader will not be able to read the memory.
 *
 * @param rb    the ring buffer
 * @param ptr   the write data pointer is returned here. Can be NULL.
 * @param size  the requested memory size behind the write date pointer [bytes].
 *              Can be set to 0.
 *
 * @return the byte size of all the available memory pointed to by \p ptr.
 *         Can be 0 or more than the requested size.
 */
int am_ringbuf_get_write_ptr(struct am_ringbuf *rb, uint8_t **ptr, int size);

/**
 * Increase by offset bytes the amount of written data available to reader.
 *
 * Called once or many times after calling am_ringbuf_get_write_ptr().
 *
 * The offset or sum of offsets of multiple calls to this function must not
 * exceed the size returned by the last call to am_ringbuf_get_write_ptr().
 *
 * @param rb      the ring buffer
 * @param offset  the number of bytes of write data available to reader
 */
void am_ringbuf_flush(struct am_ringbuf *rb, int offset);

/**
 * Increase by offset bytes the amount of data available to writer.
 *
 * Called once or many times after calling am_ringbuf_get_read_ptr().
 *
 * The offset or sum of offsets of multiple calls to this function must not
 * exceed the size returned by the last call to am_ringbuf_get_read_ptr().
 *
 * @param rb      the ring buffer
 * @param offset  the number of bytes of memory available to writer
 */
void am_ringbuf_seek(struct am_ringbuf *rb, int offset);

/**
 * Return total number of data bytes available for reading.
 *
 * @param rb  the ring buffer
 *
 * @return the number of data bytes available for reading
 */
int am_ringbuf_get_data_size(const struct am_ringbuf *rb);

/**
 * Return total number of free memory bytes available for writing.
 *
 * @param rb  the ring buffer
 *
 * @return the number of memory bytes available for writing
 */
int am_ringbuf_get_free_size(const struct am_ringbuf *rb);

/**
 * Add to the number of dropped bytes.
 *
 * @param rb       the ring buffer
 * @param dropped  add this many dropped bytes
 */
void am_ringbuf_add_dropped(struct am_ringbuf *rb, int dropped);

/**
 * Get the number of dropped bytes.
 *
 * @param rb  the ring buffer
 *
 * @return the number of dropped bytes
 */
unsigned am_ringbuf_get_dropped(const struct am_ringbuf *rb);

/**
 * Clear the number of dropped bytes.
 *
 * @param rb  the ring buffer
 */
void am_ringbuf_clear_dropped(struct am_ringbuf *rb);

#ifdef __cplusplus
}
#endif

#endif /* AM_RINGBUF_H_INCLUDED */
