/*
  The MIT License (MIT)

  Copyright (c) 2015-2019 Adel Mamin

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 */

/*

  Copyright (c) 2018 Martin Sustrik

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

/**
 * @file
 * Compiler specific defines and macros.
 * Inspired by:
 * http://stackoverflow.com/questions/1786257/properly-handling-compiler-specifics-unix-windows-in-c
 */

#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include <limits.h>

#define COMPILER_UNKNOWN 0  /*!< Compiler is unknown */
#define COMPILER_GCC 1000   /*!< GCC compiler */
#define COMPILER_CLANG 2000 /*!< Clang compiler */

/* COMPILER was not needed anywhere so far */
/* #ifndef COMPILER */
/* #if defined(linux) || defined(__linux) || defined(__linux__) */
/* #define COMPILER COMPILER_LINUX */
/* #define COMPILER_NAME "linux" */
/* #else */
/* #error "Unsupported compiler!" */
/* #endif */
/* #endif /\* #ifndef COMPILER *\/ */

#ifndef COMPILER_BITS
#if (defined(__x86_64__) || defined(__64BIT__) || (defined(__WORDSIZE) && (__WORDSIZE == 64)))
#define COMPILER_BITS 64 /*!< Compiler native word size */
#define INT_BITS 32
#define LONG_BITS 64
#else
#define COMPILER_BITS 32 /*!< Compiler native word size */
#define INT_BITS 32      /*!< integer size in bits */
#define LONG_BITS 32     /*!< long size in bits */
#endif
#endif /* #ifndef COMPILER_BITS */

#ifndef COMPILER_ID
/* Taken from
 * http://nadeausoftware.com/articles/2012/10/c_c_tip_how_detect_compiler_name_and_version_using_compiler_predefined_macros
 */
#if defined(__clang__)

