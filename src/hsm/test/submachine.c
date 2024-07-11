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

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "test.h"

#define TEST_LOG_SIZE 256 /* [bytes] */

static char m_log_buf[TEST_LOG_SIZE];

#undef LOG
#define LOG(s, ...) \
    str_lcatf(m_log_buf, (int)sizeof(m_log_buf), s, ##__VA_ARGS__)

/* Test submachines. */

static struct test m_test;

#define SM_0 0
#define SM_1 1
#define SM_2 2

static enum hsm_rc s1(struct test *me, const struct event *event);
static enum hsm_rc s11(struct test *me, const struct event *event);
static enum hsm_rc s2(struct test *me, const struct event *event);
static enum hsm_rc s21(struct test *me, const struct event *event);

static enum hsm_rc s1(struct test *me, const struct event *event) {
    const int instance = hsm_get_instance(&me->hsm);
    switch (event->id) {
        case HSM_EVT_ENTRY:
            LOG("s1/%d-ENTRY;", hsm_get_instance(&me->hsm));
            ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
            return HSM_HANDLED();

        case HSM_EVT_EXIT:
            LOG("s1/%d-EXIT;", hsm_get_instance(&me->hsm));
            ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
            return HSM_HANDLED();

        case HSM_EVT_INIT:
            LOG("s1/%d-INIT;", hsm_get_instance(&me->hsm));
            ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
            return HSM_TRAN(s11);

        case HSM_EVT_A:
            LOG("s1/%d-A;", hsm_get_instance(&me->hsm));
            ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, 0)));
            ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, 1)));
            return HSM_TRAN(s1);

        case HSM_EVT_B:
            LOG("s1/%d-B;", hsm_get_instance(&me->hsm));
            ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, 0)));
            ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, 1)));
            return HSM_TRAN(s2);

        case HSM_EVT_D: {
            LOG("s1/%d-D;", hsm_get_instance(&me->hsm));
            ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1, instance)));

            static const struct hsm_state tt[] = {
                [SM_0] = {.fn = HSM_STATE_FN(s1), .instance = SM_1},
                [SM_1] = {.fn = HSM_STATE_FN(s1), .instance = SM_0},
                [SM_2] = {.fn = HSM_STATE_FN(s1), .instance = SM_2}
            };
            ASSERT(instance < ARRAY_SIZE(tt));
            const struct hsm_state *tran = &tt[instance];

            return HSM_TRAN(tran->fn, tran->instance);
        }
        default:
            break;
    }

    static const struct hsm_state ss[] = {
        [SM_0] = {.fn = hsm_top, .instance = 0},
        [SM_1] = {.fn = HSM_STATE_FN(s1), .instance = SM_0},
        [SM_2] = {.fn = hsm_top, .instance = 0}
    };
    ASSERT(instance < ARRAY_SIZE(ss));
    const struct hsm_state *super = &ss[instance];

    return HSM_SUPER(super->fn, super->instance);
}

static enum hsm_rc s11(struct test *me, const struct event *event) {
    switch (event->id) {
        case HSM_EVT_ENTRY:
            LOG("s11/%d-ENTRY;", hsm_get_instance(&me->hsm));
            return HSM_HANDLED();

        case HSM_EVT_EXIT:
            LOG("s11/%d-EXIT;", hsm_get_instance(&me->hsm));
            return HSM_HANDLED();

        case HSM_EVT_INIT:
            LOG("s11/%d-INIT;", hsm_get_instance(&me->hsm));
            return HSM_HANDLED();

        case HSM_EVT_C:
            LOG("s11/%d-C;", hsm_get_instance(&me->hsm));
            return HSM_TRAN(s11);

        case HSM_EVT_E:
            LOG("s11/%d-E;", hsm_get_instance(&me->hsm));
            return HSM_TRAN(s2, SM_2);

        default:
            break;
    }
    return HSM_SUPER(s1);
}

static enum hsm_rc s2(struct test *me, const struct event *event) {
    const int instance = hsm_get_instance(&me->hsm);
    switch (event->id) {
        case HSM_EVT_ENTRY:
            LOG("s2/%d-ENTRY;", hsm_get_instance(&me->hsm));
            return HSM_HANDLED();

        case HSM_EVT_EXIT:
            LOG("s2/%d-EXIT;", hsm_get_instance(&me->hsm));
            return HSM_HANDLED();

        case HSM_EVT_INIT:
            LOG("s2/%d-INIT;", hsm_get_instance(&me->hsm));
            return HSM_TRAN(s21);

        case HSM_EVT_A:
            LOG("s2/%d-A;", hsm_get_instance(&me->hsm));
            return HSM_TRAN(s2);

        case HSM_EVT_B:
            LOG("s2/%d-B;", hsm_get_instance(&me->hsm));
            return HSM_TRAN(s1);
        default:
            break;
    }

