# Interpreted HSM design

Interpreted HSM (IHSM) allows for execution of HSM model described in JSON format.

The JSON description is provided as a text to am_ihsm_load() function.

The IHSM implementation targets the HSM JSON description formatted by
[state-machine-cat](https://github.com/sverweij/state-machine-cat).

## Design requirements

The IHSM design should permit adding support for other varieties of
HSM JSON descriptions in future.

## Not Supported Features

- [Forks, Joins and Junctions](https://github.com/sverweij/state-machine-cat/blob/main/README.md#forks-joins-and-junctions---)
- [Parallel States](https://github.com/sverweij/state-machine-cat/blob/main/README.md#parallel-states)

## Memory Management

IHSM uses user provided memory pool to accommodate HSM model.
The memory pool is set by calling am_ihsm_set_pool() API.

The pool size can be either statically set large enough to accommodate
any reasonably large HSM model or can be determined at run time for
a given HSM JSON model by calling am_ihsm_load() API.

## IHSM life cycle

The typical usage sequence of IHSM APIs:

1. am_ihsm_ctor()
2. am_ihsm_set_pool()

3. am_ihsm_set_action_fn()
4. am_ihsm_set_error_fn()
5. am_ihsm_set_choice_fn()

Steps 3-5 can be repeated at any order and time during HSM execution.

6. am_ihsm_load()
7. am_ihsm_init()
8. am_ihsm_dispatch() - zero or more
9. am_ihsm_term()

Steps 2-6 can be repeated multiple times with different HSM JSON descriptions.

10. am_ihsm_dtor()

## UML Choices And Guards

Both choices are guards are handled with single callback set
via am_ihsm_set_choice_fn() API function. The callback is given the list
of options to choose from. The list is terminated with NULL pointer.

For guards the options are always "true" and "false".

For choices the options are copied from JSON model.

## HSM Model Execution

The HSM model execution is done by using `libs/hsm` library.
For this to work the IHSM descriptor `struct am_ihsm` inherits from
`struct am_hsm` and provides a single event handler `am_ihsm_state()`.

The event handler calls `AM_HSM_SUPER()`, `AM_HSM_TRAN(_REDISPATCH)()` and
`AM_HSM_HANDLED()` macros directly.

`AM_HSM_SUPER()` is always called with `am_ihsm_state` or `am_hsm_top`
as the first parameter. In case of `am_ihsm_state` the macro is also given
the unique instance number, which is an index to memory pool's memory block
that holds `struct am_hsm_state` superstate instance.

Similarly `AM_HSM_TRAN(_REDISPATCH)()` is always called with `am_ihsm_state`
as the first parameter and the unique instance number, which is an index
to memory pool memory block that holds `struct am_hsm_state` destination
state instance.

## JSON Parser

JSON parser is to be implemented with a modified version of
[JSMN](https://github.com/zserge/jsmn). The modification are to be done to tailor
the implementation to IHSM specifics. The details are TBD.

## Internal Data Structures

Every HSM state is described by `struct am_ihsm_state`.
Every HSM transition is described by `struct am_ihsm_tran`.

Every HSM state, which has outgoing transitions holds a pointer to
the list of all outgoing transitions in its `struct am_ihsm_tran::tran_list`.
The transitions are organized into a singly linked list, where
`struct am_ihsm_tran::next_tran` points to next transition or set to 0,
which terminates the list.

The pointers are represented as the unsigned integers, which are indices
of memory pool's memory blocks.

Each HSM transition descriptor has event hash in addition to event name.
It is used to speed up the look up of outgoing transition bye event name.

## Init And Final Transitions

The init transition is done via explicit call to `am_ihsm_init()` API.
The final transition could be done as a result of a regular HSM operation
or via explicit call to am_ihsm_term() API.