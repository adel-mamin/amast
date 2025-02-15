================================
Hierarchical State Machine (HSM)
================================

Credit
======

The design and implementation of the HSM library is heavily inspired by
`Practical UML Statecharts in C/C++: Event-Driven Programming for Embedded Systems 2nd Edition <https://www.state-machine.com/psicc2>`_
by Miro Samek. Also the example HSM state diagram in **hsm.png** is borrowed
from the book.

Glossary
========

   event
       a unique ID plus optionally some data associated with it

   entry event
       an event sent to a state when the state is entered.
       The event has ID **AM_EVT_HSM_ENTRY**.

   exit event
       an event sent to a state when the state is exited.
       The event has ID **AM_EVT_HSM_EXIT**.

   init event
       an event sent to a target state, when the state is entered
       The event has ID **AM_EVT_HSM_INIT**. It immediately follows
       the entry event.

   state
       an event handler. *A*,*B*,*C*,*D*,*E*,*F*,*am_hsm_top* are all states
       (see state diagram in :ref:`example-hsm` section below)

   current state
       the state which currently gets the incoming events

   active state
       same as current state

   state transition
       the process of changing of the current state to another or to itself

   source state
       the state that initiates a state transition

   target state
       the destination state of a state transition

   initial transition
       the state transition that may optionally happen after entering a state,
       if the state is a target state of a state transition.
       In the state diagram :ref:`example-hsm` section below
       the state *D* has the initial transition,
       whereas state *B* does not. The initial transition in the state *D*
       is only activated, if the state *D* is a target state of a state transition.
       For example, if *B* triggers the transition to *D*, then the initial
       transition is activated. However if *B* triggers the transition to *A*, then
       the initial transition in *D* is not activated.

   superstate
       an HSM state that is a parent (ancestor) of one or more other states
       (children, substates). *A*,*B*,*D*,*am_hsm_top* are all superstates.

   top (super)state
       the ultimate root of the state hierarchy.
       It is defined by **am_hsm_top()** state.

   substate
       a state that has a superstate as its parent (ancestor).
       A state can be substate and superstate simultaneously.
       *A*,*B*,*C*,*D*,*E*,*F* are all substates (see state diagram in
       :ref:`example-hsm` section below).

   child state
       same as substate

   parent state
       same as superstate

   ancestor state
       same as superstate

   ancestor chain
       the parent-child relation chain from a state to the top level superstate.
       In the state diagram in :ref:`example-hsm` section below
       *B*-*A*-*am_hsm_top* is an ancestor chain. Same is *F*-*am_hsm_top etc*.

   nearest common ancestor (NCA)
       the first common ancestor in two ancestor chains constructed from
       source and target states.
       For example, given the state diagram in :ref:`example-hsm` section below:

       1. for *B*-*A*-*am_hsm_top* and *F*-*am_hsm_top* the NCA is *am_hsm_top*
       2. for *C*-*B*-*A*-*am_hsm_top* and *D*-*A*-*am_hsm_top* the NCA is *A*
       3. for *C*-*B*-*A*-*am_hsm_top* and *B*-*A*-*am_hsm_top* the NCA is *B*

   topology
       HSM topology is the architecture of HSM - the set of all parent -
       child relations between HSM states

Introduction
============

HSM differs from a Finite State Machine (FSM) in that a state can have a
parent state that can be used to share behavior via a mechanism similar to
inheritance - behavioral inheritance. The parent-child relationship between
states impacts both event handling and transitions.

The HSM is a combination of one or more state-handler functions of
type **am_hsm_state_fn**.

.. _example-hsm:

Example HSM
===========

In order to explore how event handling and transitions work in an HSM,
consider the below state machine:

::

       +----------------------------------------------+
       |                                              |
       |                am_hsm_top                    |
       |      (HSM top superstate am_hsm_top())       |
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

State Relations
===============

States *B* and *D* are children of *A*. States *C* and *E* are children
of *B* and *D*, respectively.  State *F* has no children.
Both *A* and *F* have the default parent *am_hsm_top* provided by
the library (**am_hsm_top()**).

Event Propagation
=================

Events are always sent first to the active state. The active state can choose
whether to consume the event or to pass it to its parent. If the state
chooses to consume the event then event handling ends with the state. If,
however, the state chooses to pass, then the event will be sent to the state's
parent. At this point the parent must make the same decision. Event handling
ends when the state or one of its ancestors consumes the event or the event
reaches the default superstate **am_hsm_top()**. The default top level
superstate **am_hsm_top()** always returns **AM_HSM_RC_HANDLED** for all events.

Assume that the state *C* shown in the state diagram in :ref:`example-hsm` above
is active and an event is sent to the state machine. State *C* will be the first
state to receive this event. If it chooses to pass then, the event will be sent
to state *B*, its direct parent. If state *B* also chooses to pass, then
the event will finally be sent to state *A*. If *A* chooses to pass then event
is consumed by **am_hsm_top()**.

**am_hsm_top()** does nothing with events and serves as the ultimate event
propagation termination point.

