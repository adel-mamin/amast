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

#include <string.h>

#include "event/event_sync.h"

#include "top.h"
#include "events.h"

static bool top_proc(
    void* ctx, const struct am_event* event, void* out, int out_size
) {
    (void)out;
    (void)out_size;

    struct top* top = (struct top*)ctx;

    switch (event->id) {
    case EVT_COMMIT:
        if (top->job_pend || (0 == top->rounds)) {
            break;
        }
        top->job_pend = true;
        am_event_sync_publish(top->hub, &(struct am_event){.id = EVT_JOB_REQ});
        break;

    case EVT_JOB_DONE:
        top->job_pend = false;
        top->rounds--;
        break;
    default:
        break;
    }
    return true;
}

bool top_event_post(struct top* top, const struct am_event* event) {
    return top_proc(top, event, /*out=*/NULL, /*out_size=*/0);
}

void top_init(struct top* top, struct am_event_sync_hub* hub, int rounds) {
    memset(top, 0, sizeof(*top));
    top->rounds = rounds;
    top->hub = hub;
    int handler_id = am_event_sync_register(hub, top_proc, top);
    am_event_sync_subscribe(hub, handler_id, EVT_JOB_DONE);
}

bool top_is_active(const struct top* top) { return top->rounds > 0; }
