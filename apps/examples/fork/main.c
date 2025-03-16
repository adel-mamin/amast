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
 * Demonstrate integration of HSM with fork().
 *
 * 1. take an arbitrary executable with arguments and run it via fork()
 * 2. wait for completion of the executable
 * 3. run progress indicator while the executable runs.
 *    Update the indicator every PROGRESS_UPDATE_RATE_MS.
 * 4. signal executable completion status via EVT_FORK_FAILURE/SUCCESS events
 * 5. exit once the executable completes.
 *    The return code depends on the outcome of the executable:
 *    0 in case of success and -1 in case of failure.
 *
 * The user executable is started from "job" thread.
 * The time tick is implemented with "ticker" thread.
 * The progress indicator is run by "progress" thread.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "common/compiler.h"
#include "common/alignment.h"
#include "common/macros.h"
#include "event/event.h"
#include "timer/timer.h"
#include "async/async.h"
#include "ao/ao.h"
#include "pal/pal.h"
#include "hsm/hsm.h"

#define PROGRESS_UPDATE_RATE_MS 200

enum fork_evt {
    EVT_FORK_SUCCESS = AM_EVT_USER,
    EVT_FORK_FAILURE,

    EVT_PUB_MAX,

    EVT_PROGRESS_TICK,

    EVT_MAX
};

/*
 * Event size is set to arbitrary value.
 */
static char m_event_pool[EVT_MAX][128] AM_ALIGNED(AM_ALIGN_MAX);
static struct am_ao_subscribe_list m_pubsub_list[EVT_PUB_MAX];

struct progress {
    struct am_ao ao;
    int progress_ticks;
    struct am_async async;
    struct am_timer timer;
};

static enum am_async_rc fork_progress(struct progress *me) {
    AM_ASYNC_BEGIN(&me->async);

    am_pal_printff("\r|");
    AM_ASYNC_YIELD();

    am_pal_printff("\r/");
    AM_ASYNC_YIELD();

    am_pal_printff("\r-");
    AM_ASYNC_YIELD();

    am_pal_printff("\r\\");

    AM_ASYNC_END();

    return AM_ASYNC_RC(&me->async);
}

static enum am_hsm_rc progress_top(
    struct progress *me, const struct am_event *event
) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY:
        am_async_ctor(&me->async);
        am_timer_arm_ticks(&me->timer, me->progress_ticks, me->progress_ticks);
        return AM_HSM_HANDLED();

    case AM_EVT_HSM_EXIT:
        am_timer_disarm(&me->timer);
        return AM_HSM_HANDLED();

    case EVT_FORK_SUCCESS:
        exit(0);

    case EVT_FORK_FAILURE:
        exit(-1);

    case EVT_PROGRESS_TICK:
        (void)fork_progress(me);
        return AM_HSM_HANDLED();

    default:
        break;
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_hsm_rc progress_init(
    struct progress *me, const struct am_event *event
) {
    (void)event;
    am_ao_subscribe(&me->ao, EVT_FORK_SUCCESS);
    am_ao_subscribe(&me->ao, EVT_FORK_FAILURE);
    return AM_HSM_TRAN(progress_top);
}

static void progress_ctor(struct progress *me) {
    am_ao_ctor(&me->ao, AM_HSM_STATE_CTOR(progress_init));

    am_timer_ctor(
        &me->timer,
        EVT_PROGRESS_TICK,
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT,
        &me->ao
    );
    me->progress_ticks = (int)am_pal_time_get_tick_from_ms(
        /*domain=*/AM_PAL_TICK_DOMAIN_DEFAULT, /*ms=*/PROGRESS_UPDATE_RATE_MS
    );
}

AM_NORETURN static void ticker_task(void *param) {
    (void)param;

    am_ao_wait_start_all();

    uint32_t now_ticks = am_pal_time_get_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    for (;;) {
        am_pal_sleep_till_ticks(AM_PAL_TICK_DOMAIN_DEFAULT, now_ticks + 1);
        now_ticks += 1;
        am_timer_tick(AM_PAL_TICK_DOMAIN_DEFAULT);
    }
}

static void job_task(void *param) {
    am_ao_wait_start_all();

    static struct am_event success = {.id = EVT_FORK_SUCCESS};
    static struct am_event failure = {.id = EVT_FORK_FAILURE};

    pid_t pid = fork();
    if (pid < 0) {
        am_ao_publish(&failure);
        return;
    }
    if (pid == 0) { /* Child process */
        char **argv = param;
        /* int execvp(const char *file, char *const argv[]); */
        execvp(argv[1], argv + 1);
        am_ao_publish(&failure);
        return;
    }
    /* Parent process */
    while (1) {
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == 0) {
            /* child process is still running... */
            sleep(1);
            continue;
        }
        if (result != pid) {
            am_ao_publish(&failure);
            return;
        }
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
                am_ao_publish(&success);
            } else {
                am_ao_publish(&failure);
            }
        } else if (WIFSIGNALED(status)) {
            /* child process was terminated by signal WTERMSIG(status) */
            am_ao_publish(&failure);
        }
        break;
    }
    am_ao_publish(&success);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct am_ao_state_cfg cfg = {
        .on_idle = am_pal_on_idle,
        .crit_enter = am_pal_crit_enter,
        .crit_exit = am_pal_crit_exit
    };
    am_ao_state_ctor(&cfg);

    am_ao_init_subscribe_list(m_pubsub_list, AM_COUNTOF(m_pubsub_list));

    am_event_add_pool(
        m_event_pool,
        sizeof(m_event_pool),
        sizeof(m_event_pool[0]),
        AM_ALIGN_MAX
    );

    struct progress m;
    progress_ctor(&m);

    static const struct am_event *m_queue[EVT_MAX];

    am_ao_start(
        &m.ao,
        /*prio=*/AM_AO_PRIO_MIN + 1,
        /*queue=*/m_queue,
        /*nqueue=*/AM_COUNTOF(m_queue),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"progress",
        /*init_event=*/NULL
    );

    am_pal_task_create(
        "ticker",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/ticker_task,
        /*arg=*/NULL
    );

    am_pal_task_create(
        "job",
        AM_AO_PRIO_MAX,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*entry=*/job_task,
        /*arg=*/AM_CAST(void *, argv)
    );

    for (;;) {
        am_ao_run_all();
    }

    return EXIT_SUCCESS;
}
