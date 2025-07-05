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
 * @file types.h
 * @brief Common types.
 */

#ifndef AM_TYPES_H_INCLUDED
#define AM_TYPES_H_INCLUDED

/**
 * Return codes.
 */
enum am_rc {
    /** Operation is ongoing. */
    AM_RC_BUSY = 0,
    /** Operation is complete. */
    AM_RC_DONE,
    /** Event was handled. */
    AM_RC_HANDLED = AM_RC_DONE,
    /** Event caused state transition. */
    AM_RC_TRAN,
    /** Event caused state transition and redispatch. */
    AM_RC_TRAN_REDISPATCH,
    /** Event propagation to superstate(s) was requested. */
    AM_RC_SUPER
};

/** Memory block descriptor. */
struct am_blk {
    void *ptr; /**< memory pointer */
    int size;  /**< memory size [bytes] */
};

#endif /* AM_TYPES_H_INCLUDED */
