/*
 * The MIT License (MIT)
 *
 * Copyright (c) Adel Mamin
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

#ifndef CALC_H_INCLUDED
#define CALC_H_INCLUDED

#include <stdbool.h>

#include "common/types.h"
#include "event/event.h"
#include "hsm/hsm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EVT_OP (AM_EVT_USER)
#define EVT_DIGIT_0 (AM_EVT_USER + 1)
#define EVT_DIGIT_1_9 (AM_EVT_USER + 2)
#define EVT_POINT (AM_EVT_USER + 3)
#define EVT_CANCEL (AM_EVT_USER + 4)
#define EVT_DEL (AM_EVT_USER + 5) /* delete last character */
#define EVT_OFF (AM_EVT_USER + 6)
#define EVT_EQUAL (AM_EVT_USER + 7)

struct calc_event {
    struct am_event event;
    char data;
};

extern struct am_hsm *g_calc;

struct am_blk calc_get_operand(struct am_hsm *me, int index);
char calc_get_operator(struct am_hsm *me);
bool calc_get_result(struct am_hsm *me, double *res);
void calc_ctor(void (*log)(const char *fmt, ...));

#ifdef __cplusplus
}
#endif

#endif /* CALC_H_INCLUDED */
