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
#define LOG(s) str_lcat(m_log_buf, s, (int)sizeof(m_log_buf))

/*
 * Contrived hierarchical state machine (HSM) that contains all possible
 * state transition topologies up to four level of state nesting.
 * Depicted by hsm.png file borrowed from
 * "Practical UML Statecharts in C/C++: Event-Driven Programming for Embedded
 * Systems 2nd Edition" by Miro Samek <https://www.state-machine.com/psicc2>
 */

static struct test m_test;

static enum hsm_rc s2(struct test *me, const struct event *event);
static enum hsm_rc s21(struct test *me, const struct event *event);
static enum hsm_rc s211(struct test *me, const struct event *event);
static enum hsm_rc s1(struct test *me, const struct event *event);
static enum hsm_rc s11(struct test *me, const struct event *event);

static enum hsm_rc test_init(struct test *me, const struct event *event) {
    (void)event;

    memset(&m_log_buf, 0, sizeof(m_log_buf));
    me->foo = 0;
    LOG("top-INIT;");
    return HSM_TRAN(s2);
}

static enum hsm_rc s(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        LOG("s-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        LOG("s-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_TRAN(s11);

    case HSM_EVT_EXIT:
        LOG("s-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_I: {
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s211)));
        if (me->foo) {
            me->foo = 0;
            LOG("s-I;");
            return HSM_HANDLED();
        }
        return HSM_HANDLED();
    }
    case HSM_EVT_E:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        ASSERT(
            (hsm_state_is_eq(&me->hsm, &HSM_STATE(s211))) ||
            (hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)))
        );
        me->foo = 0;
        LOG("s-E;");
        return HSM_TRAN(s11);

    case HSM_EVT_TERM:
        LOG("s->TERM");
        return HSM_HANDLED();

    default:
        break;
    }
    return HSM_SUPER(hsm_top);
}

static enum hsm_rc s1(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        LOG("s1-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        LOG("s1-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_TRAN(s11);

    case HSM_EVT_EXIT:
        LOG("s1-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_I:
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s1-I;");
        return HSM_HANDLED();

    case HSM_EVT_C:
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s1-C;");
        return HSM_TRAN(s2);

    case HSM_EVT_F:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s1-F;");
        return HSM_TRAN(s211);

    case HSM_EVT_A:
        LOG("s1-A;");
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_TRAN(s1);

    case HSM_EVT_B:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s1-B;");
        return HSM_TRAN(s11);

    case HSM_EVT_D:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        if (!me->foo) {
            me->foo = 1;
            LOG("s1-D;");
            return HSM_TRAN(s);
        }
        return HSM_HANDLED();

    default:
        break;
    }
    return HSM_SUPER(s);
}

static enum hsm_rc s11(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        LOG("s11-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_EXIT:
        LOG("s11-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_G:
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s11-G;");
        return HSM_TRAN(s211);

    case HSM_EVT_H:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s11-H;");
        return HSM_TRAN(s);

    case HSM_EVT_D:
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s11)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        if (me->foo) {
            me->foo = 0;
            LOG("s11-D;");
            return HSM_TRAN(s1);
        }
        break;

    default:
        break;
    }
    return HSM_SUPER(s1);
}

static enum hsm_rc s2(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        LOG("s2-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        LOG("s2-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_TRAN(s211);

    case HSM_EVT_EXIT:
        LOG("s2-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_I:
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        if (!me->foo) {
            me->foo = 1;
            LOG("s2-I;");
            return HSM_HANDLED();
        }
        break;

    case HSM_EVT_F:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s2-F;");
        return HSM_TRAN(s11);

    case HSM_EVT_C:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s2-C;");
        return HSM_TRAN(s1);

    default:
        break;
    }
    return HSM_SUPER(s);
}

static enum hsm_rc s21(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        LOG("s21-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        LOG("s21-INIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_TRAN(s211);

    case HSM_EVT_EXIT:
        LOG("s21-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_A:
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s21-A;");
        return HSM_TRAN(s21);

    case HSM_EVT_B:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s21-B;");
        return HSM_TRAN(s211);

    case HSM_EVT_G:
        LOG("s21-G;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_state_is_eq(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_TRAN(s1);

    default:
        break;
    }
    return HSM_SUPER(s2);
}

static enum hsm_rc s211(struct test *me, const struct event *event) {
    switch (event->id) {
    case HSM_EVT_ENTRY:
        LOG("s211-ENTRY;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_EXIT:
        LOG("s211-EXIT;");
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_INIT:
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        return HSM_HANDLED();

    case HSM_EVT_D:
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s11)));
        ASSERT(!hsm_is_in(&me->hsm, &HSM_STATE(s1)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s211)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s21)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s2)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(s)));
        ASSERT(hsm_is_in(&me->hsm, &HSM_STATE(hsm_top)));
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s211-D;");
        return HSM_TRAN(s21);

    case HSM_EVT_H:
        ASSERT(hsm_get_instance(&me->hsm) == 0);
        LOG("s211-H;");
        return HSM_TRAN(s);

    default:
        break;
    }
    return HSM_SUPER(s21);
}

