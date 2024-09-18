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

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "hsm/hsm.h"
#include "event/event.h"
#include "test_event.h"

static const struct am_event *m_events[16] = {0};
static int m_events_num = 0;

void test_event_post(struct am_hsm *hsm, const struct am_event *event) {
    (void)hsm;
    AM_ASSERT((m_events_num + 1) < AM_COUNTOF(m_events));
    m_events[m_events_num++] = event;
}

const struct am_event *test_event_get(void) {
    if (0 == m_events_num) {
        return NULL;
    }
    const struct am_event *event = m_events[0];
    --m_events_num;
    for (int i = 0; i < m_events_num; ++i) {
        m_events[i] = m_events[i + 1];
    }
    return event;
}
