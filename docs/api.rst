API
===

.. module:: amast

This part of the documentation covers all the Amast interfaces.

.. _common_constants:

Common Constants
----------------

.. doxygenfile:: alignment.h

.. _common_types:

Common Types
------------

.. doxygenstruct:: am_blk
   :members:

.. _singly_linked_list_api:

Singly Linked List
------------------

Singly linked list API documentation.

The source code of the corresponding header file is in `slist.h <https://github.com/adel-mamin/amast/blob/main/libs/slist/slist.h>`_.

.. doxygenstruct:: am_slist_item

.. doxygenstruct:: am_slist

.. doxygenstruct:: am_slist_iterator

.. doxygenfunction:: am_slist_ctor

.. doxygenfunction:: am_slist_is_empty

.. doxygenfunction:: am_slist_item_is_linked

.. doxygenfunction:: am_slist_item_ctor

.. doxygenfunction:: am_slist_push_after

.. doxygenfunction:: am_slist_pop_after

.. doxygentypedef:: am_slist_item_found_fn

.. doxygenfunction:: am_slist_find

.. doxygenfunction:: am_slist_peek_front

.. doxygenfunction:: am_slist_peek_back

.. doxygenfunction:: am_slist_push_front

.. doxygenfunction:: am_slist_pop_front

.. doxygenfunction:: am_slist_push_back

.. doxygenfunction:: am_slist_owns

.. doxygenfunction:: am_slist_next_item

.. doxygenfunction:: am_slist_append

.. doxygenfunction:: am_slist_iterator_ctor

.. doxygenfunction:: am_slist_iterator_next

.. doxygenfunction:: am_slist_iterator_pop

.. _bit_api:

Bit
---

Bit API documentation.

The source code of the corresponding header file is in `bit.h <https://github.com/adel-mamin/amast/blob/main/libs/bit/bit.h>`_.

.. doxygenstruct:: am_bit_u64

.. doxygenfunction:: am_bit_u64_is_empty

.. doxygenfunction:: am_bit_u64_msb

.. doxygenfunction:: am_bit_u8_msb

.. doxygenfunction:: am_bit_u64_set

.. doxygenfunction:: am_bit_u64_clear

.. _ring_buffer_api:

Ring Buffer
-----------

Ring buffer API documentation.

The source code of the corresponding header file is in `ringbuf.h <https://github.com/adel-mamin/amast/blob/main/libs/ringbuf/ringbuf.h>`_.

.. doxygenstruct:: am_ringbuf

.. doxygenfunction:: am_ringbuf_ctor

.. doxygenfunction:: am_ringbuf_get_read_ptr

.. doxygenfunction:: am_ringbuf_get_write_ptr

.. doxygenfunction:: am_ringbuf_flush

.. doxygenfunction:: am_ringbuf_seek

.. doxygenfunction:: am_ringbuf_get_data_size

.. doxygenfunction:: am_ringbuf_get_free_size

.. doxygenfunction:: am_ringbuf_add_dropped

.. doxygenfunction:: am_ringbuf_get_dropped

.. doxygenfunction:: am_ringbuf_clear_dropped

.. _queue_api:

Queue
-----

Queue API documentation.

The source code of the corresponding header file is in `queue.h <https://github.com/adel-mamin/amast/blob/main/libs/bit/bit.h>`_.

.. doxygenstruct:: am_queue

.. doxygenfunction:: am_queue_ctor

.. doxygenfunction:: am_queue_dtor

.. doxygenfunction:: am_queue_is_valid

.. doxygenfunction:: am_queue_is_empty

.. doxygenfunction:: am_queue_is_full

.. doxygenfunction:: am_queue_get_nbusy

.. doxygenfunction:: am_queue_get_nfree

.. doxygenfunction:: am_queue_get_nfree_min

.. doxygenfunction:: am_queue_get_capacity

.. doxygenfunction:: am_queue_item_size

.. doxygenfunction:: am_queue_pop_front

.. doxygenfunction:: am_queue_pop_front_and_copy

.. doxygenfunction:: am_queue_peek_front

.. doxygenfunction:: am_queue_peek_back

.. doxygenfunction:: am_queue_push_front

.. doxygenfunction:: am_queue_push_back

.. _onesize_api:

Onesize
-------

Onesize memory allocator API documentation.

The source code of the corresponding header file is in `onesize.h <https://github.com/adel-mamin/amast/blob/main/libs/onesize/onesize.h>`_.

.. doxygenstruct:: am_onesize

.. doxygenstruct:: am_onesize_cfg
   :members:

