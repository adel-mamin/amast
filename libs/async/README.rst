===========
Async-Await
===========

Provides a lightweight implementation of asynchronous programming using
a custom async-await mechanism in C. It allows a stateful asynchronous
execution flow be represented in a linear structured, readable manner.

Overview
========

The core of the implementation revolves around managing async state
in a `struct am_async` object. The object combined with a set of
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

The async macros and functions below help create asynchronous functions,
manage state, and handle control flow.

Macros
------

- **AM_ASYNC_BEGIN(me)**

  Begins an asynchronous function and initializes the async state.
  It takes a pointer `me` to the `struct am_async` managing the async state.

- **AM_ASYNC_BREAK()**

  Marks the end of the async function. This macro resets the async state
  to the initial value and returns `AM_ASYNC_RC_DONE`, indicating that
  the function has completed.

- **AM_ASYNC_END()**

  Ends the asynchronous function, ensuring that completed or unexpected
  states are handled correctly. It internally calls `AM_ASYNC_BREAK()`
  to reset the async state.

- **AM_ASYNC_LABEL()**

  Sets a "label" in the async function by storing the current line number
  in the `state` field. This allows the async function to resume execution
  from this point when re-entered.

- **AM_ASYNC_AWAIT(cond)**

  Awaits a specified condition `cond` before proceeding with execution.
  If the condition is not met, the function returns `AM_ASYNC_RC_BUSY`
  and can be re-entered later. This allows the async function to wait
  for external conditions without blocking.

- **AM_ASYNC_YIELD()**

  Yields control back to the caller without completing the function,
  returning `AM_ASYNC_RC_BUSY`. This enables the async function to be
  resumed later from this point.

Functions
---------

- **void am_async_init(struct am_async *me)**

  Initializes an `am_async` structure by setting its `state` field
  to `AM_ASYNC_STATE_INIT`. This prepares the structure to be used in
  an async function.

Enumerations
------------

- **enum am_async_rc**

  Defines return codes used in async functions:

  - **AM_ASYNC_RC_DONE**: Indicates that the async function has
    completed successfully.
  - **AM_ASYNC_RC_BUSY**: Indicates that the async function is still
    busy and should be re-entered later.

Usage Example
=============

The following example demonstrates how to use this async implementation in C.

.. code-block:: c

    #include "async.h"

    struct my_async {
        struct am_async async;
        int foo;
    };

    int async_function(struct my_async *me) {
        AM_ASYNC_BEGIN(me);

        /* Await some condition before continuing */
        AM_ASYNC_AWAIT(me->foo);

        /* Yield control back to the caller */
        AM_ASYNC_YIELD();

        if (some_condition) {
            /* Complete the function with AM_ASYNC_RC_DONE */
            AM_ASYNC_BREAK();
        }

        /* Await another condition */
        AM_ASYNC_AWAIT(another_condition());

        /* Complete the function with AM_ASYNC_RC_DONE */
        AM_ASYNC_END();
    }

    int main() {
        struct my_async me;
        am_async_init(&me);

        while (async_function(&me) == AM_ASYNC_RC_BUSY) {
            /* Perform other work while async function is busy */
        }

        return 0;
    }

Notes
=====

- Avoid using switch-case constructs withing asynchronous function
  using the macros
- Keep the variables that should preserve their values across async
  function calls in a state stored outside of the async function.
- See `test.c` for usage examples
