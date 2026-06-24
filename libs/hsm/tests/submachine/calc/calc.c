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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

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
    void (*log)(const char* fmt, ...);
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

static enum am_rc calc_on(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc calc_off(struct am_hsm* hsm, const struct am_event* event);

static enum am_rc calc_result(struct am_hsm* hsm, const struct am_event* event);

static enum am_rc calc_data(struct am_hsm* hsm, const struct am_event* event);
static enum am_rc calc_data_nan(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc calc_data_nan_point(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc calc_data_num(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc calc_data_num_int(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc calc_data_num_int_zero(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc calc_data_num_int_point(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc calc_data_num_point_frac(
    struct am_hsm* hsm, const struct am_event* event
);
static enum am_rc calc_data_num_int_point_frac(
    struct am_hsm* hsm, const struct am_event* event
);

static enum am_rc calc_op_entered(
    struct am_hsm* hsm, const struct am_event* event
);

static enum am_rc calc_on(struct am_hsm* hsm, const struct am_event* event) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY: {
        memset(me->data, 0, sizeof(me->data));
        me->op = '\0';
        me->result = NAN;
        me->result_valid = false;
        return am_hsm_handled(hsm);
    }
    case EVT_CANCEL: {
        me->log("on-CANCEL;");
        return am_hsm_tran(hsm, calc_on);
    }
    case EVT_OFF: {
        me->log("on-OFF;");
        return am_hsm_tran(hsm, calc_off);
    }
    case EVT_OP: {
        me->log("on-OP;");
        const struct calc_event* e = (const struct calc_event*)event;
        if ('-' == e->data) {
            me->data[DATA_0].data[0] = '-';
            me->data[DATA_0].len = 1;
            return am_hsm_tran_i(hsm, calc_data_nan, DATA_0);
        }
        return am_hsm_handled(hsm);
    }
    case EVT_DIGIT_0: {
        me->log("on-0;");
        me->data[DATA_0].data[0] = '0';
        me->data[DATA_0].len = 1;
        return am_hsm_tran_i(hsm, calc_data_num_int_zero, DATA_0);
    }
    case EVT_DIGIT_1_9: {
        me->log("on-1_9;");
        return am_hsm_tran_redispatch_i(hsm, calc_data_num_int, DATA_0);
    }
    case EVT_POINT: {
        me->log("on-POINT;");
        me->data[DATA_0].data[0] = '.';
        me->data[DATA_0].len = 1;
        return am_hsm_tran_i(hsm, calc_data_nan_point, DATA_0);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc calc_result(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY: {
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
        return am_hsm_handled(hsm);
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
    return am_hsm_super(hsm, calc_on);
}

static const struct am_hsm_state m_tt[] = {
    [DATA_0] = {.fn = calc_on}, [DATA_1] = {.fn = calc_op_entered}
};

static enum am_rc calc_data(struct am_hsm* hsm, const struct am_event* event) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case EVT_OP: {
        me->log("data/%d-OP;", instance);
        if (DATA_1 == instance) {
            return am_hsm_handled(hsm);
        }
        const struct calc_event* e = (const struct calc_event*)event;
        me->op = e->data;
        me->data[DATA_0].history = am_hsm_get_state(hsm);
        return am_hsm_tran(hsm, calc_op_entered);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, calc_on);
}

static enum am_rc calc_data_nan(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    struct data* data = &me->data[instance];

    switch (event->id) {
    case EVT_DEL: {
        me->log("nan/%d-DEL;", instance);
        data->data[0] = '\0';
        data->len = 0;
        if (DATA_1 == instance) {
            return am_hsm_tran(hsm, calc_op_entered);
        }
        return am_hsm_tran(hsm, calc_on);
    }
    case EVT_DIGIT_0: {
        me->log("nan/%d-0;", instance);
        data->data[data->len++] = '0';
        return am_hsm_tran_i(hsm, calc_data_num_int_zero, instance);
    }
    case EVT_DIGIT_1_9: {
        me->log("nan/%d-1_9;", instance);
        return am_hsm_tran_redispatch_i(hsm, calc_data_num_int, instance);
    }
    case EVT_POINT: {
        me->log("nan/%d-POINT;", instance);
        data->data[data->len++] = '.';
        return am_hsm_tran_i(hsm, calc_data_nan_point, instance);
    }
    case EVT_OP: {
        me->log("nan/%d-OP;", instance);
        if (data->len) {
            return am_hsm_handled(hsm);
        }
        const struct calc_event* e = (const struct calc_event*)event;
        if ('-' == e->data) {
            data->data[0] = '-';
            data->len = 1;
        }
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super_i(hsm, calc_data, instance);
}

static enum am_rc calc_data_nan_point(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case EVT_DEL: {
        me->log("nan_point/%d-DEL;", instance);
        struct data* data = &me->data[instance];
        data->data[--data->len] = '\0';
        if (data->len) {
            return am_hsm_tran_i(hsm, calc_data_nan, instance);
        }
        if (DATA_1 == instance) {
            return am_hsm_tran(hsm, calc_op_entered);
        }
        return am_hsm_tran(hsm, calc_on);
    }
    case EVT_DIGIT_0:
    case EVT_DIGIT_1_9: {
        me->log("nan_point/%d-0_9;", instance);
        return am_hsm_tran_redispatch_i(
            hsm, calc_data_num_point_frac, instance
        );
    }
    case EVT_POINT: {
        me->log("nan_point/%d-POINT;", instance);
        return am_hsm_handled(hsm);
    }
    case EVT_OP: {
        me->log("nan_point/%d-OP;", instance);
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super_i(hsm, calc_data_nan, instance);
}

static enum am_rc calc_data_num(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    struct data* data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("num/%d-0;", instance);
        data->data[data->len++] = '0';
        return am_hsm_handled(hsm);
    }
    case EVT_DIGIT_1_9: {
        me->log("num/%d-1_9;", instance);
        const struct calc_event* e = (const struct calc_event*)event;
        data->data[data->len++] = e->data;
        return am_hsm_handled(hsm);
    }
    case EVT_POINT: {
        AM_ASSERT(0);
        return am_hsm_handled(hsm);
    }
    case EVT_EQUAL: {
        if (DATA_1 == instance) {
            return am_hsm_tran(hsm, calc_result);
        }
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super_i(hsm, calc_data, instance);
}

static enum am_rc calc_data_num_int(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    struct data* data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("int/%d-0;", instance);
        data->data[data->len++] = '0';
        return am_hsm_handled(hsm);
    }
    case EVT_DIGIT_1_9: {
        me->log("int/%d-1_9;", instance);
        const struct calc_event* e = (const struct calc_event*)event;
        data->data[data->len++] = e->data;
        return am_hsm_handled(hsm);
    }
    case EVT_POINT: {
        me->log("int/%d-POINT;", instance);
        data->data[data->len++] = '.';
        return am_hsm_tran_i(hsm, calc_data_num_int_point, instance);
    }
    case EVT_DEL: {
        me->log("int/%d-DEL;", instance);
        data->data[--data->len] = '\0';
        if (0 == data->len) {
            return am_hsm_tran(hsm, m_tt[instance].fn);
        }
        if (1 == data->len) {
            if ('-' == data->data[0]) {
                return am_hsm_tran_i(hsm, calc_data_nan, instance);
            }
            if ('0' == data->data[0]) {
                return am_hsm_tran_i(hsm, calc_data_num_int_zero, instance);
            }
            return am_hsm_handled(hsm);
        }
        if (2 == data->len) {
            if (('-' == data->data[0]) && ('0' == data->data[1])) {
                return am_hsm_tran_i(hsm, calc_data_num_int_zero, instance);
            }
            return am_hsm_handled(hsm);
        }
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super_i(hsm, calc_data_num, instance);
}

static enum am_rc calc_data_num_int_zero(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("int_zero/%d-0;", instance);
        return am_hsm_handled(hsm);
    }
    case EVT_DIGIT_1_9: {
        me->log("int_zero/%d-1_9;", instance);
        return am_hsm_tran_redispatch_i(hsm, calc_data_num_int, instance);
    }
    default:
        break;
    }
    return am_hsm_super_i(hsm, calc_data_num_int, instance);
}

static enum am_rc calc_data_num_int_point(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    struct data* data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0:
    case EVT_DIGIT_1_9: {
        me->log("int_point/%d-0_9;", instance);
        return am_hsm_tran_redispatch_i(
            hsm, calc_data_num_int_point_frac, instance
        );
    }
    case EVT_DEL: {
        me->log("int_point/%d-DEL;", instance);
        --data->len;
        char c = data->data[data->len];
        data->data[data->len] = '\0';
        if ('.' == c) {
            return am_hsm_tran_i(hsm, calc_data_num_int, instance);
        }
        return am_hsm_handled(hsm);
    }
    case EVT_POINT: {
        me->log("int_point/%d-POINT;", instance);
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super_i(hsm, calc_data_num_int, instance);
}

static enum am_rc calc_data_num_int_point_frac(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    struct data* data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("int_point_frac/%d-0;", instance);
        data->data[data->len++] = '0';
        return am_hsm_handled(hsm);
    }
    case EVT_DIGIT_1_9: {
        me->log("int_point_frac/%d-1_9;", instance);
        const struct calc_event* e = (const struct calc_event*)event;
        data->data[data->len++] = e->data;
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super_i(hsm, calc_data_num_int_point, instance);
}

static enum am_rc calc_data_num_point_frac(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    const uint8_t instance = am_hsm_get_instance(hsm);
    struct data* data = &me->data[instance];
    switch (event->id) {
    case EVT_DIGIT_0: {
        me->log("point_frac/%d-0;", instance);
        data->data[data->len++] = '0';
        return am_hsm_handled(hsm);
    }
    case EVT_DIGIT_1_9: {
        me->log("point_frac/%d-1_9;", instance);
        const struct calc_event* e = (const struct calc_event*)event;
        data->data[data->len++] = e->data;
        return am_hsm_handled(hsm);
    }
    case EVT_DEL: {
        me->log("point_frac/%d-DEL;", instance);
        --data->len;
        data->data[data->len] = '\0';
        AM_ASSERT(data->len);
        char c = data->data[data->len - 1];
        if ('.' == c) {
            return am_hsm_tran_i(hsm, calc_data_nan_point, instance);
        }
        return am_hsm_handled(hsm);
    }
    case EVT_POINT: {
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super_i(hsm, calc_data_num, instance);
}

static enum am_rc calc_op_entered(
    struct am_hsm* hsm, const struct am_event* event
) {
    struct calc* me = AM_CONTAINER_OF(hsm, struct calc, hsm);
    switch (event->id) {
    case EVT_OP: {
        me->log("op-OP;");
        const struct calc_event* e = (const struct calc_event*)event;
        if ('-' == e->data) {
            me->data[DATA_1].data[0] = '-';
            me->data[DATA_1].len = 1;
            return am_hsm_tran_i(hsm, calc_data_nan, DATA_1);
        }
        return am_hsm_handled(hsm);
    }
    case EVT_DEL: {
        me->log("op-DEL;");
        me->op = '\0';
        return am_hsm_tran_i(hsm, me->data[DATA_0].history.fn, DATA_0);
    }
    case EVT_DIGIT_0: {
        me->log("op-0;");
        me->data[DATA_1].data[0] = '0';
        me->data[DATA_1].len = 1;
        return am_hsm_tran_redispatch_i(hsm, calc_data_num_int_zero, DATA_1);
    }
    case EVT_DIGIT_1_9: {
        me->log("op-1_9;");
        return am_hsm_tran_redispatch_i(hsm, calc_data_num_int, DATA_1);
    }
    case EVT_POINT: {
        me->log("op-POINT;");
        me->data[DATA_1].data[0] = '.';
        me->data[DATA_1].len = 1;
        return am_hsm_tran_i(hsm, calc_data_nan_point, DATA_1);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, calc_on);
}

static enum am_rc calc_off(struct am_hsm* hsm, const struct am_event* event) {
    switch (event->id) {
    case AM_EVT_ENTRY: {
        exit(0);
        return am_hsm_handled(hsm);
    }
    default:
        break;
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc calc_init(struct am_hsm* hsm, const struct am_event* event) {
    (void)event;
    return am_hsm_tran(hsm, calc_on);
}

void calc_create(void (*log)(const char* fmt, ...)) {
    struct calc* me = &m_calc;
    am_hsm_create(&me->hsm, am_hsm_state(calc_init));
    me->log = log;
}

struct am_blk calc_get_operand(struct am_hsm* me, int index) {
    AM_ASSERT(me);
    AM_ASSERT((0 <= index) && (index <= 1));
    struct am_blk blk = {
        .ptr = ((struct calc*)me)->data[index].data,
        .size = ((struct calc*)me)->data[index].len
    };
    return blk;
}

char calc_get_operator(struct am_hsm* me) {
    AM_ASSERT(me);
    return ((struct calc*)me)->op;
}

bool calc_get_result(struct am_hsm* me, double* res) {
    AM_ASSERT(me);
    AM_ASSERT(res);
    *res = ((struct calc*)me)->result;
    return ((struct calc*)me)->result_valid;
}

struct am_hsm* calc_get_obj(void) { return &m_calc.hsm; }
