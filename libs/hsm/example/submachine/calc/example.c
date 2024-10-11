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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "common/compiler.h"
#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "common.h"

/** calculator example */

/* data submachine indices */
#define DATA_0 0
#define DATA_1 1

#define EVT_OP (AM_EVT_USER)
#define EVT_DIGIT_0 (AM_EVT_USER + 1)
#define EVT_DIGIT_1_9 (AM_EVT_USER + 2)
#define EVT_POINT (AM_EVT_USER + 3)
#define EVT_CANCEL (AM_EVT_USER + 4)
#define EVT_DEL (AM_EVT_USER + 5) /* delete last character */
#define EVT_OFF (AM_EVT_USER + 6)
#define EVT_EQUAL (AM_EVT_USER + 7)

enum op {
    OP_NONE = 0,
    OP_PLUS,
    OP_MINUS,
    OP_MULT,
    OP_DIV,
};

struct event_op {
    struct am_event event;
    enum op op;
}

struct event_digit {
    struct am_event event;
    char digit;
}
#define CALC_DATA_SIZE_MAX 32

struct calc {
    struct am_hsm hsm;
    void (*log)(char *fmt, ...);
    struct data {
        char data[CALC_DATA_SIZE_MAX];
        int len;
        struct am_hsm_state history;
    } data[2];
    double result;
    enum op op;
};

static struct calc m_calc;

static enum am_hsm_rc calc_on(struct calc *me, const struct am_event *event);
static enum am_hsm_rc calc_off(struct calc *me, const struct am_event *event);

static enum am_hsm_rc calc_result(
    struct calc *me, const struct am_event *event
);
static enum am_hsm_rc calc_error(struct calc *me, const struct am_event *event);

static enum am_hsm_rc calc_data(struct calc *me, const struct am_event *event);
static enum am_hsm_rc calc_data_nan(
    struct calc *me, const struct am_event *event
);
static enum am_hsm_rc calc_data_nan_point(
    struct calc *me, const struct am_event *event
);
static enum am_hsm_rc calc_data_num(
    struct calc *me, const struct am_event *event
);
static enum am_hsm_rc calc_data_num_int(
    struct calc *me, const struct am_event *event
);
static enum am_hsm_rc calc_data_num_int_zero(
    struct calc *me, const struct am_event *event
);
static enum am_hsm_rc calc_data_num_int_point(
    struct calc *me, const struct am_event *event
);
static enum am_hsm_rc calc_data_num_point_frac(
    struct calc *me, const struct am_event *event
);
static enum am_hsm_rc calc_data_num_int_point_frac(
    struct calc *me, const struct am_event *event
);

static enum am_hsm_rc calc_op_entered(
    struct calc *me, const struct am_event *event
);

static enum am_hsm_rc calc_on(struct calc *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        me->log("on-ENTRY;");
        memset(me->data, 0, sizeof(me->data));
        me->op = OP_NONE;
        me->result = NAN;
        return AM_HSM_HANDLED();
    }
    case EVT_CANCEL: {
        me->log("on-CANCEL;");
        return AM_HSM_TRAN(calc_on);
    }
    case EVT_OFF: {
        me->log("on-OFF;");
        return AM_HSM_TRAN(calc_off);
    }
    case EVT_OP: {
        me->log("on-OP;");
        const struct event_op *e = (const struct event_op *)event;
        if (OP_MINUS == e->op) {
            me->data[DATA_0].data[0] = '-';
            me->data[DATA_0].len = 1;
            return AM_HSM_TRAN(calc_data_nan, DATA_0);
        }
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_0: {
        me->log("on-DIGIT_0;");
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int_zero, DATA_0);
    }
    case EVT_DIGIT_1_9: {
        me->log("on-DIGIT_1_9;");
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int, DATA_0);
    }
    case EVT_POINT: {
        me->log("on-POINT;");
        data->data[data->len++] = '.';
        return AM_HSM_TRAN(calc_data_nan_point, DATA_0);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc calc_result(
    struct calc *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        double data_0 = strtod(me->data[0].data, /*endptr=*/NULL);
        double data_1 = strtod(me->data[1].data, /*endptr=*/NULL);
        double result = 0.;
        me->result = data_0;
        switch (me->op) {
        case OP_PLUS:
            me->result += data_1;
            break;
        case OP_MINUS:
            me->result -= data_1;
            break;
        case OP_MULT:
            me->result *= data_1;
            break;
        case OP_DIV:
            me->result /= data_1;
            break;
        default:
            AM_ASSERT(0);
            break;
        }
        return AM_HSM_HANDLED();
    }
    case AM_HSM_EXIT: {
        memset(me->data, 0, sizeof(me->data));
        me->op = OP_NONE;
        me->result = NAN;
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_on);
}

