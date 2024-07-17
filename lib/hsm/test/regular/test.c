/*
 *  The MIT License (MIT)
 *
 * Copyright (c) 2020-2023 Adel Mamin
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "common.h"
#include "regular.h"

#define TEST_LOG_SIZE 256 /* [bytes] */

static char m_log_buf[TEST_LOG_SIZE];

void test_log(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(m_log_buf, (int)sizeof(m_log_buf), fmt, ap);
    va_end(ap);
}

/*
 * Contrived hierarchical state machine (HSM) that contains all possible
 * state transition topologies up to four level of state nesting.
 * Depicted by hsm.png file borrowed from
 * "Practical UML Statecharts in C/C++: Event-Driven Programming for Embedded
 * Systems 2nd Edition" by Miro Samek <https://www.state-machine.com/psicc2>
 */

static void test_hsm(void) {
    regular_ctor(test_log);

    hsm_init(g_regular, /*init_event=*/NULL);

    {
        const char *out =
            "top-INIT;s-ENTRY;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;";
        ASSERT(0 == strncmp(m_log_buf, out, strlen(out)));
        m_log_buf[0] = '\0';
    }

    struct test2 {
        int evt;
        const char *out;
    };
    static const struct test2 in[] = {
        /* clang-format off */
        {HSM_EVT_G, "s21-G;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_I, "s1-I;"},
        {HSM_EVT_A, "s1-A;s11-EXIT;s1-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_B, "s1-B;s11-EXIT;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_D, "s1-D;s11-EXIT;s1-EXIT;s-INIT;s1-ENTRY;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_H, "s11-H;s11-EXIT;s1-EXIT;s-INIT;s1-ENTRY;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_D, "s11-D;s11-EXIT;s1-INIT;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_F, "s1-F;s11-EXIT;s1-EXIT;s2-ENTRY;s21-ENTRY;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_F, "s2-F;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_E, "s-E;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_E, "s-E;s11-EXIT;s1-EXIT;s1-ENTRY;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_G, "s11-G;s11-EXIT;s1-EXIT;s2-ENTRY;s21-ENTRY;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_I, "s2-I;"},
        {HSM_EVT_C, "s2-C;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_D, "s211-D;s211-EXIT;s21-INIT;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_H, "s211-H;s211-EXIT;s21-EXIT;s2-EXIT;s-INIT;s1-ENTRY;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_A, "s21-A;s211-EXIT;s21-EXIT;s21-ENTRY;s21-INIT;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_G, "s21-G;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_F, "s2-F;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s11-ENTRY;s11-INIT;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;s211-INIT;"},
        {HSM_EVT_I, "s-I;"}
        /* clang-format on */
    };

    for (int i = 0; i < ARRAY_SIZE(in); i++) {
        struct event e = {.id = in[i].evt};
        hsm_dispatch(g_regular, &e);
        ASSERT(0 == strncmp(m_log_buf, in[i].out, strlen(in[i].out)));
        m_log_buf[0] = '\0';
    }

    hsm_dtor(g_regular);

    {
        static const char *destruction = "s211-EXIT;s21-EXIT;s2-EXIT;s-EXIT;";
        ASSERT(0 == strncmp(m_log_buf, destruction, strlen(destruction)));
        m_log_buf[0] = '\0';
    }
}

int main(void) {
    test_hsm();

    return 0;
}
