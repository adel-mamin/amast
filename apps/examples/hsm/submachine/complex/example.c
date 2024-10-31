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

#include <stdio.h>
#include <ctype.h>
/* IWYU pragma: no_include <__stdarg_va_arg.h> */

#include "common/constants.h"
#include "common/macros.h"
#include "event/event.h"
#include "strlib/strlib.h"
#include "hsm/hsm.h"
#include "common.h"
#include "submachine.h"

static char m_log_buf[256];

static void test_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    str_vlcatf(m_log_buf, (int)sizeof(m_log_buf), fmt, ap);
    va_end(ap);
}

static void test_print(int c) {
    printf(AM_COLOR_YELLOW_BOLD);
    printf("%c", (char)c);
    printf(AM_COLOR_RESET);
    printf(": %s\n", m_log_buf);
}

int main(void) {
    complex_sm_ctor(test_log);

    printf(AM_COLOR_BLUE_BOLD);
    printf("Type event [A,B,C,D,E,F,G,H] (T to terminate)\n");
    printf(AM_COLOR_RESET);

    m_log_buf[0] = '\0';
    am_hsm_init(g_complex_sm, /*init_event=*/NULL);
    test_print('*');

    static const char *blank = "        ";
    static const int e[] = {
        HSM_EVT_A,
        HSM_EVT_B,
        HSM_EVT_C,
        HSM_EVT_D,
        HSM_EVT_E,
        HSM_EVT_F,
        HSM_EVT_G,
        HSM_EVT_H
    };

    for (;;) {
        int c = getchar();
        /* move the cursor up one line */
        printf("\033[A\r");
        if ('\n' == c) {
            continue;
        }
        printf("\r");
        printf("%s", blank);

        int n = getchar();
        while (n != '\n') {
            printf("%s", blank);
            n = getchar();
        }
        printf("\r");

        c = toupper(c);
        int terminate = 'T' == c;
        int index = c - 'A';
        int valid = (0 <= index) && (index < AM_COUNTOF(e));
        if (!valid && !terminate) {
            continue;
        }
        m_log_buf[0] = '\0';

        if (terminate) {
            am_hsm_dispatch(
                g_complex_sm, &(struct am_event){.id = HSM_EVT_TERM}
            );
            test_print(c);
            break;
        }
        am_hsm_dispatch(g_complex_sm, &(struct am_event){.id = e[index]});
        test_print(c);
    }
    m_log_buf[0] = '\0';
    am_hsm_dtor(g_complex_sm);
    test_print('*');

    return 0;
}