static void test_ctor(void) { hsm_ctor(&m_test.hsm, &HSM_STATE(test_init)); }

static void test_hsm(void) {
    test_ctor();

    hsm_init(&m_test.hsm, /*init_event=*/NULL);

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
        {HSM_EVT_G, "s21-G;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;"},
        {HSM_EVT_I, "s1-I;"},
        {HSM_EVT_A, "s1-A;s11-EXIT;s1-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;"},
        {HSM_EVT_B, "s1-B;s11-EXIT;s11-ENTRY;"},
        {HSM_EVT_D, "s1-D;s11-EXIT;s1-EXIT;s-INIT;s1-ENTRY;s11-ENTRY;"},
        {HSM_EVT_H, "s11-H;s11-EXIT;s1-EXIT;s-INIT;s1-ENTRY;s11-ENTRY;"},
        {HSM_EVT_D, "s11-D;s11-EXIT;s1-INIT;s11-ENTRY;"},
        {HSM_EVT_F, "s1-F;s11-EXIT;s1-EXIT;s2-ENTRY;s21-ENTRY;s211-ENTRY;"},
        {HSM_EVT_F, "s2-F;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s11-ENTRY;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;"},
        {HSM_EVT_E, "s-E;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s11-ENTRY;"},
        {HSM_EVT_E, "s-E;s11-EXIT;s1-EXIT;s1-ENTRY;s11-ENTRY;"},
        {HSM_EVT_G, "s11-G;s11-EXIT;s1-EXIT;s2-ENTRY;s21-ENTRY;s211-ENTRY;"},
        {HSM_EVT_I, "s2-I;"},
        {HSM_EVT_C, "s2-C;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;"},
        {HSM_EVT_D, "s211-D;s211-EXIT;s21-INIT;s211-ENTRY;"},
        {HSM_EVT_H, "s211-H;s211-EXIT;s21-EXIT;s2-EXIT;s-INIT;s1-ENTRY;s11-ENTRY;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;"},
        {HSM_EVT_A, "s21-A;s211-EXIT;s21-EXIT;s21-ENTRY;s21-INIT;s211-ENTRY;"},
        {HSM_EVT_G, "s21-G;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s1-INIT;s11-ENTRY;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;"},
        {HSM_EVT_F, "s2-F;s211-EXIT;s21-EXIT;s2-EXIT;s1-ENTRY;s11-ENTRY;"},
        {HSM_EVT_C, "s1-C;s11-EXIT;s1-EXIT;s2-ENTRY;s2-INIT;s21-ENTRY;s211-ENTRY;"},
        {HSM_EVT_I, "s-I;"}
        /* clang-format on */
    };

    for (int i = 0; i < ARRAY_SIZE(in); i++) {
        struct event e = {.id = in[i].evt};
        hsm_dispatch(&m_test.hsm, &e);
        ASSERT(0 == strncmp(m_log_buf, in[i].out, strlen(in[i].out)));
        m_log_buf[0] = '\0';
    }

    hsm_dtor(&m_test.hsm);

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
