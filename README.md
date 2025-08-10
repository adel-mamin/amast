[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.13.0-blue)](https://github.com/adel-mamin/amast/releases)
[![Sphynx](https://img.shields.io/badge/Docs-Sphinx-gainsboro)](https://amast.readthedocs.io/)
![Visitors](https://visitor-badge.laobi.icu/badge?page_id=adel-mamin.amast)

# Amast

## Introduction
<a name="introduction"></a>

Amast is a minimalist asynchronous toolkit to help developing projects with asynchronous intractions and state machines.
Written in C99.

## What is it useful for?

Here are several usecases in increasing level of complexity.

### Finite state machine (FSM)

The FSM has two states:

```mermaid
stateDiagram-v2
    [*] --> state_a

    state_a --> state_b : B
    state_b --> state_a : A
```

Here is the full implementation of the FSM:

```C
struct myfsm {
    struct am_fsm fsm;
    /* my data */
} myfsm;

static enum am_rc state_a(struct myfsm *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_FSM_ENTRY: break;
    case AM_EVT_FSM_EXIT: break;
    case AM_EVT_B: return AM_FSM_TRAN(state_b);
    }
    return AM_FSM_HANDLED();
}

static enum am_rc state_b(struct myfsm *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_FSM_ENTRY: break;
    case AM_EVT_FSM_EXIT: break;
    case AM_EVT_A: return AM_FSM_TRAN(state_a);
    }
    return AM_FSM_HANDLED();
}

static enum am_rc init(struct myfsm *me, const struct am_event *event) {
    return AM_FSM_TRAN(state_a);
}

int main(void) {
    am_fsm_ctor(&myfsm, AM_FSM_STATE_CTOR(init));
    am_fsm_init(&myfsm, /*init_event=*/NULL);
    am_fsm_dispatch(&myfsm, &(struct am_event){.id = AM_EVT_B});
    am_fsm_dispatch(&myfsm, &(struct am_event){.id = AM_EVT_A});
    return 0;
}
```

The FSM API can be found [here](https://amast.readthedocs.io/api.html#fsm).
The FSM documenation is [here](https://amast.readthedocs.io/fsm.html).
The library requires less than 1kB of memory.

### Hierarchical state machine (HSM)

The HSM has two sub-states and one superstate:

```mermaid
stateDiagram-v2
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
struct myhsm {
    struct am_hsm hsm;
    /* my data */
} myhsm;

static enum am_rc superstate(struct myhsm *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY: break;
    case AM_EVT_HSM_EXIT: break;
    case AM_EVT_HSM_INIT: return AM_HSM_TRAN(substate_a);
    case AM_EVT_C: return AM_HSM_TRAN(substate_b);
    }
    return AM_HSM_SUPER(am_hsm_top);
}

static enum am_rc substate_a(struct myhsm *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY: break;
    case AM_EVT_HSM_EXIT: break;
    case AM_EVT_B: return AM_HSM_TRAN(substate_b);
    }
    return AM_HSM_SUPER(superstate);
}

static enum am_rc substate_b(struct myhsm *me, const struct am_event *event) {
    switch (event->id) {
    case AM_EVT_HSM_ENTRY: break;
    case AM_EVT_HSM_EXIT: break;
    case AM_EVT_A: return AM_HSM_TRAN(state_a);
    }
    return AM_HSM_SUPER(superstate);
}

static enum am_rc init(struct myhsm *me, const struct am_event *event) {
    return AM_HSM_TRAN(superstate);
}

int main(void) {
    am_hsm_ctor(&myhsm, AM_HSM_STATE_CTOR(init));
    am_hsm_init(&myhsm, /*init_event=*/NULL);
    am_hsm_dispatch(&myhsm, &(struct am_event){.id = AM_EVT_B});
    am_hsm_dispatch(&myhsm, &(struct am_event){.id = AM_EVT_A});
    am_hsm_dispatch(&myhsm, &(struct am_event){.id = AM_EVT_C});
    return 0;
}
```

The HSM API can be found [here](https://amast.readthedocs.io/api.html#hsm).
The HSM documenation is [here](https://amast.readthedocs.io/hsm.html).
The library requires less than 3kB of memory.

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
hsm | hierarchical state machine (HSM) with submachines support ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/hsm/README.rst), [examples](https://github.com/adel-mamin/amast/tree/main/apps/examples/hsm))
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
-->

Library name | Code size [kB] | Data size [kB]
-------------|----------------|---------------
ao_cooperative | 3.93 | 0.57
ao_preemptive | 3.91 | 0.56
dlist | 1.29 | 0.00
event | 3.04 | 0.23
fsm | 0.91 | 0.02
hsm | 2.58 | 0.03
onesize | 1.42 | 0.00
queue | 1.82 | 0.00
ringbuf | 1.39 | 0.00
slist | 1.21 | 0.00
timer | 1.81 | 0.08

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

[![Star History Chart](https://api.star-history.com/svg?repos=adel-mamin/amast&type=Date)](https://star-history.com/#adel-mamin/amast&Date)
