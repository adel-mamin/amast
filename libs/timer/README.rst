=====
Timer
=====

Overview
========

The Timer module is an event-based timer library for handling timeouts.
It uses tick level time resolution and allows user to explicitly manage
the expired timer events.

Key Features
============

1. **Timer Event Management**:

   - Support for one-shot and periodic timer events.
   - Timer events can be of an arbitrary type with the only
     requirement of being inherited from ``struct am_timer_event``.
   - Expired events are iterated explicitly. It is up to users what to
     do with the expired event.

2. **Tick-Based Operation**:

   - Timers operate on a tick-based system, updated with
     :cpp:func:`am_timer_tick_iterator_init` and
     :cpp:func:`am_timer_tick_iterator_next`.
   - Multiple tick rates can be applied to different instances of
     ```struct am_timer``.

3. **Thread Safety**:

   - Critical sections managed using user-defined enter/exit callbacks.
   - Safe timer operations across concurrent tasks and/or ISRs.

4. **Dynamic and Static Timer Allocation**:

   - Timer events can be allocated in any suitable way - the timer library
     is fully agnostic of it as long as the memory content remains
     valid throughout the lifetime of the timer event.

Design Considerations
=====================

1. **Timer Events**:

   Timer events (``am_event_timer``) encapsulate the details of scheduled
   operations. Each timer event includes:

   - Shot time: Number of ticks after which the event is fired.
   - Interval: Period between successive event firings (0 for one-shot events).

3. **Critical Section Management**:

   User-defined ``crit_enter`` and ``crit_exit`` callbacks protect shared resources
   during timer updates and event handling.