.. doxygenfunction:: am_onesize_ctor

.. doxygenfunction:: am_onesize_allocate_x

.. doxygenfunction:: am_onesize_allocate

.. doxygenfunction:: am_onesize_free

.. doxygenfunction:: am_onesize_free_all

.. doxygentypedef:: am_onesize_iterate_fn

.. doxygenfunction:: am_onesize_iterate_over_allocated

.. doxygenfunction:: am_onesize_get_nfree

.. doxygenfunction:: am_onesize_get_nfree_min

.. doxygenfunction:: am_onesize_get_block_size

.. doxygenfunction:: am_onesize_get_nblocks

.. _event_api:

Event
-----

Event API documentation.

The source code of the corresponding header file is in `event.h <https://github.com/adel-mamin/amast/blob/main/libs/event/event.h>`_.

.. doxygendefine:: AM_EVT_USER

.. doxygendefine:: AM_EVENT_POOLS_NUM_MAX

.. doxygendefine:: AM_EVENT_HAS_USER_ID

.. doxygenenum:: am_event_rc

.. doxygenstruct:: am_event

.. doxygenstruct:: am_event_state_cfg
   :members:

.. doxygenfunction:: am_event_state_ctor

.. doxygenfunction:: am_event_add_pool

.. doxygenfunction:: am_event_get_pool_nfree

.. doxygenfunction:: am_event_get_pool_nfree_min

.. doxygenfunction:: am_event_get_pool_nblocks

.. doxygenfunction:: am_event_get_npools

.. doxygenfunction:: am_event_allocate_x

.. doxygenfunction:: am_event_allocate

.. doxygenfunction:: am_event_free

.. doxygenfunction:: am_event_dup_x

.. doxygenfunction:: am_event_dup

.. doxygentypedef:: am_event_log_fn

.. doxygenfunction:: am_event_log_pools

.. doxygenfunction:: am_event_is_static

.. doxygenfunction:: am_event_inc_ref_cnt

.. doxygenfunction:: am_event_dec_ref_cnt

.. doxygenfunction:: am_event_get_ref_cnt

.. doxygenfunction:: am_event_push_back_x

.. doxygenfunction:: am_event_push_back

.. doxygenfunction:: am_event_push_back_unsafe

.. doxygenfunction:: am_event_push_front_x

.. doxygenfunction:: am_event_push_front

.. doxygentypedef:: am_event_handle_fn

.. doxygenfunction:: am_event_pop_front

.. doxygenfunction:: am_event_flush_queue

.. _timer_api:

Timer
-----

Timer API documentation.

The source code of the corresponding header file is in `timer.h <https://github.com/adel-mamin/amast/blob/main/libs/timer/timer.h>`_.

.. doxygentypedef:: am_timer_post_unsafe_fn

.. doxygentypedef:: am_timer_publish_fn

.. doxygenstruct:: am_timer_state_cfg
   :members:

.. doxygenstruct:: am_timer

.. doxygenfunction:: am_timer_state_ctor

.. doxygenfunction:: am_timer_ctor

.. doxygenfunction:: am_timer_allocate

.. doxygenfunction:: am_timer_tick

.. doxygenfunction:: am_timer_arm_ticks

.. doxygenfunction:: am_timer_arm_ms

.. doxygenfunction:: am_timer_disarm

.. doxygenfunction:: am_timer_is_armed

.. doxygenfunction:: am_timer_domain_is_empty_unsafe

.. doxygenfunction:: am_timer_get_ticks

.. doxygenfunction:: am_timer_get_interval

.. _async_api:

Async
-----

Async API documentation.

The source code of the corresponding header file is in `async.h <https://github.com/adel-mamin/amast/blob/main/libs/async/async.h>`_.

.. doxygendefine:: AM_ASYNC_STATE_INIT

.. doxygenstruct:: am_async

.. doxygendefine:: AM_ASYNC_BEGIN

.. doxygendefine:: AM_ASYNC_EXIT

.. doxygendefine:: AM_ASYNC_END

.. doxygendefine:: AM_ASYNC_AWAIT

.. doxygendefine:: AM_ASYNC_YIELD

.. doxygenfunction:: am_async_is_busy

.. doxygenfunction:: am_async_ctor

.. _hsm_api:

HSM
---

Hierarchical State Machine (HSM) API documentation.

The source code of the corresponding header file is in `hsm.h <https://github.com/adel-mamin/amast/blob/main/libs/hsm/hsm.h>`_.

.. doxygenenum:: am_hsm_evt_id

.. doxygenenum:: am_hsm_rc

