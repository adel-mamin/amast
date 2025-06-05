==================
Active Object (AO)
==================

Overview
========

The Active Object (AO) module provides a framework for implementing the Active
Object design pattern in embedded systems and other concurrent environments. It
facilitates the encapsulation of tasks, event-driven state machines, and
message queues to enable asynchronous communication and cooperative
multitasking.

Key Features
============

1. **Active Object Management**:

   - Encapsulation of state machines (``am_hsm``) with thread-like behavior.
   - Priority-based scheduling, supporting up to 64 priority levels
     (configurable).
   - Human-readable naming for better debugging and system diagnostics.

2. **Event Handling**:

   - Publish-subscribe mechanism for event distribution.
   - FIFO (First-In-First-Out) and LIFO (Last-In-First-Out) queuing for event
     posting.
   - Support for gracefully handling queue overflow and event recycling.

3. **Thread Safety and Debugging**:

   - Critical section management with customizable callbacks.
   - Debug hooks for monitoring AO state transitions and events.

4. **Resource Configuration**:

   - Configurable event queue sizes and AO stack sizes to optimize resource
     usage.
   - Maximum limit for the number of active objects ensures predictable memory
     usage.

5. **Subscription Management**:

   - Flexible subscription model allowing objects to subscribe to specific
     events.
   - Efficient bitmask-based subscription list representation.
   - APIs for subscribing, unsubscribing, and bulk subscription operations.

6. **System-Level Management**:

   - Utilities to start, stop, and initialize Active Objects.
   - Support for logging and inspecting event queues and last processed events
     for debugging.

Usage Scenarios
===============

The AO module is particularly suited for systems that require:

- **Asynchronous Processing**: Decoupling event producers and consumers for
  better responsiveness.
- **State-Driven Logic**: Managing complex workflows through hierarchical state
  machines.
- **Scalability**: Supporting multiple concurrent tasks with varying priorities.

Design Considerations
=====================

1. **Priority-Based Scheduling**:

   Active objects operate with a priority model, ranging from ``AM_AO_PRIO_MIN``
   to ``AM_AO_PRIO_MAX``. The module ensures deterministic behavior by processing
   higher-priority tasks first.

2. **Event Queue Management**:

   Each Active Object maintains an event queue. Events can be posted in FIFO or
   LIFO order. Overflow scenarios are handled either by assertion or graceful
   degradation, depending on the API used.

3. **Resource Safety**:

   Critical sections are managed via user-defined callbacks (``crit_enter`` and
   ``crit_exit``), allowing seamless integration with platform-specific
   synchronization primitives.

4. **Debugging and Diagnostics**:

   Debug hooks and logging utilities enable real-time inspection of system
   behavior, facilitating efficient troubleshooting during development and
   deployment.

Module Configuration
====================

The module allows for extensive customization through the following:

- **AO State Configuration**: Define callbacks for debugging and critical
  section management.
- **Subscription List Initialization**: Optional support for initializing
  subscription data structures.
- **Event Queue Size and AO Stack Size**: Adjust to meet application-specific
  requirements.

System Integration
==================

The module can be seamlessly integrated into event-driven systems, real-time
operating systems (RTOS), or standalone applications. Starting Active Objects
in priority order (lowest first) is recommended to optimize event queue
allocation and system startup.

Limitations
===========

- The maximum number of active objects (``AM_AO_NUM_MAX``) is constrained to 64
  by design for simplicity and resource efficiency.
- Pub/sub functionality requires explicit initialization, if used.

