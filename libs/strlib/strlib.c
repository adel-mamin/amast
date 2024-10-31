/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
 * Copyright (c) Martin Sustrik
 * Copyright (c) William Ahern
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
 * string utilities API implementation
 */

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <complex.h>
/* IWYU pragma: no_include <__stdarg_va_arg.h> */

#include "common/macros.h"
#include "common/compiler.h"
#include "strlib/strlib.h"

/* https://stackoverflow.com/a/30734030/2410359 */
int str_icmp(const char *s1, const char *s2) {
    int ca;
    int cb;
    do {
        ca = *(const unsigned char *)s1;
        cb = *(const unsigned char *)s2;
        ca = tolower(toupper(ca));
        cb = tolower(toupper(cb));
        s1++;
        s2++;
    } while (ca == cb && ca != '\0');
    return ca - cb;
}

bool str_is_bool(const char *str, bool *extracted_val) {
    if (0 == str_icmp(str, "true")) {
        if (extracted_val) {
            *extracted_val = true;
        }
        return true;
    }
    if (0 == str_icmp(str, "false")) {
        if (extracted_val) {
            *extracted_val = false;
        }
        return true;
    }
    return false;
}

bool str_is_null(const char *str) { return (0 == str_icmp(str, "null")); }

bool str_is_intmax(const char *str, intmax_t *extracted_val, int base) {
    errno = 0;
    char *endptr = NULL;
    /* could not link with strtoimax() on arm */
    intmax_t int_val = (intmax_t)strtoll(str, &endptr, base);

    if (endptr == str) {
        return false; /* no digits were found */
    }

    bool isintmax = (0 == errno);
    if (isintmax && extracted_val) {
        *extracted_val = int_val;
    }
    return isintmax;
}

/**
 * Checks if the string represents a number of a given base and
 * extracts the number.
 * @param str string to analyze.
 * @param extracted_val the extracted number.
 * @param prefixes the allowed prefixes (e.g. "0b", "0B" etc)
 * @param prefixes_num the number of prefixes in prefixes.
 * @param base the number base.
 * @retval true the string contains the number.
 * @retval false the string does not contain the number.
 */
static bool str_is_base_num(
    const char *str,
    intmax_t *extracted_val,
    const char *prefixes[],
    int prefixes_num,
    int base
) {
    bool has_prefix = false;
    int sign = 1; /* positive by default */
    int str_offset = 0;

    for (int i = 0; i < prefixes_num; i++) {
        int prefix_len = (int)strlen(prefixes[i]);
        if (0 == strncmp(str, prefixes[i], (unsigned long)prefix_len)) {
            has_prefix = true;
            str_offset = prefix_len;
            if ('-' == prefixes[i][0]) {
                sign = -1;
            }
            break;
        }
    }
    if (!has_prefix) {
        return false;
    }

    bool is_num = str_is_intmax(str + str_offset, extracted_val, base);

    if (extracted_val) {
        *extracted_val *= sign;
    }

    return is_num;
}

bool str_is_binary(const char *str, intmax_t *extracted_val) {
    const char *prefixes[] = {"0b", "0B", "-0b", "+0b", "-0B", "+0B"};
    return str_is_base_num(
        str,
        extracted_val,
        prefixes,
        AM_COUNTOF(prefixes),
        /* base = */ 2
    );
}

bool str_is_octal(const char *str, intmax_t *extracted_val) {
    const char *prefixes[] = {"0", "-0", "+0"};
    return str_is_base_num(
        str,
        extracted_val,
        prefixes,
        AM_COUNTOF(prefixes),
        /* base = */ 8
    );
}

bool str_is_hex(const char *str, intmax_t *extracted_val) {
    const char *prefixes[] = {"0x", "0X", "-0x", "+0x", "-0X", "+0X"};
    return str_is_base_num(
        str,
        extracted_val,
        prefixes,
        AM_COUNTOF(prefixes),
        /* base = */ 16
    );
}

bool str_is_decimal(const char *str, intmax_t *extracted_val) {
    return str_is_intmax(str, extracted_val, /* base = */ 10);
}

bool str_to_double(const char *str, double *val, char **endptr) {
    errno = 0;
    char *endptr_;
    double v = strtod(str, &endptr_);
    if (str == endptr_) {
        return false;
    }
    if (ERANGE == errno) {
        return false;
    }
    if (val) {
        *val = v;
    }
    if (endptr) {
        *endptr = endptr_;
    }
    return true;
}

float complex str_to_complex(char *str) {
    char *start = str;
    char *end = NULL;
    float real = strtof(start, &end);
    AM_ASSERT(start != end);

    start = end;
    end = NULL;
    float imag = strtof(start, &end);
    AM_ASSERT(start != end);
    AM_ASSERT(*end == 'i');

    return real + imag * I;
}

