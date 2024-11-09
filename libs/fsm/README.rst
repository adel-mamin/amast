==========================
Finite State Machine (FSM)
==========================

GLOSSARY
========

   event
       a unique ID plus optionally some data associated with it

   entry event
       an event sent to a state when the state is entered (**AM_FSM_EVT_ENTRY**)

   exit event
       an event sent to a state when the state is exited (**AM_FSM_EVT_EXIT**)

   state
       an event handler. `Idle`, `Processing` and `Completed` are all states
       (see the diagram below)

   current state
       the state which currently gets the incoming events

   active state
       same as current state

   state transition
       the process of changing of the current state to another or to itself

   source state
       the state that initiates the state transition

   target state
       the destination state of a state transition

INTRODUCTION
============

This FSM (Finite State Machine) library provides a lightweight API for defining,
creating, and managing state machines in C. The library includes support for
state transitions, entry/exit actions, and event handling.
It also provides a debugging mechanism through the "spy" callback,
allowing users to observe events as they pass through the state machine.
This library is ideal for applications that require structured and manageable
state control in an embedded system or other C-based environments.

The FSM is a combination of one or more state-handler functions of
type **am_fsm_state**.

EXAMPLE FSM
============

Below is an example of a simple FSM with three states
(`Idle`, `Processing`, and `Completed`) and basic transitions:

::

      +----------+    Start     +------------+
      |   Idle   |------------->| Processing |
      +----------+              +------------+
           ^                         |
           | Reset                   | Complete
           |     +-------------+     |
           +-----|  Completed  |<----+
                 +-------------+

This FSM transitions from `Idle` to `Processing` upon receiving a `Start` event,
completes the processing, and then returns to `Idle` with a `Reset` event.

EVENT HANDLING
==============

Events in this FSM are defined by the **struct am_event** structure,
with device IDs starting from **AM_EVT_USER**.

Each event is handled in a state handler function that receives the event and
processes it accordingly.

FSM reserved events are defined as follows:

- **AM_FSM_EVT_ENTRY**: Indicates the entry into a state. Entry actions are executed here.
- **AM_FSM_EVT_EXIT**: Indicates the exit from a state. Exit actions are executed here.

The **am_fsm_dispatch()** function is used to send events to the FSM.

STATE TRANSITION
================

The library supports two main types of state transitions:

1. Standard Transition (**AM_FSM_TRAN()**):
   Moves directly from the current state to the new state.
2. Redispatch Transition (**AM_FSM_TRAN_REDISPATCH()**):
   Transitions to a new state and redispatches the event for further processing.

Both type of state transitions are used within state handlers to initiate
a transition, updating the FSM's state and returning control to the dispatcher.

If state handler function returns **AM_FSM_TRAN_REDISPATCH(target_state)**,
then the transition is executed first and then the same event is
dispatched to the new current state. This is a convenience feature,
that allows FSM to handle the event in the state that expects it.

FSM states cannot initiate state transitions when processing entry and exit
events.

INITIAL STATE
=============

The initial state of the FSM is provided during the FSM’s construction
using the **am_fsm_ctor()** function.

This state is set to handle any initial setup required by the FSM and
ensures that the FSM begins with a predictable configuration.

The function **am_fsm_init()** initiates the FSM with an optional initial event.

Example:

.. code:: c

    struct am_fsm my_fsm;
    am_fsm_ctor(&my_fsm, initial_state);
    am_fsm_init(&my_fsm, NULL); /* initiates with no event */

The initial state must always return `AM_FSM_TRAN(new_state)` macro
to proceed to the appropriate active state.

FSM INITIALIZATION
==================

FSM initialization is divided into the following two steps for increased
flexibility and better control of the initialization timeline:

1. the state machine constructor (**am_fsm_ctor()**)
2. the initial transition (**am_fsm_init()**).

TRANSITION TO HISTORY
=====================

The library does not natively support history transitions;
however, an FSM can retain its last active state by tracking it in the user code.

To implement a history mechanism:

1. Store the current state before each transition.
2. Use this stored state as a "return-to" state whenever necessary.

Example:

.. code:: c

    static am_fsm_state last_state;

    void some_state(struct am_fsm *fsm, const struct am_event *event) {
        switch (event->id) {
        case HISTORY_EVENT:
            return AM_FSM_TRAN(last_state);
        default:
            last_state = fsm->state;
            break;
        }
        return AM_FSM_HANDLED();
    }

Using this approach, the FSM can revert to the most recent state whenever needed,
simulating a "history" functionality.

