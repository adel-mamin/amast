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
 * Alignment API declaration.
 */

#ifndef AM_ALIGNMENT_H_INCLUDED
#define AM_ALIGNMENT_H_INCLUDED

#include <stddef.h>

/**
 * Use this macro before using AM_ALIGNOF() for the same type.
 *
 * @param type  the type to define. Must be one word.
 *              For example, "struct foo" will not work,
 *              but foo_t will, where "typedef struct foo foo_t".
 */
#define AM_ALIGNOF_DEFINE(type)    \
    struct alignof_helper_##type { \
        char c;                    \
        type member;               \
    }

/** The maximum compiler alignment [bytes]. */
#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L))
AM_ALIGNOF_DEFINE(max_align_t);
#define AM_ALIGN_MAX AM_ALIGNOF(max_align_t)
#else
#define AM_ALIGN_MAX 16
#endif

/**
 * Type alignment.
 *
 * Must be preceded by AM_ALIGNOF_DEFINE() for the same type.
 *
 * @param type  return the alignment of this type [bytes]
 *
 * @return the type alignment [bytes]
 */
#define AM_ALIGNOF(type) (int)offsetof(struct alignof_helper_##type, member)

/**
 * Pointer alignment.
 *
 * @param ptr  return the alignment of this pointer [bytes]
 *
 * @return the pointer alignment [bytes]
 */
#define AM_ALIGNOF_PTR(ptr) \
    ((int)(1U << (unsigned)__builtin_ctzl(AM_CAST(uintptr_t, ptr))))

/**
 * Align pointer to the bigger value, which has \p align alignment.
 *
 * @param ptr    the pointer to align
 * @param align  the alignment [bytes]. Must be power of 2.
 *
 * @return the aligned pointer
 */
#define AM_ALIGN_PTR_UP(ptr, align)                           \
    ((void *)(((uintptr_t)(ptr) + (uintptr_t)((align) - 1)) & \
              ~(uintptr_t)((align) - 1)))

/**
 * Align pointer to the smaller value, which has \p align alignment.
 *
 * @param ptr    the pointer to align
 * @param align  the alignment [bytes]. Must be power of 2.
 *
 * @return the aligned pointer
 */
#define AM_ALIGN_PTR_DOWN(ptr, align) \
    ((void *)((uintptr_t)(ptr) & ~(uintptr_t)((align) - 1)))

/**
 * Return \p size + the byte difference between aligned and unaligned \p size.
 *
 * For example,
 *
 * `AM_ALIGN_SIZE(3, 4) == 4`
 *
 * `AM_ALIGN_SIZE(4, 4) == 4`
 *
 * `AM_ALIGN_SIZE(5, 4) == 8`
 */
#define AM_ALIGN_SIZE(size, align) \
    (((size_t)(size) + (size_t)(align) - 1) & ~(size_t)((align) - 1))

#endif /* AM_ALIGNMENT_H_INCLUDED */
