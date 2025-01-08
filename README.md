# Amast

## Introduction
<a name="introduction"></a>

Amast is a minimalist asynchronous toolkit that makes it easier to develop C language based projects. Written in C99.

## What Is Inside

- finite state machine (FSM) ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/fsm/README.rst))
- hierarchical state machine (HSM) with submachines support ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/hsm/README.rst), [examples](https://github.com/adel-mamin/amast/tree/main/apps/examples/hsm))
- async/await ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/async/README.rst))
- active object ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/ao/README.rst), [example](https://github.com/adel-mamin/amast/tree/main/apps/examples/dpp))
- onesize memory allocator ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/onesize/README.rst))
- timers ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/timer/README.rst))
- events ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/event/README.rst))
- ring buffer ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/ringbuf/README.rst), [example](https://github.com/adel-mamin/amast/tree/main/apps/examples/ringbuf))
- doubly and singly linked lists

## How Big Are Compile Sizes

Some x86 size figures to get an idea:

Library name | Code size [kB] | Data size [kB]
-------------|----------------|---------------
slist | 0.74 | 0.01
dlist | 1.37 | 0.00
queue | 1.30 | 0.00
onesize | 1.36 | 0.00
event | 3.30 | 0.24
timer | 1.39 | 0.05
ringbuf | 1.38 | 0.00
fsm | 0.65 | 0.02
hsm | 2.34 | 0.03
ao_cooperative | 3.30 | 0.57
ao_preemptive | 3.29 | 0.56

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

If you want to use Amast features that require porting, then also add one of the following
ports to you project:

- `amast_posix.c`
- `amast_freertos.c`

If you want to run Amast unit tests, then also include `amast_test.h` and `amast_test.c`.

`Makefile` is available for optional use. Run `make test` to run unit tests.

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
