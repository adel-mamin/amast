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
   - Support for pushing, popping, and deferring events in queues.
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

