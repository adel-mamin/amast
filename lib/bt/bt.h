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

/**
 * @file
 *
 * Behavioral tree framework API declaration.
 */

#ifndef BT_H_INCLUDED
#define BT_H_INCLUDED

#include <stdbool.h>
#include "dlist/dlist.h"
#include "event/event.h"
#include "hsm/hsm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AM_BT_EVT_SUCCESS (AM_HSM_EVT_MAX + 1)
#define AM_BT_EVT_FAILURE (AM_HSM_EVT_MAX + 2)
#define AM_BT_EVT_MAX AM_BT_EVT_FAILURE
AM_ASSERT_STATIC(EVT_USER > AM_BT_EVT_MAX);

enum am_bt_type {
    AM_BT_INVERT = 1,
    AM_BT_FORCE_SUCCESS,
    AM_BT_FORCE_FAILURE,
    AM_BT_REPEAT
};

struct am_bt_cfg {
    struct am_dlist_item item;
    struct am_hsm *hsm;
    void (*post)(struct am_hsm *hsm, const struct am_event *event);
};

struct am_bt_node {
    enum am_bt_type type;
    struct am_hsm_state super;
};

struct am_bt_repeat {
    struct am_bt_node node;
    struct am_hsm_state child;
    int num;
};

enum am_hsm_rc am_bt_invert(struct am_hsm *hsm, const struct am_event *event);
enum am_hsm_rc am_bt_force_success(struct am_hsm *hsm, const struct am_event *event);
enum am_hsm_rc am_bt_force_failure(struct am_hsm *hsm, const struct am_event *event);
enum am_hsm_rc am_bt_repeat(struct am_hsm *hsm, const struct am_event *event);

void am_bt_add_cfg(struct am_bt_cfg *cfg);
struct am_bt_cfg *am_bt_get_cfg(struct am_hsm *hsm);
struct am_hsm_state *am_bt_get_superstate(enum am_bt_type type, struct am_hsm *hsm, int instance);
void am_bt_ctor(void);

#ifdef __cplusplus
}
#endif

#endif /* HSM_H_INCLUDED */
