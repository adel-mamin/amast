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

/*
 * This simple example checks keyboard input against the two allowed strings
 * "ha\n" and "hi\n". If an unrecognised character is read, a top state will
 * handle this by printing a message and returning to the idle state. If the
 * character '!' is encountered, a "reset" message is printed, and the top
 * state's entry state will be entered (the idle state).
 *
 *                   print 'reset'
 *       o      +---------------------+
 *       |      |                     | '!'
 *       |      v     top state       |
 * +-----v----------------------------------------+
 * |  +------+  'h'  +---+  'a'  +---+  '\n'      |
 * +->| idle | ----> | h | ----> | a | ---------+ |
 * |  +------+       +---+\      +---+          | |
 * |   ^ ^ ^               \'i'  +---+  '\n'    | |
 * |   | | |                \--> | i | ------+  | |
 * |   | | |                     +---+       |  | |
 * +---|-|-|----------------+----------------|--|-+
 *     | | |                |                |  |
 *     | | |                | '[^hai!\n]'    |  |
 *     | | | print 'unknown'|                |  |
 *     | | +----------------+   print 'hi'   |  |
 *     | +-----------------------------------+  |
 *     |               print 'ha'               |
 *     +----------------------------------------+
 *
 * The example description was taken from:
 * https://github.com/misje/stateMachine/blob/master/examples/stateMachineExample.c
 */

#include <stdio.h>

#include "common/types.h"
#include "event/event.h"
#include "hsm/hsm.h"

#define HAHI_EVT_USER_INPUT AM_EVT_USER

struct hahi {
    struct am_hsm hsm;
};

struct hahi_event {
    struct am_event super;
    int ch;
};

static enum am_rc hahi_top(struct hahi *me, const struct am_event *event);
static enum am_rc hahi_h(struct hahi *me, const struct am_event *event);
static enum am_rc hahi_a(struct hahi *me, const struct am_event *event);
static enum am_rc hahi_i(struct hahi *me, const struct am_event *event);
static enum am_rc hahi_idle(struct hahi *me, const struct am_event *event);

static enum am_rc hahi_top(struct hahi *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_INIT:
        return AM_HSM_TRAN(hahi_idle);

    case HAHI_EVT_USER_INPUT: {
        const struct hahi_event *evt = (const struct hahi_event *)event;
        if ('!' == evt->ch) {
            printf("'reset'\n");
            return AM_HSM_TRAN(hahi_top);
        }
        printf("'unknown'\n");
        return AM_HSM_TRAN(hahi_idle);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc hahi_idle(struct hahi *me, const struct am_event *event) {
    switch (event->id) {
    case HAHI_EVT_USER_INPUT: {
        const struct hahi_event *evt = (const struct hahi_event *)event;
        if ('h' == evt->ch) {
            return AM_HSM_TRAN(hahi_h);
        }
        break;
    }
    default:
        break;
    }
    return AM_HSM_SUPER(hahi_top);
}

static enum am_rc hahi_h(struct hahi *me, const struct am_event *event) {
    switch (event->id) {
    case HAHI_EVT_USER_INPUT: {
        const struct hahi_event *evt = (const struct hahi_event *)event;
        if ('a' == evt->ch) {
            return AM_HSM_TRAN(hahi_a);
        }
        if ('i' == evt->ch) {
            return AM_HSM_TRAN(hahi_i);
        }
        break;
    }
    default:
        break;
    }
    return AM_HSM_SUPER(hahi_top);
}

static enum am_rc hahi_a(struct hahi *me, const struct am_event *event) {
    switch (event->id) {
    case HAHI_EVT_USER_INPUT: {
        const struct hahi_event *evt = (const struct hahi_event *)event;
        if ('\n' == evt->ch) {
            printf("'ha'\n");
            return AM_HSM_TRAN(hahi_idle);
        }
        break;
    }
    default:
        break;
    }
    return AM_HSM_SUPER(hahi_top);
}

static enum am_rc hahi_i(struct hahi *me, const struct am_event *event) {
    switch (event->id) {
    case HAHI_EVT_USER_INPUT: {
        const struct hahi_event *evt = (const struct hahi_event *)event;
        if ('\n' == evt->ch) {
            printf("'hi'\n");
            return AM_HSM_TRAN(hahi_idle);
        }
        break;
    }
    default:
        break;
    }
    return AM_HSM_SUPER(hahi_top);
}

static enum am_rc hahi_init(struct hahi *me, const struct am_event *event) {
    (void)event;
    return AM_HSM_TRAN(hahi_idle);
}

int main(void) {
    struct hahi m;
    am_hsm_ctor(&m.hsm, AM_HSM_STATE_CTOR(hahi_init));
    am_hsm_init(&m.hsm, /*init_event=*/NULL);

    int ch;
    while ((ch = getc(stdin)) != EOF) {
        struct hahi_event event = {{.id = HAHI_EVT_USER_INPUT}, .ch = ch};
        am_hsm_dispatch(&m.hsm, &event.super);
    }

    return 0;
}
