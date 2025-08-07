/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 * Copyright (c) Martin Sustrik
 * Copyright (c) William Ahern
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
 * string utilities API implementation
 */

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>

#include "common/macros.h"
#include "strlib/strlib.h"

int str_lcpy(char *dst, const char *src, int lim) {
    char *d = dst;
    const char *e = &dst[lim];
    const char *s = src;

    if (d < e) {
        do {
            if ('\0' == (*d++ = *s++)) {
                return (int)(s - src - 1);
            }
        } while (d < e);

        d[-1] = '\0';
    }

    while (*s++ != '\0') {
    }

    return (int)(s - src - 1);
}

int str_lcat(char *dst, const char *src, int lim) {
    AM_ASSERT(lim > 0);

    char *d = memchr(dst, '\0', (size_t)lim);
    const char *e = &dst[lim];
    const char *s = src;
    const char *p;

    if (d && (d < e)) {
        do {
            if ('\0' == (*d++ = *s++)) {
                return (int)(d - dst - 1);
            }
        } while (d < e);

        d[-1] = '\0';
    }

    p = s;

    while (*s++ != '\0') {
    }

    return (int)(lim + (s - p - 1));
}

int str_vlcatf(char *dst, int lim, const char *fmt, va_list ap) {
    AM_ASSERT(dst);
    AM_ASSERT(lim > 0);
    AM_ASSERT(fmt);

    long len = (int)strlen(dst);
    AM_ASSERT(len <= lim);

    len += vsnprintf(dst + len, (size_t)(lim - len), fmt, ap); /* NOLINT */

    AM_ASSERT(len <= INT_MAX);

    return (int)len;
}