/* Clang */
#define COMPILER_VERSION \
    (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#define COMPILER_NAME "clang"
#define COMPILER_ID COMPILER_CLANG

#elif defined(__ICC) || defined(__INTEL_COMPILER)

/* Intel ICC/ICPC. */
#elif defined(__GNUC__) || defined(__GNUG__)
#define COMPILER_VERSION \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define COMPILER_NAME "gcc"
#define COMPILER_ID COMPILER_GCC

#elif defined(__HP_cc) || defined(__HP_aCC)
/* Hewlett-Packard C/aC++ */
#elif defined(__IBMC__) || defined(__IBMCPP__)
/* IBM XL C/C++ */
#elif defined(_MSC_VER)
/* Microsoft Visual Studio */
#elif defined(__PGI)
/* Portland Group PGCC/PGCPP */
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
/* Oracle Solaris Studio */
#else
#error "Unknown compiler!"
#endif
#endif /* #ifndef COMPILER_ID */

#if (64 == COMPILER_BITS)
typedef long ssize_t;
#define SSIZE_MAX LONG_MAX
#define SSIZE_MIN LONG_MINXS
#else
/** ssize_t definition */
typedef int ssize_t;
#define SSIZE_MAX INT_MAX /*!< ssize_t maximum value */
#define SSIZE_MIN INT_MIN /*!< ssize_t minimum value */
#endif                    /* #if (64 == COMPILER_BITS) */

/** do not inline instruction */
#define NOINLINE __attribute__((noinline))
#define SECTION(name) __attribute__((section(name))) /**< section name */

/* Optimisation hints. */
#if (COMPILER_ID == COMPILER_GCC) || (COMPILER_ID == COMPILER_CLANG)
/** the condition x is likely */
#define LIKELY(x) __builtin_expect(!!(x), 1)
/** the condition x is unlikely */
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
/** no return compiler instruction */
#define NORETURN __attribute__((noreturn))
/** alignment compiler instruction */
#define ALIGNED(x) __attribute__((aligned(x)))
#else
/** a stub */
#define LIKELY(x) (x)
/** a stub */
#define UNLIKELY(x) (x)
/** a stub */
#define NORETURN
/** a stub */
#define ALIGNED(x)
#endif

#if (COMPILER_ID == COMPILER_GCC) || (COMPILER_ID == COMPILER_CLANG)
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH /*!< explicit fallthrough compiler instruction*/
#endif

#if (COMPILER_ID == COMPILER_GCC) || (COMPILER_ID == COMPILER_CLANG)
/** safe cast operation */
#define CAST(TYPE, PTR) (((TYPE)(uintptr_t)(const void *)(PTR)))
#define VCAST(TYPE, PTR) (((TYPE)(uintptr_t)(const volatile void *)(PTR)))
#else
/** a stub */
#define CAST(PTR)
/** a stub */
#define VCAST(PTR)
#endif

/* Taken from
   https://stackoverflow.com/questions/3378560/how-to-disable-gcc-warnings-for-a-few-lines-of-code
 */

/** Stringification macro */
#define STRINGIFY__(s) #s
/** Join x and y together */
#define JOINSTR_(x, y) STRINGIFY__(x##y)

/*! @cond Doxygen_Suppress */
#ifdef _MSC_VER
#define DIAG_DO_PRAGMA(x) __pragma(#x)
#define DIAG_PRAGMA(compiler, x) DIAG_DO_PRAGMA(warning(x))
#else
#define DIAG_DO_PRAGMA(x) _Pragma(#x)
#define DIAG_PRAGMA(compiler, x) DIAG_DO_PRAGMA(compiler diagnostic x)
#endif
/*! @endcond */

#if ((COMPILER_ID == COMPILER_CLANG) || (COMPILER_ID == COMPILER_GCC))
/* clang-format off */
#define W_SHADOW shadow
#define W_FORMAT format
#define W_REDUNDANT_DECLS redundant-decls /* NOLINT */
#define W_MISSING_PROTOTYPES missing-prototypes /* NOLINT */
#define W_CAST_ALIGN cast-align /* NOLINT */
#define W_CAST_QUAL cast-qual /* NOLINT */
#define W_MISSING_NORETURN missing-noreturn /* NOLINT */
#define W_IMPLICIT_FUNCTION_DECLARATION error-implicit-function-declaration /* NOLINT */
#define W_STRICT_OVERFLOW strict-overflow /* NOLINT */
#define W_UNDEF undef /* NOLINT */
#define W_DISCARDED_QUALIFIERS discarded-qualifiers /* NOLINT */
#define W_WRITE_STRINGS write-strings /* NOLINT */
#define W_SWITCH_DEFAULT switch-default /* NOLINT */
#define W_SWITCH_ENUM switch-enum /* NOLINT */
#define W_SUGGEST_ATTRIBUTE_NORETURN suggest-attribute=noreturn /* NOLINT */
#define W_SUGGEST_ATTRIBUTE_FORMAT suggest-attribute=format /* NOLINT */
#define W_UNUSED_LOCAL_TYPEDEF unused-local-typedef /* NOLINT */
#define W_UNUSED_PARAMETER unused-parameter /* NOLINT */
#define W_UNUSED_FUNCTION unused-function /* NOLINT */
#define W_MISSING_FIELD_INITIALIZERS missing-field-initializers /* NOLINT */
#define W_CONVERSION conversion /* NOLINT */
#define W_SIGN_CONVERSION sign-conversion /* NOLINT */
#define W_FLOAT_CONVERSION float-conversion /* NOLINT */
#define W_FLOAT_EQUAL float-equal /* NOLINT */
#define W_UNUSED_MACROS unused-macros /* NOLINT */
#define W_LOGICAL_OP logical-op /* NOLINT */
#define W_IMPLICIT_FALLTHROUGH implicit-fallthrough /* NOLINT */
#define W_STRICT_PROTOTYPES strict-prototypes /* NOLINT */
#define W_EXPANSION_TO_DEFINED expansion-to-defined /* NOLINT */
#define W_DOUBLE_PROMOTION double-promotion         /* NOLINT */
#define W_UNUSED_BUT_SET_VARIABLE unused-but-set-variable /* NOLINT */
#define W_UNUSED_CONST_VARIABLE unused-const-variable= /* NOLINT */
#define W_UNUSED_VALUE unused-value                    /* NOLINT */
#define W_UNUSED_VARIABLE unused-variable                    /* NOLINT */
#define W_FORMAT_NONLITERAL format-nonliteral          /* NOLINT */
#define W_TYPE_LIMITS type-limits                      /* NOLINT */
#define W_PEDANTIC pedantic
#define W_LANGUAGE_EXTENSION_TOKEN language-extension-token /* NOLINT */
#define W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS gnu-zero-variadic-macro-arguments /* NOLINT */
#define W_MISSING_VARIABLE_DECLARATIONS missing-variable-declarations /* NOLINT */
#define W_RESERVED_ID_MACRO reserved-id-macro                         /* NOLINT */
#define W_PADDED padded
#define W_COVERED_SWITCH_DEFAULT covered-switch-default /* NOLINT */
#define W_VLA vla
#define W_INLINE inline
#define W_DEPRECATED_COPY deprecated-copy /* NOLINT */
#define W_IMPLICIT_FLOAT_CONVERSION implicit-float-conversion /* NOLINT */
#define W_BAD_FUNCTION_CAST bad-function-cast                 /* NOLINT */
#define W_INT_TO_POINTER_CAST int-to-pointer-cast             /* NOLINT */
#define W_OLD_STYLE_DEFINITION old-style-definition           /* NOLINT */
#define W_DANGLING_ELSE dangling-else                         /* NOLINT */
#define W_NULL_DEREFERENCE null-dereference                   /* NOLINT */
#define W_CONDITIONAL_UNINITIALIZED conditional-uninitialized /* NOLINT */
#define W_NESTED_EXTERNS nested-externs                       /* NOLINT */
#define W_ATTRIBUTES attributes                           /* NOLINT */
#define W_PACKED packed                           /* NOLINT */
#define W_DATE_TIME date-time                     /* NOLINT */
#define W_NULL_DEREFERENCE null-dereference       /* NOLINT */
#define W_ENUM_COMPARE enum-compare               /* NOLINT */
#define W_CLASS_MEMACCESS class-memaccess         /* NOLINT */

/* clang-format on */
#else
#error "Missing compiler warning definitions"
#endif /* #if ((COMPILER_ID == COMPILER_CLANG) || (COMPILER_ID == \
          COMPILER_GCC)) */

