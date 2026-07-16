[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.17.1-blue)](https://github.com/adel-mamin/amast/releases)
[![Sphynx](https://img.shields.io/badge/Docs-Sphinx-gainsboro)](https://amast.readthedocs.io/)
![Visitors](https://visitor-badge.laobi.icu/badge?page_id=adel-mamin.amast)

# Amast

## Introduction
<a name="introduction"></a>

Amast is a minimalist asynchronous toolkit to help developing projects with asynchronous interactions and state machines.
Written in C99.

## What is it useful for?

Here are several use cases in increasing level of complexity.

### Finite state machine (FSM)

The FSM has two states:

```mermaid
stateDiagram-v2
    direction LR

    [*] --> state_a

    state_a --> state_b : B
    state_b --> state_a : A
```

Here is the full implementation of the FSM:

```C
#include "amast_config.h"
#include "amast.h"

enum { EVT_A = AM_EVT_USER, EVT_B };

struct app {
    struct am_fsm fsm;
    /* app data */
} app;

static enum am_rc state_b(struct am_fsm *me, const struct am_event *event);

static enum am_rc state_a(struct am_fsm *fsm, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("state_a entry\n");
        return am_fsm_handled(fsm);

    case AM_EVT_EXIT:
        am_printf("state_a exit\n");
        return am_fsm_handled(fsm);

    case EVT_B:
        am_printf("state_a got EVT_B\n");
        return am_fsm_tran(fsm, state_b);
    }
    return am_fsm_handled(fsm);
}

static enum am_rc state_b(struct am_fsm *fsm, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("state_b entry\n");
        return am_fsm_handled(fsm);

    case AM_EVT_EXIT:
        am_printf("state_b exit\n");
        return am_fsm_handled(fsm);

    case EVT_A:
        am_printf("state_b got EVT_A\n");
        return am_fsm_tran(fsm, state_a);
    }
    return am_fsm_handled(fsm);
}

static enum am_rc init(struct am_fsm *fsm, const struct am_event *event) {
    return am_fsm_tran(fsm, state_a);
}

int main(void) {
    am_fsm_init(&app.fsm, init);
    am_fsm_start(&app.fsm, /*init_event=*/NULL);

    am_fsm_dispatch(&app.fsm, &(struct am_event){.id = EVT_B});
    am_fsm_dispatch(&app.fsm, &(struct am_event){.id = EVT_A});

    am_fsm_deinit(&app.fsm);

    return 0;
}
```

The console output:

```
state_a entry
state_a got EVT_B
state_a exit
state_b entry
state_b got EVT_A
state_b exit
state_a entry
state_a exit
```

The FSM API can be found [here](https://amast.readthedocs.io/api.html#fsm).
The FSM documenation is [here](https://amast.readthedocs.io/fsm.html).

The compiled binary on x86 is about 2.8kB of memory (gcc, `-Os`, `-flto`).

### Hierarchical state machine (HSM)

The HSM has two sub-states and one superstate.

It demonstrates:

1. creating of hierarchy of states
2. behavioral inheritance by handling the event C in superstate

```mermaid
stateDiagram-v2
    direction LR

    [*] --> superstate : init

    state superstate {
        [*] --> substate_a

        substate_a --> substate_b : B
        substate_b --> substate_a : A
    }

    superstate --> substate_b : C
```

Here is the full implementation of the HSM:

```C
#include <stdio.h>

#include "amast_config.h"
#include "amast.h"

enum { EVT_A = AM_EVT_USER, EVT_B, EVT_C };

struct app {
    struct am_hsm hsm;
    /* app data */
} app;

static enum am_rc substate_a(struct am_hsm* hsm, const struct am_event *event);
static enum am_rc substate_b(struct am_hsm* hsm, const struct am_event *event);

static enum am_rc superstate(struct am_hsm* hsm, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("superstate entry\n");
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        am_printf("superstate exit\n");
        return am_hsm_handled(hsm);

    case AM_EVT_INIT:
        return am_hsm_tran(hsm, substate_a);

    case EVT_C:
        am_printf("superstate got EVT_C\n");
        return am_hsm_tran(hsm, substate_b);
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc substate_a(struct am_hsm* hsm, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("substate_a entry\n");
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        am_printf("substate_a exit\n");
        return am_hsm_handled(hsm);

    case EVT_B:
        am_printf("substate_a got EVT_B\n");
        return am_hsm_tran(hsm, substate_b);
    }
    return am_hsm_super(hsm, superstate);
}

static enum am_rc substate_b(struct am_hsm* hsm, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("substate_b entry\n");
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        am_printf("substate_b exit\n");
        return am_hsm_handled(hsm);

    case EVT_A:
        am_printf("substate_b got EVT_A\n");
        return am_hsm_tran(hsm, substate_a);
    }
    return am_hsm_super(hsm, superstate);
}

static enum am_rc init(struct am_hsm* hsm, const struct am_event *event) {
    return am_hsm_tran(hsm, superstate);
}

int main(void) {
    am_hsm_init(&app.hsm, am_hsm_state_make(init));
    am_hsm_start(&app.hsm, /*init_event=*/NULL);

    am_hsm_dispatch(&app.hsm, &(struct am_event){.id = EVT_B});
    am_hsm_dispatch(&app.hsm, &(struct am_event){.id = EVT_A});
    am_hsm_dispatch(&app.hsm, &(struct am_event){.id = EVT_C});

    am_hsm_deinit(&app.hsm);

    return 0;
}
```

The console output:

```
superstate entry
substate_a entry
substate_a got EVT_B
substate_a exit
substate_b entry
substate_b got EVT_A
substate_b exit
substate_a entry
superstate got EVT_C
substate_a exit
substate_b entry
substate_b exit
superstate exit
```

The HSM API can be found [here](https://amast.readthedocs.io/api.html#hsm).
The HSM documenation is [here](https://amast.readthedocs.io/hsm.html).

The compiled binary on x86 is about 4.8kB of memory (gcc, `-Os`, `-flto`).

### Active Object

Here is a full implementation of one active object with two states.

It demonstrates:

1. creating the active object
2. creating and maintaining a timer
3. event publishing
4. creating regular tasks, for blocking calls like
   sleep and waiting for user input

```C
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "amast_config.h"
#include "amast.h"

enum { EVT_SWITCH_MODE = AM_EVT_USER, EVT_PUB_MAX, EVT_TIMER };

struct app {
    struct am_hsm hsm;
    struct am_ao ao;
    struct am_timer *timer;
    struct am_timer_event_x timeout;
    int ticks;
};

static enum am_rc app_state_a(struct am_hsm* hsm, const struct am_event *event);
static enum am_rc app_state_b(struct am_hsm* hsm, const struct am_event *event);

static enum am_rc app_state_a(struct am_hsm* hsm, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("state A\n");
        return am_hsm_handled(hsm);

    case EVT_SWITCH_MODE:
        return am_hsm_tran(hsm, app_state_b);
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc app_state_b(struct am_hsm* hsm, const struct am_event *event) {
    struct app* me = AM_CONTAINER_OF(hsm, struct app, hsm);
    switch (event->id) {
    case AM_EVT_ENTRY:
        am_printf("state B\n");
        am_timer_arm(me->timer, &me->timeout.event, me->ticks, /*interval=*/0);
        return am_hsm_handled(hsm);

    case AM_EVT_EXIT:
        am_timer_disarm(me->timer, &me->timeout.event);
        return am_hsm_handled(hsm);

    case EVT_SWITCH_MODE:
        return am_hsm_tran(hsm, app_state_a);

    case EVT_TIMER:
        am_printf("timer\n");
        am_timer_arm(me->timer, &me->timeout.event, me->ticks, /*interval=*/0);
        return am_hsm_handled(hsm);
    }
    return am_hsm_super(hsm, am_hsm_top);
}

static enum am_rc app_initial(struct am_hsm* hsm, const struct am_event *event) {
    struct app* me = AM_CONTAINER_OF(hsm, struct app, hsm);
    am_ao_subscribe(&me->ao, EVT_SWITCH_MODE);
    return am_hsm_tran(hsm, app_state_a);
}

static void app_init(struct app *me, struct am_timer *timer) {
    memset(me, 0, sizeof(*me));
    am_hsm_init(&me->hsm, am_hsm_state_make(app_initial));
    am_ao_init(&me->ao, am_hsm_start_cb, am_hsm_dispatch_cb, &me->hsm);
    me->timer = timer;
    me->timeout = am_timer_event_create_x(EVT_TIMER, &me->ao);
    me->ticks = am_time_get_ticks_from_ms(AM_TIMEBASE_DEFAULT, 1000);
}

static void ticker_cb(void* param) {
    struct am_timer* timer = param;

    am_timer_tick_iterator_init(timer);
    struct am_timer_event* fired = NULL;
    while ((fired = am_timer_tick_iterator_next(timer)) != NULL) {
        void* owner = AM_CAST(struct am_timer_event_x*, fired)->ctx;
        if (owner) {
            am_ao_post_fifo(owner, &fired->event);
        } else {
            am_ao_publish(&fired->event);
        }
    }
}

static void input_task(void *param) {
    int ch;
    while ((ch = getc(stdin)) != EOF) {
        if ('\n' == ch) {
            static struct am_event event = {.id = EVT_SWITCH_MODE};
            am_ao_publish(&event);
        }
    }
}

int main(void) {
    am_pal_global_init(/*arg=*/NULL);

    /* event publish/subscribe memory */
    struct am_event_subscribe_list pubsub_list[EVT_PUB_MAX];
    struct am_ao_cfg cfg = {
        .crit_enter = am_crit_enter, .crit_exit = am_crit_exit,
        .alloc = NULL
    };
    am_ao_global_init(&cfg, pubsub_list, AM_COUNTOF(pubsub_list));

    struct am_timer timer;
    am_timer_init(&timer);

    struct app app;
    app_init(&app, &timer);

    /* active object incoming events queue */
    const struct am_event *event_queue[2];

    am_ao_start(
        &app.ao,
        (struct am_ao_prio){.ao = AM_AO_PRIO_MAX, .task = AM_AO_PRIO_MAX},
        /*queue=*/event_queue,
        /*queue_size=*/AM_COUNTOF(event_queue),
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*name=*/"app",
        /*init_event=*/NULL
    );

    /* user input controlling thread */
    am_task_create(
        "input",
        AM_AO_PRIO_MIN,
        /*stack=*/NULL,
        /*stack_size=*/0,
        /*init=*/NULL,
        /*entry=*/input_task,
        /*flags=*/AM_TASK_FLAG_WAIT_INIT,
        /*arg=*/&app
    );

    int ticker = am_ticker_create(&(struct am_ticker_cfg){
        .timebase = AM_TIMEBASE_DEFAULT,
        .ticker_cb = ticker_cb,
        .ctx = &timer,
        .priority_hint = AM_AO_PRIO_MIN,
    });
    am_ticker_start(ticker);

    while (am_ao_get_cnt() > 0) {
        am_ao_run_all();
    }

    am_ticker_stop(ticker);

    am_ao_global_deinit();

    am_pal_global_deinit();

    return 0;
}
```

The AO API can be found [here](https://amast.readthedocs.io/api.html#ao).
The Event API can be found [here](https://amast.readthedocs.io/api.html#event).
The Timer API can be found [here](https://amast.readthedocs.io/api.html#timer).

The compiled binary on x86 is about 13.1kB of code memory (gcc, `-Os`, `-flto`).

## Architecture Diagram

![Architecture Diagram](docs/amast-app-diagram.jpg)

## What Is Inside

Library name | Description
-------------|------------
ao | active object (preemptive and cooperative) ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/ao/README.rst), [example](https://github.com/adel-mamin/amast/tree/main/apps/examples/dpp))
async | async/await ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/async/README.rst), [example](https://github.com/adel-mamin/amast/tree/main/apps/examples/async))
dlist | doubly linked list
event | events ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/event/README.rst))
fsm | finite state machine (FSM) ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/fsm/README.rst))
hsm | hierarchical state machine (HSM) with sub-machines support ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/hsm/README.rst), [examples](https://github.com/adel-mamin/amast/tree/main/apps/examples/hsm))
onesize | onesize memory allocator ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/onesize/README.rst))
ringbuf | ring buffer ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/ringbuf/README.rst), [example](https://github.com/adel-mamin/amast/tree/main/apps/examples/ringbuf))
slist | singly linked list
timer | timers ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/timer/README.rst))

## How Big Are Compile Sizes

Some x86-64 size figures to get an idea:

<!--
Generated by disabling `-ftrav`, address and undefined sanitizers.
Meson build type is set to `minsize`.
LTO is disabled.
Copied from builds/posix/libs/library_sizes.rst
-->

Library name | Code size [kB] | Data size [kB]
-------------|----------------|---------------
ao_cooperative | 2.30 | 0.58
ao_preemptive | 2.26 | 0.57
dlist | 0.99 | 0.01
event | 5.77 | 1.10
fsm | 0.64 | 0.01
hsm | 2.35 | 0.01
onesize | 1.01 | 0.01
ringbuf | 1.06 | 0.01
slist | 0.92 | 0.01
timer | 1.01 | 0.01

## How To Compile For Amast Development
<a name="how-to-compile"></a>

On Linux or WSL:

Install [pixi](https://pixi.sh/latest/#installation).
Run `pixi run all`.

## How To Use The Latest Amast Release
<a name="how-to-use"></a>

Include

- `amast.h`
- `amast_config.h`
- `amast.c`
- `amast_preemptive.c` or `amast_cooperative.c`

from the latest release to your project.

If you want to use Amast features that require porting, then also add the following
port to you project:

- `amast_posix.c`
- `amast_libuv.c`

If you want to run Amast unit tests, then also include `amast_test.h` and `amast_test.c`.

`Makefile` is available for optional use. Run `make test` to run the unit tests.

## Features, Bugs, etc.

The project uses "Discussions" instead of "Issues".

"Discussions" tab has different discussion groups for "Features" and "Bugs".

For making sure issues are addressed, both me and the community can better evaluate which issues and features are high priority because they can be "upvoted".

## How To Contribute

If you find the project useful, then please star it. It helps promoting it.

If you find any bugs, please report them.

## License
<a name="license"></a>

Amast is open-sourced software licensed under the [MIT license](LICENSE.md).

## Star History

<a href="https://www.star-history.com/?repos=adel-mamin%2Famast&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=adel-mamin/amast&type=date&theme=dark&legend=top-left&sealed_token=wHMHbJhDjjBtM3PyFsVMMfJwiR6sndtcXNpB0zVc6X931mUp3dPpwIsELzlpimzY0K8IYW_NMPNZ8xHQdEfOTaVz5A42ixRb5oI6HAw6PLdLhhmoqlOnWCKtnX0VDLbVS_xoyJNDtvefe8hGRae-iacsmge0G7xi6aAGPvhJKuQuaQcTgq6KICrxY2eB" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=adel-mamin/amast&type=date&legend=top-left&sealed_token=wHMHbJhDjjBtM3PyFsVMMfJwiR6sndtcXNpB0zVc6X931mUp3dPpwIsELzlpimzY0K8IYW_NMPNZ8xHQdEfOTaVz5A42ixRb5oI6HAw6PLdLhhmoqlOnWCKtnX0VDLbVS_xoyJNDtvefe8hGRae-iacsmge0G7xi6aAGPvhJKuQuaQcTgq6KICrxY2eB" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=adel-mamin/amast&type=date&legend=top-left&sealed_token=wHMHbJhDjjBtM3PyFsVMMfJwiR6sndtcXNpB0zVc6X931mUp3dPpwIsELzlpimzY0K8IYW_NMPNZ8xHQdEfOTaVz5A42ixRb5oI6HAw6PLdLhhmoqlOnWCKtnX0VDLbVS_xoyJNDtvefe8hGRae-iacsmge0G7xi6aAGPvhJKuQuaQcTgq6KICrxY2eB" />
 </picture>
</a>