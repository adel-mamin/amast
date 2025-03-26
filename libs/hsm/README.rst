================================
Hierarchical State Machine (HSM)
================================

Credit
======

The design and implementation of the HSM library is heavily inspired by
`Practical UML Statecharts in C/C++: Event-Driven Programming for Embedded Systems 2nd Edition <https://www.state-machine.com/psicc2>`_
by Miro Samek. Also the example HSM state diagram in
`hsm.png <https://github.com/adel-mamin/amast/blob/main/libs/hsm/hsm.png>`_ is borrowed from the book.

Glossary
========

   event
       a unique ID plus optionally some data associated with it

   entry event
       an event sent to a state when the state is entered.
       The state can optionally run entry actions on it.
       The event has ID :cpp:enumerator:`AM_EVT_HSM_ENTRY <am_hsm_evt_id::AM_EVT_HSM_ENTRY>`.

   exit event
       an event sent to a state when the state is exited.
       The state can optionally run exit actions on it.
       The event has ID :cpp:enumerator:`AM_EVT_HSM_EXIT <am_hsm_evt_id::AM_EVT_HSM_EXIT>`.

   external transition
       An external transition is a transition that moves from one state to another,
       possibly re-entering the same state.
       The source state is exited, the target state is entered and initialized.
       Synonym of **state transition**.

   init event
       an event sent to a target state, right after the state was entered.
       The state can optionally trigger transition to a substate on it.
       The event has ID :cpp:enumerator:`AM_EVT_HSM_INIT <am_hsm_evt_id::AM_EVT_HSM_INIT>`.
       It immediately follows the entry event.

   internal transition
       An internal transition is a transition that occurs within a state without
       causing an exit and re-entry.

   state
       an event handler. *A*, *B*, *C*, *D*, *E*, *F*, *am_hsm_top* are all
       states (see state diagram in :ref:`example-hsm` section below)

   current state
       the state which currently receives incoming events

   active state
       same as current state

   state transition
       the process of changing of the current state to another or to itself

   source state
       the state that initiates the state transition

   target state
       the destination state of the state transition

   initial transition
       the state transition that may optionally happen after entering a state,
       if the state is a target state of the state transition.
       In the state diagram :ref:`example-hsm` section below
       the state *D* has the initial transition,
       whereas state *B* does not. The initial transition in the state *D*
       is only activated, if the state *D* is a target state of a state transition.
       For example, if *B* triggers the transition to *D*, then the initial
       transition is activated. However if *B* triggers the transition to *A*, then
       the initial transition in *D* is not activated.

   superstate
       an HSM state that is a parent (ancestor) of one or more other states
       (children, substates). *A*, *B*, *D*, *am_hsm_top* are all superstates.

   top (super)state
       the ultimate root of the state hierarchy.
       It is predefined by :cpp:func:`am_hsm_top()` state.

   substate
       a state that has a superstate as its parent (ancestor).
       A state can be substate and superstate simultaneously.
       *A*, *B*, *C*, *D*, *E*, *F* are all substates (see state diagram in
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
       *B*-*A*-*am_hsm_top* is the ancestor chain.
       Another one is *F* - *am_hsm_top* etc.

   nearest common ancestor (NCA)
       the first common ancestor in two ancestor chains constructed from
       source and target states to the top level superstate.
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
inheritance, which is called behavioral inheritance.
The parent-child relationship between states impacts both event handling and
transitions.

The HSM is a combination of one or more state-handler functions of
type :cpp:type:`am_hsm_state_fn`.

.. _example-hsm:

Example HSM
===========

In order to explore how event handling and transitions work in an HSM,
consider the below state machine:

.. uml::

    @startuml

    [*] --> A

    state am_hsm_top #LightBlue {
        state A #LightBlue {
            state B #LightBlue {
                state C #LightBlue
            }
            state D #LightBlue {
                [*] --> E
                state E #LightBlue
            }
        }
        state F #LightBlue
    }

    @enduml

State Relations
===============

States *B* and *D* are children of *A*. States *C* and *E* are children
of *B* and *D*, respectively.  State *F* has no children.
Both *A* and *F* have the default parent *am_hsm_top* provided by
the library (:cpp:func:`am_hsm_top()`).

Event Dispatching
=================

Event dispatching is always done by calling :cpp:func:`am_hsm_dispatch()`
function. It takes state machine as first parameter and event to dispatch
as second parameter.

The dispatching is the synchronous procedure, which means that by the time
the function returns the event is processed by the state machine.
If event triggers a state transition, then the state transition including
all exit, entry and init actions is also complete.

