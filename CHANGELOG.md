# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [0-based versioning](https://0ver.org/).

## [Unreleased]

### Changed

- Optimize `am_ao_unsubscribe_all()` implemenation
- Merge `enum am_event_rc` into `enum am_rc`
- Remove AM_MAY_ALIAS as redundant
- Reduce onesize allocator initialization time.
  This is done by replacing freelist full initialization at initialization time with
  gradual initialization during the normal operation time.

### Fixed

- Documentation build

## v0.11.0 - July 15, 2025

### Added

- Common return type `enum am_rc`
- `lib/async`, `lib/hsm` and `lib/fsm` APIs are all now return `enum am_rc`
  This allows for seamsless integration of async/await code with HSM and FSM code.
  See `apps/examples/async` for example usage.
- New API `AM_ASYNC_CHAIN()` to chain multiple async functions together
- `AM_UNIQUE(name)` macro
- Unit tests for `AM_DO_ONCE()`, `AM_DO_EVERY()`, `AM_DO_EACH_MS()` macros
- Extra event value invariant assert to `am_ao_publish_exclude_x()`

### Fixed

- Event value usage as a subscript to event subscription array in `am_ao_unsubscribe()`
  This bug led to incorrect unsubsciption from a given event.

### Changed

- Rework `AM_DO_ONCE()`, `AM_DO_EVERY()`, `AM_DO_EACH_MS()` macros

## v0.10.1 - July 4, 2025

### Added

- `libs/event/test.c`
- Valid ranges of configuration parameters in `tools/unity/amast_config.h`

### Fixed

- Event enum name in `apps/examples/workers/main.c`
- Markup in `libs/event/README.rst`

### Changed

- Rework `am_event_free()` to take pointer to event - not double pointer
- Use binary search of memory allocator in `am_event_allocate_x()`
- Rework `am_bit_u8_msb()`
- Rework `am_queue_ctor()` to take pointer and size separately
- Add `am_queue::capacity`
- Review IWYU pragmas usage
- Expand `AM_ASYNC_EXIT()` in `AM_ASYNC_END()`
- Use `AM_ASYNC_AWAIT()` to check for end of timeout in `apps/examples/async/main.c`
- Use debug non-optimized builds for pal set to stubs
- Improve `libs/timer/timer.h` API documentation
- Rename `am_timer::shot_in_ticks` to `am_timer::oneshot_ticks`
- Add comment to `am_ao_state_cfg::on_debug()`
- Update `libs/async/README.rst`
- Add asserts to async example async functions
- Move cppcheck warning suppression to `AM_ASYNC_BEGIN()`
- Amend `libs/hsm/README.rst`
- Improve `libs/event` comments
