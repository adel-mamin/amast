===========
Async-Await
===========

Provides a lightweight implementation of asynchronous programming using
a custom coroutine mechanism in C. It allows a stateful asynchronous
execution flow be represented in a linear structured, readable manner.

Overview
========

The core of the implementation revolves around managing async state
in a ``struct am_coro`` object. The object combined with a set of
state-tracking macros allows to mimic async-await behavior.

Useful for the cases when the sequence of states (operations) is strictly
pre-defined.

Best effect is achieved when combined with hierarchical state machines -
the more powerful concept, which allows to implement more complex interaction
of states.

Inspired by:

- `Coroutines in C <https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html>`_
- `Async.h - asynchronous, stackless subroutines <https://github.com/naasking/async.h>`_
- `Protothreads <https://dunkels.com/adam/pt/>`_

Coroutine Macros and Functions
==============================

The coroutine macros and functions below help create coroutine functions/blocks,
manage state, and handle control flow.

Macros
------

- **AM_CORO_BEGIN(me)**

  Begins an coroutine function/block and initializes the coroutine state.
  It takes a pointer ``me`` to the ``struct am_coro`` managing the coroutine state.

- **AM_CORO_END()**

  Ends the coroutine function/block.

- **AM_CORO_AWAIT(me, cond)**

  Awaits a specified condition ``cond`` before proceeding with execution.
  If the condition is not met, the function/block returns and can be re-entered later.
  This allows the coroutine function/block to wait for external conditions without blocking.

- **AM_CORO_CALL(me, func)**

  Call an coroutine function and evaluate its return value.
  Returns, if the coroutine function call return value is not ``AM_RC_ASYNC_DONE``,
  in which case the function call is evaluated again on next invocation.

- **AM_CORO_YIELD(me)**

  Yields control back to the caller without completing the function/block
  This enables the coroutine function/block to be resumed later from this point.

Functions
---------

- **void am_coro_ctor(struct am_coro *me)**

  Initializes an ``struct am_coro`` structure by setting its ``state`` field
  to ``AM_CORO_STATE_INIT``. This prepares the structure to be used in
  an coroutine function.

- **bool am_coro_is_busy(const struct am_coro *me)**

  Checks if async operation is in progress.

Usage Example
=============

Check `async unit tests <https://github.com/adel-mamin/amast/blob/main/libs/async/test.c>`_ and
`async example application <https://github.com/adel-mamin/amast/blob/main/apps/examples/async/main.c>`_
for usage examples.

Notes
=====

- Avoid using switch-case constructs within coroutine function
  using the macros
- Keep the variables that should preserve their values across async
  function calls in a state stored outside of the coroutine function.