#if (COMPILER_ID == COMPILER_GCC)

#define ENABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION() \
    ENABLE_WARNING(W_IMPLICIT_FUNCTION_DECLARATION)
#define DISABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION() \
    DISABLE_WARNING(W_IMPLICIT_FUNCTION_DECLARATION)

#define ENABLE_WARNING_DISCARDED_QUALIFIERS() \
    ENABLE_WARNING(W_DISCARDED_QUALIFIERS)
#define DISABLE_WARNING_DISCARDED_QUALIFIERS() \
    DISABLE_WARNING(W_DISCARDED_QUALIFIERS)

#define ENABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN() \
    ENABLE_WARNING(W_SUGGEST_ATTRIBUTE_NORETURN)
#define DISABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN() \
    DISABLE_WARNING(W_SUGGEST_ATTRIBUTE_NORETURN)

#define DISABLE_WARNING_LANGUAGE_EXTENSION_TOKEN()
#define ENABLE_WARNING_LANGUAGE_EXTENSION_TOKEN()

#define ENABLE_WARNING_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS()
#define DISABLE_WARNING_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS()

#define ENABLE_WARNING_RESERVED_ID_MACRO()
#define DISABLE_WARNING_RESERVED_ID_MACRO()

#define ENABLE_WARNING_MISSING_VARIABLE_DECLARATIONS()
#define DISABLE_WARNING_MISSING_VARIABLE_DECLARATIONS()

#define ENABLE_WARNING_COVERED_SWITCH_DEFAULT()
#define DISABLE_WARNING_COVERED_SWITCH_DEFAULT()

#define ENABLE_WARNING_DEPRECATED_COPY() ENABLE_WARNING(W_DEPRECATED_COPY)
#define DISABLE_WARNING_DEPRECATED_COPY() DISABLE_WARNING(W_DEPRECATED_COPY)

#define ENABLE_WARNING_IMPLICIT_FLOAT_CONVERSION()
#define DISABLE_WARNING_IMPLICIT_FLOAT_CONVERSION()

#define ENABLE_WARNING_CONDITIONAL_UNINITIALIZED()
#define DISABLE_WARNING_CONDITIONAL_UNINITIALIZED()

