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
 * Commonly used macros.
 */

#ifndef AM_MACROS_H_INCLUDED
#define AM_MACROS_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "common/compiler.h"

/**
 * Returns number of elements in the array.
 * Note two tricks:
 * 1. putting the array name in the index operator ('[]') instead of the 0.
 *    This is done in case the macro is mistakenly used in C++ code
 *    with an item that overloads operator[]().
 *    The compiler will complain instead of giving a bad result.
 * 2. if a pointer is mistakenly passed as the argument, the compiler will
 *    complain in some cases - specifically, if the pointer's size isn't evenly
 *    divisible by the size of the object the pointer points to.
 *    In that situation a divide-by-zero will cause the compiler to error out.
 */
#ifdef __cppcheck__
/* to avoid "Modulo of one is always equal to zero [moduloofone]" warnings */
#define AM_COUNTOF(arr) ((int)(sizeof(arr) / sizeof(0 [arr])))
#else
#define AM_COUNTOF(arr) \
    ((int)(sizeof(arr) / sizeof(0 [arr]) / !(sizeof(arr) % sizeof(0 [arr]))))
#endif /* __cppcheck__ */

/** Returns maximum element. */
#define AM_MAX(a, b) ((a) > (b) ? (a) : (b))
/** Returns minimum element. */
#define AM_MIN(a, b) ((a) < (b) ? (a) : (b))
/** Returns absolute value. */
#define AM_ABS(x) (((x) >= 0) ? (x) : -(x))

#ifdef __FILE_NAME__
#define AM_FILE_NAME __FILE_NAME__
#else
#define AM_FILE_NAME __FILE__
#endif /* __FILE_NAME__ */

/** defined in libs/pal */
AM_NORETURN void am_assert_failure(
    const char *assertion, const char *file, int line
);

/** Assert macro. */
#define AM_ASSERT(x) \
    AM_LIKELY(x) ? (void)(0) : am_assert_failure(#x, AM_FILE_NAME, __LINE__)

/** Checks if \p x is a power of two */
#define AM_IS_POW2(x) (0 == (((x) - 1u) & (x)))

/** Do division and round up the result. */
#define AM_DIV_CEIL(n, d) ((n) / (d) + ((n) % (d) != 0))

/** Taken from http://nullprogram.com/blog/2015/02/17/ */
#define AM_CONTAINER_OF(ptr, type, member) \
    (AM_CAST(type *, ((char *)(ptr) - offsetof(type, member)))) /* NOLINT */

/*
 * Taken from
 * https://stackoverflow.com/questions/3378560/how-to-disable-gcc-warnings-for-a-few-lines-of-code
 */

/** Stringification macro. */
#define AM_STRINGIFY_(s) #s
#define AM_STRINGIFY(s) AM_STRINGIFY_(s)
/** Join x and y together. */
#define AM_JOINSTR(x, y) AM_STRINGIFY(x##y)

/** AM_CONCAT() helper. */
#define AM_CONCAT_(a, b) a##b

/** Concatenate two tokens. */
#define AM_CONCAT(a, b) AM_CONCAT_(a, b)

/** Make a unique name. */
#define AM_UNIQUE(name) AM_CONCAT(name, __LINE__)

/** Context used with AM_DO_...() macros */
struct am_do_ctx {
    /** previous ms reading */
    union {
        uint32_t prev_ms;
        uint32_t call_cnt;
    } state;
    /** init done */
    unsigned char init_done : 1;
    /** action done */
    unsigned char done : 1;
};

/**
 * Example:
 *
 * @code{.c}
 * int i = 0;
 * for (int j = 0; j < 3; ++j) {
 *     AM_DO_EVERY(2) {
 *         ++i;
 *     }
 * }
 * @endcode
 *
 * Results:
 *
 * ```
 * iteration number| value of i
 * ----------------+-----------
 *   1             | 1
 *   2             | 1
 *   3             | 2
 *   4             | 2
 *   5             | 3
 *   6             | 3
 * ```
 *
 * Note:
 *
 * The macro requires explicit curly braces, when used
 * in the context of if, switch-case, for, while:
 *
 * E.g., this is correct:
 *
 * @code{.c}
 * if (smth) {
 *     AM_DO_EVERY(2, ctx) {
 *         smth
 *     }
 * }
 * @endcode
 *
 * This is NOT correct and will not compile:
 *
 * @code{.c}
 * if (smth)
 *     AM_DO_EVERY(2, ctx) {
 *         smth
 *     }
 * @endcode
 *
 * @param cnt  the count
 * @param ctx  the context
 */
#define AM_DO_EVERY(/*int*/ cnt, /*struct am_do_ctx*/ ctx)             \
    if (0 == ctx->init_done) {                                         \
        ctx->state.call_cnt = 0;                                       \
        ctx->init_done = 1;                                            \
    }                                                                  \
    int AM_UNIQUE(do_now) = (0 == ctx->state.call_cnt);                \
    ctx->state.call_cnt = (ctx->state.call_cnt + 1) % (unsigned)(cnt); \
    if (AM_UNIQUE(do_now))

/**
 * Example:
 *
 * @code{.c}
 * int i = 0;
 * for (int j = 0; j < 3; ++j) {
 *     AM_DO_ONCE(ctx) {
 *         ++i;
 *     }
 * }
 * @endcode
 *
 * Results:
 *
 * ```
 * iteration number | value of i
 * -----------------+-----------
 *   1              | 1
 *   2              | 1
 *   3              | 1
 * ```
 *
 * Note:
 *
 * The macro requires explicit curly braces, when used
 * in the context of if, switch-case, for, while:
 *
 * E.g., this is correct:
 *
 * @code{.c}
 * if (smth) {
 *     AM_DO_ONCE(ctx) {
 *         // smth
 *     }
 * }
 * @endcode
 *
 * This is NOT correct and will not compile:
 *
 * @code{.c}
 * if (smth)
 *     AM_DO_ONCE(ctx) {
 *         smth
 *     }
 * @endcode
 *
 * @param ctx  the context
 */
#define AM_DO_ONCE(/*struct am_do_ctx*/ ctx) if (!ctx->done && (ctx->done = 1))

/**
 * Execute code in the attached scope immediately and
 * then repeatedly every \p ms milliseconds.
 *
 * Note:
 *
 * The macro requires explicit curly braces, when used
 * in the context of if, switch-case, for, while:
 *
 * E.g., this is correct:
 *
 * @code{.c}
 * if (smth) {
 *     AM_DO_EACH_MS(5, ctx, now_ms) {
 *         // smth
 *     }
 * }
 * @endcode
 *
 * This is NOT correct and will not compile:
 *
 * @code{.c}
 * if (smth)
 *     AM_DO_EACH_MS(5, ctx, now_ms) {
 *         // smth
 *     }
 * @endcode
 *
 * Example:
 *
 * @code{.c}
 * AM_DO_EACH_MS(100, ctx, now_ms) {
 *     am_printf("Hello, world!\n");
 * }
 * @endcode
 *
 * @param each_ms  the interval
 * @param ctx      the context
 * @param now_ms   the current ms time reading
 */
#define AM_DO_EACH_MS(/*uint32_t*/ each_ms,                             \
                      /*struct am_do_ctx*/ ctx,                         \
                      /*uint32_t*/ now_ms)                              \
    if (!ctx->init_done) {                                              \
        ctx->init_done = 1;                                             \
        /* make sure to do the first time around */                     \
        ctx->state.prev_ms = now_ms - each_ms;                          \
    }                                                                   \
    if ((each_ms >= 0) && ((now_ms - ctx->state.prev_ms) >= each_ms) && \
        (ctx->state.prev_ms += each_ms, 1))

