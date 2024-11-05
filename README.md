# Amast

## Introduction
<a name="introduction"></a>

Amast is a minimalist asynchronous toolkit that makes it easier to develop C language based projects. Written in C99.

## What Is Inside

- hierarchical state machine (HSM) with submachines support ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/hsm/README.rst), [examples](https://github.com/adel-mamin/amast/tree/main/apps/examples/hsm))
- async/await ([documentation](https://github.com/adel-mamin/amast/blob/main/libs/async/README.rst))
- doubly and singly linked lists
- onesize memory allocator
- timers
- events

## How To Use
<a name="how-to-use"></a>

Just include `amast.h` and `amast.c` from the latest release to your project.

If you want to run Amast unit tests, then also include `amast_test.h` and `amast_test.c`. Also `Makefile` is available: run `make test` to run unit tests.

## Features, Bugs, etc.

The project uses "Discussions" instead of "Issues".

"Discussions" tab has different discussion groups for "Features" and "Bugs".

For making sure issues are addressed, both me and the community can better evaluate which issues and features are high priority because they can be "upvoted".

## License
<a name="license"></a>

Amast is open-sourced software licensed under the [MIT license](LICENSE.md).
