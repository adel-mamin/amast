/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 * Copyright (c) Martin Sustrik
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

#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include <limits.h>

#define AM_COMPILER_UNKNOWN 0  /*!< Compiler is unknown */
#define AM_COMPILER_GCC 1000   /*!< GCC compiler */
#define AM_COMPILER_CLANG 2000 /*!< Clang compiler */

#ifndef AM_COMPILER_BITS
#if (defined(__x86_64__) || defined(__64BIT__) || \
     (defined(__WORDSIZE) && (__WORDSIZE == 64)))
#define AM_COMPILER_BITS 64 /*!< Compiler native word size */
#define INT_BITS 32
#define LONG_BITS 64
#else
#define AM_COMPILER_BITS 32 /*!< Compiler native word size */
#define INT_BITS 32         /*!< integer size in bits */
#define LONG_BITS 32        /*!< long size in bits */
#endif
#endif /* #ifndef AM_COMPILER_BITS */

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
/* Hewlett-Packard C/aC++ */
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

#if (64 == AM_COMPILER_BITS)
typedef long ssize_t;
#define SSIZE_MAX LONG_MAX
#define SSIZE_MIN LONG_MINXS
#else
/** ssize_t definition */
typedef int ssize_t;
#define SSIZE_MAX INT_MAX /*!< ssize_t maximum value */
#define SSIZE_MIN INT_MIN /*!< ssize_t minimum value */
#endif

/* Optimisation hints. */
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
 * (starting from 1), while `ftc` is the number of the first argument to check
 * against the format string. For functions where the arguments are not
 * available to be checked (such as vprintf), specify the third parameter as
 * zero. In this case the compiler only checks the format string for
 * consistency.
 */
#define AM_PRINTF(si, ftc) __attribute__((format(printf, (si), (ftc))))
#define AM_ATTR(...) __attribute__((__VA_ARGS__))
#define AM_FALLTHROUGH __attribute__((fallthrough))
/** do not inline instruction */
#define AM_NOINLINE __attribute__((noinline))
#define AM_SECTION(name) __attribute__((section(name))) /**< section name */
#define AM_NOINLINE __attribute__((noinline))
/** Packed data structure/union attribute */
#define AM_PACKED __attribute__((packed))
/** Compiler extension macro */
#define EXTENSION __extension__
#else
#error "Define macros"
#endif

#define AM_CAST(TYPE, PTR) (((TYPE)(uintptr_t)(const void *)(PTR)))
#define AM_VCAST(TYPE, PTR) (((TYPE)(uintptr_t)(const volatile void *)(PTR)))

/**
 * Choose one of two macros.
 *
 * Given:
 * #define BAR1(a) (a)
 * #define BAR2(a, b) (a, b)
 * #define BAR(...) AM_GET_MACRO_2_(__VA_ARGS__, FOO2, FOO1)(__VA_ARGS__)
 * Then:
 * BAR(a)    expands to BAR1(a)
 * BAR(a, b) expands to BAR2(a, b)
 */
#define AM_GET_MACRO_2_(_1, _2, NAME, ...) NAME

/*! @cond Doxygen_Suppress */
#ifdef _MSC_VER
#define AM_DIAG_DO_PRAGMA(x) __pragma(#x)
#define AM_DIAG_PRAGMA(compiler, x) AM_DIAG_DO_PRAGMA(warning(x))
#else
#define AM_DIAG_DO_PRAGMA(x) _Pragma(#x)
#define AM_DIAG_PRAGMA(compiler, x) AM_DIAG_DO_PRAGMA(compiler diagnostic x)
#endif
/*! @endcond */

#if ((AM_COMPILER_ID == AM_COMPILER_CLANG) || \
     (AM_COMPILER_ID == AM_COMPILER_GCC))
