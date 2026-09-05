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
 * Throttler utility
 */

#ifndef AM_THROTTLER_H_INCLUDED
#define AM_THROTTLER_H_INCLUDED

#include <stdbool.h>
#include <inttypes.h>

/** The throttler state */
struct am_throttler {
    /** last allow timestamp */
    uint32_t last_ticks;
    /** the state initialization status */
    bool initialized;
};

/** The throttler state constructor */
#define AM_THROTTLER_CREATE() (struct am_throttler){.initialized = 0}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return true at most once per interval_ticks.
 *
 * First call returns true immediately.
 * Then returns true again only after interval_ticks has elapsed.
 *
 * Typical use:
 *   if (am_throttler_allow(&throttler, 1000, now_ticks)) {
 *       ...
 *   }
 *
 * @param throttler       the throttler state
 * @param interval_ticks  the throttler interval [ticks]
 * @param now_ticks       current time [ticks]
 *
 * @retval true   on the first call and after interval_ticks has elapsed
 * @retval false  otherwise
 */
bool am_throttler_allow(
    struct am_throttler* throttler, uint32_t interval_ticks, uint32_t now_ticks
);

#ifdef __cplusplus
}
#endif

#endif /* AM_THROTTLER_H_INCLUDED */
