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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "event/event.h"
#include "hsm/hsm.h"
#include "bt/bt.h"

static const struct am_event am_bt_evt_success = {.id = AM_BT_EVT_SUCCESS};
static const struct am_event am_bt_evt_failure = {.id = AM_BT_EVT_FAILURE};

static struct am_dlist m_bt_cfg;

void am_bt_add_cfg(struct am_bt_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->hsm);
    AM_ASSERT(cfg->post);
    am_dlist_push_front(&m_bt_cfg, &cfg->item);
}

struct am_bt_cfg *am_bt_get_cfg(struct am_hsm *hsm) {
    struct am_dlist_iterator it;
    am_dlist_iterator_init(&m_bt_cfg, &it, AM_DLIST_FORWARD);
    struct am_dlist_item *item;
    struct am_bt_cfg *cfg = NULL;
    while ((item = am_dlist_iterator_next(&it)) != NULL) {
        cfg = AM_CONTAINER_OF(item, struct am_bt_cfg, item);
        if (cfg->hsm == hsm) {
            break;
        }
    }
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->post);
    return cfg;
}

struct am_hsm_state *am_bt_get_superstate(enum am_bt_type type, struct am_hsm *hsm, int instance) {
    (void)type;
    (void)hsm;
    (void)instance;
    return NULL;
}

enum am_hsm_rc am_bt_invert(struct am_hsm *me, const struct am_event *event) {
    switch (event->id) {
    case AM_BT_EVT_SUCCESS: {
        struct am_bt_cfg *cfg = am_bt_get_cfg(me);
        cfg->post(me, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    }
    case AM_BT_EVT_FAILURE: {
        struct am_bt_cfg *cfg = am_bt_get_cfg(me);
        cfg->post(me, &am_bt_evt_success);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    int i = am_hsm_get_state_instance(me);
    struct am_hsm_state *s = am_bt_get_superstate(AM_BT_INVERT, me, i);
    return AM_HSM_SUPER(s->fn, s->ifn);
}

enum am_hsm_rc am_bt_force_success(
    struct am_hsm *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_BT_EVT_FAILURE: {
        struct am_bt_cfg *cfg = am_bt_get_cfg(me);
        cfg->post(me, &am_bt_evt_success);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    int i = am_hsm_get_state_instance(me);
    struct am_hsm_state *s = am_bt_get_superstate(AM_BT_FORCE_SUCCESS, me, i);
    return AM_HSM_SUPER(s->fn, s->ifn);
}

enum am_hsm_rc am_bt_force_failure(
    struct am_hsm *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_BT_EVT_SUCCESS: {
        struct am_bt_cfg *cfg = am_bt_get_cfg(me);
        cfg->post(me, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    int i = am_hsm_get_state_instance(me);
    struct am_hsm_state *s = am_bt_get_superstate(AM_BT_FORCE_FAILURE, me, i);
    return AM_HSM_SUPER(s->fn, s->ifn);
}

enum am_hsm_rc am_bt_repeat(struct am_hsm *me, const struct am_event *event) {
    switch (event->id) {
    case AM_BT_EVT_SUCCESS: {
        struct am_bt_cfg *cfg = am_bt_get_cfg(me);
        cfg->post(me, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    int i = am_hsm_get_state_instance(me);
    struct am_hsm_state *s = am_bt_get_superstate(AM_BT_FORCE_FAILURE, me, i);
    return AM_HSM_SUPER(s->fn, s->ifn);
}

void am_bt_ctor(void) { am_dlist_init(&m_bt_cfg); }