#if defined __cplusplus
#define ENABLE_WARNING_CLASS_MEMACCESS() ENABLE_WARNING(W_CLASS_MEMACCESS)
#define DISABLE_WARNING_CLASS_MEMACCESS() DISABLE_WARNING(W_CLASS_MEMACCESS)
#else
#define ENABLE_WARNING_CLASS_MEMACCESS()
#define DISABLE_WARNING_CLASS_MEMACCESS()
#endif

#elif (COMPILER_ID == COMPILER_CLANG)

#define ENABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION()
#define DISABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION()

#define ENABLE_WARNING_DISCARDED_QUALIFIERS()
#define DISABLE_WARNING_DISCARDED_QUALIFIERS()

#define ENABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN()
#define DISABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN()

#define ENABLE_WARNING_LANGUAGE_EXTENSION_TOKEN() \
    ENABLE_WARNING(W_LANGUAGE_EXTENSION_TOKEN)

#define DISABLE_WARNING_LANGUAGE_EXTENSION_TOKEN() \
    DISABLE_WARNING(W_LANGUAGE_EXTENSION_TOKEN)

#define ENABLE_WARNING_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS() \
    ENABLE_WARNING(W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)

#define DISABLE_WARNING_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS() \
    DISABLE_WARNING(W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)

#define ENABLE_WARNING_RESERVED_ID_MACRO() ENABLE_WARNING(W_RESERVED_ID_MACRO)

#define DISABLE_WARNING_RESERVED_ID_MACRO() DISABLE_WARNING(W_RESERVED_ID_MACRO)

#define ENABLE_WARNING_MISSING_VARIABLE_DECLARATIONS() \
    ENABLE_WARNING(W_MISSING_VARIABLE_DECLARATIONS)

#define DISABLE_WARNING_MISSING_VARIABLE_DECLARATIONS() \
    DISABLE_WARNING(W_MISSING_VARIABLE_DECLARATIONS)

#define ENABLE_WARNING_COVERED_SWITCH_DEFAULT() \
    ENABLE_WARNING(W_COVERED_SWITCH_DEFAULT)

#define DISABLE_WARNING_COVERED_SWITCH_DEFAULT() \
    DISABLE_WARNING(W_COVERED_SWITCH_DEFAULT)

#define ENABLE_WARNING_DEPRECATED_COPY()
#define DISABLE_WARNING_DEPRECATED_COPY()

#define ENABLE_WARNING_IMPLICIT_FLOAT_CONVERSION() \
    ENABLE_WARNING(W_IMPLICIT_FLOAT_CONVERSION)
#define DISABLE_WARNING_IMPLICIT_FLOAT_CONVERSION() \
    DISABLE_WARNING(W_IMPLICIT_FLOAT_CONVERSION)

#define ENABLE_WARNING_CONDITIONAL_UNINITIALIZED() \
    ENABLE_WARNING(W_CONDITIONAL_UNINITIALIZED)

#define DISABLE_WARNING_CONDITIONAL_UNINITIALIZED() \
    DISABLE_WARNING(W_CONDITIONAL_UNINITIALIZED)

#define ENABLE_WARNING_CLASS_MEMACCESS()
#define DISABLE_WARNING_CLASS_MEMACCESS()

#endif /* #if (COMPILER_ID == COMPILER_GCC) */

