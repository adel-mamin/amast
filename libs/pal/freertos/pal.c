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
 * Platform abstraction layer (PAL) API implementation for FreeRTOS
 */

#include "FreeRTOS.h"

#include "pal/pal.h"

struct am_pal_task {
    /* pthread data */
    static StaticTask_t task;
    /* mutex to protect condition variable */
    pthread_mutex_t mutex;
    /* condition variable for signaling */
    pthread_cond_t cond;
    /* flag to track notification state */
    bool notified;
    /* the task is valid */
    bool valid;
    /* entry function */
    void (*entry)(void *arg);
    /* entry function argument */
    void *arg;
};

static struct am_pal_task task_main_ = {0};
static struct am_pal_task task_arr_[AM_PAL_TASK_NUM_MAX] = {0};

void am_pal_crit_enter(void) {
    if (xPortIsInsideInterrupt()) {
        taskENTER_CRITICAL_FROM_ISR();
    } else {
        taskENTER_CRITICAL()
    }
}

void am_pal_crit_exit(void) {
    if (xPortIsInsideInterrupt()) {
        taskEXIT_CRITICAL_FROM_ISR();
    } else {
        taskEXIT_CRITICAL()
    }
}

int am_pal_task_get_own_id(void) {
    TaskHandle_t h = xTaskGetCurrentTaskHandle();
}

void *am_pal_task_create(
    const char *name,
    int prio,
    void *stack,
    int stack_size,
    void (*entry)(void *arg),
    void *arg
) {
    AM_ASSERT(AM_ALIGNOF_PTR(stack) >= AM_ALIGNOF(StackType_t));
    AM_ASSERT(stack_size > 0);
    AM_ASSERT(entry);
    AM_ASSERT(prio >= 0);

    int index = -1;
    struct am_pal_task *task = NULL;
    for (int i = 0; i < AM_COUNTOF(task_arr_); ++i) {
        if (!task_arr_[i].valid) {
            index = i;
            task = &task_arr_[i];
            task_arr_[i].valid = true;
            break;
        }
    }
    AM_ASSERT(task && (index >= 0));

    TaskHandle_t h = xTaskCreateStatic(
        entry,
        name,
        (uint32_t)((unsigned)stack_size / sizeof(StackType_t)),
        (void *)&tcb[ntcb],
        (unsigned)prio + tskIDLE_PRIORITY,
        (StackType_t *)stack,
        &tcb[ntcb]
    );
    AM_ASSERT(h);
    ++ntcb;

    return h;
}

void am_pal_task_notify(void *task) {
    if (xPortIsInsideInterrupt()) {
        xTaskNotifyGiveFromIsr((TaskHandle_t)task);
    } else {
        xTaskNotifyGive((TaskHandle_t)task);
    }
}

void am_pal_task_wait(void *task) {
    (void)task;
    if (xPortIsInsideInterrupt()) {
        ulTaskNotifyTakeFromIsr(
            /*xClearCountOnExit=*/pdTRUE, /*xTicksToWait=*/portMAX_DELAY
        );
    } else {
        ulTaskNotifyTake(
            /*xClearCountOnExit=*/pdTRUE, /*xTicksToWait=*/portMAX_DELAY
        );
    }
}

uint32_t am_pal_time_get_ms(void) {
    uint32_t ticks = am_pal_time_get_tick();
    return ticks * portTICK_PERIOD_MS;
}

uint32_t am_pal_time_get_tick(int domain) {
    (void)domain;
    if (xPortIsInsideInterrupt()) {
        return (uint32_t)xTaskGetTickCountFromISR();
    }
    return (uint32_t)xTaskGetTickCount();
}

uint32_t am_pal_time_get_tick_from_ms(int domain, uint32_t ms) {
    (void)domain;
    if (0 == ms) {
        return 0;
    }
    return AM_MAX(1, AM_DIV_CEIL(ms, portTICK_PERIOD_MS));
}

void am_pal_sleep_ticks(int domain, int ticks) {
    AM_ASSERT(AM_PAL_TICK_DOMAIN_DEFAULT == domain);
    vTaskDelay(ticks);
}

void am_pal_sleep_ms(int ms) {
    uint32_t ticks =
        am_pal_time_get_tick_from_ms(AM_PAL_TICK_DOMAIN_DEFAULT, ms);
    vTaskDelay(ticks);
}

int am_pal_get_cpu_count(void) {
#ifdef configNUMBER_OF_CORES
    return configNUMBER_OF_CORES;
#else
    return 1;
#endif
}

void am_pal_wait_all(void) {}

void am_pal_lock_all(void) {}

void am_pal_unlock_all(void) {}

void am_pal_run_all(void) { vTaskStartScheduler(); }