bool str_is_all_decimal_digits(const char *str) {
    for (int i = 0; i < (int)strlen(str); i++) {
        if ((str[i] < '0') || (str[i] > '9')) {
            return false;
        }
    }
    return true;
}

bool str_is_all_hexadecimal_digits(const char *str) {
    int len = (int)strlen(str);
    if (len <= 2) {
        return false;
    }
    if (('0' != str[0]) || ('x' != str[1])) {
        return false;
    }
    for (int i = 2; i < len; i++) {
        if ((str[i] >= '0') && (str[i] <= '9')) {
            continue;
        }
        if ((str[i] >= 'a') && (str[i] <= 'f')) {
            continue;
        }
        if ((str[i] >= 'A') && (str[i] <= 'F')) {
            continue;
        }
        return false;
    }
    return true;
}

bool str_is_double(const char *str, double *extracted_val) {
    char *endptr;
    bool ok = str_to_double(str, extracted_val, &endptr);
    if (!ok) {
        return false;
    }
    char *found = strchr(str, '.');
    if (!found || (found >= endptr)) {
        found = strchr(str, 'E');
    }
    if (!found || (found >= endptr)) {
        found = strchr(str, 'e');
    }
    if (!found || (found >= endptr)) {
        found = strchr(str, 'P');
    }
    if (!found || (found >= endptr)) {
        found = strchr(str, 'p');
    }
    if (!found || (found >= endptr)) {
        found = strchr(str, 'i'); /* inf */
    }
    if (!found || (found >= endptr)) {
        found = strchr(str, 'I'); /* INF */
    }
    if (!found || (found >= endptr)) {
        found = strchr(str, 'N'); /* NAN */
    }
    if (!found || (found >= endptr)) {
        found = strchr(str, 'N'); /* nan */
    }

    return (found != NULL);
}

/** The binary string maximum size [bytes] */
#define BIN_STR_MAX_SIZE_BYTES 256

int uintmax_to_binstr(char *str, int strsize, uintmax_t uintmax) {
    AM_ASSERT(strsize > 0);

    char buf[BIN_STR_MAX_SIZE_BYTES];
    char *bufptr = buf;
    int bufsize = sizeof(buf);

    int written = snprintf(bufptr, (unsigned long)strsize, "0b");
    if ((written < 0) || (written >= bufsize)) {
        return written;
    }
    bufsize -= written;
    bufptr += written;

    int nybbles = (sizeof(uintmax_t) * CHAR_BIT) / 4;

    static const char *pat[16] = {
        [0] = "0000",
        [1] = "0001",
        [2] = "0010",
        [3] = "0011",
        [4] = "0100",
        [5] = "0101",
        [6] = "0110",
        [7] = "0111",
        [8] = "1000",
        [9] = "1001",
        [10] = "1010",
        [11] = "1011",
        [12] = "1100",
        [13] = "1101",
        [14] = "1110",
        [15] = "1111"
    };

    bool printed = false;
    for (int ni = 0; ni < nybbles; ni++) {
        unsigned char nybble =
            (unsigned char)(uintmax >> (unsigned)((nybbles - 1 - ni) * 4));
        nybble &= 0xFU;
        if (!nybble && !printed) { /* do not print leading zeros */
            continue;
        }
        written = snprintf(bufptr, (unsigned long)bufsize, "%s", pat[nybble]);
        AM_ASSERT(written > 0);
        AM_ASSERT(written < bufsize);
        bufptr += written;
        bufsize += written;
        printed = true;
    }
    if (!printed) {
        written = snprintf(bufptr, (unsigned long)bufsize, "0");
        AM_ASSERT(written > 0);
        AM_ASSERT(written < bufsize);
    }
    return snprintf(str, (unsigned long)strsize, "%s", buf);
}

const char *str_lstrip(const char *string, char delim) {
    const char *pos = string;
    while (*pos && *pos == delim) {
        ++pos;
    }
    return pos;
}

const char *str_rstrip(const char *string, char delim) {
    const char *end = string + strlen(string) - 1;
    while (end > string && *end == delim) {
        --end;
    }
    return ++end;
}

