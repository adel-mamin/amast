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

#include "common/compiler.h" /* IWYU pragma: keep */
#include "common/macros.h"
#include "blk/blk.h"

int main(void) {
    {
        unsigned long adata = 1;
        struct am_blk a = {&adata, sizeof(adata)};
        unsigned long bdata = 1;
        struct am_blk b = {&bdata, sizeof(bdata)};
        AM_ASSERT(0 == am_blk_cmp(&a, &b));
    }

    {
        unsigned long adata = 1;
        struct am_blk a = {&adata, sizeof(adata)};
        unsigned long bdata = 2;
        struct am_blk b = {&bdata, sizeof(bdata)};
        AM_ASSERT(am_blk_cmp(&a, &b) < 0);
    }

    {
        unsigned long adata = 2;
        struct am_blk a = {&adata, sizeof(adata)};
        unsigned long bdata = 1;
        struct am_blk b = {&bdata, sizeof(bdata)};
        AM_ASSERT(am_blk_cmp(&a, &b) > 0);
    }

    {
        unsigned char adata = 1;
        struct am_blk a = {&adata, sizeof(adata)};
        unsigned long bdata = 1;
        struct am_blk b = {&bdata, sizeof(bdata)};
        AM_ASSERT(am_blk_cmp(&a, &b) < 0);
    }

    return 0;
}