/* clang-format off */
#define AM_W_SHADOW shadow
#define AM_W_FORMAT format
#define AM_W_REDUNDANT_DECLS redundant-decls /* NOLINT */
#define AM_W_MISSING_PROTOTYPES missing-prototypes /* NOLINT */
#define AM_W_CAST_ALIGN cast-align /* NOLINT */
#define AM_W_CAST_QUAL cast-qual /* NOLINT */
#define AM_W_MISSING_NORETURN missing-noreturn /* NOLINT */
#define AM_W_IMPLICIT_FUNCTION_DECLARATION error-implicit-function-declaration /* NOLINT */
#define AM_W_STRICT_OVERFLOW strict-overflow /* NOLINT */
#define AM_W_UNDEF undef /* NOLINT */
#define AM_W_DISCARDED_QUALIFIERS discarded-qualifiers /* NOLINT */
#define AM_W_WRITE_STRINGS write-strings /* NOLINT */
#define AM_W_SWITCH_DEFAULT switch-default /* NOLINT */
#define AM_W_SWITCH_ENUM switch-enum /* NOLINT */
#define AM_W_SUGGEST_ATTRIBUTE_NORETURN suggest-attribute=noreturn /* NOLINT */
#define AM_W_SUGGEST_ATTRIBUTE_FORMAT suggest-attribute=format /* NOLINT */
#define AM_W_UNUSED_LOCAL_TYPEDEF unused-local-typedef /* NOLINT */
#define AM_W_UNUSED_PARAMETER unused-parameter /* NOLINT */
#define AM_W_UNUSED_FUNCTION unused-function /* NOLINT */
#define AM_W_UNREACHABLE_CODE unreachable-code /* NOLINT */
#define AM_W_MISSING_FIELD_INITIALIZERS missing-field-initializers /* NOLINT */
#define AM_W_CONVERSION conversion /* NOLINT */
#define AM_W_SIGN_CONVERSION sign-conversion /* NOLINT */
#define AM_W_FLOAT_CONVERSION float-conversion /* NOLINT */
#define AM_W_FLOAT_EQUAL float-equal /* NOLINT */
#define AM_W_UNUSED_MACROS unused-macros /* NOLINT */
#define AM_W_LOGICAL_OP logical-op /* NOLINT */
#define AM_W_IMPLICIT_FALLTHROUGH implicit-fallthrough /* NOLINT */
#define AM_W_STRICT_PROTOTYPES strict-prototypes /* NOLINT */
#define AM_W_EXPANSION_TO_DEFINED expansion-to-defined /* NOLINT */
#define AM_W_DOUBLE_PROMOTION double-promotion         /* NOLINT */
#define AM_W_UNUSED_BUT_SET_VARIABLE unused-but-set-variable /* NOLINT */
#define AM_W_UNUSED_CONST_VARIABLE unused-const-variable= /* NOLINT */
#define AM_W_UNUSED_VALUE unused-value                    /* NOLINT */
#define AM_W_UNUSED_VARIABLE unused-variable                    /* NOLINT */
#define AM_W_FORMAT_NONLITERAL format-nonliteral          /* NOLINT */
#define AM_W_TYPE_LIMITS type-limits                      /* NOLINT */
#define AM_W_PEDANTIC pedantic
#define AM_W_LANGUAGE_EXTENSION_TOKEN language-extension-token /* NOLINT */
#define AM_W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS gnu-zero-variadic-macro-arguments /* NOLINT */
#define AM_W_MISSING_VARIABLE_DECLARATIONS missing-variable-declarations /* NOLINT */
#define AM_W_RESERVED_ID_MACRO reserved-id-macro                         /* NOLINT */
#define AM_W_PADDED padded
#define AM_W_COVERED_SWITCH_DEFAULT covered-switch-default /* NOLINT */
#define AM_W_VLA vla
#define AM_W_INLINE inline
#define AM_W_DEPRECATED_COPY deprecated-copy /* NOLINT */
#define AM_W_IMPLICIT_FLOAT_CONVERSION implicit-float-conversion /* NOLINT */
#define AM_W_BAD_FUNCTION_CAST bad-function-cast                 /* NOLINT */
#define AM_W_INT_TO_POINTER_CAST int-to-pointer-cast             /* NOLINT */
#define AM_W_OLD_STYLE_DEFINITION old-style-definition           /* NOLINT */
#define AM_W_DANGLING_ELSE dangling-else                         /* NOLINT */
#define AM_W_NULL_DEREFERENCE null-dereference                   /* NOLINT */
#define AM_W_CONDITIONAL_UNINITIALIZED conditional-uninitialized /* NOLINT */
#define AM_W_NESTED_EXTERNS nested-externs                       /* NOLINT */
#define AM_W_ATTRIBUTES attributes                           /* NOLINT */
#define AM_W_PACKED packed                           /* NOLINT */
#define AM_W_DATE_TIME date-time                     /* NOLINT */
#define AM_W_NULL_DEREFERENCE null-dereference       /* NOLINT */
#define AM_W_ENUM_COMPARE enum-compare               /* NOLINT */
#define AM_W_CLASS_MEMACCESS class-memaccess         /* NOLINT */
#define AM_W_GNU_OFFSETOF_EXTENSIONS gnu-offsetof-extensions /* NOLINT */

