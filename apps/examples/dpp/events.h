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

#ifndef DPP_EVENTS_H_INCLUDED
#define DPP_EVENTS_H_INCLUDED

#include "common/macros.h"

enum events {
    EVT_DONE = AM_EVT_USER,
    EVT_EAT,
    EVT_TIMEOUT,
    EVT_STOP,
    EVT_STOPPED,
    AM_AO_EVT_PUB_MAX,

    EVT_HUNGRY,
    EVT_MAX
};

const char* event_to_str(int id);

struct hungry {
    struct am_event event;
    int philo;
};

struct done {
    struct am_event event;
    int philo;
};

struct eat {
    struct am_event event;
    int philo;
};

#endif /* #ifndef DPP_EVENTS_H_INCLUDED */
