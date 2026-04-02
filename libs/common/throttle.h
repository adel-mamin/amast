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
 * Throttle utility
 */

#ifndef AM_THROTTLE_H_INCLUDED
#define AM_THROTTLE_H_INCLUDED

#include <stdbool.h>
#include <inttypes.h>

/** The throttle state */
struct am_throttle {
    /** last allow timestamp */
    uint32_t last_ms;
    /** the state initialization status */
    bool initialized;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return true at most once per interval_ms.
 *
 * First call returns true immediately.
 * Then returns true again only after interval_ms has elapsed.
 *
 * Typical use:
 *   if (am_throttle_allow(&err_throttle, 1000, now_ms)) {
 *       ...
 *   }
 *
 * @param throttle     the throttle state
 * @param interval_ms  the throttle interval [ms]
 * @param now_ms       current time [ms]
 *
 * @retval true   on the first call and after interval_ms has elapsed
 * @retval false  otherwise
 */
bool am_throttle_allow(
    struct am_throttle* throttle, uint32_t interval_ms, uint32_t now_ms
);

#ifdef __cplusplus
}
#endif

#endif /* AM_THROTTLE_H_INCLUDED */
