===========================
HSM Examples And Unit Tests
===========================

.. _hsm-example-defer:

Defer
=====

Test simple HSM with event queue and deferred event queue.

The source code can be found `here <https://github.com/adel-mamin/amast/blob/main/libs/hsm/tests/defer.c>`_.

The HSM topology:

::

    +-------------+
    |             |
    | defer_sinit |
    |             |
    +------+------+
           |
    +------|--------------------------+
    |      |    am_hsm_top            |
    | +----v-----+       +----------+ |
    | |          |   B   |          | |
    | | A/defer  |       |          | |
    | | X:recall |       | A/       | |
    | |          |       |          | |
    | | defer_s1 +-------> defer_s2 | |
    | |          |       |          | |
    | +-+------^-+       +-+------^-+ |
    |   |  A   |           |  A   |   |
    |   +------+           +------+   |
    +---------------------------------+

, where

- A is short of **HSM_EVT_A**
- B is short of **HSM_EVT_B**
- X is short of :cpp:enumerator:`AM_EVT_HSM_EXIT <am_hsm_evt_id::AM_EVT_HSM_EXIT>`

The test steps:

1. Initialize the HSM. The init state transition activates **defer_s1**
2. Send **A** event, which triggers an internal transition in **defer_s1** by deferring the event.
3. Send **B** event, which triggers an external transition to **defer_s2** and
   recalls **A** on exit.
4. Event **A** is handled in **defer_s2**.

All internal and external transitions in HSM are logged and compared against
expected patterns stored in **struct test::out**.