To inform the library that an event is handled the event handler function
must return **AM_HSM_HANDLED()**.

To inform the library that an event is passed to superstate the event
handler function must return **AM_HSM_SUPER(superstate)**.

State Transition
================

When transitioning it is important to distinguish the current state and the
source state. They are not necessarily the same state.

In the state diagram in :ref:`example-hsm` above consider the case when
the current state is *C*, an event is received by *C* and passed first to the
superstate *B* and then to the superstate *A*, which decides to make
a transition to the state *F*.  In this case the current state is *C*,
the source state is *A* and the target state is *F*.

When transitioning, exit events (**AM_EVT_HSM_EXIT**) are sent up the ancestor
chain until reaching the nearest common ancestor (NCA) of the source and
target states. Then, entry events (**AM_EVT_HSM_ENTRY**) are sent down
the ancestor chain to the target state. Finally the library sends init event
(**AM_EVT_HSM_INIT**) to the target state. The NCA does not receive
the exit event nor does it receive the entry and init events.

There is a special case when the source and target states match
(a self-transition). In this scenario the source state will be sent
the exit and then the entry event followed by the init event.

For example, if *C* is the source state and *E* is the target state, then the
NCA is state *A*. This means that the exit events are sent to *C*
and *B* and then the entry events are sent to *D* and *E*. Then the init event
is sent to *E*.

If *B* is the source state and *F* is the target state, then the NCA
is the default top level state *am_hsm_top*, so exit events are sent
to *B* and *A* and then an entry event is sent to *F*.
Then the init event is sent to *F*.

If *C* is the source state and the target state, this exercises the special
case of the self-transition. So *C* will be sent the exit event then
the entry event followed by the init event.

If *C* is the current state and the transition is initiated by *A* with the
target state *A*, then NCA is *A*, the exit events are sent to *C*,*B*,*A* and
then the entry event is sent to *A* followed by the init event.

If *C* is the current state and the transition is initiated by *C* with the
target state *A*, then NCA is *A*, the exit events are sent to *C*,*B* and then
the init event is sent to *A*. Please note that the state *A* is not exited in
this case.

To initiate a transition the state handler function must return
**AM_HSM_TRAN(target_state)** or **AM_HSM_TRAN_REDISPATCH(target_state)**.

If state handler function returns **AM_HSM_TRAN_REDISPATCH(target_state)**,
then the transition is executed first and then the same event is
dispatched to the new current state. This is a convenience feature,
that allows HSM to handle the event in the state that expects it.

HSM states cannot initiate state transitions when processing entry and exit
events.

Initial State Transition
========================

If *C* is the current state and the transition is initiated by *A* with the
target state *D*, then NCA is *A*, the exit events are sent to *C*,*B* and
then the entry event is sent to *D* followed by the init event. The init event
triggers the initial state transition to *E*. So, the entry event is sent to *E*
followed by the init event.

If *E* had an initial transition, then that transition would be executed too
in a similar manner all the way down the hierarchy chain until target state
does not do initial transition anymore.

The initial state transition must necessarily target a direct or transitive
substate of a given state. An initial transition cannot target a peer state
or go up in state hierarchy to higher-level states.

For example, the initial transition of state *D* can only target *E* and no any
other state.

Initial State
=============

In addition to regular states every HSM must declare the initial state,
which the HSM library invokes to execute the topmost initial transition.

HSM Initialization
==================

HSM initialization is divided into the following two steps for increased
flexibility and better control of the initialization timeline:

1. the state machine constructor (**am_hsm_ctor()**)
2. the top-most initial transition (**am_hsm_init()**).

HSM Topology
============

HSM library discovers the user HSM topology by sending **AM_EVT_HSM_EMPTY** event
to state event handlers. The state event handlers should explicitly process
the event and always return **AM_HSM_SUPER(superstate)** in response.

HSM Coding Rules
================

1. HSM states must be represented by event handlers of type **am_hsm_state_fn**.
2. The name of the first argument of all user event handler functions
   must be **me**.
3. For convenience instead of using **struct am_hsm *me** the first argument
   can point to a user structure. In this case the user structure
   must have **struct am_hsm** instance as its first field.

   For example, the first argument can be **struct foo *me**, where
   **struct foo** is defined like this:

.. code-block:: C

   struct foo {
       struct am_hsm hsm;
       ...
   };

4. Each user event handler should be implemented as a switch-case of handled
   events.
5. Avoid placing any code with side effects outside of the switch-case of
   event handlers.
6. Processing of **AM_EVT_HSM_ENTRY** and **AM_EVT_HSM_EXIT** events should
   not trigger state transitions. It means that user event handlers should
   not return **AM_HSM_TRAN()** or **AM_HSM_TRAN_REDISPATCH()** for
   these events.
7. Processing of **AM_EVT_HSM_INIT** event can optionally only trigger
   transition by returning the result of **AM_HSM_TRAN()** macro.
   The use of **AM_HSM_TRAN_REDISPATCH()** is not allowed in this case.