/* clang-format on */
#else
#error "Missing compiler warning definitions"
#endif /* AM_COMPILER_ID */

#if (AM_COMPILER_ID == AM_COMPILER_GCC)

#define AM_ENABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION() \
    AM_ENABLE_WARNING(AM_W_IMPLICIT_FUNCTION_DECLARATION)
#define AM_DISABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION() \
    AM_DISABLE_WARNING(AM_W_IMPLICIT_FUNCTION_DECLARATION)

#define AM_ENABLE_WARNING_DISCARDED_QUALIFIERS() \
    AM_ENABLE_WARNING(AM_W_DISCARDED_QUALIFIERS)
#define AM_DISABLE_WARNING_DISCARDED_QUALIFIERS() \
    AM_DISABLE_WARNING(AM_W_DISCARDED_QUALIFIERS)

#define AM_ENABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN() \
    AM_ENABLE_WARNING(AM_W_SUGGEST_ATTRIBUTE_NORETURN)
#define AM_DISABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN() \
    AM_DISABLE_WARNING(AM_W_SUGGEST_ATTRIBUTE_NORETURN)

#define AM_ENABLE_WARNING_SUGGEST_ATTRIBUTE_FORMAT() \
    AM_ENABLE_WARNING(AM_W_SUGGEST_ATTRIBUTE_FORMAT)
#define AM_DISABLE_WARNING_SUGGEST_ATTRIBUTE_FORMAT() \
    AM_DISABLE_WARNING(AM_W_SUGGEST_ATTRIBUTE_FORMAT)

#define AM_ENABLE_WARNING_GNU_OFFSETOF_EXTENSIONS()
#define AM_DISABLE_WARNING_GNU_OFFSETOF_EXTENSIONS()

#define AM_DISABLE_WARNING_LANGUAGE_EXTENSION_TOKEN()
#define AM_ENABLE_WARNING_LANGUAGE_EXTENSION_TOKEN()

#define AM_ENABLE_WARNING_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS()
#define AM_DISABLE_WARNING_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS()

#define AM_ENABLE_WARNING_RESERVED_ID_MACRO()
#define AM_DISABLE_WARNING_RESERVED_ID_MACRO()

#define AM_ENABLE_WARNING_MISSING_VARIABLE_DECLARATIONS()
#define AM_DISABLE_WARNING_MISSING_VARIABLE_DECLARATIONS()

#define AM_ENABLE_WARNING_COVERED_SWITCH_DEFAULT()
#define AM_DISABLE_WARNING_COVERED_SWITCH_DEFAULT()

#define AM_ENABLE_WARNING_DEPRECATED_COPY() \
    AM_ENABLE_WARNING(AM_W_DEPRECATED_COPY)
#define AM_DISABLE_WARNING_DEPRECATED_COPY() \
    AM_DISABLE_WARNING(AM_W_DEPRECATED_COPY)

#define AM_ENABLE_WARNING_IMPLICIT_FLOAT_CONVERSION()
#define AM_DISABLE_WARNING_IMPLICIT_FLOAT_CONVERSION()

#define AM_ENABLE_WARNING_CONDITIONAL_UNINITIALIZED()
#define AM_DISABLE_WARNING_CONDITIONAL_UNINITIALIZED()

