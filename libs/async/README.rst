===========
Async-Await
===========

Provides a lightweight implementation of asynchronous programming using
a custom async-await mechanism in C. It allows a stateful asynchronous
execution flow be represented in a linear structured, readable manner.

Overview
========

The core of the implementation revolves around managing async state
in a ``struct am_async`` object. The object combined with a set of
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

Async Macros and Functions
==========================

The async macros and functions below help create asynchronous functions/blocks,
manage state, and handle control flow.

Macros
------

- **AM_ASYNC_BEGIN(me)**

  Begins an asynchronous function/block and initializes the async state.
  It takes a pointer ``me`` to the ``struct am_async`` managing the async state.

- **AM_ASYNC_END()**

  Ends the asynchronous function/block.

- **AM_ASYNC_AWAIT(cond)**

  Awaits a specified condition ``cond`` before proceeding with execution.
  If the condition is not met, the function/block returns and can be re-entered later.
  This allows the async function/block to wait for external conditions without blocking.

- **AM_ASYNC_CHAIN(call)**

  Chain an async function call and evaluate its return value.
  Returns, if the async function call return value is not ``AM_RC_DONE``,
  in which case the function call is evaluated again on next invocation.

- **AM_ASYNC_YIELD()**

  Yields control back to the caller without completing the function/block
  This enables the async function/block to be resumed later from this point.

Functions
---------

- **void am_async_ctor(struct am_async *me)**

  Initializes an ``struct am_async`` structure by setting its ``state`` field
  to ``AM_ASYNC_STATE_INIT``. This prepares the structure to be used in
  an async function.

- **bool am_async_is_busy(const struct am_async *me)**

  Checks if async operation is in progress.

Usage Example
=============

Check `async unit tests <https://github.com/adel-mamin/amast/blob/main/libs/async/test.c>`_ and
`async example application <https://github.com/adel-mamin/amast/blob/main/apps/examples/async/main.c>`_
for usage examples.

Notes
=====

- Avoid using switch-case constructs within asynchronous function
  using the macros
- Keep the variables that should preserve their values across async
  function calls in a state stored outside of the async function.
- See `test.c <https://github.com/adel-mamin/amast/blob/main/libs/async/test.c>`_
  for usage examples.