static const struct am_hsm_state m_tt[] = {
    [DATA_0] = {.fn = (am_hsm_state_fn)calc_on},
    [DATA_1] = {.fn = (am_hsm_state_fn)calc_op_entered}
};

static enum am_hsm_rc calc_data(struct calc *me, const struct am_event *event) {
    int instance = am_hsm_get_state_instance(&me->hsm);
    switch (event->id) {
    case EVT_OP: {
        if (DATA_1 == instance) {
            return AM_HSM_HANDLED();
        }
        const struct event_op *e = (const struct event_op *)event;
        me->op = e->op;
        return AM_HSM_TRAN(calc_op_entered);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_on);
}

static enum am_hsm_rc calc_data_nan(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_state_instance(&me->hsm);
    struct data *data = &me->data[instance];

    switch (event->id) {
    case EVT_DEL: {
        data->data[0] = '\0';
        data->len = 0;
        return AM_HSM_TRAN(m_tt[instance]);
    }
    case EVT_DIGIT_0: {
        data->data[data->len++] = '0';
        return AM_HSM_TRAN(calc_data_num_int_zero, instance);
    }
    case EVT_DIGIT_1_9: {
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int, instance);
    }
    case EVT_POINT: {
        data->data[data->len++] = '.';
        return AM_HSM_TRAN(calc_data_nan_point, instance);
    }
    case EVT_OP: {
        if (data->len) {
            return AM_HSM_HANDLED();
        }
        const struct event_op *e = (const struct event_op *)event;
        if (OP_MINUS == e->op) {
            data->data[0] = '-';
            data->len = 1;
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data, instance);
}

static enum am_hsm_rc calc_data_nan_point(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_state_instance(&me->hsm);
    switch (event->id) {
    case EVT_DEL: {
        struct data *data = &me->data[instance];
        data->data[data->--len] = '\0';
        if (data->len) {
            return AM_HSM_TRAN(calc_data_nan, instance);
        }
        return AM_HSM_TRAN(m_tt[instance]);
    }
    case EVT_DIGIT_0:
    case EVT_DIGIT_1_9: {
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_point_frac, instance);
    }
    case EVT_POINT:
    case EVT_OP: {
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data_nan, instance);
}

static enum am_hsm_rc calc_data_num(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_state_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->data[instance].data[len++] = '0';
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        const struct event_digit *e = (const struct event_digit *)event;
        me->data[instance].data[len++] = e->digit;
        return AM_HSM_HANDLED();
    }
    case EVT_POINT: {
        return AM_HSM_TRAN(calc_data_frac, instance);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data, instance);
}

static enum am_hsm_rc calc_data_num_int(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_state_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->data[instance].data[len++] = '0';
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        const struct event_digit *e = (const struct event_digit *)event;
        me->data[instance].data[len++] = e->digit;
        return AM_HSM_HANDLED();
    }
    case EVT_POINT: {
        me->data[instance].data[len++] = '.';
        return AM_HSM_TRAN(calc_data_num_int_point, instance);
    }
    case EVT_DEL: {
        struct data *data = &me->data[instance];
        data->data[data->--len] = '\0';
        if (0 == data->len) {
            return AM_HSM_TRAN(m_tt[instance]);
        }
        if (1 == data->len) {
            if ('-' == data->data[0]) {
                return AM_HSM_TRAN(calc_data_nan, instance);
            }
            if ('0' == data->data[0]) {
                return AM_HSM_TRAN(calc_data_num_int_zero, instance);
            }
            return AM_HSM_HANDLED();
        }
        if (2 == data->len) {
            if (('-' == data->data[0]) && ('0' == data->data[1])) {
                return AM_HSM_TRAN(calc_data_num_int_zero, instance);
            }
            return AM_HSM_HANDLED();
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data_num, instance);
}

static enum am_hsm_rc calc_data_num_int_zero(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_state_instance(&me->hsm);
    switch (event->id) {
    case EVT_DIGIT_0: {
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int, instance);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data_num_int, instance);
}

static enum am_hsm_rc calc_data_num_int_point(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_state_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0:
    case EVT_DIGIT_1_9: {
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int_point_frac, instance);
    }
    case EVT_DEL: {
        --len;
        char c = me->data[instance].data[len];
        me->data[instance].data[len] = '\0';
        if ('.' == c) {
            return AM_HSM_TRAN(calc_data_num_int, instance);
        }
        return AM_HSM_HANDLED();
    }
    case EVT_POINT: {
        me->data[instance].data[len++] = '.';
        return AM_HSM_TRAN(calc_data_num_int_point, instance);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data_num_int, instance);
}