/** Test \p d1 and \p d2 for equality within \p tolerance. */
#define AM_DOUBLE_EQ(d1, d2, tolerance) (fabs((d1) - (d2)) <= (tolerance))

/** Test \p d1 and \p d2 for equality within \p tolerance. */
#define AM_FLOAT_EQ(d1, d2, tolerance) (fabsf((d1) - (d2)) <= (tolerance))

/*
 * The compile time assert code is taken from here:
 * http://stackoverflow.com/questions/3385515/static-assert-in-c
 */

/** Compile time assert helper */
#define AM_COMPILE_TIME_ASSERT3(cond, msg) \
    typedef char static_assertion_##msg[(cond) ? 1 : -1]
/** Compile time assert helper */
#define AM_COMPILE_TIME_ASSERT2(cond, line) \
    AM_COMPILE_TIME_ASSERT3(cond, static_assertion_at_line_##line)
/** Compile time assert helper */
#define AM_COMPILE_TIME_ASSERT(cond, line) AM_COMPILE_TIME_ASSERT2(cond, line)

/** Compile time assert */
#define AM_ASSERT_STATIC(cond) AM_COMPILE_TIME_ASSERT(cond, __LINE__)

/**
 * Cast pointer to type.
 *
 * @param TYPE  the type
 * @param PTR   the pointer
 */
#define AM_CAST(TYPE, PTR) (((TYPE)(uintptr_t)(const void *)(PTR)))

/**
 * Cast volatile pointer to type.
 *
 * @param TYPE  the type
 * @param PTR   the volatile pointer
 */
#define AM_VCAST(TYPE, PTR) (((TYPE)(uintptr_t)(const volatile void *)(PTR)))

/**
 * Choose one of two macros.
 *
 * Given:
 *
 * #define BAR1(a) (a)
 * #define BAR2(a, b) (a, b)
 * #define BAR(...) AM_GET_MACRO_2_(__VA_ARGS__, BAR2, BAR1)(__VA_ARGS__)
 *
 * Produces:
 *
 * BAR(a)    expands to BAR1(a)
 * BAR(a, b) expands to BAR2(a, b)
 */
#define AM_GET_MACRO_2_(_1, _2, NAME, ...) NAME

#endif /* AM_MACROS_H_INCLUDED */
