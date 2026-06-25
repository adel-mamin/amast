==========================
Finite State Machine (FSM)
==========================

See the rendered version of this document `here <https://amast.readthedocs.io/fsm.html>`_.

Glossary
========

   *event*
       a unique ID plus optionally some data associated with it

   *entry event*
       an event sent to a state when the state is entered.
       The event has ID :c:macro:`AM_EVT_ENTRY`.

   *exit event*
       an event sent to a state when the state is exited.
       The event has ID :c:macro:`AM_EVT_EXIT`.

   *state*
       an event handler. `Idle`, `Processing` and `Completed` are all states
       (see the diagram below)

   *current state*
       the state which currently gets incoming events

   *active state*
       same as current state

   *state transition*
       the process of changing of the current state to another or to itself

   *source state*
       the state that initiates the state transition

   *target state*
       the destination state of a state transition

Introduction
============

This FSM (Finite State Machine) library provides a lightweight API for defining,
creating, and managing state machines in C. The library includes support for
state transitions, entry/exit actions, and event handling.
This library is ideal for applications that require structured and manageable
state control in an embedded system or other C-based environments.

The FSM is a combination of one or more state-handler functions of
type :cpp:type:`am_fsm_state_fn`.

Example FSM
============

Below is an example of a simple FSM with three states
(`Idle`, `Processing`, and `Completed`) and basic transitions:

.. uml::

    @startuml

    left to right direction

    [*] --> Idle

    state Idle #LightBlue
    state Processing #LightBlue
    state Completed #LightBlue

    Idle --> Processing : Start
    Processing --> Completed : Complete
    Completed --> Idle : Reset

    @enduml

This FSM transitions from `Idle` to `Processing` upon receiving a `Start` event,
completes the processing, and then returns to `Idle` with a `Reset` event.

Event Handling
==============

Events in this FSM are defined by the **struct am_event** structure,
with device IDs starting from **AM_EVT_USER**.

Each event is handled in a state handler function that receives the event and
processes it accordingly.

FSM reserved events are defined as follows:

- :c:macro:`AM_EVT_ENTRY`: Indicates the entry into a state. Entry actions are executed here.
- :c:macro:`AM_EVT_EXIT`: Indicates the exit from a state. Exit actions are executed here.

The :cpp:func:`am_fsm_dispatch()` function is used to send events to the FSM.

State Transition
================

The library supports two main types of state transitions:

1. Standard Transition (:cpp:func:`am_fsm_tran()`):
   Moves directly from the current state to the new state.
2. Redispatch Transition (:cpp:func:`am_fsm_tran_redispatch()`):
   Transitions to a new state and redispatches the event for further processing.

Both type of state transitions are used within state handlers to initiate
a transition, updating the FSM's state and returning control to the dispatcher.

If state handler function returns **am_fsm_tran_redispatch(fsm, target_state)**,
then the transition is executed first and then the same event is
dispatched to the new current state. This is a convenience feature,
that allows FSM to handle the event in the state that expects it.

FSM states cannot initiate state transitions when processing entry and exit
events.

Initial State
=============

The initial state of the FSM is provided during the FSM’s initialization
using the :cpp:func:`am_fsm_init()` function.

This state is set to handle any initial setup required by the FSM and
ensures that the FSM begins with a predictable configuration.

The function :cpp:func:`am_fsm_start()` initiates the FSM with an optional initial event.

Example:

.. code:: c

    struct am_fsm my_fsm;
    am_fsm_init(&my_fsm, initial_state);
    am_fsm_start(&my_fsm, NULL); /* initiates with no event */

The initial state must always return **am_fsm_tran(fsm, new_state)** macro
to proceed to the appropriate active state.

FSM Coding Rules
================

1. FSM states must be represented by event handlers of type :cpp:type:`am_fsm_state_fn`.
2. If the :cpp:struct:`am_fsm` instance is embedded in a user structure,
   use :c:macro:`AM_CONTAINER_OF` inside the state handler to access user data.
   For example:

.. code-block:: C

   struct foo {
       struct am_fsm fsm;
       ...
   };

   static enum am_rc foo_state(struct am_fsm *fsm, const struct am_event *event) {
       struct foo *me = AM_CONTAINER_OF(fsm, struct foo, fsm);
       ...
       return am_fsm_handled(fsm);
   }

3. Each user event handler should be implemented as a switch-case of handled
   events. To report that an event is handled, return :cpp:func:`am_fsm_handled()`.
   To initiate a transition, return :cpp:func:`am_fsm_tran()` or
   :cpp:func:`am_fsm_tran_redispatch()`.
4. Avoid placing any code with side effects outside of the switch-case of
   event handlers.
5. Processing of :c:macro:`AM_EVT_ENTRY` and :c:macro:`AM_EVT_EXIT` events should
   not trigger state transitions. It means that user event handlers should
   not return :cpp:func:`am_fsm_tran()` or :cpp:func:`am_fsm_tran_redispatch()` for
   these events.
6. Processing of :c:macro:`AM_EVT_ENTRY` and :c:macro:`AM_EVT_EXIT` events should be
   done at the top of the corresponding event handler for better readability.

FSM Initialization
==================

FSM initialization is divided into the following two steps for increased
flexibility and better control of the initialization timeline:

1. the state machine initialization (:cpp:func:`am_fsm_init()`)
2. the initial transition (:cpp:func:`am_fsm_start()`).

Transition To History
=====================

Transition to history is a useful technique that is convenient to apply in
certain use cases. It does not require to use any dedicated FSM API.

Given the following three states:

.. uml::

    @startuml

    [*] --> A

    state A #LightBlue {
        C --> [H] : E4
    }
    state B #LightBlue {
        C --> [H] : E4
    }
    state C #LightBlue

    A --> C : E1
    A --> B : E2
    B --> C : E3

    @enduml

the transition to history technique can be
demonstrated as follows. Assume that transition to the state *C* may
happen from state *A* or state *B*. As an example, assume the the FSM
is in the state *A*.

The user code stores the current state in a local variable of type
:cpp:type:`am_fsm_state_fn`. This is done with:

.. code-block:: C

   struct foo {
   struct am_fsm fsm;
       ...
       am_fsm_state_fn history;
       ...
   };
   ...
   static enum am_rc A(struct am_fsm *fsm, const struct event *event) {
       struct foo* me = AM_CONTAINER_OF(fsm, struct foo, fsm);
       switch (event->id) {
       case AM_EVT_ENTRY:
           me->history = am_fsm_state(&me->fsm);
           return am_fsm_handled(fsm);
       ...
       }
       return am_fsm_handled(fsm);
   }

Then the transition to the state *C* happens, which is then followed by a request
to transition back to the previous state. Since the previous state is captured
in **me->history** it can be done by doing this:

.. code-block:: C

   static enum am_rc C(struct am_fsm *fsm, const struct event *event) {
       struct foo* me = AM_CONTAINER_OF(fsm, struct foo, fsm);
       switch (event->id) {
       case FSM_EVT_E4:
           return am_fsm_tran(fsm, me->history);
       ...
       }
       return am_fsm_handled(fsm);
   }

As you can see the state *C* does not need to specify the previous
state explicitly - it simply uses whatever state was previously stored in
**me->history** as the target state of the transition.

So, this is essentially all about it.

Another example of the usage of the transition to history technique can be seen
in **tests/history.c** unit test.