Event Propagation
=================

Events are always sent first to the active state. The active state can choose
whether to consume the event or to pass it to its parent. If the state
chooses to consume the event then event handling ends with the state. If,
however, the state chooses to pass, then the event will be sent to the state's
parent. At this point the parent must make the same decision. Event handling
ends when the state or one of its ancestors consumes the event or the event
reaches the default superstate :cpp:func:`am_hsm_top()`. The default top level
superstate :cpp:func:`am_hsm_top()` always returns
:cpp:enumerator:`AM_HSM_RC_HANDLED <am_hsm_rc::AM_HSM_RC_HANDLED>` for
all events meaning that it is consumed.

Assume that the state *C* shown in the state diagram in :ref:`example-hsm` above
is active and an event is sent to the state machine. State *C* will be the first
state to receive this event. If it chooses to pass then, the event will be sent
to state *B*, which is its direct parent. If state *B* also chooses to pass,
then the event will finally be sent to state *A*. If *A* chooses to pass, then
the event is consumed by :cpp:func:`am_hsm_top()`.

*am_hsm_top* (:cpp:func:`am_hsm_top()`) does nothing with events and serves as
the ultimate event propagation termination point.

To inform the library that an event is handled the event handler function
must return :c:macro:`AM_HSM_HANDLED()`.

To inform the library that an event is passed to superstate the event
handler function must return :c:macro:`AM_HSM_SUPER()`, which provides the
name of the superstate event handler.

State Transition
================

When transitioning it is important to distinguish the current state and the
source state. They are not necessarily the same state.

In the state diagram in :ref:`example-hsm` above consider the case when
the current state is *C*, an event is received by *C* and passed first to the
superstate *B* and then to the superstate *A*, which decides to make
a transition to the state *F*.  In this case the current state is *C*,
the source state is *A* and the target state is *F*.

When transitioning, exit events
(:cpp:enumerator:`AM_EVT_HSM_EXIT <am_hsm_evt_id::AM_EVT_HSM_EXIT>`) are sent
by the library automatically up the ancestor chain until reaching the nearest
common ancestor (NCA) of the source and target states.
Then, entry events (:cpp:enumerator:`AM_EVT_HSM_ENTRY <am_hsm_evt_id::AM_EVT_HSM_ENTRY>`)
are sent automatically by the library down the ancestor chain to the target state.
Finally the library sends the init event
(:cpp:enumerator:`AM_EVT_HSM_INIT <am_hsm_evt_id::AM_EVT_HSM_INIT>`) to the target state.
The NCA does not receive the exit event nor does it receive the entry and init events.

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
target state *A*, then NCA is *A*, the exit events are sent to *C*, *B*, *A* and
then the entry event is sent to *A* followed by the init event.

If *C* is the current state and the transition is initiated by *C* with the
target state *A*, then NCA is *A*, the exit events are sent to *C*, *B* and then
the init event is sent to *A*. Please note that the state *A* is not exited in
this case.

To initiate a transition the state handler function must return
:c:macro:`AM_HSM_TRAN()` or :c:macro:`AM_HSM_TRAN_REDISPATCH()` pointing
to target state.

If state handler function returns :c:macro:`AM_HSM_TRAN_REDISPATCH()` pointing
to target state, then the transition is executed first and then the same event is
dispatched to the new current state in the same :cpp:func:`am_hsm_dispatch()` call.
This is a convenience feature, that allows HSM to handle the event in
the state that expects it.

HSM states cannot initiate state transitions when processing entry and exit
events. This means that the HSM states cannot return :c:macro:`AM_HSM_TRAN()`
or :c:macro:`AM_HSM_TRAN_REDISPATCH()` pointing to target state.

Initial State Transition
========================

If *C* is the current state and the transition is initiated by *A* with the
target state *D*, then NCA is *A*, the exit events are sent to *C*, *B* and
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

The initial state is entered, when calling :cpp:func:`am_hsm_init()` function.
The initial state must always return :c:macro:`AM_HSM_TRAN()` pointing to
target state.

The transition from the initial state to the target state is done by
the time :cpp:func:`am_hsm_init()` exits.


HSM Initialization
==================

HSM initialization is divided into the following two steps for increased
flexibility and better control of the initialization timeline:

1. the state machine constructor (:cpp:func:`am_hsm_ctor()`)
2. the top-most initial transition (:cpp:func:`am_hsm_init()`).

HSM Topology
============

