/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 * Copyright (c) Martin Sustrik
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
 * Compiler specific defines and macros.
 * Inspired by:
 * http://stackoverflow.com/questions/1786257/properly-handling-compiler-specifics-unix-windows-in-c
 */

#ifndef AM_COMPILER_H_INCLUDED
#define AM_COMPILER_H_INCLUDED

#include <limits.h>

#define AM_COMPILER_UNKNOWN 0  /*!< Compiler is unknown */
#define AM_COMPILER_GCC 1000   /*!< GCC compiler */
#define AM_COMPILER_CLANG 2000 /*!< Clang compiler */

#ifndef AM_COMPILER_ID
/* Taken from
 * http://nadeausoftware.com/articles/2012/10/c_c_tip_how_detect_compiler_name_and_version_using_compiler_predefined_macros
 */
#if defined(__clang__)

/* Clang */
#define AM_COMPILER_VERSION \
    (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#define AM_COMPILER_ID AM_COMPILER_CLANG

#elif defined(__ICC) || defined(__INTEL_COMPILER)

/* Intel ICC/ICPC. */
#elif defined(__GNUC__) || defined(__GNUG__)
#define AM_COMPILER_VERSION \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define AM_COMPILER_ID AM_COMPILER_GCC

#elif defined(__HP_cc) || defined(__HP_aCC)
/* Hewlett-Packard C/C++ */
#elif defined(__IBMC__) || defined(__IBMCPP__)
/* IBM XL C/C++ */
#elif defined(_MSC_VER)
/* Microsoft Visual Studio */
#elif defined(__PGI)
/* Portland Group PGCC/PGCPP */
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
/* Oracle Solaris Studio */
#elif defined(__cppcheck__)
/* cppcheck run */
#define AM_COMPILER_ID AM_COMPILER_GCC
#else
#error "Unknown compiler!"
#endif
#endif /* AM_COMPILER_ID */

/* Optimization hints. */
#if (AM_COMPILER_ID == AM_COMPILER_GCC) || (AM_COMPILER_ID == AM_COMPILER_CLANG)
/** the condition x is likely */
#define AM_LIKELY(x) __builtin_expect(!!(x), 1)
/** the condition x is unlikely */
#define AM_UNLIKELY(x) __builtin_expect(!!(x), 0)
/** no return compiler instruction */
#define AM_NORETURN __attribute__((noreturn))
/** alignment compiler instruction */
#define AM_ALIGNED(x) __attribute__((aligned(x)))

/**
 * The parameter `si` specifies which argument is the format string argument
 * (starting from 1), while `ftc` is the 1-based index of the first argument
 * to check against the format string.
 *
 * For functions where the arguments are not available to be checked
 * (such as vprintf), specify the third parameter as zero.
 * In this case the compiler only checks the format string for consistency.
 */
#define AM_PRINTF(si, ftc) __attribute__((format(printf, (si), (ftc))))
#else
#error "Define macros"
#endif

/** Atomic store operation. */
#define AM_ATOMIC_STORE_N(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)

/** Atomic load operation. */
#define AM_ATOMIC_LOAD_N(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)

/** Atomic fetch and add operation. */
#define AM_ATOMIC_FETCH_ADD(ptr, val) \
    __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST)

#endif /* AM_COMPILER_H_INCLUDED */