#if defined __cplusplus
#define AM_ENABLE_WARNING_CLASS_MEMACCESS() \
    AM_ENABLE_WARNING(AM_W_CLASS_MEMACCESS)
#define AM_DISABLE_WARNING_CLASS_MEMACCESS() \
    AM_DISABLE_WARNING(AM_W_CLASS_MEMACCESS)
#else
#define AM_ENABLE_WARNING_CLASS_MEMACCESS()
#define AM_DISABLE_WARNING_CLASS_MEMACCESS()
#endif

#elif (AM_COMPILER_ID == AM_COMPILER_CLANG)

#define AM_ENABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION()
#define AM_DISABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION()

#define AM_ENABLE_WARNING_DISCARDED_QUALIFIERS()
#define AM_DISABLE_WARNING_DISCARDED_QUALIFIERS()

#define AM_ENABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN()
#define AM_DISABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN()

#define AM_ENABLE_WARNING_SUGGEST_ATTRIBUTE_FORMAT()
#define AM_DISABLE_WARNING_SUGGEST_ATTRIBUTE_FORMAT()

#define AM_ENABLE_WARNING_LANGUAGE_EXTENSION_TOKEN() \
    AM_ENABLE_WARNING(AM_W_LANGUAGE_EXTENSION_TOKEN)

#define AM_DISABLE_WARNING_LANGUAGE_EXTENSION_TOKEN() \
    AM_DISABLE_WARNING(AM_W_LANGUAGE_EXTENSION_TOKEN)

#define AM_ENABLE_WARNING_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS() \
    AM_ENABLE_WARNING(AM_W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)

#define AM_DISABLE_WARNING_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS() \
    AM_DISABLE_WARNING(AM_W_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)

#define AM_ENABLE_WARNING_RESERVED_ID_MACRO() \
    AM_ENABLE_WARNING(AM_W_RESERVED_ID_MACRO)

#define AM_DISABLE_WARNING_RESERVED_ID_MACRO() \
    AM_DISABLE_WARNING(AM_W_RESERVED_ID_MACRO)

#define AM_ENABLE_WARNING_MISSING_VARIABLE_DECLARATIONS() \
    AM_ENABLE_WARNING(AM_W_MISSING_VARIABLE_DECLARATIONS)

#define AM_DISABLE_WARNING_MISSING_VARIABLE_DECLARATIONS() \
    AM_DISABLE_WARNING(AM_W_MISSING_VARIABLE_DECLARATIONS)

#define AM_ENABLE_WARNING_COVERED_SWITCH_DEFAULT() \
    AM_ENABLE_WARNING(AM_W_COVERED_SWITCH_DEFAULT)

#define AM_DISABLE_WARNING_COVERED_SWITCH_DEFAULT() \
    AM_DISABLE_WARNING(AM_W_COVERED_SWITCH_DEFAULT)

#define AM_ENABLE_WARNING_DEPRECATED_COPY()
#define AM_DISABLE_WARNING_DEPRECATED_COPY()

#define AM_ENABLE_WARNING_IMPLICIT_FLOAT_CONVERSION() \
    AM_ENABLE_WARNING(AM_W_IMPLICIT_FLOAT_CONVERSION)
#define AM_DISABLE_WARNING_IMPLICIT_FLOAT_CONVERSION() \
    AM_DISABLE_WARNING(AM_W_IMPLICIT_FLOAT_CONVERSION)

#define AM_ENABLE_WARNING_CONDITIONAL_UNINITIALIZED() \
    AM_ENABLE_WARNING(AM_W_CONDITIONAL_UNINITIALIZED)

#define AM_DISABLE_WARNING_CONDITIONAL_UNINITIALIZED() \
    AM_DISABLE_WARNING(AM_W_CONDITIONAL_UNINITIALIZED)

#define AM_ENABLE_WARNING_CLASS_MEMACCESS()
#define AM_DISABLE_WARNING_CLASS_MEMACCESS()

#define AM_ENABLE_WARNING_GNU_OFFSETOF_EXTENSIONS() \
    AM_ENABLE_WARNING(AM_W_GNU_OFFSETOF_EXTENSIONS)