    static const struct hsm_state ss[] = {
        [SM_0] = {.fn = hsm_top, .instance = 0},
        [SM_1] = {.fn = HSM_STATE_FN(s2), .instance = SM_0},
        [SM_2] = {.fn = hsm_top, .instance = 0}
    };
    ASSERT(instance < ARRAY_SIZE(ss));
    const struct hsm_state *super = &ss[instance];

    return HSM_SUPER(super->fn, super->instance);
}

static enum hsm_rc s21(struct test *me, const struct event *event) {
    switch (event->id) {
        case HSM_EVT_ENTRY:
            LOG("s21/%d-ENTRY;", hsm_get_instance(&me->hsm));
            return HSM_HANDLED();

        case HSM_EVT_EXIT:
            LOG("s21/%d-EXIT;", hsm_get_instance(&me->hsm));
            return HSM_HANDLED();

        case HSM_EVT_INIT:
            LOG("s21/%d-INIT;", hsm_get_instance(&me->hsm));
            return HSM_HANDLED();

        case HSM_EVT_C:
            LOG("s11/%d-C;", hsm_get_instance(&me->hsm));
            return HSM_TRAN(s21);

        default:
            break;
    }
    return HSM_SUPER(s2);
}

static enum hsm_rc sinit(struct test *me, const struct event *event) {
    (void)event;

    memset(&m_log_buf, 0, sizeof(m_log_buf));
    LOG("top/%d-INIT;", hsm_get_instance(&me->hsm));

    return HSM_TRAN(s1, SM_1);
}

static void test_submachine(void) {
    struct test *me = &m_test;
    hsm_ctor(&me->hsm, &HSM_STATE(sinit));

    m_log_buf[0] = '\0';
    hsm_init(&me->hsm, /*init_event=*/NULL);
    ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11, SM_1)));

    const char *out =
        "top/0-INIT;s1/0-ENTRY;s1/1-ENTRY;"
        "s1/1-INIT;s11/1-ENTRY;s11/1-INIT;";

    ASSERT(0 == strncmp(m_log_buf, out, strlen(out)));
    m_log_buf[0] = '\0';

    struct test2 {
        int evt;
        const char *out;
    };
    static const struct test2 in[] = {
        /* clang-format off */
        {HSM_EVT_A, "s1/1-A;s11/1-EXIT;s1/1-EXIT;s1/1-ENTRY;s1/1-INIT;"
                    "s11/1-ENTRY;s11/1-INIT;"},

        {HSM_EVT_C, "s11/1-C;s11/1-EXIT;s11/1-ENTRY;s11/1-INIT;"},

        {HSM_EVT_B, "s1/1-B;s11/1-EXIT;s1/1-EXIT;s1/0-EXIT;s2/0-ENTRY;"
                    "s2/1-ENTRY;s2/1-INIT;s21/1-ENTRY;s21/1-INIT;"},

        {HSM_EVT_A, "s2/1-A;s21/1-EXIT;s2/1-EXIT;s2/1-ENTRY;s2/1-INIT;"
                    "s21/1-ENTRY;s21/1-INIT;"},

        {HSM_EVT_C, "s11/1-C;s21/1-EXIT;s21/1-ENTRY;s21/1-INIT;"},

        {HSM_EVT_B, "s2/1-B;s21/1-EXIT;s2/1-EXIT;s2/0-EXIT;s1/0-ENTRY;"
                    "s1/1-ENTRY;s1/1-INIT;s11/1-ENTRY;s11/1-INIT;"},

        {HSM_EVT_D, "s1/1-D;s11/1-EXIT;s1/1-EXIT;s1/0-INIT;s11/0-ENTRY;"
                    "s11/0-INIT;"},

        {HSM_EVT_D, "s1/0-D;s11/0-EXIT;s1/1-ENTRY;s1/1-INIT;s11/1-ENTRY;"
                    "s11/1-INIT;"},

        {HSM_EVT_E, "s11/1-E;s11/1-EXIT;s1/1-EXIT;s1/0-EXIT;s2/2-ENTRY;"
                    "s2/2-INIT;s21/2-ENTRY;s21/2-INIT;"},
        /* clang-format on */
    };

    for (int i = 0; i < ARRAY_SIZE(in); i++) {
        struct event e = {.id = in[i].evt};
        hsm_dispatch(&m_test.hsm, &e);
        ASSERT(0 == strncmp(m_log_buf, in[i].out, strlen(in[i].out)));
        m_log_buf[0] = '\0';
    }

    {
        static const char *destruction = "s21/2-EXIT;s2/2-EXIT;";
        hsm_dtor(&m_test.hsm);
        ASSERT(0 == strncmp(m_log_buf, destruction, strlen(destruction)));
        m_log_buf[0] = '\0';
    }
}

int main(void) {
    test_submachine();

    return 0;
}