int str_lcpy(char *dst, const char *src, int lim) {
    char *d = dst;
    char *e = &dst[lim];
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
    char *e = &dst[lim];
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

int str_lcatf(char *dst, int lim, const char *fmt, ...) {
    AM_ASSERT(dst);
    AM_ASSERT(lim > 0);
    AM_ASSERT(fmt);

    long len = (int)strlen(dst);
    AM_ASSERT(len <= lim);

    va_list ap;
    va_start(ap, fmt);
    len += vsnprintf(dst + len, (size_t)(lim - len), fmt, ap); /* NOLINT */
    va_end(ap);

    AM_ASSERT(len <= INT_MAX);

    return (int)len;
}

AM_DISABLE_WARNING_SUGGEST_ATTRIBUTE_FORMAT()
int str_vlcatf(char *dst, int lim, const char *fmt, va_list ap) {
    AM_ASSERT(dst);
    AM_ASSERT(lim > 0);
    AM_ASSERT(fmt);

    long len = (int)strlen(dst);
    AM_ASSERT(len <= lim);

    AM_DISABLE_WARNING(AM_W_FORMAT_NONLITERAL);
    len += vsnprintf(dst + len, (size_t)(lim - len), fmt, ap); /* NOLINT */
    AM_ENABLE_WARNING(AM_W_FORMAT_NONLITERAL);

    AM_ASSERT(len <= INT_MAX);

    return (int)len;
}
AM_ENABLE_WARNING_SUGGEST_ATTRIBUTE_FORMAT()

char *str_sep(char **sp, const char *delim) {
    AM_ASSERT(sp);
    AM_ASSERT(delim);

    if (!*sp) {
        return NULL;
    }

    AM_DISABLE_WARNING(AM_W_SIGN_CONVERSION)
    *sp += strspn(*sp, /*accept=*/delim);
    AM_ENABLE_WARNING(AM_W_SIGN_CONVERSION)

    char *begin = *sp;
    *sp += strcspn(begin, /*reject=*/delim);
    char *end = *sp;

    if (begin == end) {
        *sp = NULL;
        return NULL;
    }

    if (**sp != '\0') {
        **sp = '\0';
        ++*sp;
    } else {
        *sp = NULL;
    }

    return begin;
}

bool str_has_prefix(const char *str, const char *prefix) {
    AM_ASSERT(str);
    AM_ASSERT(prefix);
    return (0 == strncmp(str, prefix, strlen(prefix)));
}

char *str_upr(char *str) {
    for (int i = 0; i < (int)strlen(str); i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = (char)(str[i] - ('a' - 'A'));
        }
    }
    return str;
}

const char *str_skip_prefix(const char *str, const char *prefix) {
    AM_ASSERT(str);
    AM_ASSERT(prefix);
    int strsz = (int)strlen(str);
    int prefixsz = (int)strlen(prefix);
    while (strsz && prefixsz) {
        if (*str != *prefix) {
            return str;
        }
        str++;
        prefix++;
        strsz--;
        prefixsz--;
    }
    return str;
}

char *str_add_prefix(
    char *out, int outsz, const char *str, const char *prefix
) {
    int copied = str_lcpy(out, prefix, outsz);
    if (copied >= outsz) {
        return out;
    }
    str_lcpy(out + copied, str, outsz - copied);

    return out;
}

void str_split_path(
    const char *path,
    struct str_token *head,
    struct str_token *tail,
    const char *delim
) {
    AM_ASSERT(path);
    AM_ASSERT(head || tail);
    AM_ASSERT(delim);

    if (head) {
        head->start = head->end = -1;
    }
    if (tail) {
        tail->start = tail->end = -1;
    }
    const int ndelim = (int)strlen(delim);
    int nstart = 0;
    int end = (int)strlen(path);
    int ndot = 0;
    int dir = 0;
    for (int i = 0; i < end; i++) {
        int isdelim = 0;
        if ('.' == path[i]) {
            ndot++;
        } else {
            ndot = 0;
            for (int j = 0; j < ndelim; j++) {
                if (path[i] == delim[j]) {
                    isdelim = 1;
                    break;
                }
            }
        }
        int first = (0 == i);
        int last = (i == (end - 1));
        if (isdelim) {
            dir = 1;
        } else if (first && ((1 == ndot) || (2 == ndot))) {
            dir = 1;
        } else if (!ndot || (ndot > 2)) {
            dir = 0;
        }
        if (isdelim || (dir && last)) {
            if (head) {
                head->start = 0;
                head->end = i + 1;
            }
            nstart = i + 1;
        }
        if (tail && last && !dir) {
            tail->start = nstart;
            tail->end = end;
        }
    }
}

int str_lcat_path(char *head, const char *tail, int lim, char delim) {
    AM_ASSERT(head);
    AM_ASSERT(tail);
    AM_ASSERT(lim > 0);

    int l = (int)strlen(head);
    if (l && (head[l - 1] == delim)) {
        head[l - 1] = '\0';
    }
    if (delim == tail[0]) {
        tail++;
    }
    char d[2] = {delim, '\0'};
    str_lcat(head, d, lim);
    return str_lcat(head, tail, lim);
}
