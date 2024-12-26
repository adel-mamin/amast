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

#ifndef AM_BLK_H_INCLUDED
#define AM_BLK_H_INCLUDED

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Memory block descriptor */
struct am_blk {
    void *ptr; /**< memory */
    int size;  /**< memory size [bytes] */
};

struct am_blk am_blk_ctor(void *ptr, int size);
struct am_blk am_blk_ctor_empty(void);
bool am_blk_is_empty(const struct am_blk *blk);
int am_blk_cmp(const struct am_blk *a, const struct am_blk *b);
void am_blk_zero(struct am_blk *blk);
void *am_blk_copy(struct am_blk *dst, const struct am_blk *src);

#ifdef __cplusplus
}
#endif

#endif /* AM_BLK_H_INCLUDED */
