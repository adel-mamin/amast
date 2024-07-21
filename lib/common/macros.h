/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2018 Adel Mamin
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
 * Commonly used macros.
 */

#include <stdbool.h>
#include <assert.h>

#ifndef COMMON_MACROS_H_INCLUDED
#define COMMON_MACROS_H_INCLUDED

/** Returns number of elements in the array */
#define ARRAY_SIZE(p) ((int)(sizeof(p) / sizeof((p)[0])))

/** Returns maximum element */
#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
/** Returns minimum element */
#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
/** Returns absolute value */
#define ABS(x) (((x) >= 0) ? (x) : -(x))

#undef ASSERT

/** Assert macro */
#define ASSERT(x) assert(LIKELY(x))

/** Checks if #x is a power of two */
#define IS_POWER_OF_TWO(x) (0 == (((x) - 1u) & (x)))

/** Check if a floating point number is NaN */
#define ISNAN(x) ((x) != (x))

/**
   Determines whether the memory architecture of current processor is
   little-endian.

   @retval true  Little-endian
   @retval false Not little-endian
 */
#define IS_LITTLE_ENDIAN() (((*(char *)"21") & 0xFF) == '2')

/**
   Determines whether the memory architecture of current processor is
   big-endian.

   @retval true  Big-endian
   @retval false Not big-endian
 */
#define IS_BIG_ENDIAN() (((*(char *)"21") & 0xFF) == '1')

/** Convert degrees to radians */
#define DEG2RAD(x) ((x) * (PI / 180.))

/** Convert radians to degrees */
#define RAD2DEG(x) ((x) * (180. / PI))

/** Do division and round up the result */
#define DIVIDE_ROUND_UP(divident, divisor) \
    (((divident) + ((divisor) - 1)) / (divisor))

#define ROUND_UP_TO_MULTIPLE_OF(n, m) (DIVIDE_ROUND_UP(n, m) * (m))

/** Return number of bits in the representation of the given parameter */
#define BITS_IN_REPRESENTATION(x) ((int)(sizeof(x) * CHAR_BIT))

/** A value with a magnitude of to and the sign of from */
#define COPYSIGN(to, from) (((from) > 0) ? (to) : -(to))

#define STRINGIFY(s) #s
#define JOINSTR(x, y) STRINGIFY(x##y)

/** Taken from http://nullprogram.com/blog/2015/02/17/ */
#define CONTAINER_OF(ptr, type, member) \
    (CAST(type *, ((char *)(ptr) - offsetof(type, member)))) /* NOLINT */

/** Counts the number of trailing zeros in a word */
#define COUNT_TRAILING_ZEROS(word)              \
    EXTENSION({                                 \
        int ret__;                              \
        (word) = ((word) ^ ((word) - 1)) >> 1u; \
        for (ret__ = 0; (word); ret__++) {      \
            (word) >>= 1u;                      \
        }                                       \
        ret__;                                  \
    })

/** Example: int i = 0; DO_EVERY(2, i++;);
    call N | i
    -------+--
    0      | 0
    1      | 1
    2      | 1
    3      | 2
*/
#define DO_EVERY(cnt, cmd)         \
    do {                           \
        static unsigned cnt__ = 0; \
        cnt__++;                   \
        cnt__ %= (unsigned)(cnt);  \
        if (0 == cnt__) {          \
            cmd;                   \
        }                          \
    } while (0)

/** Example: int i = 0; DO_ONCE(2, i++;);
    call N | i
    -------+--
    0      | 1
    1      | 1
    2      | 1
    3      | 1
*/
#define DO_ONCE(cmd)           \
    do {                       \
        static int done__ = 0; \
        if (!done__) {         \
            done__ = 1;        \
            cmd;               \
        }                      \
    } while (0)

#define DO_EACH_MS(ms, cmd)                       \
    do {                                          \
        static struct timer timer__ = TIMER_INIT; \
        if (!timer_armed(&timer__)) {             \
            timer_arm_timeout_ms(&timer__, (ms)); \
        }                                         \
        if (timer_expired(&timer__)) {            \
            timer_arm_timeout_ms(&timer__, (ms)); \
            cmd;                                  \
        }                                         \
    } while (0)

#endif /* COMMON_MACROS_H_INCLUDED */
