/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h> /* IWYU pragma: keep */
/* IWYU pragma: no_include <__stdarg_va_arg.h> */

#include "common/macros.h"
#include "strlib/strlib.h"
#include "blk/blk.h"
#include "hsm/hsm.h"
#include "calc.h"

/** calculator test */

static char m_calc_log_buf[256];

static void test_calc_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(m_calc_log_buf, (int)sizeof(m_calc_log_buf), fmt, ap);
    va_end(ap);
}

static void test_calc(void) {
    calc_ctor(test_calc_log);

    am_hsm_init(g_calc, /*init_event=*/NULL);

    /* NOLINTBEGIN(clang-analyzer-optin.performance.Padding) */
    struct test {
        int event;
        char event_data;
        const char *out;
        const char *data[2];
        char op;
    };
    /* NOLINTEND(clang-analyzer-optin.performance.Padding) */
    static const struct test in[] = {
        /* clang-format off */
        [0] = {EVT_DIGIT_0,  '\0', "on-0;", {"0", ""}, '\0'},
        [1] = {EVT_DEL,      '\0', "int/0-DEL;", {"", ""}, '\0'},
        [2] = {EVT_DIGIT_0,  '\0', "on-0;", {"0", ""}, '\0'},
        [3] = {EVT_DIGIT_1_9, '1', "int_zero/0-1_9;int/0-1_9;", {"01", ""}, '\0'},
        [4] = {EVT_DEL,      '\0', "int/0-DEL;", {"0", ""}, '\0'},
        [5] = {EVT_DIGIT_1_9, '2', "int_zero/0-1_9;int/0-1_9;", {"02", ""}, '\0'},
        [6] = {EVT_POINT,    '\0', "int/0-POINT;", {"02.", ""}, '\0'},
        [7] = {EVT_DEL,      '\0', "int_point/0-DEL;", {"02", ""}, '\0'},
        [8] = {EVT_POINT,    '\0', "int/0-POINT;", {"02.", ""}, '\0'},
        [9] = {EVT_DIGIT_1_9, '3', "int_point/0-0_9;int_point_frac/0-1_9;", {"02.3", ""}, '\0'},
        [10] = {EVT_EQUAL,    '\0', "", {"02.3", ""}, '\0'},
        [11] = {EVT_DEL,      '\0', "int_point/0-DEL;", {"02.", ""}, '\0'},
        [12] = {EVT_DIGIT_1_9, '3', "int_point_frac/0-1_9;", {"02.3", ""}, '\0'},
        [13] = {EVT_OP,        '+', "data/0-OP;", {"02.3", ""}, '+'},
        [14] = {EVT_DEL,      '\0', "op-DEL;", {"02.3", ""}, '\0'},
        [15] = {EVT_OP,        '-', "data/0-OP;", {"02.3", ""}, '-'},
        [16] = {EVT_EQUAL,    '\0', "", {"02.3", ""}, '-'},
        [17] = {EVT_DIGIT_0,  '\0', "op-0;int_zero/1-0;", {"02.3", "0"}, '-'},
        [18] = {EVT_CANCEL,   '\0', "on-CANCEL;", {"", ""}, '\0'},

        [19] = {EVT_DIGIT_1_9, '2', "on-1_9;int/0-1_9;", {"2", ""}, '\0'},
        [20] = {EVT_POINT,     '2', "int/0-POINT", {"2.", ""}, '\0'},
        [21] = {EVT_DIGIT_1_9, '3', "int_point/0-0_9;int_point_frac/0-1_9;", {"2.3", ""}, '\0'},
        [22] = {EVT_OP,        '-', "data/0-OP;", {"2.3", ""}, '-'},

        [23] = {EVT_DIGIT_0,  '\0', "op-0;int_zero/1-0;", {"2.3", "0"}, '-'},
        [24] = {EVT_DEL,      '\0', "int/1-DEL;", {"2.3", ""}, '-'},
        [25] = {EVT_DIGIT_1_9, '1', "op-1_9;int/1-1_9;", {"2.3", "1"}, '-'},
        [26] = {EVT_POINT,    '\0', "int/1-POINT;", {"2.3", "1."}, '-'},
        [27] = {EVT_DIGIT_1_9, '4', "int_point/1-0_9;int_point_frac/1-1_9;", {"2.3", "1.4"}, '-'},

        [28] = {EVT_EQUAL,    '\0', "", {"2.3", "1.4"}, '-'},
        /* clang-format on */
    };

    for (int i = 0; i < AM_COUNTOF(in); i++) {
        const struct test *test = &in[i];
        struct calc_event e = {{.id = test->event}, .data = test->event_data};
        am_hsm_dispatch(g_calc, &e.event);

        AM_ASSERT(0 == strncmp(m_calc_log_buf, test->out, strlen(test->out)));

        struct am_blk data0 = calc_get_operand(g_calc, /*index=*/0);
        size_t len0 = strlen(data0.ptr) + 1;
        AM_ASSERT(0 == memcmp(data0.ptr, test->data[0], len0));

        struct am_blk data1 = calc_get_operand(g_calc, /*index=*/1);
        size_t len1 = strlen(data1.ptr) + 1;
        AM_ASSERT(0 == memcmp(data1.ptr, test->data[1], len1));

        AM_ASSERT(test->op == calc_get_operator(g_calc));

        m_calc_log_buf[0] = '\0';
    }

    double result;
    bool result_valid = calc_get_result(g_calc, &result);
    AM_ASSERT(result_valid && AM_DOUBLE_EQ(0.9, result, 1e-9));

    am_hsm_dtor(g_calc);
}

int main(void) {
    test_calc();

    return 0;
}
