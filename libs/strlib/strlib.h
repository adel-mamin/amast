/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 * Copyright (c) Martin Sustrik
 * Copyright (c) William Ahern
 * Copyright (c) Przemo Nowaczyk <pnowaczyk.mail@gmail.com>
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
 * String utilities API
 */

#ifndef AM_STRLIB_H_INCLUDED
#define AM_STRLIB_H_INCLUDED

#include <stdarg.h>

#include "common/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Copy strings.
 *
 * The custom implementation of strlcpy() from BSD systems.
 * Designed to be safer, more consistent, and less error prone
 * replacement for strncpy. It takes the full size of the buffer (not just the
 * length) and guarantee to NUL-terminate the result (as long as size is larger
 * than 0.
 * Note that a byte for the NUL should be included in size.  Also
 * note that it only operates on true “C” strings.  This
 * means that src must be NUL-terminated.
 *
 * It copies up to size - 1 characters from the
 * NUL-terminated string src to dst, NUL-terminating the result.
 *
 * @param dst  the destination string buffer
 * @param src  the source string buffer
 * @param lim  the full size of dst buffer [bytes]
 *
 * @return the total length of the string it tried to create, i.e.
 *         the length of src
 */
int str_lcpy(char *dst, const char *src, int lim);

/**
 * Concatenate strings.
 *
 * It is designed to be safer, more consistent, and less
 * error prone replacement for strncat.
 * It takes the full size of the buffer (not just the length)
 * and guarantees to NUL-terminate the result (as long as there is at
 * least one byte free in dst). Note that a byte for the NUL should be
 * included in size.  Also it only operates on true “C” strings.
 * This means that both src and dst must be NUL-terminated.
 *
 * It appends the NUL-terminated string src to the end of dst.
 * It will append at most lim - strlen(dst) - 1 bytes, NUL-terminating
 * the result.
 *
 * Note, however, that if it traverses size characters without finding
 * a NUL, the length of the string is considered to be size and the
 * destination string will not be NUL-terminated (since there was no space
 * for the NUL). This keeps it from running off the end of a string.
 * In practice this should not happen (as it means that either size is
 * incorrect or that dst is not a proper “C” string).
 * The check exists to prevent potential security problems in incorrect code.
 *
 * @param dst  the destination buffer
 * @param src  the source buffer
 * @param lim  the full size of the buffer (not just the length) [bytes]
 *
 * @return the total length of the string it tried to create
 *         It means the initial length of dst plus the length of src.
 */
int str_lcat(char *dst, const char *src, int lim);

/**
 * Same as str_lcat(), but the source buffer is replaced with format string.
 *
 * @param dst  the destination buffer
 * @param lim  the full size of the buffer (not just the length) [bytes]
 * @param fmt  the format string
 * @param ap   arguments to format
 *
 * @return Returns the total length of the string they tried to create.
 *         It means the initial length of dst plus the length of src.
 */
AM_PRINTF(3, 0) int str_vlcatf(char *dst, int lim, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* AM_STRLIB_H_INCLUDED */