.. doxygentypedef:: am_hsm_state_fn

.. doxygentypedef:: am_hsm_spy_fn

.. doxygenstruct:: am_hsm_state

.. doxygendefine:: AM_HSM_STATE_CTOR

.. doxygendefine:: AM_HSM_HIERARCHY_DEPTH_MAX

.. doxygenstruct:: am_hsm

.. doxygendefine:: AM_HSM_HANDLED

.. doxygendefine:: AM_HSM_TRAN

.. doxygendefine:: AM_HSM_TRAN_REDISPATCH

.. doxygendefine:: AM_HSM_SUPER

.. doxygenfunction:: am_hsm_dispatch

.. doxygenfunction:: am_hsm_is_in

.. doxygenfunction:: am_hsm_state_is_eq

.. doxygenfunction:: am_hsm_get_instance

.. doxygenfunction:: am_hsm_get_state

.. doxygenfunction:: am_hsm_ctor

.. doxygenfunction:: am_hsm_dtor

.. doxygenfunction:: am_hsm_init

.. doxygenfunction:: am_hsm_set_spy

.. doxygenfunction:: am_hsm_top

.. _fsm_api:

FSM
---

Finite State Machine (FSM) API documentation.

The source code of the corresponding header file is in `fsm.h <https://github.com/adel-mamin/amast/blob/main/libs/fsm/fsm.h>`_.

.. doxygenenum:: am_fsm_evt_id

.. doxygenenum:: am_fsm_rc

.. doxygentypedef:: am_fsm_state_fn

.. doxygentypedef:: am_fsm_spy_fn

.. doxygendefine:: AM_FSM_STATE_CTOR

.. doxygenstruct:: am_fsm

.. doxygendefine:: AM_FSM_HANDLED

.. doxygendefine:: AM_FSM_TRAN

.. doxygendefine:: AM_FSM_TRAN_REDISPATCH

.. doxygenfunction:: am_fsm_dispatch

.. doxygenfunction:: am_fsm_is_in

.. doxygenfunction:: am_fsm_get_state

.. doxygenfunction:: am_fsm_ctor

.. doxygenfunction:: am_fsm_dtor

.. doxygenfunction:: am_fsm_init

.. doxygenfunction:: am_fsm_set_spy

.. _ao_api:

AO
--

Active Object (AO) API documentation.

The source code of the corresponding header file is in `ao.h <https://github.com/adel-mamin/amast/blob/main/libs/ao/ao.h>`_.

.. doxygenstruct:: am_ao

.. doxygenstruct:: am_ao_state_cfg
   :members:

.. doxygenstruct:: am_ao_prio
   :members:

.. doxygendefine:: AM_AO_NUM_MAX

.. doxygendefine:: AM_AO_PRIO_INVALID

.. doxygendefine:: AM_AO_PRIO_MIN

.. doxygendefine:: AM_AO_PRIO_MAX

.. doxygendefine:: AM_AO_PRIO_IS_VALID

.. doxygenstruct:: am_ao_subscribe_list

.. doxygenfunction:: am_ao_publish_exclude_x

.. doxygenfunction:: am_ao_publish_exclude

.. doxygenfunction:: am_ao_publish_x

.. doxygenfunction:: am_ao_publish

.. doxygenfunction:: am_ao_post_fifo_x

.. doxygenfunction:: am_ao_post_fifo

.. doxygenfunction:: am_ao_post_lifo_x

.. doxygenfunction:: am_ao_post_lifo

.. doxygenfunction:: am_ao_ctor

.. doxygenfunction:: am_ao_start

.. doxygenfunction:: am_ao_stop

.. doxygenfunction:: am_ao_state_ctor

.. doxygenfunction:: am_ao_state_dtor

.. doxygenfunction:: am_ao_subscribe

.. doxygenfunction:: am_ao_unsubscribe

.. doxygenfunction:: am_ao_unsubscribe_all

.. doxygenfunction:: am_ao_init_subscribe_list

.. doxygenfunction:: am_ao_run_all

.. doxygenfunction:: am_ao_event_queue_is_empty

.. doxygenfunction:: am_ao_log_event_queues

.. doxygenfunction:: am_ao_log_last_events

.. doxygenfunction:: am_ao_wait_start_all

.. doxygenfunction:: am_ao_get_cnt

.. doxygenfunction:: am_ao_get_own_prio

.. _pal_api:

PAL
---

The source code of the corresponding header file is in `pal.h <https://github.com/adel-mamin/amast/blob/main/libs/pal/pal.h>`_.

.. doxygenfile:: pal.h
