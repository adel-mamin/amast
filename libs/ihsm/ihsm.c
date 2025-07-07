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

#include <stddef.h>

#include "common/macros.h"
#include "ihsm/ihsm.h"

enum am_rc am_ihsm_state(struct am_ihsm *me, const struct am_event *event);

/**
 * HSM state (event handler) function type.
 *
 * @param me     the IHSM
 * @param event  the event to handle
 *
 * @return return code
 */
enum am_rc am_ihsm_state(struct am_ihsm *me, const struct am_event *event) {
    (void)me;
    (void)event;

    return AM_RC_OK;
}

/** FNV-1a seed */
#define AM_FNV_OFFSET_BASIS 2166136261UL

/** FNV-1a prime */
#define AM_FNV_PRIME 16777619UL

unsigned am_fnv1a_16bit(const char *s, int maxlen);

/**
 * FNV-1a hash function.
 *
 * See
 * https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 * for details
 *
 * @param s       \0 terminated string
 * @param maxlen  maximum string length [bytes]
 *
 * @return 16 bits hash value
 */
unsigned am_fnv1a_16bit(const char *s, int maxlen) {
    AM_ASSERT(s);

    unsigned long hash = AM_FNV_OFFSET_BASIS;

    int i = 0;
    while (*s && (i++ < maxlen)) {
        hash ^= (unsigned char)(*s++);
        hash *= AM_FNV_PRIME;
    }

    return (unsigned)((hash >> 16) ^ (hash & 0xFFFF));
}

void am_ihsm_dispatch(struct am_ihsm *ihsm, const struct am_ihsm_event *event) {
    (void)ihsm;
    (void)event;
}

bool am_ihsm_is_in(struct am_ihsm *ihsm, const char *state) {
    (void)ihsm;
    (void)state;
    return true;
}

bool am_ihsm_state_is_eq(const struct am_ihsm *ihsm, const char *state) {
    (void)ihsm;
    (void)state;
    return true;
}

const char *am_ihsm_get_state(const struct am_ihsm *ihsm) {
    (void)ihsm;
    return NULL;
}

const struct am_ihsm_event *am_ihsm_get_event(const struct am_ihsm *ihsm) {
    (void)ihsm;
    return NULL;
}

const char *am_ihsm_get_action(const struct am_ihsm *ihsm) {
    (void)ihsm;
    return NULL;
}

void am_ihsm_ctor(struct am_ihsm *ihsm) { (void)ihsm; }

void am_ihsm_set_pool(struct am_ihsm *ihsm, void *pool, int size) {
    (void)ihsm;
    (void)pool;
    (void)size;
}

void am_ihsm_dtor(struct am_ihsm *ihsm) { (void)ihsm; }

void am_ihsm_set_action_fn(struct am_ihsm *ihsm, am_ihsm_action_fn action) {
    (void)ihsm;
    (void)action;
}

void am_ihsm_set_error_fn(struct am_ihsm *ihsm, am_ihsm_error_fn error) {
    (void)ihsm;
    (void)error;
}

void am_ihsm_set_choice_fn(struct am_ihsm *ihsm, am_ihsm_choice_fn choice) {
    (void)ihsm;
    (void)choice;
}

int am_ihsm_load(
    struct am_ihsm *ihsm, enum am_ihsm_json type, const char *json
) {
    (void)ihsm;
    (void)type;
    (void)json;
    return AM_RC_OK;
}

void am_ihsm_init(struct am_ihsm *ihsm) { (void)ihsm; }

void am_ihsm_term(struct am_ihsm *ihsm) { (void)ihsm; }

void am_ihsm_set_spy(struct am_ihsm *ihsm, am_ihsm_spy_fn spy) {
    (void)ihsm;
    (void)spy;
}