#define AM_DISABLE_WARNING_GNU_OFFSETOF_EXTENSIONS() \
    AM_DISABLE_WARNING(AM_W_GNU_OFFSETOF_EXTENSIONS)

#endif /* AM_COMPILER_ID */

/** Disable all warnings */
#define AM_AM_DISABLE_WARNINGS()                        \
    AM_DISABLE_WARNING(AM_W_PEDANTIC)                   \
    AM_DISABLE_WARNING(AM_W_SHADOW)                     \
    AM_DISABLE_WARNING(AM_W_FORMAT)                     \
    AM_DISABLE_WARNING(AM_W_REDUNDANT_DECLS)            \
    AM_DISABLE_WARNING(AM_W_MISSING_PROTOTYPES)         \
    AM_DISABLE_WARNING(AM_W_CAST_ALIGN)                 \
    AM_DISABLE_WARNING(AM_W_CAST_QUAL)                  \
    AM_DISABLE_WARNING(AM_W_MISSING_NORETURN)           \
    AM_DISABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION()  \
    AM_DISABLE_WARNING(AM_W_STRICT_OVERFLOW)            \
    AM_DISABLE_WARNING(AM_W_UNDEF)                      \
    AM_DISABLE_WARNING_DISCARDED_QUALIFIERS()           \
    AM_DISABLE_WARNING(AM_W_WRITE_STRINGS)              \
    AM_DISABLE_WARNING(AM_W_SWITCH_DEFAULT)             \
    AM_DISABLE_WARNING(AM_W_SWITCH_ENUM)                \
    AM_DISABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN()     \
    AM_DISABLE_WARNING(AM_W_UNUSED_PARAMETER)           \
    AM_DISABLE_WARNING(AM_W_UNUSED_FUNCTION)            \
    AM_DISABLE_WARNING(AM_W_MISSING_FIELD_INITIALIZERS) \
    AM_DISABLE_WARNING(AM_W_CONVERSION)                 \
    AM_DISABLE_WARNING(AM_W_SIGN_CONVERSION)            \
    AM_DISABLE_WARNING(AM_W_FLOAT_CONVERSION)           \
    AM_DISABLE_WARNING(AM_W_FLOAT_EQUAL)                \
    AM_DISABLE_WARNING(AM_W_UNUSED_MACROS)              \
    AM_DISABLE_WARNING(AM_W_IMPLICIT_FALLTHROUGH)       \
    AM_DISABLE_WARNING(AM_W_STRICT_PROTOTYPES)          \
    AM_DISABLE_WARNING(AM_W_EXPANSION_TO_DEFINED)       \
    AM_DISABLE_WARNING(AM_W_DOUBLE_PROMOTION)           \
    AM_DISABLE_WARNING_MISSING_VARIABLE_DECLARATIONS()  \
    AM_DISABLE_WARNING_RESERVED_ID_MACRO()              \
    AM_DISABLE_WARNING(AM_W_PADDED)                     \
    AM_DISABLE_WARNING(AM_W_INLINE)                     \
    AM_DISABLE_WARNING(AM_W_OLD_STYLE_DEFINITION)       \
    AM_DISABLE_WARNING(AM_W_NESTED_EXTERNS)             \
    AM_DISABLE_WARNING(AM_W_ATTRIBUTES)                 \
    AM_DISABLE_WARNING(AM_W_PACKED)                     \
    AM_DISABLE_WARNING(AM_W_DATE_TIME)                  \
    AM_DISABLE_WARNING(AM_W_NULL_DEREFERENCE)

