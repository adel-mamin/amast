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
 * Ring buffer unit tests.
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "common/macros.h"
#include "ringbuf/ringbuf.h"

static void test_ringbuf_1(void) {
    struct am_ringbuf rb;
    uint8_t buf[8];
    uint8_t data[] = {1, 2, 3, 4};

    am_ringbuf_ctor(&rb, buf, AM_COUNTOF(buf));
    for (int i = 1; i <= AM_COUNTOF(data); ++i) {
        {
            int size = am_ringbuf_get_data_size(&rb);
            AM_ASSERT(0 == size);
        }
        {
            uint8_t *ptr = NULL;
            int size = am_ringbuf_get_write_ptr(&rb, &ptr, /*size=*/i);
            AM_ASSERT(size >= i);
            memcpy(ptr, data, (size_t)i);
            am_ringbuf_flush(&rb, /*offset=*/i);
        }
        {
            int size = am_ringbuf_get_data_size(&rb);
            AM_ASSERT(i == size);
        }
        {
            uint8_t *ptr = NULL;
            int size = am_ringbuf_get_read_ptr(&rb, &ptr);
            AM_ASSERT(i == size);
            uint8_t tmp[AM_COUNTOF(data)];
            memcpy(tmp, ptr, (size_t)size);
            AM_ASSERT(0 == memcmp(tmp, data, (size_t)size));
            am_ringbuf_seek(&rb, /*offset=*/size);
        }
    }
}

int main(void) {
    test_ringbuf_1();
    return 0;
}
