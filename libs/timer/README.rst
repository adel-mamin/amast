=====
Timer
=====

Overview
========

The Timer module provides an event-based timer framework for scheduling and
dispatching events in embedded systems. It enables both one-to-one (post) and
one-to-many (publish) delivery mechanisms for expired timer events, supporting
high-resolution timing in multi-tick-rate-domain environments.

Key Features
============

1. **Timer Event Management**:

   - Support for one-shot and periodic timer events.
   - Timer events are associated with owners or broadcasted using publish
     mechanisms.
   - Configurable callbacks for handling expired timer events.

2. **Tick-Based Operation**:

   - Timers operate on a tick-based system, updated with ``am_timer_tick``.
   - Multiple tick domains are supported for independent timer operation.

3. **Thread Safety**:

   - Critical sections managed using user-defined enter/exit callbacks.
   - Safe timer operations across concurrent threads.

4. **Dynamic and Static Timer Allocation**:

   - Timer events can be allocated dynamically for improved cache locality.
   - Support for static allocation for predictable memory usage.

5. **Customization**:

   - Optional timer event update callbacks for dynamic modifications.
   - Configurable tick domains for flexible operation.

Usage Scenarios
===============

The Timer module is ideal for systems requiring:

- **Event Scheduling**: Scheduling one-shot or periodic events at specific
  intervals.
- **Multi-Domain Timing**: Supporting independent timing domains for various
  subsystems.
- **Concurrent and Real-Time Applications**: Safely handling timer events in
  concurrent environments.

Design Considerations
=====================

1. **Timer Events**:

   Timer events (``am_event_timer``) encapsulate the details of scheduled
   operations. Each timer event includes:
   - Owner: The recipient of the timer event.
   - Shot time: Number of ticks after which the event is fired.
   - Interval: Period between successive event firings (0 for one-shot events).

2. **Callback Mechanisms**:

   - **Post Callback**: Delivers the timer event to a specific owner.
   - **Publish Callback**: Broadcasts the timer event to all subscribers.

3. **Critical Section Management**:

   User-defined ``crit_enter`` and ``crit_exit`` callbacks protect shared resources
   during timer updates and event handling.

4. **Dynamic Timer Event Allocation**:

   Dynamic allocation of timer events improves memory access patterns and
   reduces cache misses. Allocated events are fully constructed and ready for
   use.

Module Configuration
====================

The module configuration (``am_timer_cfg``) defines:

- **Post and Publish Callbacks**: Specify the mechanisms for handling expired
  timer events.
- **Event Update Callback**: Optional callback for dynamically modifying event
  properties.
- **Critical Section Callbacks**: Ensure safe execution in multi-threaded
  environments.

System Integration
==================

The Timer module integrates seamlessly with event-driven systems and RTOS,
providing a tick-based mechanism for precise timing. Key integration points
include:

- **Tick Update**: Regular calls to ``am_timer_tick`` to update timer states and
  dispatch expired events.
- **Event Construction**: Use ``am_timer_event_ctor`` for static events or
  ``am_timer_event_allocate`` for dynamic events.
- **Event Handling**: Utilize post or publish mechanisms for event delivery.
