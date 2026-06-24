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

#include "coro/coro.h"

static int am_coro_reentrant(struct am_coro* me, int* reent, int* state) {
    ++(*reent);
    AM_CORO_BEGIN(me);
    if (*state == 0) {
        *state = 1;
        return AM_RC_CORO_DONE;
    }
    if (*state == 1) {
        *state = 2;
        return AM_RC_CORO_DONE;
    }
    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static void test_coro_local_continuation(void) {
    int reent = 0;
    int state = 0;
    struct am_coro me;

    am_coro_ctor(&me);

    am_coro_reentrant(&me, &reent, &state);
    AM_ASSERT(!am_coro_is_busy(&me) && (reent == 1) && (state == 1));

    am_coro_reentrant(&me, &reent, &state);
    AM_ASSERT(!am_coro_is_busy(&me) && (reent == 2) && (state == 2));

    am_coro_reentrant(&me, &reent, &state);
    AM_ASSERT(!am_coro_is_busy(&me) && (reent == 3) && (state == 2));

    am_coro_reentrant(&me, &reent, &state);
    AM_ASSERT(!am_coro_is_busy(&me) && (reent == 4) && (state == 2));
}

static int am_coro_empty(struct am_coro* me, int* reent) {
    AM_CORO_BEGIN(me);
    ++(*reent);
    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static void test_coro_empty(void) {
    struct am_coro me;
    am_coro_ctor(&me);

    int reent = 0;
    am_coro_empty(&me, &reent);
    AM_ASSERT(reent == 1);
    am_coro_empty(&me, &reent);
    AM_ASSERT(reent == 2);
}

static int am_coro_wait_ready(struct am_coro* me, int* reent, int ready) {
    AM_CORO_BEGIN(me);
    ++(*reent);
    AM_CORO_AWAIT(me, ready);
    ++(*reent);
    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static void test_coro_wait_ready(void) {
    struct am_coro me;
    am_coro_ctor(&me);

    int ready = 0;
    int reent = 0;
    am_coro_wait_ready(&me, &reent, ready);
    AM_ASSERT(reent == 1);
    am_coro_wait_ready(&me, &reent, ready);
    AM_ASSERT(reent == 1);

    ready = 1;

    am_coro_wait_ready(&me, &reent, ready);
    AM_ASSERT(reent == 2);
}

static int am_coro_yield(struct am_coro* me, int* state) {
    AM_CORO_BEGIN(me);
    (*state) = 1;
    AM_CORO_YIELD(me);
    (*state) = 2;
    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static void test_coro_yield(void) {
    struct am_coro me1;
    struct am_coro me2;

    am_coro_ctor(&me1);
    am_coro_ctor(&me2);

    int state = 0;
    am_coro_yield(&me1, &state);
    AM_ASSERT(am_coro_is_busy(&me1) && (1 == state));

    am_coro_yield(&me2, &state);
    AM_ASSERT(am_coro_is_busy(&me2) && (1 == state));

    am_coro_yield(&me1, &state);
    AM_ASSERT(!am_coro_is_busy(&me1) && (2 == state));

    am_coro_yield(&me2, &state);
    AM_ASSERT(!am_coro_is_busy(&me2) && (2 == state));
}

static int am_coro_exit(struct am_coro* me, int* state) {
    AM_CORO_BEGIN(me);
    (*state) = 1;
    return AM_RC_CORO_DONE;
    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static void test_coro_exit(void) {
    struct am_coro me;
    am_coro_ctor(&me);

    int state = 0;
    am_coro_exit(&me, &state);
    AM_ASSERT((1 == state) && !am_coro_is_busy(&me));

    am_coro_exit(&me, &state);
    AM_ASSERT((1 == state) && !am_coro_is_busy(&me));
}

static struct am_coro_chain {
    struct am_coro coro;
    int ready;
    int foo;
} test_coro_chain[3];

static enum am_rc am_coro_call_1(struct am_coro_chain* me);
static enum am_rc am_coro_call_2(struct am_coro_chain* me);

static enum am_rc am_coro_call_1(struct am_coro_chain* me) {
    struct am_coro* coro = &me->coro;

    AM_CORO_BEGIN(coro);
    AM_CORO_CALL(coro, am_coro_call_2(&test_coro_chain[1]));
    AM_CORO_AWAIT(coro, me->ready);
    me->foo = 1;
    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static enum am_rc am_coro_call_2(struct am_coro_chain* me) {
    struct am_coro* coro = &me->coro;

    AM_CORO_BEGIN(coro);
    AM_CORO_AWAIT(coro, me->ready);
    me->foo = 1;
    AM_CORO_END();

    return AM_RC_CORO_DONE;
}

static void test_coro_call_chain(void) {
    for (int i = 0; i < 2; ++i) {
        memset(test_coro_chain, 0, sizeof(test_coro_chain));

        struct am_coro_chain* me1 = &test_coro_chain[0];
        struct am_coro_chain* me2 = &test_coro_chain[1];

        am_coro_ctor(&me1->coro);
        am_coro_ctor(&me2->coro);

        am_coro_call_1(me1);
        AM_ASSERT(am_coro_is_busy(&me1->coro));
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(0 == me2->foo);

        am_coro_call_1(me1);
        AM_ASSERT(am_coro_is_busy(&me1->coro));
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(0 == me2->foo);

        me2->ready = 1;
        am_coro_call_1(me1);
        AM_ASSERT(am_coro_is_busy(&me1->coro));
        AM_ASSERT(0 == me1->foo);
        AM_ASSERT(1 == me2->foo);

        me1->ready = 1;
        am_coro_call_1(me1);
        AM_ASSERT(!am_coro_is_busy(&me1->coro));
        AM_ASSERT(1 == me1->foo);
        AM_ASSERT(1 == me2->foo);
    }
}

int main(void) {
    test_coro_local_continuation();
    test_coro_empty();
    test_coro_wait_ready();
    test_coro_yield();
    test_coro_exit();
    test_coro_call_chain();

    return 0;
}
