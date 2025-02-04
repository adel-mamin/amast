API
===

.. module:: amast

This part of the documentation covers all the interfaces of Amast.

Common Types
------------

.. cpp:type:: uint8_t

.. doxygenstruct:: am_blk
   :members:

Singly Linked List
------------------

.. doxygenstruct:: am_slist_item

.. doxygenstruct:: am_slist

.. doxygenstruct:: am_slist_iterator

.. doxygenfunction:: am_slist_init

.. doxygenfunction:: am_slist_is_empty

.. doxygenfunction:: am_slist_item_is_linked

.. doxygenfunction:: am_slist_item_init

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

.. doxygenfunction:: am_slist_iterator_init

.. doxygenfunction:: am_slist_iterator_next

.. doxygenfunction:: am_slist_iterator_pop

Ring Buffer
-----------

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

Queue
-----

.. doxygenstruct:: am_queue

.. doxygenfunction:: am_queue_ctor

.. doxygenfunction:: am_queue_dtor

.. doxygenfunction:: am_queue_is_empty

.. doxygenfunction:: am_queue_is_full

.. doxygenfunction:: am_queue_length

.. doxygenfunction:: am_queue_capacity

.. doxygenfunction:: am_queue_item_size

.. doxygenfunction:: am_queue_pop_front

.. doxygenfunction:: am_queue_pop_front_and_copy

.. doxygenfunction:: am_queue_peek_front

.. doxygenfunction:: am_queue_peek_back

.. doxygenfunction:: am_queue_push_front

.. doxygenfunction:: am_queue_push_back

Event
-----

.. doxygendefine:: AM_EVT_USER

.. doxygendefine:: AM_EVENT_POOL_NUM_MAX

.. doxygendefine:: AM_EVENT_HAS_USER_ID

.. doxygenenum:: am_event_rc

.. doxygenstruct:: am_event

.. doxygenstruct:: am_event_state_cfg
   :members:

.. doxygenfunction:: am_event_state_ctor

.. doxygenfunction:: am_event_add_pool

.. doxygenfunction:: am_event_get_pool_nfree_min

.. doxygenfunction:: am_event_get_pool_nfree_now

.. doxygenfunction:: am_event_get_pool_nblocks

.. doxygenfunction:: am_event_get_pools_num

.. doxygenfunction:: am_event_allocate

.. doxygenfunction:: am_event_free

.. doxygenfunction:: am_event_dup

.. doxygentypedef:: am_event_log_fn

.. doxygenfunction:: am_event_log_pools

.. doxygenfunction:: am_event_is_static

.. doxygenfunction:: am_event_inc_ref_cnt

.. doxygenfunction:: am_event_dec_ref_cnt

.. doxygenfunction:: am_event_get_ref_cnt

.. doxygenfunction:: am_event_push_back_x

.. doxygenfunction:: am_event_push_back

.. doxygenfunction:: am_event_push_front_x

.. doxygenfunction:: am_event_push_front

.. doxygenfunction:: am_event_pop_front

.. doxygenfunction:: am_event_defer

.. doxygenfunction:: am_event_defer_x

.. doxygentypedef:: am_event_recall_fn

.. doxygenfunction:: am_event_recall

.. doxygenfunction:: am_event_flush_queue

Timer
-----

.. doxygentypedef:: am_timer_post_fn

.. doxygentypedef:: am_timer_publish_fn

.. doxygenstruct:: am_timer_state_cfg
   :members:

.. doxygenstruct:: am_timer

.. doxygenfunction:: am_timer_state_ctor

.. doxygenfunction:: am_timer_ctor

.. doxygenfunction:: am_timer_allocate

.. doxygenfunction:: am_timer_tick

.. doxygenfunction:: am_timer_arm

.. doxygenfunction:: am_timer_disarm

.. doxygenfunction:: am_timer_is_armed

.. doxygenfunction:: am_timer_domain_is_empty

.. doxygenfunction:: am_timer_get_ticks

.. doxygenfunction:: am_timer_get_interval

Async
-----

.. doxygendefine:: AM_ASYNC_STATE_INIT

.. doxygenenum:: am_async_rc

.. doxygenstruct:: am_async

.. doxygendefine:: AM_ASYNC_BEGIN

.. doxygendefine:: AM_ASYNC_BREAK

.. doxygendefine:: AM_ASYNC_END

.. doxygendefine:: AM_ASYNC_LABEL

.. doxygendefine:: AM_ASYNC_AWAIT

.. doxygendefine:: AM_ASYNC_YIELD

.. doxygendefine:: AM_ASYNC_RC

.. doxygenfunction:: am_async_init