/** Disable all warnings */
#define DISABLE_WARNINGS()                          \
    DISABLE_WARNING(W_PEDANTIC)                     \
    DISABLE_WARNING(W_SHADOW)                       \
    DISABLE_WARNING(W_FORMAT)                       \
    DISABLE_WARNING(W_REDUNDANT_DECLS)              \
    DISABLE_WARNING(W_MISSING_PROTOTYPES)           \
    DISABLE_WARNING(W_CAST_ALIGN)                   \
    DISABLE_WARNING(W_CAST_QUAL)                    \
    DISABLE_WARNING(W_MISSING_NORETURN)             \
    DISABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION() \
    DISABLE_WARNING(W_STRICT_OVERFLOW)              \
    DISABLE_WARNING(W_UNDEF)                        \
    DISABLE_WARNING_DISCARDED_QUALIFIERS()          \
    DISABLE_WARNING(W_WRITE_STRINGS)                \
    DISABLE_WARNING(W_SWITCH_DEFAULT)               \
    DISABLE_WARNING(W_SWITCH_ENUM)                  \
    DISABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN()    \
    DISABLE_WARNING(W_UNUSED_PARAMETER)             \
    DISABLE_WARNING(W_UNUSED_FUNCTION)              \
    DISABLE_WARNING(W_MISSING_FIELD_INITIALIZERS)   \
    DISABLE_WARNING(W_CONVERSION)                   \
    DISABLE_WARNING(W_SIGN_CONVERSION)              \
    DISABLE_WARNING(W_FLOAT_CONVERSION)             \
    DISABLE_WARNING(W_FLOAT_EQUAL)                  \
    DISABLE_WARNING(W_UNUSED_MACROS)                \
    DISABLE_WARNING(W_IMPLICIT_FALLTHROUGH)         \
    DISABLE_WARNING(W_STRICT_PROTOTYPES)            \
    DISABLE_WARNING(W_EXPANSION_TO_DEFINED)         \
    DISABLE_WARNING(W_DOUBLE_PROMOTION)             \
    DISABLE_WARNING_MISSING_VARIABLE_DECLARATIONS() \
    DISABLE_WARNING_RESERVED_ID_MACRO()             \
    DISABLE_WARNING(W_PADDED)                       \
    DISABLE_WARNING(W_INLINE)                       \
    DISABLE_WARNING(W_OLD_STYLE_DEFINITION)         \
    DISABLE_WARNING(W_NESTED_EXTERNS)               \
    DISABLE_WARNING(W_ATTRIBUTES)                   \
    DISABLE_WARNING(W_PACKED)                       \
    DISABLE_WARNING(W_DATE_TIME)                    \
    DISABLE_WARNING(W_NULL_DEREFERENCE)

/** Re-enable warnings disabled by DISABLE_WARNINGS() */
#define ENABLE_WARNINGS()                          \
    ENABLE_WARNING(W_PEDANTIC)                     \
    ENABLE_WARNING(W_SHADOW)                       \
    ENABLE_WARNING(W_FORMAT)                       \
    ENABLE_WARNING(W_REDUNDANT_DECLS)              \
    ENABLE_WARNING(W_MISSING_PROTOTYPES)           \
    ENABLE_WARNING(W_CAST_ALIGN)                   \
    ENABLE_WARNING(W_CAST_QUAL)                    \
    ENABLE_WARNING(W_MISSING_NORETURN)             \
    ENABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION() \
    ENABLE_WARNING(W_STRICT_OVERFLOW)              \
    ENABLE_WARNING(W_UNDEF)                        \
    ENABLE_WARNING_DISCARDED_QUALIFIERS()          \
    ENABLE_WARNING(W_WRITE_STRINGS)                \
    ENABLE_WARNING(W_SWITCH_DEFAULT)               \
    ENABLE_WARNING(W_SWITCH_ENUM)                  \
    ENABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN()    \
    ENABLE_WARNING(W_UNUSED_PARAMETER)             \
    ENABLE_WARNING(W_UNUSED_FUNCTION)              \
    ENABLE_WARNING(W_MISSING_FIELD_INITIALIZERS)   \
    ENABLE_WARNING(W_CONVERSION)                   \
    ENABLE_WARNING(W_SIGN_CONVERSION)              \
    ENABLE_WARNING(W_FLOAT_CONVERSION)             \
    ENABLE_WARNING(W_FLOAT_EQUAL)                  \
    ENABLE_WARNING(W_UNUSED_MACROS)                \
    ENABLE_WARNING(W_IMPLICIT_FALLTHROUGH)         \
    ENABLE_WARNING(W_STRICT_PROTOTYPES)            \
    ENABLE_WARNING(W_EXPANSION_TO_DEFINED)         \
    ENABLE_WARNING(W_DOUBLE_PROMOTION)             \
    ENABLE_WARNING_MISSING_VARIABLE_DECLARATIONS() \
    ENABLE_WARNING_RESERVED_ID_MACRO()             \
    ENABLE_WARNING(W_PADDED)                       \
    ENABLE_WARNING(W_INLINE)                       \
    ENABLE_WARNING(W_NESTED_EXTERNS)               \
    ENABLE_WARNING(W_ATTRIBUTES)                   \
    ENABLE_WARNING(W_PACKED)                       \
    ENABLE_WARNING(W_DATE_TIME)                    \
    ENABLE_WARNING(W_NULL_DEREFERENCE)

