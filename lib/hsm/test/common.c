/*
 *  The MIT License (MIT)
 *
 * Copyright (c) 2020-2023 Adel Mamin
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"

#define TEST_LOG_SIZE 256 /* [bytes] */

char g_log_buf[TEST_LOG_SIZE];

int str_lcat(char *dst, const char *src, int lim) {
    ASSERT(lim > 0);

    char *d_ = (char *)memchr(dst, '\0', (size_t)lim);
    const char *e_ = &dst[lim];
    const char *s_ = src;

    if (d_ && (d_ < e_)) {
        do {
            /* NOLINTNEXTLINE(bugprone-assignment-in-if-condition) */
            if ('\0' == (*d_++ = *s_++)) {
                return (int)(d_ - dst - 1);
            }
        } while (d_ < e_);

        d_[-1] = '\0';
    }

    const char *p_ = s_;

    while (*s_++ != '\0') {
    }

    return (int)(lim + (s_ - p_ - 1));
}

/**
 * Same as str_lcat(), but the source buffer is replaced with format string.
 * @param dst the destination buffer.
 * @param lim the full size of the buffer (not just the length) [bytes].
 * @param fmt the format string.
 * @return Returns the total length of the string they tried to create.
 *         It means the initial length of dst plus the length of src.
 */
int str_vlcatf(char *dst, int lim, const char *fmt, va_list ap) {
    ASSERT(dst);
    ASSERT(lim > 0);
    ASSERT(fmt);

    long len = (int)strlen(dst);
    ASSERT(len <= lim);

    len += vsnprintf(dst + len, (size_t)(lim - len), fmt, ap); /* NOLINT */

    ASSERT(len <= INT_MAX);

    return (int)len;
}

/**
 * Same as str_lcat(), but the source buffer is replaced with format string.
 * @param dst the destination buffer.
 * @param lim the full size of the buffer (not just the length) [bytes].
 * @param fmt the format string.
 * @return Returns the total length of the string they tried to create.
 *         It means the initial length of dst plus the length of src.
 */
int str_lcatf(char *dst, int lim, const char *fmt, ...) {
    ASSERT(dst);
    ASSERT(lim > 0);
    ASSERT(fmt);

    long len = (int)strlen(dst);
    ASSERT(len <= lim);

    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(dst, lim, fmt, ap);
    va_end(ap);

    ASSERT(len <= INT_MAX);

    return (int)len;
}