HSM library discovers the user HSM topology at run time by sending
:cpp:enumerator:`AM_EVT_HSM_EMPTY <am_hsm_evt_id::AM_EVT_HSM_EMPTY>` event
to state event handlers. The state event handlers should always return
:c:macro:`AM_HSM_SUPER()` in response.

HSM Coding Rules
================

1. HSM states must be represented by event handlers of type :cpp:type:`am_hsm_state_fn`.
2. The name of the first argument of all user event handler functions
   must be **me**.
3. For convenience instead of using **struct** :cpp:struct:`am_hsm` ***me**
   the first argument can point to a user structure. In this case the user structure
   must have **struct** :cpp:struct:`am_hsm` instance as its first field.

   For example, the first argument can be **struct foo *me**, where
   **struct foo** is defined like this:

   .. code-block:: C

      struct foo {
          struct am_hsm hsm;
          ...
      };

   The event handler in this case could look like this:

   .. code-block:: C

      enum am_hsm_rc foo_handler(struct foo *me, const struct am_event *event);

4. Each user event handler should be implemented as a switch-case of handled
   events.
5. Avoid placing any code with side effects outside of the switch-case of
   event handlers.
6. Processing of :cpp:enumerator:`AM_EVT_HSM_ENTRY <am_hsm_evt_id::AM_EVT_HSM_ENTRY>`
   and :cpp:enumerator:`AM_EVT_HSM_EXIT <am_hsm_evt_id::AM_EVT_HSM_EXIT>` events should
   not trigger state transitions. It means that user event handlers should
   not return :c:macro:`AM_HSM_TRAN()` or :c:macro:`AM_HSM_TRAN_REDISPATCH()` for
   these events.
7. Processing of :cpp:enumerator:`AM_EVT_HSM_INIT <am_hsm_evt_id::AM_EVT_HSM_INIT>`
   event can optionally only trigger transition by returning the result of
   :c:macro:`AM_HSM_TRAN()` macro.
   The use of :c:macro:`AM_HSM_TRAN_REDISPATCH()` is not allowed in this case.
8. Processing of :cpp:enumerator:`AM_EVT_HSM_INIT <am_hsm_evt_id::AM_EVT_HSM_INIT>`
   event can optionally only trigger transition to a substate of the state triggering
   the transition.
   Transition to peer states of superstates is not allowed in this case.

Transition To History
=====================

Transition to history is a useful technique that is convenient to apply in
certain use cases. It does not require to use any dedicated HSM library API.

Given the state diagram :ref:`example-hsm` section above the transition
to history technique can be demonstrated as follows. Assume that the HSM
is in the state *B*.
On entry to the state user code stores the state in a local variable
of type **struct** :cpp:struct:`am_hsm_state`. This is done with:

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
in **me->history** the transition can be achieved by doing this:

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
in `tests/history.c <https://github.com/adel-mamin/amast/blob/main/libs/hsm/tests/history.c>`_ unit test.

Submachines
===========

Submachines are reusable HSMs. They can be as simple as one reusable state.
The more complex submachines can be multi state interconnected HSMs.

The main purpose of submachines is code reuse.

Here is an example of submachine with one reusable state *s1*.
It shows two instances of *s1* called *s1/0* and *s1/1*.

.. uml::

    @startuml

    [*] --> s

    state am_hsm_top #LightBlue {
        state s #LightBlue {
            state s1_0 #LightBlue {
                [*] --> s2
                state s2 #LightBlue
            }
            state s1_1 #LightBlue {
                [*] --> s3
                state s3 #LightBlue
            }
        }

        s --> s1_0 : FOO
        s --> s1_1 : BAR
        s --> s : BAZ
    }

    @enduml

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
all references to any submachine state via :c:macro:`AM_HSM_SUPER()`  must be done
with explicit specification of state instance, which can be retrieved by
calling :cpp:func:`am_hsm_get_instance()` API.

The complete implementation of the given submachine example can be found
in `tests/submachine/basic/test.c <https://github.com/adel-mamin/amast/blob/main/libs/hsm/tests/submachine/basic/test.c>`_

A submachine (sub)state can also be a superstate of itself, which creates
a recursion. The example of the submachines recursion can be seen in
`tests/submachine/complex/submachine.c <https://github.com/adel-mamin/amast/blob/main/libs/hsm/tests/submachine/complex/submachine.c>`_.

HSM Examples And Unit Tests
===========================

HSM With Event Queue
--------------------

Different libraries are mixed together to demonstrate:

- the use of event queue with HSM
- how HSM can send events to itself
- how the events sent to itself are then dispatched back the the HSM
- how events can be allocated on stack or from event memory pool
- how the events allocated from the memory pool are then freed
  by the event library