#if (COMPILER_ID == COMPILER_CLANG)
#define DISABLE_WARNING(warning) \
    DIAG_PRAGMA(clang, push)     \
    DIAG_PRAGMA(clang, ignored JOINSTR_(-W, warning))
#define ENABLE_WARNING(warning) DIAG_PRAGMA(clang, pop)
#elif defined(_MSC_VER)
#define DISABLE_WARNING(warning) \
    DIAG_PRAGMA(msvc, push)      \
    DIAG_DO_PRAGMA(warning(disable :##warning))
#define ENABLE_WARNING(warning) DIAG_PRAGMA(msvc, pop)
#elif (COMPILER_ID == COMPILER_GCC)
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#define DISABLE_WARNING(warning) \
    DIAG_PRAGMA(GCC, push)       \
    DIAG_PRAGMA(GCC, ignored JOINSTR_(-W, warning))
#define ENABLE_WARNING(warning) DIAG_PRAGMA(GCC, pop)
#else
#define DISABLE_WARNING(warning) DIAG_PRAGMA(GCC, ignored JOINSTR_(-W, warning))
#define ENABLE_WARNING(warning) DIAG_PRAGMA(GCC, warning JOINSTR_(-W, warning))
#endif /* #if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406 */
#endif /* #if (COMPILER_ID == COMPILER_CLANG) */

/* taken from https://github.com/sustrik/libdill */
/*! @cond Doxygen_Suppress */
#if defined(_WIN32) || defined(_WIN64)
#define PRIuZ "Iu"
#else
#define PRIuZ "zu"
#endif
/*! @endcond */

/* Taken from https://github.com/sustrik/libdill */
/* Workaround missing __rdtsc in Clang < 3.5 (or Clang < 6.0 on Xcode) */
#if defined(__x86_64__) || defined(__i386__)
#if (COMPILER_ID == COMPILER_CLANG)
#if (!defined(__apple_build_version__) && ((__clang_major__ < 3) || ((__clang_major__ == 3) && (__clang_minor__ < 5)))) || \
    (defined(__apple_build_version__) && (__clang_major__ < 6))
static inline unsigned long long __rdtsc() {
#if defined __i386__
    unsigned long long x;
    asm volatile("rdtsc" : "=A"(x));
    return x;
#else
    unsigned long long a, d;
    asm volatile("rdtsc" : "=a"(a), "=d"(d));
    return (d << 32) | a;
#endif /* #if defined __i386__ */
}
#endif /* #if (!defined(__apple_build_version__) && ... */
#endif /* #if defined __clang__ */
#endif /* #if defined(__x86_64__) || defined(__i386__) */

#define NOINLINE __attribute__((noinline))

/** Packed data structure/union attribute */
#define PACKED __attribute__((packed))

#if COMPILER_ID == COMPILER_GCC
/** Disables compiler reordering.
    See
    http://stackoverflow.com/questions/13540810/compile-time-barriers-compiler-code-reordering-gcc-and-pthreads#13544831
    for details */
#define COMPILER_BARRIER() asm volatile("" : : : "memory")
#endif /* #if COMPILER_ID == COMPILER_GCC */

/* static assert */
/* clang-format off */
#if (COMPILER_ID == COMPILER_GCC) || (COMPILER_ID == COMPILER_CLANG)
#   ifdef __cplusplus
#        define ASSERT_STATIC(expr) static_assert(expr, "")
#   else
#        define ASSERT_STATIC(expr) _Static_assert(expr, "")
#    endif
#else /*#if (COMPILER_ID == COMPILER_GCC) || (COMPILER_ID == COMPILER_CLANG)*/
/* The compile time assert code is taken from here:
   http://stackoverflow.com/questions/3385515/static-assert-in-c */

/** Compile time assert helper */
#define COMPILE_TIME_ASSERT3(cond, msg) \
    typedef char static_assertion_##msg[(!!(cond)) * 2 - 1]
/** Compile time assert helper */
#define COMPILE_TIME_ASSERT2(x, l) \
    COMPILE_TIME_ASSERT3(x, static_assertion_at_line_##l)
/** Compile time assert helper */
#define COMPILE_TIME_ASSERT(x, l) COMPILE_TIME_ASSERT2(x, l)

/** Compile time assert */
#define ASSERT_STATIC(x) COMPILE_TIME_ASSERT(x, __LINE__)
#endif /*#if (COMPILER_ID == COMPILER_GCC) || (COMPILER_ID == \
          COMPILER_CLANG)*/
/* clang-format on */

/*! @cond Doxygen_Suppress */
ASSERT_STATIC(INT_MAX == ((1ULL << (unsigned)(INT_BITS - 1)) - 1));
ASSERT_STATIC(LONG_MAX == ((1ULL << (unsigned)(LONG_BITS - 1)) - 1));
/*! @endcond */

#if (COMPILER_ID == COMPILER_GCC) || (COMPILER_ID == COMPILER_CLANG)
/** Compiler extension macro */
#define EXTENSION __extension__
#else /*COMPILER_ID*/
/** a stub */
#define EXTENSION
#endif /*COMPILER_ID*/

#if (COMPILER_ID == COMPILER_GCC) || (COMPILER_ID == COMPILER_CLANG)
#define ADD_INT_OVERFLOWED(a, b, res) __builtin_sadd_overflow(a, b, &(res))
#define MUL_INT_OVERFLOWED(a, b, res) __builtin_smul_overflow(a, b, &(res))
#else
/** Check if integer addition overflows */
#define ADD_INT_OVERFLOWED(a, b, c)                             \
    EXTENSION({                                                 \
        unsigned res_ = (unsigned)(a) + (unsigned)(b);          \
        bool ovf_ = (res_ < (unsigned)(a)) || (res_ > INT_MAX); \
        if (!ovf_) {                                            \
            (c) = (int)res_;                                    \
        }                                                       \
        ovf_;                                                   \
    })

/** Check if integer multiplication overflows */
#define MUL_INT_OVERFLOWED(a, b, c)                             \
    EXTENSION({                                                 \
        unsigned res_ = (unsigned)(a) * (unsigned)(b);          \
        bool ovf_ = (res_ < (unsigned)(a)) || (res_ > INT_MAX); \
        if (!ovf_) {                                            \
            (c) = (int)res_;                                    \
        }                                                       \
        ovf_;                                                   \
    })
#endif /*COMPILER_ID*/

/** Add integers and assert if it overflows */
#define ADD_INT(a, b)                             \
    EXTENSION({                                   \
        int add_res_;                             \
        if (ADD_INT_OVERFLOWED(a, b, add_res_)) { \
            ASSERT(0); /* NOLINT */               \
        }                                         \
        add_res_;                                 \
    })

/** Multiply integers and assert if it overflows */
#define MUL_INT(a, b)                             \
    EXTENSION({                                   \
        int mul_res_;                             \
        if (MUL_INT_OVERFLOWED(a, b, mul_res_)) { \
            ASSERT(0); /* NOLINT */               \
        }                                         \
        mul_res_;                                 \
    })

#if (COMPILER_ID == COMPILER_CLANG) || (COMPILER_ID == COMPILER_GCC)
/**
  The parameter `si` specifies which argument is the format string argument
  (starting from 1), while `ftc` is the number of the first argument to check
  against the format string. For functions where the arguments are not available
  to be checked (such as vprintf), specify the third parameter as zero.
  In this case the compiler only checks the format string for consistency.
 */
#define PRINTF(si, ftc) __attribute__((format(printf, (si), (ftc))))
#else
#error "Define PRINTF() macro"
#endif /*COMPILER_ID*/

#if (COMPILER_ID == COMPILER_CLANG) || (COMPILER_ID == COMPILER_GCC)
#define COMPILER_ATTR(...) __attribute__((__VA_ARGS__))
#else
#warning "Define ATTR()"
#endif

int compiler_alignment(void);

#endif /* COMPILER_H_INCLUDED */
