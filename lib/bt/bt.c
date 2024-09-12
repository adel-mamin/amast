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
#include "dlist/dlist.h"
#include "hsm/hsm.h"
#include "bt/bt.h"
#include "timer/timer.h"

struct am_bt {
    struct am_dlist cfg;
    struct {
        struct am_bt_node *node;
        int num;
    } types[AM_BT_TYPES_NUM];
};

static struct am_bt m_bt;

static const struct am_event am_bt_evt_success = {.id = AM_BT_EVT_SUCCESS};
static const struct am_event am_bt_evt_failure = {.id = AM_BT_EVT_FAILURE};

void am_bt_add_cfg(struct am_bt_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->hsm);
    AM_ASSERT(cfg->post);
    am_dlist_push_front(&m_bt.cfg, &cfg->item);
}

void am_bt_add_type(enum am_bt_type type, struct am_bt_node *node, int num) {
    AM_ASSERT(type >= AM_BT_TYPES_MIN);
    AM_ASSERT(type < AM_COUNTOF(m_bt.types));
    AM_ASSERT(node);
    AM_ASSERT(num > 0);
    m_bt.types[type].node = node;
    m_bt.types[type].num = num;
}

struct am_bt_cfg *am_bt_get_cfg(struct am_hsm *hsm) {
    struct am_dlist_iterator it;
    am_dlist_iterator_init(&m_bt.cfg, &it, AM_DLIST_FORWARD);
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

struct am_bt_node *am_bt_get_node(enum am_bt_type type, int instance) {
    AM_ASSERT(type >= AM_BT_TYPES_MIN);
    AM_ASSERT(type < AM_COUNTOF(m_bt.types));
    AM_ASSERT(instance < m_bt.types[type].num);
    return m_bt.types[type].node;
}

enum am_hsm_rc am_bt_invert(struct am_hsm *me, const struct am_event *event) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_INVERT, i);
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        struct am_bt_invert *p = (struct am_bt_invert *)node;
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
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
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

