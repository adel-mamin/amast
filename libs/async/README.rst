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

- **AM_ASYNC_EXIT()**

  Marks the end of the async function. This macro resets the async state
  to the initial value, indicating that the function has completed.

- **AM_ASYNC_END()**

  Ends the asynchronous function, ensuring that completed or unexpected
  states are handled correctly. It internally calls `AM_ASYNC_EXIT()`
  to reset the async state.

- **AM_ASYNC_AWAIT(cond)**

  Awaits a specified condition `cond` before proceeding with execution.
  If the condition is not met, the function returns and can be re-entered later.
  This allows the async function to wait for external conditions without blocking.

- **AM_ASYNC_YIELD()**

  Yields control back to the caller without completing the function
  This enables the async function to be resumed later from this point.

Functions
---------

- **void am_async_ctor(struct am_async *me)**

  Initializes an `am_async` structure by setting its `state` field
  to `AM_ASYNC_STATE_INIT`. This prepares the structure to be used in
  an async function.

- **bool am_async_is_busy(const struct am_async *me)**

  Checks if async operation is in progress.

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

        if (some_other_condition) {
            /* Complete the function */
            AM_ASYNC_EXIT();
        }

        /* Await yet another condition */
        AM_ASYNC_AWAIT(yet_another_condition());

        /* Complete the function */
        AM_ASYNC_END();
    }

    int main() {
        struct my_async me;
        am_async_ctor(&me);

        async_function(&me);

        while (am_async_is_busy(&me)) {
            /* Perform other work while async function is busy */
            async_function(&me)
        }

        return 0;
    }

Notes
=====

- Avoid using switch-case constructs within asynchronous function
  using the macros
- Keep the variables that should preserve their values across async
  function calls in a state stored outside of the async function.
- See `test.c` for usage examples
