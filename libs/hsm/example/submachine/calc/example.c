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
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/constants.h"
#include "common/types.h"
#include "blk/blk.h"
#include "hsm/hsm.h"
#include "common.h"
#include "calc.h"

/** calculator example */

static void calc_print(void) {
    static char buf_prev[256];
    static char buf[256];

    struct am_blk d0 = calc_get_operand(g_calc, 0);
    struct am_blk d1 = calc_get_operand(g_calc, 1);
    char op = calc_get_operator(g_calc);
    double res;
    bool result_valid = calc_get_result(g_calc, &res);
    if (result_valid) {
        snprintf(
            buf,
            sizeof(buf),
            "%.*s%c%.*s=%f",
            d0.size,
            (char *)d0.ptr,
            op,
            d1.size,
            (char *)d1.ptr,
            res
        );
    } else {
        snprintf(
            buf,
            sizeof(buf),
            "%.*s%c%.*s",
            d0.size,
            (char *)d0.ptr,
            op,
            d1.size,
            (char *)d1.ptr
        );
    }
    if (0 == strcmp(buf, buf_prev)) {
        return;
    }
    printf("%s\n", buf);
    memcpy(buf_prev, buf, sizeof(buf_prev));
}

static bool calc_set_event_id(struct calc_event *e, char c) {
    if ('0' == c) {
        e->event.id = EVT_DIGIT_0;
    } else if (('1' <= c) && (c <= '9')) {
        e->event.id = EVT_DIGIT_1_9;
    } else if ('.' == c) {
        e->event.id = EVT_POINT;
    } else if (('+' == c) || ('-' == c) || ('*' == c) || ('/' == c)) {
        e->event.id = EVT_OP;
    } else if ('c' == c) {
        e->event.id = EVT_CANCEL;
    } else if ('d' == c) {
        e->event.id = EVT_DEL;
    } else if ('=' == c) {
        e->event.id = EVT_EQUAL;
    } else {
        return false;
    }
    return true;
}

static void calc_log(char *fmt, ...) {
    (void)fmt;
}

int main(void) {
    calc_ctor(calc_log);

    printf(AM_COLOR_BLUE_BOLD);
    printf("Interactive calculator.\n");
    printf("Type [0-9 . / * + - c d =] (x to turn off)\n");
    printf(AM_COLOR_RESET);

    am_hsm_init(g_calc, /*init_event=*/NULL);
    calc_print();

    static const char *blank = " ";

    for (;;) {
        char c = getchar();
        if ('\n' == c) {
            printf("\033[A\r"); /* move the cursor up one line */
            continue;
        }
        char n = getchar();
        if (EOF == c) {
            am_hsm_dispatch(g_calc, &(struct am_event){.id = EVT_OFF});
            goto end;
        }
        printf("\033[A\r"); /* move the cursor up one line */
        printf("\r");
        printf("%s", blank);

        while (n != '\n') {
            printf("%s", blank);
            n = getchar();
            if (EOF == n) {
                am_hsm_dispatch(g_calc, &(struct am_event){.id = EVT_OFF});
                goto end;
            }
        }
        printf("\r");

        c = tolower(c);
        if ('x' == c) {
            am_hsm_dispatch(g_calc, &(struct am_event){.id = EVT_OFF});
            goto end;
        }
        struct calc_event e = {
             .data = c
        };
        bool valid = calc_set_event_id(&e, c);
        if (!valid) {
            continue;
        }
        am_hsm_dispatch(g_calc, &e.event);
        calc_print();
    }

end:
    am_hsm_dtor(g_calc);
    calc_print();

    return 0;
}