static enum am_hsm_rc calc_data_num_point_frac(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_state_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->data[instance].data[len++] = '0';
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        const struct event_digit *e = (const struct event_digit *)event;
        me->data[instance].data[len++] = e->digit;
        return AM_HSM_HANDLED();
    }
    case EVT_DEL: {
        --len;
        char c = me->data[instance].data[len];
        me->data[instance].data[len] = '\0';
        if ('.' == c) {
            return AM_HSM_TRAN(calc_data_nan_point, instance);
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data_num_point, instance);
}

static enum am_hsm_rc calc_error(
    struct calc *me, const struct am_event *event
) {
    return AM_HSM_SUPER(calc_on);
}

static enum am_hsm_rc calc_op_entered(
    struct calc *me, const struct am_event *event
) {
    switch (event->id) {
    case EVT_OP: {
        const struct event_op *e = (const struct event_op *)event;
        if (OP_MINUS == e->op) {
            me->data[DATA_1].data[0] = '-';
            me->data[DATA_1].len = 1;
            return AM_HSM_TRAN(calc_data_nan, DATA_1);
        }
        return AM_HSM_HANDLED();
    }
    case EVT_DEL: {
        return AM_HSM_TRAN(m_tt[instance]);
    }
    case EVT_DIGIT_0: {
        return AM_HSM_TRAN(calc_data_zero, DATA_1);
    }
    case EVT_DIGIT_1_9: {
        return AM_HSM_TRAN(calc_data_int, DATA_1);
    }
    case EVT_POINT: {
        return AM_HSM_TRAN(calc_data_frac, DATA_1);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_on);
}

static enum am_hsm_rc calc_off(struct calc *me, const struct am_event *event) {
    switch (event->id) {
    case AM_HSM_EVT_ENTRY: {
        me->log("final-ENTRY;");
        exit(0);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc calc_init(struct calc *me, const struct am_event *event) {
    (void)event;
    return AM_HSM_TRAN(calc_on);
}

static void calc_ctor(void (*log)(char *fmt, ...)) {
    struct calc *me = &m_calc;
    am_hsm_ctor(&me->hsm, &AM_HSM_STATE(calc_init));
    me->log = log;
}

static void calc_print(char c) {
    printf(ANSI_COLOR_YELLOW_BOLD);
    printf("%c", c);
    printf(ANSI_COLOR_RESET);
    printf(": %s\n", m_regular_log_buf);
}

int main(void) {
    calc_ctor(test_log);

    printf(ANSI_COLOR_BLUE_BOLD);
    printf("Type [0-9 . % / * + - C D =] (X to terminate)\n");
    printf(ANSI_COLOR_RESET);

    am_hsm_init(g_calc, /*init_event=*/NULL);
    test_print('*');

    static const char *blank = "        ";
    static const int e[] = {
        HSM_EVT_A,
        HSM_EVT_B,
        HSM_EVT_C,
        HSM_EVT_D,
        HSM_EVT_E,
        HSM_EVT_F,
        HSM_EVT_G,
        HSM_EVT_H,
        HSM_EVT_I
    };

    for (;;) {
        char c = getchar();
        /* move the cursor up one line */
        printf("\033[A\r");
        if ('\n' == c) {
            continue;
        }
        printf("\r");
        printf("%s", blank);

        char n = getchar();
        while (n != '\n') {
            printf("%s", blank);
            n = getchar();
        }
        printf("\r");

        c = toupper(c);
        int terminate = 'T' == c;
        int index = c - 'A';
        int valid = (0 <= index) && (index < AM_COUNTOF(e));
        if (!valid && !terminate) {
            continue;
        }

        if (terminate) {
            am_hsm_dispatch(g_regular, &(struct am_event){.id = HSM_EVT_TERM});
            test_print(c);
            break;
        }
        am_hsm_dispatch(g_regular, &(struct am_event){.id = e[index]});
        test_print(c);
    }
    am_hsm_dtor(g_regular);
    test_print('*');

    return 0;
}
