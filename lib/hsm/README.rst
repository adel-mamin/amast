================================
Hierarchical State Machine (HSM)
================================

CREDIT
======

The design and implementation of the HSM library is heavily inspired by
`Practical UML Statecharts in C/C++: Event-Driven Programming for Embedded Systems 2nd Edition <https://www.state-machine.com/psicc2>`_
by Miro Samek. Also the example HSM state diagram for unit testing is borrowed
from the book.

GLOSSARY
========

   event
       a unique ID plus optionally some data associated with it

   entry event
       an event sent to a state when the state is entered (**HSM_EVT_ENTRY**)

   exit event
       an event sent to a state when the state is exited (**HSM_EVT_EXIT**)

   init event
       an event sent to a target state when the state is entered
       (**HSM_EVT_INIT**). It immediately follows the entry event.

   state
       an event handler. A,B,C,D,E,F,Z are all states

   current state
       the state which currently gets the incoming events

   active state
       same as current state

   state transition
       the process of changing of the current state to another or to itself

   initial transition
       the state transition that may optionally happen after entering a state
       if the state is a target state of a state transition.
       The state D has the initial transition, whereas state B does not.

   source state
       the state that initiates the state transition

   target state
       the destination state of a state transition

   superstate
       an HSM state that is a parent (ancestor) of one or more other states
       (children, substates). A,B,D,Z are all superstates.

   top (super)state
       the ultimate root of the state hierarchy (**hsm_top()**)

   substate
       a state that has a superstate as its parent (ancestor).
       A state can be substate and superstate simultaneously.
       A,B,C,D,E,F,Z are all substates.

   child state
       same as substate

   parent state
       same as superstate

   ancestor state
       same as superstate

   ancestor chain
       the parent-child relation chain from a state to the top level superstate.
       B-A-Z is an ancestor chain. Same is F-Z etc.

   nearest common ancestor (NCA)
       the first common ancestor in two ancestor chains.
       For B-A-Z and F-Z the NCA is Z.
       For C-B-A-Z and D-A-Z the NCA is A.
       For C-B-A-Z and B-A-Z the NCA is B.

   topology
       HSM topology is the architecture of HSM - the set of all parent -
       child relations between HSM states

INTRODUCTION
============

HSM differs from a Finite State Machine (FSM) in that a state can have a
parent state that can be used to share behavior via a mechanism similar to
inheritance - behavioral inheritance. The parent-child relationship between
states impacts both event handling and transitions.

The HSM is a combination of one or more state-handler functions of
type **hsm_state_fn**.

EXAMPLE HSM
===========

In order to explore how event handling and transitions work in an HSM,
consider the below state machine:

::

       +----------------------------------------------+
       |                                              |
       |                     Z                        |
       |      (HSM top superstate hsm_top())          |
       |                                              |
       |  +---------------------------------+  +---+  |
       |  |  A                              |  | F |  |
       |  |  +-----------+  +------------+  |  +---+  |
       |  |  |  B        |  |  D    *    |  |         |
       |  |  |           |  |       |    |  |         |
       |  |  |  +-----+  |  |  +----v-+  |  |         |
       |  |  |  |  C  |  |  |  |   E  |  |  |         |
       |  |  |  +-----+  |  |  +------+  |  |         |
       |  |  |           |  |            |  |         |
       |  |  +-----------+  +------------+  |         |
       |  |                                 |         |
       |  +---------------------------------+         |
       |                                              |
       +----------------------------------------------+

STATE RELATIONS
===============

States B and D are children of A. States C and E are children of B and D,
respectively.  State F has no children. Both A and F have the default parent
Z provided by the framework (**hsm_top()**).

EVENT PROPAGATION
=================

Events are always sent first to the active state. The active state can choose
whether to consume the event or to pass it to its parent. If the state
chooses to consume the event then event handling ends with the state. If,
however, the state chooses to pass, then the event will be sent to the state's
parent. At this point the parent must make the same decision. Event handling
ends when the state or one of its ancestors consumes the event or the event
reaches the default superstate **hsm_top()**. The default top level
superstate **hsm_top()** always ignores all events.

Assume that the state C shown above is active and an event is sent to the
state machine. State C will be the first state to receive this event. If it
chooses to pass then, the event will be sent to state B, its direct parent. If
state B also chooses to pass then the event will finally be sent to state
A. If A chooses to pass then event is consumed by **hsm_top()**.

To inform the framework that an event is handled the event handler function
must return **HSM_HANDLED()**.
To inform the framework that an event is passed to a superstate the event
handler function must return **HSM_SUPER(superstate)**.

STATE TRANSITION
================

When transitioning it is important to distinguish the current state and the
source state. They are not necessarily the same state. Consider the case when
the current state is C, an event is received by C and passed to the
superstate A, which decides to make a transition to the state F.  In this
case the current state is C, the source state is A and the target state is F.

When transitioning, exit events are sent up the ancestor chain until reaching
the nearest common ancestor (NCA) of the current and target states. Then,
entry events are sent down the ancestor chain to the target state. Finally
the framework sends init event to the target state. The NCA does not receive
an exit event nor does it receive an entry and init events. There is a
special case when the source and target states match (a self-transition). In
this scenario the source state will be sent an exit and then an entry event
followed by the init event.

For example, if C is the current state and E is the target state, then the
NCA is state A. This means that exit events are sent to C
and B and then entry events are sent to D and E. Then the init event is sent
to E.

If B is the current state and F is the target state, then the NCA
is the default top level state Z, so exit events are sent to B and A
and then an entry event is sent to F. Then the init event is sent to F.

If C is the current state and the target state, this exercises the special
case of a self-transition so C will be sent an exit event then an entry event
followed by the init event.

If C is the current state and the transition is initiated by A with the
target state A, then NCA is A, the exit events are sent to C,B,A and then the
entry event is sent to A followed by the init event.

If C is the current state and the transition is initiated by C with the
target state A, then NCA is A, the exit events are sent to C,B and then the
init event is sent to A. Please note that the state A is not exited in
this case.

To initiate a transition the state handler function must return
**HSM_TRAN(target_state)** or **HSM_TRAN_REDISPATCH(target_state)**.

If state handler function returns **HSM_TRAN_REDISPATCH(target_state)**,
then the transition is executed first and then the same event is
dispatched to the new current state. This is a convenience feature,
that allows HSM to handle the event in the state that expects it.

HSM states cannot initiate state transitions when processing entry and exit
events.

INITIAL STATE TRANSITION
========================

If C is the current state and the transition is initiated by A with the
target state D, then NCA is A, the exit events are sent to C,B and then the
entry event is sent to D followed by the init event. The init event triggers
the initial state transition to E. So, the entry event is sent to E followed
by the init event.

The initial state transition must necessarily target a direct or transitive
substate of a given state. An initial transition cannot target a peer state
or go up in state hierarchy to higher-level states.

For example, the initial transition of state D can only target E and no any
other state.

INITIAL STATE
=============

In addition to regular states every HSM must declare the initial state,
which the HSM framework invokes to execute the topmost initial transition.

HSM INITIALIZATION
==================

HSM initialization is divided into the following two steps for increased
flexibility and better control of the initialization timeline:

1. the state machine constructor (**hsm_ctor()**)
2. the top-most initial transition (**hsm_init()**).

HSM TOPOLOGY
============

HSM framework discovers the HSM topology by sending **HSM_EVT_EMPTY** event
to state event handlers. The state event handlers should explicitly process
the event and always return **HSM_SUPER(superstate)** in response.