/** Re-enable warnings disabled by AM_AM_DISABLE_WARNINGS() */
#define AM_ENABLE_WARNINGS()                           \
    AM_ENABLE_WARNING(AM_W_PEDANTIC)                   \
    AM_ENABLE_WARNING(AM_W_SHADOW)                     \
    AM_ENABLE_WARNING(AM_W_FORMAT)                     \
    AM_ENABLE_WARNING(AM_W_REDUNDANT_DECLS)            \
    AM_ENABLE_WARNING(AM_W_MISSING_PROTOTYPES)         \
    AM_ENABLE_WARNING(AM_W_CAST_ALIGN)                 \
    AM_ENABLE_WARNING(AM_W_CAST_QUAL)                  \
    AM_ENABLE_WARNING(AM_W_MISSING_NORETURN)           \
    AM_ENABLE_WARNING_IMPLICIT_FUNCTION_DECLARATION()  \
    AM_ENABLE_WARNING(AM_W_STRICT_OVERFLOW)            \
    AM_ENABLE_WARNING(AM_W_UNDEF)                      \
    AM_ENABLE_WARNING_DISCARDED_QUALIFIERS()           \
    AM_ENABLE_WARNING(AM_W_WRITE_STRINGS)              \
    AM_ENABLE_WARNING(AM_W_SWITCH_DEFAULT)             \
    AM_ENABLE_WARNING(AM_W_SWITCH_ENUM)                \
    AM_ENABLE_WARNING_SUGGEST_ATTRIBUTE_NORETURN()     \
    AM_ENABLE_WARNING(AM_W_UNUSED_PARAMETER)           \
    AM_ENABLE_WARNING(AM_W_UNUSED_FUNCTION)            \
    AM_ENABLE_WARNING(AM_W_MISSING_FIELD_INITIALIZERS) \
    AM_ENABLE_WARNING(AM_W_CONVERSION)                 \
    AM_ENABLE_WARNING(AM_W_SIGN_CONVERSION)            \
    AM_ENABLE_WARNING(AM_W_FLOAT_CONVERSION)           \
    AM_ENABLE_WARNING(AM_W_FLOAT_EQUAL)                \
    AM_ENABLE_WARNING(AM_W_UNUSED_MACROS)              \
    AM_ENABLE_WARNING(AM_W_IMPLICIT_FALLTHROUGH)       \
    AM_ENABLE_WARNING(AM_W_STRICT_PROTOTYPES)          \
    AM_ENABLE_WARNING(AM_W_EXPANSION_TO_DEFINED)       \
    AM_ENABLE_WARNING(AM_W_DOUBLE_PROMOTION)           \
    AM_ENABLE_WARNING_MISSING_VARIABLE_DECLARATIONS()  \
    AM_ENABLE_WARNING_RESERVED_ID_MACRO()              \
    AM_ENABLE_WARNING(AM_W_PADDED)                     \
    AM_ENABLE_WARNING(AM_W_INLINE)                     \
    AM_ENABLE_WARNING(AM_W_NESTED_EXTERNS)             \
    AM_ENABLE_WARNING(AM_W_ATTRIBUTES)                 \
    AM_ENABLE_WARNING(AM_W_PACKED)                     \
    AM_ENABLE_WARNING(AM_W_DATE_TIME)                  \
    AM_ENABLE_WARNING(AM_W_NULL_DEREFERENCE)

#if (AM_COMPILER_ID == AM_COMPILER_CLANG)
#define AM_DISABLE_WARNING(warning) \
    AM_DIAG_PRAGMA(clang, push)     \
    AM_DIAG_PRAGMA(clang, ignored AM_JOINSTR_(-W, warning))
