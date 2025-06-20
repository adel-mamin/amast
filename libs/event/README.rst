=====
Event
=====

Overview
========

The Event module provides a flexible and efficient framework for event
management in embedded systems. It supports event allocation, queuing,
reference counting, and memory pool management to enable reliable event-driven
programming.

Key Features
============

1. **Event Management**:

   - Unique identifiers for events, with ranges reserved for user and internal
     events.
   - Reference counting for event lifecycle management.
   - Static and dynamic allocation of events for predictable and flexible
     memory usage.

2. **Memory Pool Support**:

   - Events are allocated from memory pools initialized at startup.
   - Support for querying memory pool usage statistics.
   - Efficient alignment and size configuration for optimal memory performance.

3. **Event Queues**:

   - FIFO (First-In-First-Out) and LIFO (Last-In-First-Out) queue operations.
   - Support for pushing and popping events in queues.
   - Graceful handling of full queues with optional margin checks.

4. **Concurrency and Thread Safety**:

   - Critical section callbacks to protect shared resources during event
     operations.
   - Safe handling of events in multi-threaded environments.

5. **Debugging and Diagnostics**:

   - Logging utilities to inspect memory pools and event content.
   - Callbacks for detailed inspection of events.

Usage Scenarios
===============

The Event module is well-suited for systems that require:

- **Dynamic Event Allocation**: Support for on-demand event creation.
- **Efficient Queue Management**: Reliable queuing operations for asynchronous
  processing.
- **Multi-Threaded Event Handling**: Thread-safe mechanisms for managing events
  across multiple execution contexts.

Design Considerations
=====================

1. **Event Representation**:

   - Each event (``am_event``) contains metadata such as its ID, reference count,
     tick domain (optional), and memory pool index.
   - Reference counting ensures that events are safely recycled or retained as
     needed.

2. **Memory Pools**:

   - Memory pools are initialized at startup and provide storage for events.
   - Pools are indexed, and their usage statistics can be queried to monitor
     system performance.
   - Events can be dynamically allocated or statically defined, based on the
     application's requirements.

3. **Event Queuing**:

   - Events can be pushed to the front or back of queues, supporting different
     prioritization strategies.
   - Deferred events can be recalled for future processing, ensuring
     flexibility in handling complex workflows.

4. **Concurrency Management**:

   - User-defined critical section callbacks (``crit_enter`` and ``crit_exit``)
     ensure safe access to shared resources during event operations.
   - The module is designed to integrate seamlessly into multi-threaded
     environments.

Module Configuration
====================

The module configuration (``am_event_cfg``) defines:

- **Critical Section Callbacks**: Protect shared resources during event
  operations.
- **Memory Pools**: Must be added during initialization in ascending order of
  block size.

System Integration
==================

The Event module integrates seamlessly with event-driven systems and RTOS. Key
integration points include:

- **Initialization**: Use ``am_event_state_ctor`` to configure the event system
  and ``am_event_add_pool`` to add memory pools.
- **Event Handling**: Allocate events using ``am_event_allocate`` or create
  static events. Push and pop events to/from queues for asynchronous
  processing.
- **Debugging**: Utilize logging utilities to inspect event pools and track
  event usage.

Event Ownership Diagram
=======================

::

  +--------------+  event = am_event_allocate_x() +---------------------+
  |              |  event = am_event_allocate()   |                     |
  |              |  event = am_event_dup()        |                     |
  |              |  event = am_event_dup_x()      |                     |
  |              |------------------------------->|   event owned by    |
  |              |                                |   application       |
  |              |  am_event_free(event)          |   with read and     |
  |              |  am_event_push_...(event)      |   write permissions |
  |              |  am_ao_publish_...(event)      |                     |
  |              |  am_ao_post_...(event)         |                     |
  |              |<-------------------------------|                     |
  |              |                                +---------------------+
  |              |                                           |
  |              |                                           |
  |              |                            am_event_inc_ref_cnt(event) and
  |              |                            am_event_push_...(event) and/or
  |              |                            am_ao_publish_...(event) and/or
  | event owned  |                            am_ao_post_...(event)
  | by event lib |                                           |
  |              |                                           v
  |              |  am_hsm_dispatch(event)        +---------------------+
  |              |  am_fsm_dispatch(event)        |                     |
  |              |  am_event_pop_front()          |                     |
  |              |------------------------------->|                     |
  |              |                                |   event owned by    |
  |              |  return from                   |   application       |
  |              |  am_hsm_dispatch(event) or     |   with read only    |
  |              |  am_fsm_dispatch(event)        |   permission        |
  |              |<-------------------------------|                     |
  |              |                                |                     |
  |              |  am_event_dec_ref_cnt(event)   |                     |
  |              |<-------------------------------|                     |
  |              |                                +---------------------+
  |              |                                    |              ^
  |              |                                    |              |
  |              |                                    +--------------+
  |              |                                 am_event_push_...(event)
  |              |                                 am_ao_publish_...(event)
  +--------------+                                 am_ao_post_...(event)


Please note that the following pseudocode is incorrect:

.. code-block:: C

    struct my_event *event = am_event_allocate(MY_EVENT, sizeof(*event));

    am_ao_post_fifo(ao1, event);
    am_ao_post_fifo(ao2, event);

This is because event could become invalid after posting it to ``ao``.
Consider the case when ``ao1`` preempts the execution task executing the code above
(let's call it ``ao0``) once the event is posted to ``ao1``.
Then ``ao1`` consumes the event, decrements the event's reference counter and frees the event.
After that ``ao0`` resumes the execution and tries to post the already freed event
which leads to undefined behavior.

The proper way of doing it is as follows:

.. code-block:: C

    struct my_event *event = am_event_allocate(MY_EVENT, sizeof(*event));

    am_event_inc_ref_cnt(event);

    am_ao_post_fifo(ao1, event);
    am_ao_post_fifo(ao2, event);

    am_event_dec_ref_cnt(event);

Note how incrementing the event reference counter by calling ``am_event_inc_ref_cnt(event)``
the event is guaranteed to be owned by application (``ao0``) and it becomes safe
to post/publish event multiple times.

Also please note that it is also crucial to call ``am_event_dec_ref_cnt(event)``
at the end to return the ownership of the event to event library and
avoid event memory leak.
