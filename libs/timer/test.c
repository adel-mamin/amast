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
 * Timer unit tests.
 */

#include <stddef.h>
#include <string.h>

#include "common/alignment.h"
#include "common/compiler.h"
#include "common/macros.h"
#include "event/event.h"
#include "timer/timer.h"
#include "onesize/onesize.h"
#include "pal/pal.h"

#define EVT_TEST AM_EVT_USER
#define EVT_TEST2 (AM_EVT_USER + 1)

static struct owner {
    int npost;
} m_owner;

/* cppcheck-suppress-begin constParameterCallback */
static void post_cb(void *owner, const struct am_event *event) {
    (void)event;
    AM_ASSERT(owner == &m_owner);
    AM_ASSERT((EVT_TEST == event->id) || (EVT_TEST2 == event->id));
    ++m_owner.npost;
}
/* cppcheck-suppress-end constParameterCallback */

static void test_arm(void) {
    struct am_event_state_cfg cfg_event = {
        .crit_enter = am_pal_crit_enter, .crit_exit = am_pal_crit_exit
    };
    am_event_state_ctor(&cfg_event);

    {
        static char
            pool[2 * AM_POOL_BLOCK_SIZEOF(struct am_event_timer)] AM_ALIGNED(
                AM_ALIGN_MAX
            );
        am_event_add_pool(
            pool,
            (int)sizeof(pool),
            AM_POOL_BLOCK_SIZEOF(struct am_event_timer),
            AM_POOL_BLOCK_ALIGNMENT(AM_ALIGNOF_EVENT)
        );
    }

    memset(&m_owner, 0, sizeof(m_owner));
    struct am_timer_state_cfg cfg_timer = {
        .post = post_cb,
        .publish = NULL,
        .update = NULL,
        .crit_enter = am_pal_crit_enter,
        .crit_exit = am_pal_crit_exit,
    };
    am_timer_state_ctor(&cfg_timer);

    struct am_event_timer event;
    am_timer_event_ctor(
        &event, /*id=*/EVT_TEST, AM_PAL_TICK_DOMAIN_DEFAULT, &m_owner
    );
    am_timer_arm(&event, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&event));

    struct am_event_timer *event2 = am_timer_event_allocate(
        /*id=*/EVT_TEST2,
        /*size=*/(int)sizeof(*event2),
        AM_PAL_TICK_DOMAIN_DEFAULT,
        &m_owner
    );
    AM_ASSERT(event2);

    am_timer_arm(event2, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(event2));
    am_timer_disarm(event2);
    AM_ASSERT(!am_timer_is_armed(event2));
    AM_ASSERT(!am_timer_domain_is_empty(AM_PAL_TICK_DOMAIN_DEFAULT));

    am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    AM_ASSERT(1 == m_owner.npost);

    AM_ASSERT(am_timer_domain_is_empty(AM_PAL_TICK_DOMAIN_DEFAULT));

    am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    AM_ASSERT(1 == m_owner.npost);

    am_timer_arm(&event, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(&event));

    am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    AM_ASSERT(2 == m_owner.npost);

    am_timer_arm(event2, /*ticks=*/1, /*interval=*/0);
    AM_ASSERT(am_timer_is_armed(event2));

    am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    AM_ASSERT(3 == m_owner.npost);

    AM_ASSERT(am_timer_domain_is_empty(AM_PAL_TICK_DOMAIN_DEFAULT));
}

int main(void) {
    test_arm();
    return 0;
}