#define AM_ENABLE_WARNING(warning) AM_DIAG_PRAGMA(clang, pop)
#elif defined(_MSC_VER)
#define AM_DISABLE_WARNING(warning) \
    AM_DIAG_PRAGMA(msvc, push)      \
    AM_DIAG_DO_PRAGMA(warning(disable :##warning))
#define AM_ENABLE_WARNING(warning) AM_DIAG_PRAGMA(msvc, pop)
#elif (AM_COMPILER_ID == AM_COMPILER_GCC)
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#define AM_DISABLE_WARNING(warning) \
    AM_DIAG_PRAGMA(GCC, push)       \
    AM_DIAG_PRAGMA(GCC, ignored AM_JOINSTR_(-W, warning))
#define AM_ENABLE_WARNING(warning) AM_DIAG_PRAGMA(GCC, pop)
#else
#define AM_DISABLE_WARNING(warning) \
    AM_DIAG_PRAGMA(GCC, ignored AM_JOINSTR_(-W, warning))
#define AM_ENABLE_WARNING(warning) \
    AM_DIAG_PRAGMA(GCC, warning AM_JOINSTR_(-W, warning))
#endif /* __GNUC__ */
#endif /* AM_COMPILER_ID */

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
#if (AM_COMPILER_ID == AM_COMPILER_CLANG)
#if (!defined(__apple_build_version__) &&                    \
     ((__clang_major__ < 3) ||                               \
      ((__clang_major__ == 3) && (__clang_minor__ < 5)))) || \
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

#if AM_COMPILER_ID == AM_COMPILER_GCC
/** Disables compiler reordering.
    See
    http://stackoverflow.com/questions/13540810/compile-time-barriers-compiler-code-reordering-gcc-and-pthreads#13544831
    for details */
#define AM_COMPILER_BARRIER() asm volatile("" : : : "memory")
#endif /* AM_COMPILER_ID */

/* clang-format off */

/*
 * The compile time assert code is taken from here:
 * http://stackoverflow.com/questions/3385515/static-assert-in-c
 */

/** Compile time assert helper */
#define AM_COMPILE_TIME_ASSERT3(cond, msg) \
    typedef char static_assertion_##msg[(!!(cond)) * 2 - 1]
/** Compile time assert helper */
#define AM_COMPILE_TIME_ASSERT2(cond, line) \
    AM_COMPILE_TIME_ASSERT3(cond, static_assertion_at_line_##line)
/** Compile time assert helper */
#define AM_COMPILE_TIME_ASSERT(cond, line) AM_COMPILE_TIME_ASSERT2(cond, line)

/** Compile time assert */
#define AM_ASSERT_STATIC(cond) AM_COMPILE_TIME_ASSERT(cond, __LINE__)
/* clang-format on */

/*! @cond Doxygen_Suppress */
AM_ASSERT_STATIC(INT_MAX == ((1ULL << (unsigned)(INT_BITS - 1)) - 1));
AM_ASSERT_STATIC(LONG_MAX == ((1ULL << (unsigned)(LONG_BITS - 1)) - 1));
/*! @endcond */

#if (AM_COMPILER_ID == AM_COMPILER_GCC) || (AM_COMPILER_ID == AM_COMPILER_CLANG)
#define AM_ADD_INT_OVERFLOWED(a, b, res) __builtin_sadd_overflow(a, b, &(res))
#define AM_MUL_INT_OVERFLOWED(a, b, res) __builtin_smul_overflow(a, b, &(res))
#else
/** Check if integer addition overflows */
#define AM_ADD_INT_OVERFLOWED(a, b, c)                          \
    AM_EXTENSION({                                              \
        unsigned res_ = (unsigned)(a) + (unsigned)(b);          \
        bool ovf_ = (res_ < (unsigned)(a)) || (res_ > INT_MAX); \
        if (!ovf_) {                                            \
            (c) = (int)res_;                                    \
        }                                                       \
        ovf_;                                                   \
    })

/** Check if integer multiplication overflows */
#define AM_MUL_INT_OVERFLOWED(a, b, c)                          \
    AM_EXTENSION({                                              \
        unsigned res_ = (unsigned)(a) * (unsigned)(b);          \
        bool ovf_ = (res_ < (unsigned)(a)) || (res_ > INT_MAX); \
        if (!ovf_) {                                            \
            (c) = (int)res_;                                    \
        }                                                       \
        ovf_;                                                   \
    })
#endif /*AM_COMPILER_ID*/

/** Add integers and assert if it overflows */
#define AM_ADD_INT(a, b)                             \
    AM_EXTENSION({                                   \
        int add_res_;                                \
        if (AM_ADD_INT_OVERFLOWED(a, b, add_res_)) { \
            ASSERT(0); /* NOLINT */                  \
        }                                            \
        add_res_;                                    \
    })

/** Multiply integers and assert if it overflows */
#define AM_MUL_INT(a, b)                             \
    AM_EXTENSION({                                   \
        int mul_res_;                                \
        if (AM_MUL_INT_OVERFLOWED(a, b, mul_res_)) { \
            ASSERT(0); /* NOLINT */                  \
        }                                            \
        mul_res_;                                    \
    })

int am_compiler_alignment(void);

#endif /* COMPILER_H_INCLUDED */