enum am_hsm_rc am_bt_force_success(
    struct am_hsm *me, const struct am_event *event
) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_FORCE_FAILURE, i);
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        struct am_bt_force_success *p = (struct am_bt_force_success *)node;
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
    case AM_BT_EVT_FAILURE: {
        struct am_bt_cfg *cfg = am_bt_get_cfg(me);
        cfg->post(me, &am_bt_evt_success);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

enum am_hsm_rc am_bt_force_failure(
    struct am_hsm *me, const struct am_event *event
) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_FORCE_FAILURE, i);
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        struct am_bt_force_failure *p = (struct am_bt_force_failure *)node;
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
    case AM_BT_EVT_SUCCESS: {
        struct am_bt_cfg *cfg = am_bt_get_cfg(me);
        cfg->post(me, &am_bt_evt_failure);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

enum am_hsm_rc am_bt_repeat(struct am_hsm *me, const struct am_event *event) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_REPEAT, i);
    struct am_bt_repeat *p = (struct am_bt_repeat *)node;

    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        p->done = 0;
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_INIT: {
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
    case AM_BT_EVT_SUCCESS: {
        ++p->done;
        if (p->done < p->total) {
            return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
        }
        break;
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

enum am_hsm_rc am_bt_retry_until_success(
    struct am_hsm *me, const struct am_event *event
) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_RETRY_UNTIL_SUCCESS, i);
    struct am_bt_retry_until_success *p =
        (struct am_bt_retry_until_success *)node;

    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        p->attempts_done = 0;
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_INIT: {
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
    case AM_BT_EVT_FAILURE: {
        ++p->attempts_done;
        if (p->attempts_done < p->attempts_total) {
            return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
        }
        break;
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

enum am_hsm_rc am_bt_run_until_failure(
    struct am_hsm *me, const struct am_event *event
) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_RUN_UNTIL_FAILURE, i);

    switch (event->id) {
    case AM_HSM_EVT_INIT:
    case AM_BT_EVT_SUCCESS: {
        struct am_bt_run_until_failure *p =
            (struct am_bt_run_until_failure *)node;
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

enum am_hsm_rc am_bt_delay(struct am_hsm *me, const struct am_event *event) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_DELAY, i);
    struct am_bt_delay *p = (struct am_bt_delay *)node;

    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        am_timer_event_ctor(&p->delay, AM_BT_EVT_DELAY, /*domain=*/0);
        am_timer_arm(
            &p->delay,
            me,
            /*ticks=*/am_timer_ticks_to_ms(p->delay_ms),
            /*interval=*/0
        );
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_EXIT: {
        am_timer_disarm(&p->delay);
        return AM_HSM_HANDLED();
    }
    case AM_BT_EVT_DELAY: {
        if (am_hsm_is_in(me, &AM_HSM_STATE(p->substate.fn, p->substate.ifn))) {
            /*
             * the substate is active because someone activated
             * from outside of the behavior tree
             */
            return AM_HSM_HANDLED();
        }
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

enum am_hsm_rc am_bt_fallback(struct am_hsm *me, const struct am_event *event) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_FALLBACK, i);
    struct am_bt_fallback *p = (struct am_bt_fallback *)node;

    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        AM_ASSERT(p->substates);
        AM_ASSERT(p->nsubstates > 0);
        p->isubstate = 0;
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_INIT: {
        p->init_done = 1;
        return AM_HSM_TRAN(p->substates[0].fn, p->substates[0].ifn);
    }
    case AM_HSM_EVT_EXIT: {
        p->init_done = 0;
        return AM_HSM_HANDLED();
    }
    case AM_BT_EVT_FAILURE: {
        if (!p->init_done) {
            /*
             * The substate just failed, but it is not the substate,
             * which fallback node was expected to complete.
             * The substate was likely activated from outside of
             * the behavior tree. Let's sync to the order and
             * continue from the next substate.
             */
            p->init_done = 1;
            for (int i = 0; i < p->nsubstates; ++i) {
                if (am_hsm_is_in(me, &p->substates[i])) {
                    if (p->isubstate != i) {
                        p->isubstate = i;
                    }
                }
            }
        }
        ++p->isubstate;
        if (p->isubstate >= p->nsubstates) {
            break;
        }
        struct am_hsm_state *substate = &p->substates[p->isubstate];
        return AM_HSM_TRAN(substate->fn, substate->ifn);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

enum am_hsm_rc am_bt_sequence(struct am_hsm *me, const struct am_event *event) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_SEQUENCE, i);
    struct am_bt_sequence *p = (struct am_bt_sequence *)node;

    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        AM_ASSERT(p->substates);
        AM_ASSERT(p->nsubstates > 0);
        p->isubstate = 0;
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EVT_INIT: {
        p->init_done = 1;
        return AM_HSM_TRAN(p->substates[0].fn, p->substates[0].ifn);
    }
    case AM_HSM_EVT_EXIT: {
        p->init_done = 0;
        return AM_HSM_HANDLED();
    }
    case AM_BT_EVT_SUCCESS: {
        if (!p->init_done) {
            /*
             * The substate just failed, but it is not the substate,
             * which sequence node was expected to complete.
             * The substate was likely activated from outside of
             * the behavior tree. Let's sync to the order and
             * continue from the next substate.
             */
            p->init_done = 1;
            for (int i = 0; i < p->nsubstates; ++i) {
                if (am_hsm_is_in(me, &p->substates[i])) {
                    if (p->isubstate != i) {
                        p->isubstate = i;
                    }
                }
            }
        }
        ++p->isubstate;
        if (p->isubstate >= p->nsubstates) {
            break;
        }
        struct am_hsm_state *substate = &p->substates[p->isubstate];
        return AM_HSM_TRAN(substate->fn, substate->ifn);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

void am_bt_ctor(void) { am_dlist_init(&m_bt.cfg); }