8. Processing of **AM_EVT_HSM_INIT** event can optionally only trigger
   transition to a substate of the state triggering the transition.
   Transition to peer states of superstates is not allowed in this case.

Transition To History
=====================

Transition to history is a useful technique that is convenient to apply in
certain use cases. It does not require to use any dedicated HSM API.

Given the example HSM above the transition to history technique can be
demonstrated as follows. Assume that the HSM is in the state *B*.
The user code stores the current state in a local variable of type
**struct am_hsm_state**. This is done with:

.. code-block:: C

   struct foo {
       struct am_hsm hsm;
       ...
       struct am_hsm_state history;
       ...
   };
   ...
   static enum am_hsm_rc B(struct foo *me, const struct event *event) {
       switch (event->id) {
       case AM_EVT_HSM_ENTRY:
           me->history  = am_hsm_get_state(&me->hsm);
           return AM_HSM_HANDLED();
       ...
       }
       return AM_HSM_SUPER(A);
   }

Then the transition to state *F* happens, which is then followed by a request
to transition back to the previous state. Since the previous state is captured
in **me->history** it can be achieved by doing this:

.. code-block:: C

   static enum am_hsm_rc F(struct foo *me, const struct event *event) {
       switch (event->id) {
       case HSM_EVT_FOO:
           return AM_HSM_TRAN(me->history.fn, me->history.instance);
       ...
       }
       return AM_HSM_SUPER(am_hsm_top);
   }

So, that is essentially all about it.

Another example of the usage of the transition to history technique can be seen
in **tests/history.c** unit test.

Submachines
===========

Submachines are reusable HSMs. They can be as simple as one reusable state.
The more complex submachines can be multi state interconnected HSMs.

The main purpose of submachines is code reuse.

Here is an example of submachine with one reusable state *s1*.
It shows two instances of *s1* called *s1/0* and *s1/1*.

::

            *
       +----|----------------------------------+
       |    |          am_hsm_top              |
       |    | (HSM top superstate am_hsm_top())|
       |    |                                  |
       |  +-v-------------------------------+  |
       |  |               s                 |  |
       |  |  +-----------+  +------------+  |  |
       |  |  |    s1/0   |  |    s1/1    |  |  |
       |  |  |   *       |  |   *        |  |  |
       |  |  |   |       |  |   |        |  |  |
       |  |  | +-v-----+ |  | +-v------+ |  |  |
       |  |  | |   s2  | |  | |   s3   | |  |  |
       |  |  | +-------+ |  | +--------+ |  |  |
       |  |  +---^-------+  +---^--------+  |  |
       |  |      | FOO          | BAR       |  |
       |  +------+-------^--+---+-----------+  |
       |                 |  |                  |
       |                 +--+ BAZ              |
       +---------------------------------------+

Here is how it is coded in pseudocode:

.. code-block:: C

   /* s1 submachine instances */
   #define S1_0 0
   #define S1_1 1

   struct sm {
       struct am_hsm hsm;
       ...
   };

   static enum am_hsm_rc s(struct sm *me, const struct event *event) {
       switch (event->id) {
       case FOO:
           return AM_HSM_TRAN(s1, /*instance=*/S1_0);
       case BAR:
           return AM_HSM_TRAN(s1, /*instance=*/S1_1);
       case BAZ:
           return AM_HSM_TRAN(s);
       ...
       }
       return AM_HSM_SUPER(am_hsm_top);
   }

   static enum am_hsm_rc s1(struct sm *me, const struct event *event) {
       switch (event->id) {
       case AM_EVT_HSM_INIT: {
           static const struct am_hsm_state tt[] = {
               [S1_0] = {.fn = AM_HSM_STATE_FN_CTOR(s2)},
               [S1_1] = {.fn = AM_HSM_STATE_FN_CTOR(s3)}
           };
           int instance = am_hsm_get_instance(&me->hsm);
           AM_ASSERT(instance < AM_COUNTOF(tt));
           return AM_HSM_TRAN(tt[instance].fn);
       }
       ...
       }
       return AM_HSM_SUPER(s);
   }

   static enum am_hsm_rc s2(struct sm *me, const struct event *event) {
       ...
       return AM_HSM_SUPER(s1, S1_0);
   }

   static enum am_hsm_rc s3(struct sm *me, const struct event *event) {
       ...
       return AM_HSM_SUPER(s1, S1_1);
   }

Please note that any transitions between states within submachines as well as
all references to any submachine state via **AM_HSM_SUPER()**  must be done
with explicit specification of state instance, which can be retrieved by
calling **am_hsm_get_instance()** API.

The complete implementation of the given submachine example can be found
in **tests/submachine/basic/test.c**

It is useful sometimes to instantiate a standalone submachine for the purpose
of unit testing, for example. To achieve this the transition tables outside of
the submachine must be extended with one more instance pointing to unit test
state(s). The extra instance then can be instantiated as a substate of
a unit test state machine(s).

A submachine (sub)state can also be a superstate of itself, which creates
a recursion. The example of the submachines recursion can be seen in
**tests/submachine/complex/submachine.c**.