The key libraries at play here are:

- :ref:`hsm_api`
- :ref:`event_api`
- :ref:`onesize_api`
- :ref:`queue_api`

The source code is in `event_queue.c <https://github.com/adel-mamin/amast/blob/main/libs/hsm/tests/event_queue.c>`_.

The HSM topology:

.. uml::

    @startuml

    left to right direction

    [*] --> hsmq_a

    state am_hsm_top #LightBlue {
        state hsmq_a #LightBlue
        state hsmq_b #LightBlue {
        }

        hsmq_a --> hsmq_b : A
    }

    hsmq_b : B /
    hsmq_b : C /

    @enduml

::

, where

- A is short of **HSM_EVT_A**
- B is short of **HSM_EVT_B**
- C is short of **HSM_EVT_C**

The test steps:

1. Construct the HSM by calling **hsmq_ctor()**.
   The HSM construction includes the HSM event queue setup.
2. Initialize the HSM. The init state transition activates **hsmq_a**.
3. Enter the cycle of injection of external events with ID listed in
   **in[]** array: **AM_EVT_A** and **AM_EVT_C**.
   The injection is done by calling :cpp:func:`am_hsm_dispatch()` followed
   by **hsmq_commit()** call.
   The **hsmq_commit()** call goes though all events in HSM event queue
   and dispatches them one by one until the queue is empty.
4. Each external event is associated with constant string of expected
   event processing steps in the HSM. The association is listed in
   the array of **struct hsmq_test** items.
   The constant strings are then compared to the actual HSM event processing
   log generated by HSM with **me->log()** calls.

Defer
-----

Test simple HSM with event queue and deferred event queue.

The source code is in `defer.c <https://github.com/adel-mamin/amast/blob/main/libs/hsm/tests/defer.c>`_.

The HSM topology:

.. uml::

   @startuml

   left to right direction

   [*] --> defer_s1

   state am_hsm_top #LightBlue {
       state defer_s1 #LightBlue
       state defer_s2 #LightBlue
   }

   defer_s1 : A / defer
   defer_s1 : X / recall
   defer_s1 --> defer_s2 : B

   defer_s2 : A /

   @enduml

, where

- **A** is short of **HSM_EVT_A**
- **B** is short of **HSM_EVT_B**
- **X** is short of :cpp:enumerator:`AM_EVT_HSM_EXIT <am_hsm_evt_id::AM_EVT_HSM_EXIT>`

The test steps:

1. Initialize the HSM. The init state transition activates **defer_s1**
2. Send **A** event, which triggers an internal transition in **defer_s1** by deferring the event.
3. Send **B** event, which triggers an external transition to **defer_s2** and
   recalls **A** on exit.
4. Event **A** is handled in **defer_s2**.

All internal and external transitions in HSM are logged and compared against
expected patterns stored in **struct test::out**.

HSM destructor
--------------

Tests :cpp:func:`am_hsm_dtor()` API.

The source code is in `dtor.c <https://github.com/adel-mamin/amast/blob/main/libs/hsm/tests/dtor.c>`_.

The HSM topology:

.. uml::

   @startuml

   left to right direction

   [*] --> dtor_s

   state am_hsm_top #LightBlue {
       state dtor_s #LightBlue
   }

   @enduml

The test steps:

1. Initialize the HSM. The init state transition activates **dtor_s**.
2. Call :cpp:func:`am_hsm_dtor()` for the HSM and check if it destructs the HSM.

HSM history
-----------

Demonstrates the HSM history pattern usage modeling the operation of
a microwave oven.

The source code is in `history.c <https://github.com/adel-mamin/amast/blob/main/libs/hsm/tests/history.c>`_.

The HSM topology:

.. uml::

    @startuml

    left to right direction

    [*] --> open : door open
    [*] --> closed : door closed

    state closed #LightBlue {
        [*] --> H
        H --> off
        state H <<history>>
        state on #LightBlue
        state off #LightBlue
    }
    state open #LightBlue

    open --> closed : close door
    closed --> open : open door

    on --> off : ON
    off --> on : OFF

    @enduml

The test steps:

1. Initialize the HSM.
   The init state does two things:

   - sets history state to **off**
   - requests transition to either **open** or **closed** state depending on
     whether the oven door is open or closed. The oven door is closed.
     So, the transition is done to **closed** state and **off** substate.

   Check that the current state is **off**.

2. Send **ON** event. Check that the current state is **on**.
3. Send **OPEN** event. Check that the current state is **open**.
4. Send **CLOSE** event. Check that the current state is **on**.
