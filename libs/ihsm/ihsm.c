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
 *
 * Interpreted Hierarchical State Machine (IHSM) libary API implementation.
 */

#include "common/macros.h"
#include "ihsm/ihsm.h"

enum am_ihsm_rc am_ihsm_state(struct am_ihsm *me, const struct am_event *event);

/**
 * HSM state (event handler) function type.
 *
 * @param me     the IHSM
 * @param event  the event to handle
 *
 * @return return code
 */
enum am_ihsm_rc am_ihsm_state(
    struct am_ihsm *me, const struct am_event *event
) {
    (void)me;
    (void)event;

    return AM_IHSM_RC_OK;
}

/** FNV-1a seed */
#define AM_FNV_OFFSET_BASIS 2166136261UL

/** FNV-1a prime */
#define AM_FNV_PRIME 16777619UL

unsigned am_fnv1a_16bit(const char *s);

/**
 * FNV-1a hash function.
 *
 * See
 * https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 * for details
 *
 * @param s  \0 terminated string
 *
 * @return 16 bits hash value
 */
unsigned am_fnv1a_16bit(const char *s) {
    AM_ASSERT(s);

    unsigned long hash = AM_FNV_OFFSET_BASIS;

    while (*s) {
        hash ^= (unsigned char)(*s++);
        hash *= AM_FNV_PRIME;
    }

    return (unsigned)((hash >> 16) ^ (hash & 0xFFFF));
}
