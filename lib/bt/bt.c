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
        union {
            struct am_bt_invert *invert;
            struct am_bt_force_success *force_success;
            struct am_bt_force_failure *force_failure;
            struct am_bt_repeat *repeat;
            struct am_bt_retry_until_success *retry_until_success;
            struct am_bt_run_until_failure *run_until_failure;
            struct am_bt_delay *delay;
            struct am_bt_fallback *fallback;
            struct am_bt_sequence *sequence;
            struct am_bt_node *node;
        } nodes;
        int num;
    } types[AM_BT_TYPES_NUM];
};

static struct am_bt m_bt;

const struct am_event am_bt_evt_success = {.id = AM_BT_EVT_SUCCESS};
const struct am_event am_bt_evt_failure = {.id = AM_BT_EVT_FAILURE};

void am_bt_add_cfg(struct am_bt_cfg *cfg) {
    AM_ASSERT(cfg);
    AM_ASSERT(cfg->hsm);
    AM_ASSERT(cfg->post);
    am_dlist_push_front(&m_bt.cfg, &cfg->item);
}

void am_bt_add_invert(struct am_bt_invert *nodes, int num) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_INVERT].nodes.invert = nodes;
    m_bt.types[AM_BT_INVERT].num = num;
}

void am_bt_add_force_success(struct am_bt_force_success *nodes, int num) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_FORCE_SUCCESS].nodes.force_success = nodes;
    m_bt.types[AM_BT_FORCE_SUCCESS].num = num;
}

void am_bt_add_force_failure(struct am_bt_force_failure *nodes, int num) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_FORCE_FAILURE].nodes.force_failure = nodes;
    m_bt.types[AM_BT_FORCE_FAILURE].num = num;
}

void am_bt_add_repeat(struct am_bt_repeat *nodes, int num) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_REPEAT].nodes.repeat = nodes;
    m_bt.types[AM_BT_REPEAT].num = num;
}

void am_bt_add_retry_until_success(
    struct am_bt_retry_until_success *nodes, int num
) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_RETRY_UNTIL_SUCCESS].nodes.retry_until_success = nodes;
    m_bt.types[AM_BT_RETRY_UNTIL_SUCCESS].num = num;
}

void am_bt_add_run_until_failure(
    struct am_bt_run_until_failure *nodes, int num
) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_RUN_UNTIL_FAILURE].nodes.run_until_failure = nodes;
    m_bt.types[AM_BT_RUN_UNTIL_FAILURE].num = num;
}

void am_bt_add_delay(struct am_bt_delay *nodes, int num) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_DELAY].nodes.delay = nodes;
    m_bt.types[AM_BT_DELAY].num = num;
}

void am_bt_add_fallback(struct am_bt_fallback *nodes, int num) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_FALLBACK].nodes.fallback = nodes;
    m_bt.types[AM_BT_FALLBACK].num = num;
}

void am_bt_add_sequence(struct am_bt_sequence *nodes, int num) {
    AM_ASSERT(nodes);
    AM_ASSERT(num > 0);
    m_bt.types[AM_BT_SEQUENCE].nodes.sequence = nodes;
    m_bt.types[AM_BT_SEQUENCE].num = num;
}

static struct am_bt_node *am_bt_get_node(enum am_bt_type type, int instance) {
    AM_ASSERT(type >= AM_BT_TYPES_MIN);
    AM_ASSERT(type < AM_BT_TYPES_NUM);
    AM_ASSERT(m_bt.types[type].nodes.node);
    AM_ASSERT(m_bt.types[type].num > 0);
    AM_ASSERT(instance >= 0);
    AM_ASSERT(instance < m_bt.types[type].num);

    struct am_bt_node *node = NULL;
    switch (type) {
    case AM_BT_INVERT: {
        node = &m_bt.types[type].nodes.invert[instance].node;
        break;
    }
    case AM_BT_FORCE_SUCCESS: {
        node = &m_bt.types[type].nodes.force_success[instance].node;
        break;
    }
    case AM_BT_FORCE_FAILURE: {
        node = &m_bt.types[type].nodes.force_failure[instance].node;
        break;
    }
    case AM_BT_REPEAT: {
        node = &m_bt.types[type].nodes.repeat[instance].node;
        break;
    }
    case AM_BT_RETRY_UNTIL_SUCCESS: {
        node = &m_bt.types[type].nodes.retry_until_success[instance].node;
        break;
    }
    case AM_BT_RUN_UNTIL_FAILURE: {
        node = &m_bt.types[type].nodes.run_until_failure[instance].node;
        break;
    }
    case AM_BT_DELAY: {
        node = &m_bt.types[type].nodes.delay[instance].node;
        break;
    }
    case AM_BT_FALLBACK: {
        node = &m_bt.types[type].nodes.fallback[instance].node;
        break;
    }
    case AM_BT_SEQUENCE: {
        node = &m_bt.types[type].nodes.sequence[instance].node;
        break;
    }
    default:
        AM_ASSERT(0);
        break;
    }
    return node;
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

static enum am_hsm_rc am_bt_invert_done(
    struct am_hsm *me, const struct am_event *event
);

enum am_hsm_rc am_bt_invert(struct am_hsm *me, const struct am_event *event) {
    int instance = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_INVERT, instance);
    switch (event->id) {
    case AM_HSM_EVT_INIT: {
        struct am_bt_invert *p = (struct am_bt_invert *)node;
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
    case AM_BT_EVT_SUCCESS:
    case AM_BT_EVT_FAILURE: {
        if (am_hsm_is_in(me, &AM_HSM_STATE(am_bt_invert_done, instance))) {
            break;
        }
        struct am_bt_cfg *cfg = am_bt_get_cfg(me);
        bool success = AM_BT_EVT_SUCCESS == event->id;
        cfg->post(me, success ? &am_bt_evt_failure : &am_bt_evt_success);
        return AM_HSM_TRAN(am_bt_invert_done, instance);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(node->super.fn, node->super.ifn);
}

static enum am_hsm_rc am_bt_invert_done(
    struct am_hsm *me, const struct am_event *event
) {
    (void)event;
    return AM_HSM_SUPER(am_bt_invert, am_hsm_get_state_instance(me));
}

enum am_hsm_rc am_bt_force_success(
    struct am_hsm *me, const struct am_event *event
) {
    int i = am_hsm_get_state_instance(me);
    struct am_bt_node *node = am_bt_get_node(AM_BT_FORCE_SUCCESS, i);
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
        AM_ASSERT(p->attempts_total != 0); /* no proper initialization? */
        AM_ASSERT(0 == p->attempts_done);  /* no proper initialization? */
        return AM_HSM_TRAN(p->substate.fn, p->substate.ifn);
    }
    case AM_BT_EVT_FAILURE: {
        if (p->attempts_total > 0) {
            ++p->attempts_done;
        }
        if ((p->attempts_done < p->attempts_total) || (p->attempts_total < 0)) {
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
