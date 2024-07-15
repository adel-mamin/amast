/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Adel Mamin
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

#ifndef COMMON_TYPES_H_INCLUDED
#define COMMON_TYPES_H_INCLUDED

/** Event descriptor */
struct event {
    /** event ID */
    int id;

    /**
     * Has a special purpose in active objects framework (AOF).
     * Otherwise is unused.
     *
     * Below is the description of the purpose in AOF.
     * If pool index is set to zero, then the event is statically allocated.
     *
     *  15  14  13          11 10         6 5           0
     * +---+---+--------------+------------+-------------+
     * |n/a|PST| clock domain | pool index | ref counter |
     * +---+---+--------------+------------+-------------+
     *
     * n/a - reserved
     * PST - pubsub time
     */
    unsigned flags;
};

#endif /* COMMON_TYPES_H_INCLUDED */
