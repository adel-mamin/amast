/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017-2018 Adel Mamin
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
 * alignment API declaration
 */

#ifndef ALIGNMENT_H_INCLUDED
#define ALIGNMENT_H_INCLUDED

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L))
#include <stddef.h>
#define ALIGN_MAX ALIGNOF(max_align_t)
#else
#define ALIGN_MAX 16
#endif

/** Alignment of a type */
#define ALIGNOF(type)        \
    ((int)offsetof(          \
        struct {             \
            unsigned char c; \
            type d;          \
        },                   \
        d                    \
    ))

#define ALIGNOF_PTR(ptr) (1U << (unsigned)__builtin_ctz((uintptr_t)ptr))

#define ALIGN_PTR_UP(ptr, align)                              \
    ((void *)(((uintptr_t)(ptr) + (uintptr_t)((align) - 1)) & \
              ~(uintptr_t)((align) - 1)))

#define ALIGN_PTR_DOWN(ptr, align) \
    ((void *)((uintptr_t)(ptr) & ~(uintptr_t)((align) - 1)))

/** The byte difference between aligned and unaligned size */
#define ALIGN_SIZE(size, align)                          \
    ((int)(((unsigned)(size) + (unsigned)(align) - 1u) & \
           (unsigned)~(unsigned)((unsigned)(align) - 1u)))

#endif /* ALIGNMENT_H_INCLUDED */
