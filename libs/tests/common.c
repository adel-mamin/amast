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

/**
 * @file
 * Common API unit tests
 */

#include <stdint.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "pal/pal.h"

static void do_each_ms(int *i, struct am_do_ctx *ctx, uint32_t now_ms) {
    AM_DO_EACH_MS(1, ctx, now_ms) {
        ++(*i);
    }
}

static void test_do_each_ms(void) {
    struct am_do_ctx ctx = {0};
    int i = 0;
    do_each_ms(&i, &ctx, /*now_ms=*/0);
    AM_ASSERT(1 == i);

    do_each_ms(&i, &ctx, /*now_ms=*/0);
    AM_ASSERT(1 == i);

    do_each_ms(&i, &ctx, /*now_ms=*/1);
    AM_ASSERT(2 == i);

    do_each_ms(&i, &ctx, /*now_ms=*/1);
    AM_ASSERT(2 == i);
}

static void do_once(struct am_do_ctx *ctx, int *i) {
    AM_DO_ONCE(ctx) {
        ++(*i);
    }
}

static void test_do_once(void) {
    struct am_do_ctx ctx = {0};
    int i = 0;
    do_once(&ctx, &i);
    AM_ASSERT(1 == i);

    do_once(&ctx, &i);
    AM_ASSERT(1 == i);
}

static void do_every(struct am_do_ctx *ctx, int *i) {
    AM_DO_EVERY(2, ctx) {
        ++(*i);
    }
}

static void test_do_every(void) {
    struct am_do_ctx ctx = {0};
    int i = 0;
    do_every(&ctx, &i);
    AM_ASSERT(1 == i);

    do_every(&ctx, &i);
    AM_ASSERT(1 == i);

    do_every(&ctx, &i);
    AM_ASSERT(2 == i);

    do_every(&ctx, &i);
    AM_ASSERT(2 == i);

    do_every(&ctx, &i);
    AM_ASSERT(3 == i);
}

int main(void) {
    test_do_each_ms();
    test_do_once();
    test_do_every();

    return 0;
}
