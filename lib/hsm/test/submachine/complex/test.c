/*
 *  The MIT License (MIT)
 *
 * Copyright (c) 2020-2024 Adel Mamin
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

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "common.h"
#include "submachine.h"

#define TEST_LOG_SIZE 256 /* [bytes] */

static char m_log_buf[TEST_LOG_SIZE];

void test_log(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(m_log_buf, (int)sizeof(m_log_buf), fmt, ap);
    va_end(ap);
}

/* Test submachines. */

static void test_submachine(void) {
    submachine_ctor(test_log);

    m_log_buf[0] = '\0';

    hsm_init(g_submachine, /*init_event=*/NULL);

    {
        const char *out =
            "top/0-INIT;s/0-ENTRY;s1/0-ENTRY;s1/1-ENTRY;s1/1-INIT;s11/"
            "1-ENTRY;s111/1-ENTRY;s111/1-INIT;";
        ASSERT(0 == strncmp(m_log_buf, out, strlen(out)));
        m_log_buf[0] = '\0';
    }

    struct test2 {
        int evt;
        const char *out;
    };
    static const struct test2 in[] = {
        /* clang-format off */
        {HSM_EVT_A, "s1/1-A;s111/1-EXIT;s11/1-EXIT;s1/1-EXIT;s1/1-ENTRY;"
                    "s1/1-INIT;s11/1-ENTRY;s111/1-ENTRY;s111/1-INIT;"},

        {HSM_EVT_C, "s1/1-C;s111/1-EXIT;s11/1-EXIT;s12/1-ENTRY;s121/1-ENTRY;"
                    "s121/1-INIT;"},

        {HSM_EVT_B, "s1/1-B;s121/1-EXIT;s12/1-EXIT;s11/1-ENTRY;s11/1-INIT;"},

        {HSM_EVT_D, ""},

        {HSM_EVT_A, "s1/1-A;s11/1-EXIT;s1/1-EXIT;s1/1-ENTRY;s1/1-INIT;"
                    "s11/1-ENTRY;s111/1-ENTRY;s111/1-INIT;"},

        {HSM_EVT_D, "s111/1-D;s111/1-EXIT;s11/1-EXIT;s12/1-ENTRY;s12/1-INIT;"
                    "s121/1-ENTRY;s121/1-INIT;"},

        {HSM_EVT_F, "s12/1-F;s121/1-EXIT;s12/1-EXIT;s1/1-EXIT;s1/0-EXIT;"
                    "s1/2-ENTRY;s12/2-ENTRY;s12/2-INIT;s121/2-ENTRY;"
                    "s121/2-INIT;"},

        {HSM_EVT_E, "s121/2-E;s121/2-EXIT;s12/2-INIT;s121/2-ENTRY;"
                    "s121/2-INIT;"},

        {HSM_EVT_B, "s1/2-B;s121/2-EXIT;s12/2-EXIT;s11/2-ENTRY;s11/2-INIT;"},

        {HSM_EVT_G, "s11/2-G;s11/2-EXIT;s1/2-EXIT;s1/0-ENTRY;s1/0-INIT;"
                    "s11/0-ENTRY;s111/0-ENTRY;s111/0-INIT;"},

        {HSM_EVT_H, "s1/0-H;s111/0-EXIT;s11/0-EXIT;s1/0-EXIT;s/0-INIT;"
                    "s1/2-ENTRY;s11/2-ENTRY;s111/2-ENTRY;s111/2-INIT;"},
        /* clang-format on */
    };

    for (int i = 0; i < ARRAY_SIZE(in); i++) {
        struct event e = {.id = in[i].evt};
        hsm_dispatch(g_submachine, &e);
        ASSERT(0 == strncmp(m_log_buf, in[i].out, strlen(in[i].out)));
        m_log_buf[0] = '\0';
    }

    {
        static const char *dest = "s111/2-EXIT;s11/2-EXIT;s1/2-EXIT;s/0-EXIT;";
        hsm_dtor(g_submachine);
        ASSERT(0 == strncmp(m_log_buf, dest, strlen(dest)));
        m_log_buf[0] = '\0';
    }
}

int main(void) {
    test_submachine();

    return 0;
}
