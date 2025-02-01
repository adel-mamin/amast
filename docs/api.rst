API
===

.. module:: amast

This part of the documentation covers all the interfaces of Amast.

Common Types
------------

.. cpp:type:: uint8_t

.. doxygenstruct:: am_blk
   :members:

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

.. doxygenfunction:: am_event_state_ctor

.. doxygenfunction:: am_event_add_pool

.. doxygenfunction:: am_event_get_pool_min_nfree

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

