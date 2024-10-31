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

#include <string.h>

#include "common/macros.h"
#include "common/compiler.h" /* IWYU pragma: keep */

#include "async.h"

static enum am_async_rc am_async_reentrant(
    struct am_async *me, int *reent, int *state
) {
    (*reent)++;
    AM_ASYNC_BEGIN(me);
    AM_ASYNC_LABEL();
    if (*state == 0) {
        *state = 1;
        AM_ASYNC_EXIT();
    }
    AM_ASYNC_LABEL();
    if (*state == 1) {
        *state = 2;
        AM_ASYNC_EXIT();
    }
    AM_ASYNC_END();
}

static void test_async_local_continuation(void) {
    int reent = 0;
    int state = 0;
    struct am_async me;

    am_async_init(&me);

    enum am_async_rc rc = am_async_reentrant(&me, &reent, &state);
    AM_ASSERT((AM_ASYNC_RC_DONE == rc) && (reent == 1) && (state == 1));

    rc = am_async_reentrant(&me, &reent, &state);
    AM_ASSERT((AM_ASYNC_RC_DONE == rc) && (reent == 2) && (state == 2));

    rc = am_async_reentrant(&me, &reent, &state);
    AM_ASSERT((AM_ASYNC_RC_DONE == rc) && (reent == 3) && (state == 2));

    rc = am_async_reentrant(&me, &reent, &state);
    AM_ASSERT((AM_ASYNC_RC_DONE == rc) && (reent == 4) && (state == 2));
}

static enum am_async_rc am_async_empty(struct am_async *me, int *reent) {
    AM_ASYNC_BEGIN(me);
    (*reent)++;
    AM_ASYNC_END();
}

static void test_async_empty(void) {
    struct am_async me;
    am_async_init(&me);

    int reent = 0;
    am_async_empty(&me, &reent);
    AM_ASSERT(reent == 1);
    am_async_empty(&me, &reent);
    AM_ASSERT(reent == 2);
}

static enum am_async_rc am_async_wait_ready(
    struct am_async *me, int *reent, int ready
) {
    AM_ASYNC_BEGIN(me);
    (*reent)++;
    AM_ASYNC_AWAIT(ready);
    (*reent)++;
    AM_ASYNC_END();
}

static void test_async_wait_ready(void) {
    struct am_async me;
    am_async_init(&me);

    int ready = 0;
    int reent = 0;
    am_async_wait_ready(&me, &reent, ready);
    AM_ASSERT(reent == 1);
    am_async_wait_ready(&me, &reent, ready);
    AM_ASSERT(reent == 1);

    ready = 1;

    am_async_wait_ready(&me, &reent, ready);
    AM_ASSERT(reent == 2);
}

static enum am_async_rc am_async_yield(struct am_async *me, int *state) {
    AM_ASYNC_BEGIN(me);
    (*state) = 1;
    AM_ASYNC_YIELD();
    (*state) = 2;
    AM_ASYNC_END();
}

static void test_async_yield(void) {
    struct am_async me1;
    struct am_async me2;

    am_async_init(&me1);
    am_async_init(&me2);

    int state = 0;
    enum am_async_rc rc = am_async_yield(&me1, &state);
    AM_ASSERT((AM_ASYNC_RC_BUSY == rc) && (1 == state));

    rc = am_async_yield(&me2, &state);
    AM_ASSERT((AM_ASYNC_RC_BUSY == rc) && (1 == state));

    rc = am_async_yield(&me1, &state);
    AM_ASSERT((AM_ASYNC_RC_DONE == rc) && (2 == state));

    rc = am_async_yield(&me2, &state);
    AM_ASSERT((AM_ASYNC_RC_DONE == rc) && (2 == state));
}

static enum am_async_rc am_async_exit(struct am_async *me, int *state) {
    AM_ASYNC_BEGIN(me);
    (*state) = 1;
    AM_ASYNC_EXIT();
    AM_DISABLE_WARNING(AM_W_UNREACHABLE_CODE);
    (*state) = 2;
    AM_ENABLE_WARNING(AM_W_UNREACHABLE_CODE);
    AM_ASYNC_END();
}

static void test_async_exit(void) {
    struct am_async me;
    am_async_init(&me);

    int state = 0;
    enum am_async_rc rc = am_async_exit(&me, &state);
    AM_ASSERT((1 == state) && (AM_ASYNC_RC_DONE == rc));

    rc = am_async_exit(&me, &state);
    AM_ASSERT((1 == state) && (AM_ASYNC_RC_DONE == rc));
}

static struct am_async_chain {
    struct am_async async;
    int ready;
    int foo;
} test_async_chain[3];

static enum am_async_rc am_async_call_1(struct am_async_chain *me);
static enum am_async_rc am_async_call_2(struct am_async_chain *me);

static enum am_async_rc am_async_call_1(struct am_async_chain *me) {
    AM_ASYNC_BEGIN(me);
    enum am_async_rc rc = am_async_call_2(&test_async_chain[1]);
    if (AM_ASYNC_RC_BUSY == rc) {
        return rc;
    }
    AM_ASYNC_AWAIT(me->ready);
    me->foo = 1;
    AM_ASYNC_END();
}

static enum am_async_rc am_async_call_2(struct am_async_chain *me) {
    AM_ASYNC_BEGIN(me);
    AM_ASYNC_AWAIT(me->ready);
    me->foo = 1;
    AM_ASYNC_END();
}

static void test_async_call_chain(void) {
    for (int i = 0; i < 2; ++i) {
        memset(test_async_chain, 0, sizeof(test_async_chain));

        struct am_async_chain *me1 = &test_async_chain[0];
        struct am_async_chain *me2 = &test_async_chain[1];

        am_async_init(&me1->async);
        am_async_init(&me2->async);

        enum am_async_rc rc = am_async_call_1(me1);
        AM_ASSERT(AM_ASYNC_RC_BUSY == rc);
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(0 == me2->foo);

        rc = am_async_call_1(me1);
        AM_ASSERT(AM_ASYNC_RC_BUSY == rc);
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(0 == me2->foo);

        me2->ready = 1;
        rc = am_async_call_1(me1);
        AM_ASSERT(AM_ASYNC_RC_BUSY == rc);
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(1 == me2->foo);

        me1->ready = 1;
        rc = am_async_call_1(me1);
        AM_ASSERT(AM_ASYNC_RC_DONE == rc);
        AM_ASSERT(1 == me1->foo);
        AM_ASSERT(1 == me2->foo);
    }
}

int main(void) {
    test_async_local_continuation();
    test_async_empty();
    test_async_wait_ready();
    test_async_yield();
    test_async_exit();
    test_async_call_chain();

    return 0;
}
