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

#include <string.h>

#include "common/macros.h"
#include "common/types.h"

#include "async/async.h"

static int am_async_reentrant(struct am_async *me, int *reent, int *state) {
    ++(*reent);
    AM_ASYNC_BEGIN(me);
    if (*state == 0) {
        *state = 1;
        return AM_RC_DONE;
    }
    if (*state == 1) {
        *state = 2;
        return AM_RC_DONE;
    }
    AM_ASYNC_END();

    return AM_RC_DONE;
}

static void test_async_local_continuation(void) {
    int reent = 0;
    int state = 0;
    struct am_async me;

    am_async_ctor(&me);

    am_async_reentrant(&me, &reent, &state);
    AM_ASSERT(!am_async_is_busy(&me) && (reent == 1) && (state == 1));

    am_async_reentrant(&me, &reent, &state);
    AM_ASSERT(!am_async_is_busy(&me) && (reent == 2) && (state == 2));

    am_async_reentrant(&me, &reent, &state);
    AM_ASSERT(!am_async_is_busy(&me) && (reent == 3) && (state == 2));

    am_async_reentrant(&me, &reent, &state);
    AM_ASSERT(!am_async_is_busy(&me) && (reent == 4) && (state == 2));
}

static int am_async_empty(struct am_async *me, int *reent) {
    AM_ASYNC_BEGIN(me);
    ++(*reent);
    AM_ASYNC_END();

    return AM_RC_DONE;
}

static void test_async_empty(void) {
    struct am_async me;
    am_async_ctor(&me);

    int reent = 0;
    am_async_empty(&me, &reent);
    AM_ASSERT(reent == 1);
    am_async_empty(&me, &reent);
    AM_ASSERT(reent == 2);
}

static int am_async_wait_ready(struct am_async *me, int *reent, int ready) {
    AM_ASYNC_BEGIN(me);
    ++(*reent);
    AM_ASYNC_AWAIT(ready);
    ++(*reent);
    AM_ASYNC_END();

    return AM_RC_DONE;
}

static void test_async_wait_ready(void) {
    struct am_async me;
    am_async_ctor(&me);

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

static int am_async_yield(struct am_async *me, int *state) {
    AM_ASYNC_BEGIN(me);
    (*state) = 1;
    AM_ASYNC_YIELD();
    (*state) = 2;
    AM_ASYNC_END();

    return AM_RC_DONE;
}

static void test_async_yield(void) {
    struct am_async me1;
    struct am_async me2;

    am_async_ctor(&me1);
    am_async_ctor(&me2);

    int state = 0;
    am_async_yield(&me1, &state);
    AM_ASSERT(am_async_is_busy(&me1) && (1 == state));

    am_async_yield(&me2, &state);
    AM_ASSERT(am_async_is_busy(&me2) && (1 == state));

    am_async_yield(&me1, &state);
    AM_ASSERT(!am_async_is_busy(&me1) && (2 == state));

    am_async_yield(&me2, &state);
    AM_ASSERT(!am_async_is_busy(&me2) && (2 == state));
}

static int am_async_exit(struct am_async *me, int *state) {
    AM_ASYNC_BEGIN(me);
    (*state) = 1;
    return AM_RC_DONE;
    AM_ASYNC_END();

    return AM_RC_DONE;
}

static void test_async_exit(void) {
    struct am_async me;
    am_async_ctor(&me);

    int state = 0;
    am_async_exit(&me, &state);
    AM_ASSERT((1 == state) && !am_async_is_busy(&me));

    am_async_exit(&me, &state);
    AM_ASSERT((1 == state) && !am_async_is_busy(&me));
}

static struct am_async_chain {
    struct am_async async;
    int ready;
    int foo;
} test_async_chain[3];

static enum am_rc am_async_call_1(struct am_async_chain *me);
static enum am_rc am_async_call_2(struct am_async_chain *me);

static enum am_rc am_async_call_1(struct am_async_chain *me) {
    AM_ASYNC_BEGIN(me);
    AM_ASYNC_CHAIN(am_async_call_2(&test_async_chain[1]));
    AM_ASYNC_AWAIT(me->ready);
    me->foo = 1;
    AM_ASYNC_END();

    return AM_RC_DONE;
}

static enum am_rc am_async_call_2(struct am_async_chain *me) {
    AM_ASYNC_BEGIN(me);
    AM_ASYNC_AWAIT(me->ready);
    me->foo = 1;
    AM_ASYNC_END();

    return AM_RC_DONE;
}

static void test_async_call_chain(void) {
    for (int i = 0; i < 2; ++i) {
        memset(test_async_chain, 0, sizeof(test_async_chain));

        struct am_async_chain *me1 = &test_async_chain[0];
        struct am_async_chain *me2 = &test_async_chain[1];

        am_async_ctor(&me1->async);
        am_async_ctor(&me2->async);

        am_async_call_1(me1);
        AM_ASSERT(am_async_is_busy(&me1->async));
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(0 == me2->foo);

        am_async_call_1(me1);
        AM_ASSERT(am_async_is_busy(&me1->async));
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(0 == me2->foo);

        me2->ready = 1;
        am_async_call_1(me1);
        AM_ASSERT(am_async_is_busy(&me1->async));
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(1 == me2->foo);

        me1->ready = 1;
        am_async_call_1(me1);
        AM_ASSERT(!am_async_is_busy(&me1->async));
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
