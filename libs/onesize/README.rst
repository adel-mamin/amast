========================
Onesize Memory Allocator
========================

Overview
========

The Onesize Memory Allocator module provides a simple and efficient interface
for allocating and managing fixed-size memory blocks. It is designed for
high-performance allocation and deallocation in scenarios where all memory
requests are of a uniform size or can be rounded to a fixed size.

Key Features
============

1. **Fixed-Size Memory Allocation**:

   - Handles memory allocation requests up to a predefined block size.
   - Allocations are fast due to the simplicity of a singly-linked free list.

2. **Efficient Memory Management**:

   - Reclaims memory by returning freed blocks to the front of the free list.
   - Supports batch memory reclamation with a function to free all allocated
     blocks.

3. **Diagnostics and Debugging**:

   - Utilities to inspect memory usage and iterate over allocated memory blocks
     with a callback function.
   - Tracks the minimum number of free blocks available to monitor resource
     usage.

4. **Custom Configuration**:

   - Supports custom memory pools and alignment requirements

Usage Scenarios
===============

The Onesize Memory Allocator is ideal for systems that require:

- **Uniform Memory Allocation**: Efficient handling of memory blocks of
  consistent sizes.
- **Real-Time Performance**: Fast allocation and deallocation with minimal
  overhead.

Design Considerations
=====================

1. **Memory Allocation**:

   - The allocator (``am_onesize``) maintains a singly-linked free list to store
     unused memory blocks.
   - Blocks are allocated from the front of the free list, ensuring fast
     retrieval.

2. **Configuration Options**:

   - Customizable block size and alignment settings are defined during
     construction.

3. **Diagnostics**:

   - Functions are provided to query the number of free blocks, the minimum
     free block count, and the total block size.
   - Iteration over allocated blocks allows inspection of memory usage for
     debugging.

Module Configuration
====================

The module configuration (``am_onesize_cfg``) specifies:

- **Memory Pool**: The memory pool from which blocks are allocated.
- **Block Size and Alignment**: The size and alignment of memory blocks.

System Integration
==================

The module integrates seamlessly into memory management systems for embedded
and real-time applications. Key integration points include:

- **Initialization**: Use ``am_onesize_init`` to configure and initialize the
  allocator.
- **Memory Allocation**: Allocate memory using ``am_onesize_allocate`` and free
  blocks with ``am_onesize_free``.
- **Diagnostics**: Inspect memory usage with utilities like
  ``am_onesize_get_nfree`` and ``am_onesize_iterate_over_allocated``.

Limitations
===========

- Only supports allocation requests up to the configured block size.
- Does not support dynamic resizing of blocks after initialization.
