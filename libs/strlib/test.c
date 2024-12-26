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
 * @file
 * String utilities unit tests.
 */

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "common/macros.h"
#include "strlib/strlib.h"

static void string_is_true(const char *str) {
    bool val = false;
    bool isbool = str_is_bool(str, &val);
    AM_ASSERT(isbool && (true == val));
}

static void string_is_false(const char *str) {
    bool val = true;
    bool isbool = str_is_bool(str, &val);
    AM_ASSERT(isbool && (false == val));
}

static void string_is_not_bool(const char *str) {
    bool val = true;
    bool isbool = str_is_bool(str, &val);
    AM_ASSERT(!isbool);
}

static void test_str_is_bool(void) {
    string_is_true("true");
    string_is_true("True");
    string_is_true("tRue");

    string_is_false("false");
    string_is_false("False");
    string_is_false("falsE");

    string_is_not_bool("alse");
    string_is_not_bool("fals");
    string_is_not_bool("f");
    string_is_not_bool("tru");
}

static void string_is_null(const char *str) { AM_ASSERT(str_is_null(str)); }

static void string_is_not_null(const char *str) {
    AM_ASSERT(!str_is_null(str));
}

static void test_str_is_null(void) {
    string_is_null("null");
    string_is_null("Null");
    string_is_null("nUll");

    string_is_not_null("ull");
    string_is_not_null("nul");
    string_is_not_null("nul1");
    string_is_not_null("n");
}

static void string_is_int(const char *str, intmax_t expected_val) {
    intmax_t val = 0;
    bool is_int = str_is_decimal(str, &val);
    AM_ASSERT(is_int && (val == expected_val));
}

static void string_is_not_int(const char *str) {
    intmax_t val = 0;
    bool is_int = str_is_decimal(str, &val);
    AM_ASSERT(!is_int);
}

static void test_str_is_int(void) {
    string_is_int("0", 0);
    string_is_int("+0", 0);
    string_is_int("-0", 0);
    string_is_int("-1000000000000", -1000000000000);
    string_is_int("1000000000000", 1000000000000);
    string_is_not_int("- 1");
    string_is_not_int("+ 1");
    string_is_not_int(".1");
}

static void string_is_double(const char *str, double expected_val) {
    double val = 0;
    bool is_double = str_is_double(str, &val);
    AM_ASSERT(is_double);
    AM_DISABLE_WARNING(AM_W_DOUBLE_PROMOTION);
    if (isnan((float)val)) {                   /* NOLINT */
        AM_ASSERT(isnan((float)expected_val)); /* NOLINT */
    } else if (isinf((float)val)) {            /* NOLINT */
        AM_ASSERT(isinf((float)expected_val)); /* NOLINT */
    } else {
        /* AM_ASSERT(is_double && SAME_ALMOST(val, expected_val)); */
    }
    AM_ENABLE_WARNING(AM_W_DOUBLE_PROMOTION);
}

static void string_is_not_double(const char *str) {
    double val = 0;
    bool is_double = str_is_double(str, &val);
    AM_ASSERT(!is_double);
}

static void test_str_is_double(void) {
    string_is_int("0", 0);
    string_is_not_double("0");
    string_is_int("+0", 0);
    string_is_int("-0", 0);
    string_is_int("-1000000000000", -1000000000000);
    string_is_int("1000000000000", 1000000000000);
    string_is_double("-1e3", -1e3);
    string_is_not_double("- 1");
    string_is_double("-0.1", -0.1);
    string_is_double(".1", .1);
    string_is_double("-.00314159E+003", -.00314159E+003);
    string_is_double("NaN", (double)NAN);
    string_is_double("Inf", (double)INFINITY);
}

static void string_is_hex(const char *str, intmax_t expected_val) {
    intmax_t val = 0;
    bool is_hex = str_is_hex(str, &val);
    AM_ASSERT(is_hex && (val == expected_val));
}

static void string_is_not_hex(const char *str) {
    intmax_t val = 0;
    bool is_hex = str_is_hex(str, &val);
    AM_ASSERT(!is_hex);
}

static void test_str_is_hex(void) {
    string_is_hex("0x0", 0);
    string_is_hex("-0x0", 0);
    string_is_hex("0xa", 10);
    string_is_hex("-0xa", -10);
    string_is_not_hex("0");
    string_is_not_hex("0x");
    string_is_not_hex("1e3");
    string_is_not_hex("0xg");
}

static void string_is_binary(const char *str, intmax_t expected_val) {
    intmax_t val = 0;
    bool is_binary = str_is_binary(str, &val);
    AM_ASSERT(is_binary && (val == expected_val));
}

static void string_is_not_binary(const char *str) {
    intmax_t val = 0;
    bool is_binary = str_is_binary(str, &val);
    AM_ASSERT(!is_binary);
}

static void test_str_is_binary(void) {
    string_is_binary("0b0", 0);
    string_is_binary("-0b0", 0);
    string_is_binary("0b1", 1);
    string_is_binary("-0b1", -1);
    string_is_binary("-0B1000", -8);
    string_is_binary("-0b01000", -8);
    string_is_binary("+0B01000", 8);
    string_is_not_binary("0");
    string_is_not_binary("0b");
    string_is_not_binary("0B");
    string_is_not_binary("1e3");
    string_is_not_binary("0B2");
}

static void string_is_octal(const char *str, intmax_t expected_val) {
    intmax_t val = 0;
    bool is_octal = str_is_octal(str, &val);
    AM_ASSERT(is_octal && (val == expected_val));
}

