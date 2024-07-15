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

#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED

#define HSM_EVT_A (HSM_EVT_USER)
#define HSM_EVT_B (HSM_EVT_USER + 1)
#define HSM_EVT_C (HSM_EVT_USER + 2)
#define HSM_EVT_D (HSM_EVT_USER + 3)
#define HSM_EVT_E (HSM_EVT_USER + 4)
#define HSM_EVT_F (HSM_EVT_USER + 5)
#define HSM_EVT_G (HSM_EVT_USER + 6)
#define HSM_EVT_H (HSM_EVT_USER + 7)
#define HSM_EVT_I (HSM_EVT_USER + 8)
#define HSM_EVT_TERM (HSM_EVT_USER + 9)

struct test {
    struct hsm hsm;
    int foo;
};

int str_lcat(char *dst, const char *src, int lim);
int str_lcatf(char *dst, int lim, const char *fmt, ...);

#endif /* TEST_H_INCLUDED */
