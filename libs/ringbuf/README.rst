===========
RING BUFFER
===========

OVERVIEW
========

The Ring Buffer module provides a high-performance circular buffer for
efficient data handling. It allows for lockless operation with one
producer and one consumer. It facilitates concurrent
read/write operations, supports data flow control, and minimizes memory
overhead.

KEY FEATURES
============

1. **Circular Buffer Design**:

   - Utilizes a fixed-size buffer for continuous data wrapping.
   - Supports concurrent read and write operations.

2. **Flexible Data Handling**:

   - Read and write pointers allow dynamic management of data availability.
   - Users can read and write directly to the buffer memory.

3. **Flow Control**:

   - Tracks the number of dropped bytes due to buffer overflow.
   - Provides utilities to manage and clear dropped data counters.

4. **Diagnostics**:

   - Functions to retrieve the size of available data and free space.
   - Utilities to track and adjust buffer state for debugging and optimization.

5. **Low Memory Overhead**:

   - Designed for embedded environments with constrained resources.

USAGE SCENARIOS
===============

The Ring Buffer module is well-suited for:

- **Data Streaming**: Managing continuous data streams in real-time systems.
- **Inter-Thread Communication**: Serving as a lockless shared buffer for
  producer-consumer scenarios.
- **Efficient Memory Usage**: Handling data in fixed-size buffers without
  dynamic allocation.

DESIGN CONSIDERATIONS
=====================

1. **Data Access**:

   - Provides direct access to buffer memory via read and write pointers.
   - Users must explicitly manage buffer state using ``flush`` and ``seek``
     functions.

2. **Concurrency**:

   - Reader and writer operations are synchronized through explicit calls to
     state management functions.
   - Provides mechanisms to ensure data integrity between read and write
     operations.

3. **Diagnostics and Flow Control**:

   - Tracks dropped bytes during buffer overflow conditions for performance
     analysis.
   - Provides utilities to monitor and clear dropped byte counters.

MODULE CONFIGURATION
====================

The module requires the following configuration:

- **Buffer Descriptor**: Describes the ring buffer's internal state, including
  read and write offsets, buffer size, and pointers.
- **Buffer Memory**: Preallocated buffer space to hold data.

SYSTEM INTEGRATION
==================

The module integrates seamlessly into systems requiring efficient data
handling. Key integration points include:

- **Initialization**: Use ``am_ringbuf_ctor`` to configure the buffer descriptor
  and associate it with the preallocated memory.
- **Data Handling**: Retrieve read and write pointers using
  ``am_ringbuf_get_read_ptr`` and ``am_ringbuf_get_write_ptr``, then manage data
  flow with ``am_ringbuf_flush`` and ``am_ringbuf_seek``.
- **Diagnostics**: Use utilities like ``am_ringbuf_get_data_size`` and
  ``am_ringbuf_get_free_size`` to monitor buffer state.

LIMITATIONS
===========

- Requires explicit state management through user calls to ``flush`` and ``seek``.
- Buffer size must be fixed and allocated in advance.

