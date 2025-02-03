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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "common/macros.h"
#include "common/types.h"
#include "hsm/hsm.h"
#include "calc.h"

/** calculator example */

/* data submachine indices */
#define DATA_0 0
#define DATA_1 1

#define CALC_DATA_SIZE_MAX 32

struct calc {
    struct am_hsm hsm;
    void (*log)(const char *fmt, ...);
    struct data {
        char data[CALC_DATA_SIZE_MAX];
        int len;
        struct am_hsm_state history;
    } data[2];
    char op;
    double result;
    bool result_valid;
};

static struct calc m_calc;

struct am_hsm *g_calc = &m_calc.hsm;

static enum am_hsm_rc calc_on(struct calc *me, const struct am_event *event);
static enum am_hsm_rc calc_off(struct calc *me, const struct am_event *event);

static enum am_hsm_rc calc_result(
    struct calc *me, const struct am_event *event
);

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
    case AM_EVT_HSM_ENTRY: {
        memset(me->data, 0, sizeof(me->data));
        me->op = '\0';
        me->result = NAN;
        me->result_valid = false;
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
        const struct calc_event *e = (const struct calc_event *)event;
        if ('-' == e->data) {
            me->data[DATA_0].data[0] = '-';
            me->data[DATA_0].len = 1;
            return AM_HSM_TRAN(calc_data_nan, DATA_0);
        }
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_0: {
        me->log("on-0;");
        me->data[DATA_0].data[0] = '0';
        me->data[DATA_0].len = 1;
        return AM_HSM_TRAN(calc_data_num_int_zero, DATA_0);
    }
    case EVT_DIGIT_1_9: {
        me->log("on-1_9;");
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int, DATA_0);
    }
    case EVT_POINT: {
        me->log("on-POINT;");
        me->data[DATA_0].data[0] = '.';
        me->data[DATA_0].len = 1;
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
    case AM_EVT_HSM_ENTRY: {
        double data_0 = strtod(me->data[0].data, /*endptr=*/NULL);
        double data_1 = strtod(me->data[1].data, /*endptr=*/NULL);
        me->result = data_0;
        switch (me->op) {
        case '+':
            me->result += data_1;
            break;
        case '-':
            me->result -= data_1;
            break;
        case '*':
            me->result *= data_1;
            break;
        case '/':
            me->result /= data_1;
            break;
        default:
            AM_ASSERT(0);
            break;
        }
        me->result_valid = true;
        return AM_HSM_HANDLED();
    }
    case EVT_OP:
    case EVT_DIGIT_0:
    case EVT_DIGIT_1_9:
    case EVT_POINT:
    case EVT_DEL:
    case EVT_CANCEL: {
        memset(me->data, 0, sizeof(me->data));
        me->op = '\0';
        me->result = NAN;
        me->result_valid = false;
        break;
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
    int instance = am_hsm_get_instance(&me->hsm);
    switch (event->id) {
    case EVT_OP: {
        me->log("data/%d-OP;", instance);
        if (DATA_1 == instance) {
            return AM_HSM_HANDLED();
        }
        const struct calc_event *e = (const struct calc_event *)event;
        me->op = e->data;
        me->data[DATA_0].history = am_hsm_get_state(&me->hsm);
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
    int instance = am_hsm_get_instance(&me->hsm);
    struct data *data = &me->data[instance];

    switch (event->id) {
    case EVT_DEL: {
        me->log("nan/%d-DEL;", instance);
        data->data[0] = '\0';
        data->len = 0;
        if (DATA_1 == instance) {
            return AM_HSM_TRAN(calc_op_entered);
        }
        return AM_HSM_TRAN(calc_on);
    }
    case EVT_DIGIT_0: {
        me->log("nan/%d-0;", instance);
        data->data[data->len++] = '0';
        return AM_HSM_TRAN(calc_data_num_int_zero, instance);
    }
    case EVT_DIGIT_1_9: {
        me->log("nan/%d-1_9;", instance);
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int, instance);
    }
    case EVT_POINT: {
        me->log("nan/%d-POINT;", instance);
        data->data[data->len++] = '.';
        return AM_HSM_TRAN(calc_data_nan_point, instance);
    }
    case EVT_OP: {
        me->log("nan/%d-OP;", instance);
        if (data->len) {
            return AM_HSM_HANDLED();
        }
        const struct calc_event *e = (const struct calc_event *)event;
        if ('-' == e->data) {
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
    int instance = am_hsm_get_instance(&me->hsm);
    switch (event->id) {
    case EVT_DEL: {
        me->log("nan_point/%d-DEL;", instance);
        struct data *data = &me->data[instance];
        data->data[--data->len] = '\0';
        if (data->len) {
            return AM_HSM_TRAN(calc_data_nan, instance);
        }
        if (DATA_1 == instance) {
            return AM_HSM_TRAN(calc_op_entered);
        }
        return AM_HSM_TRAN(calc_on);
    }
    case EVT_DIGIT_0:
    case EVT_DIGIT_1_9: {
        me->log("nan_point/%d-0_9;", instance);
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_point_frac, instance);
    }
    case EVT_POINT: {
        me->log("nan_point/%d-POINT;", instance);
        return AM_HSM_HANDLED();
    }
    case EVT_OP: {
        me->log("nan_point/%d-OP;", instance);
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
    int instance = am_hsm_get_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("num/%d-0;", instance);
        data->data[data->len++] = '0';
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        me->log("num/%d-1_9;", instance);
        const struct calc_event *e = (const struct calc_event *)event;
        data->data[data->len++] = e->data;
        return AM_HSM_HANDLED();
    }
    case EVT_POINT: {
        AM_ASSERT(0);
        return AM_HSM_HANDLED();
    }
    case EVT_EQUAL: {
        if (DATA_1 == instance) {
            return AM_HSM_TRAN(calc_result);
        }
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data, instance);
}

static enum am_hsm_rc calc_data_num_int(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("int/%d-0;", instance);
        data->data[data->len++] = '0';
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        me->log("int/%d-1_9;", instance);
        const struct calc_event *e = (const struct calc_event *)event;
        data->data[data->len++] = e->data;
        return AM_HSM_HANDLED();
    }
    case EVT_POINT: {
        me->log("int/%d-POINT;", instance);
        data->data[data->len++] = '.';
        return AM_HSM_TRAN(calc_data_num_int_point, instance);
    }
    case EVT_DEL: {
        me->log("int/%d-DEL;", instance);
        data->data[--data->len] = '\0';
        if (0 == data->len) {
            return AM_HSM_TRAN(m_tt[instance].fn);
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
    int instance = am_hsm_get_instance(&me->hsm);
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("int_zero/%d-0;", instance);
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        me->log("int_zero/%d-1_9;", instance);
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
    int instance = am_hsm_get_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0:
    case EVT_DIGIT_1_9: {
        me->log("int_point/%d-0_9;", instance);
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int_point_frac, instance);
    }
    case EVT_DEL: {
        me->log("int_point/%d-DEL;", instance);
        --data->len;
        char c = data->data[data->len];
        data->data[data->len] = '\0';
        if ('.' == c) {
            return AM_HSM_TRAN(calc_data_num_int, instance);
        }
        return AM_HSM_HANDLED();
    }
    case EVT_POINT: {
        me->log("int_point/%d-POINT;", instance);
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data_num_int, instance);
}

static enum am_hsm_rc calc_data_num_int_point_frac(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("int_point_frac/%d-0;", instance);
        data->data[data->len++] = '0';
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        me->log("int_point_frac/%d-1_9;", instance);
        const struct calc_event *e = (const struct calc_event *)event;
        data->data[data->len++] = e->data;
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data_num_int_point, instance);
}

static enum am_hsm_rc calc_data_num_point_frac(
    struct calc *me, const struct am_event *event
) {
    int instance = am_hsm_get_instance(&me->hsm);
    struct data *data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("point_frac/%d-0;", instance);
        data->data[data->len++] = '0';
        return AM_HSM_HANDLED();
    }
    case EVT_DIGIT_1_9: {
        me->log("point_frac/%d-1_9;", instance);
        const struct calc_event *e = (const struct calc_event *)event;
        data->data[data->len++] = e->data;
        return AM_HSM_HANDLED();
    }
    case EVT_DEL: {
        me->log("point_frac/%d-DEL;", instance);
        --data->len;
        data->data[data->len] = '\0';
        AM_ASSERT(data->len);
        char c = data->data[data->len - 1];
        if ('.' == c) {
            return AM_HSM_TRAN(calc_data_nan_point, instance);
        }
        return AM_HSM_HANDLED();
    }
    case EVT_POINT: {
        return AM_HSM_HANDLED();
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_data_num, instance);
}

static enum am_hsm_rc calc_op_entered(
    struct calc *me, const struct am_event *event
) {
    switch (event->id) {
    case EVT_OP: {
        me->log("op-OP;");
        const struct calc_event *e = (const struct calc_event *)event;
        if ('-' == e->data) {
            me->data[DATA_1].data[0] = '-';
            me->data[DATA_1].len = 1;
            return AM_HSM_TRAN(calc_data_nan, DATA_1);
        }
        return AM_HSM_HANDLED();
    }
    case EVT_DEL: {
        me->log("op-DEL;");
        me->op = '\0';
        return AM_HSM_TRAN(me->data[DATA_0].history.fn, DATA_0);
    }
    case EVT_DIGIT_0: {
        me->log("op-0;");
        me->data[DATA_1].data[0] = '0';
        me->data[DATA_1].len = 1;
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int_zero, DATA_1);
    }
    case EVT_DIGIT_1_9: {
        me->log("op-1_9;");
        return AM_HSM_TRAN_REDISPATCH(calc_data_num_int, DATA_1);
    }
    case EVT_POINT: {
        me->log("op-POINT;");
        me->data[DATA_1].data[0] = '.';
        me->data[DATA_1].len = 1;
        return AM_HSM_TRAN(calc_data_nan_point, DATA_1);
    }
    default:
        break;
    }
    return AM_HSM_SUPER(calc_on);
}

static enum am_hsm_rc calc_off(struct calc *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY: {
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
    AM_ASSERT(me);
    return AM_HSM_TRAN(calc_on);
}

void calc_ctor(void (*log)(const char *fmt, ...)) {
    struct calc *me = &m_calc;
    am_hsm_ctor(&me->hsm, AM_HSM_STATE_CTOR(calc_init));
    me->log = log;
}

struct am_blk calc_get_operand(struct am_hsm *me, int index) {
    AM_ASSERT(me);
    AM_ASSERT((0 <= index) && (index <= 1));
    struct am_blk blk = {
        .ptr = ((struct calc *)me)->data[index].data,
        .size = ((struct calc *)me)->data[index].len
    };
    return blk;
}

char calc_get_operator(struct am_hsm *me) {
    AM_ASSERT(me);
    return ((struct calc *)me)->op;
}

bool calc_get_result(struct am_hsm *me, double *res) {
    AM_ASSERT(me);
    AM_ASSERT(res);
    *res = ((struct calc *)me)->result;
    return ((struct calc *)me)->result_valid;
}