static void string_is_not_octal(const char *str) {
    intmax_t val = 0;
    bool is_octal = str_is_octal(str, &val);
    AM_ASSERT(!is_octal);
}

static void test_str_is_octal(void) {
    string_is_octal("00", 0);
    string_is_octal("-00", 0);
    string_is_octal("01", 1);
    string_is_octal("-01", -1);
    string_is_octal("-010", -8);
    string_is_octal("-01000", -8 * 8 * 8);
    string_is_octal("+01000", 8 * 8 * 8);
    string_is_not_octal("1");
    string_is_not_octal("7");
    string_is_not_octal("1e3");
    string_is_not_octal("08");
}

static void binstr_for_is(const char *str, uintmax_t uintmax) {
    char buf[128];
    int written = uintmax_to_binstr(buf, sizeof(buf), uintmax);
    AM_ASSERT(written == (int)strlen(str));
    AM_ASSERT(0 == strcmp(str, buf));
}

static void test_uintmax_to_binstr(void) {
    binstr_for_is("0b0", 0);
    binstr_for_is("0b0001", 1);
    binstr_for_is("0b00010000", 0x10);
    binstr_for_is("0b1000000000000000", 0x8000);
}

static void test_str_sep(void) {
    char str[3] = {':', ':', '\0'};
    const char delim[] = ":";
    char *ctx = &str[0];
    const char *res = str_sep(&ctx, delim);
    AM_ASSERT(res == NULL);
}

static void test_str_has_prefix(void) {
    AM_ASSERT(str_has_prefix("string", ""));
    AM_ASSERT(str_has_prefix("string", "s"));
    AM_ASSERT(!str_has_prefix("string", "S"));
    AM_ASSERT(str_has_prefix("string", "string"));
    AM_ASSERT(!str_has_prefix("string", "stringg"));
}

static void test_str_skip_prefix(void) {
    {
        const char *str = str_skip_prefix("string", "");
        AM_ASSERT(str);
        AM_ASSERT(0 == strcmp("string", str));
    }

    {
        const char *str = str_skip_prefix("string", "s");
        AM_ASSERT(str);
        AM_ASSERT(0 == strcmp("tring", str));
    }

    {
        const char *str = str_skip_prefix("string", "string");
        AM_ASSERT(str);
        AM_ASSERT(0 == strlen(str));
    }
}

static void test_str_add_prefix(void) {
    {
        char out[3];
        const char *res = str_add_prefix(out, 3, "s", "p");
        AM_ASSERT('p' == res[0]);
        AM_ASSERT('s' == res[1]);
        AM_ASSERT(0 == res[2]);
    }

    {
        char out[3];
        const char *res = str_add_prefix(out, 3, "s", "prefix");
        AM_ASSERT('p' == res[0]);
        AM_ASSERT('r' == res[1]);
        AM_ASSERT(0 == res[2]);
    }
}

static void test_split_path(void) {
    const char delim[] = "/\\";
    struct test {
        struct str_token head;
        struct str_token tail;
        char path[16];
    } t[] = {
        /* clang-format off */
        {{-1, -1}, {-1, -1}, ""},
        {{ 0,  1}, {-1, -1}, "."},
        {{ 0,  2}, {-1, -1}, ".."},
        {{-1, -1}, { 0,  3}, "..."},
        {{ 0,  3}, {-1, -1}, "../"},
        {{-1, -1}, { 0,  3}, "f.a"},
        {{ 0,  2}, { 2,  5}, "./f.a"},
        {{ 0,  3}, { 3,  6}, "../f.a"},
        {{ 0,  7}, { 7, 10}, "/a/b/c/f.a"},
        {{ 0,  7}, {-1, -1}, "/a/f.a/"},
        {{ 0,  4}, {-1, -1}, "/a/."},
        {{ 0,  5}, {-1, -1}, "/a/.."},
        {{ 0,  3}, { 3,  6}, "/a/..."},
        {{-1, -1}, { 0,  2}, ".d"},
        {{-1, -1}, { 0,  3}, ".d."},
        {{-1, -1}, { 0,  4}, ".d.."},
        /* clang-format on */
    };
    for (int i = 0; i < AM_COUNTOF(t); i++) {
        struct str_token head;
        struct str_token tail;
        str_split_path(t[i].path, &head, &tail, delim);
        AM_ASSERT(t[i].head.start == head.start);
        AM_ASSERT(t[i].head.end == head.end);
        AM_ASSERT(t[i].tail.start == tail.start);
        AM_ASSERT(t[i].tail.end == tail.end);
    }
}

static void test_lcat_path(void) {
    {
        char head[16] = "/a";
        const char tail[16] = "b";
        int rc = str_lcat_path(head, tail, sizeof(head), '/');
        AM_ASSERT(4 == rc);
        AM_ASSERT(0 == strncmp(head, "/a/b", 4));
    }
    {
        char head[16] = "/a/";
        const char tail[16] = "b";
        int rc = str_lcat_path(head, tail, sizeof(head), '/');
        AM_ASSERT(4 == rc);
        AM_ASSERT(0 == strncmp(head, "/a/b", 4));
    }
}

int main(void) {
    test_str_is_bool();
    test_str_is_int();
    test_str_is_double();
    test_str_is_hex();
    test_str_is_binary();
    test_str_is_octal();
    test_str_is_null();

    test_uintmax_to_binstr();

    test_str_sep();

    test_str_has_prefix();
    test_str_skip_prefix();
    test_str_add_prefix();

    test_split_path();

    test_lcat_path();

    return 0;
}
