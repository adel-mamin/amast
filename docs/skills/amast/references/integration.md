# AMAST integration workflow

## 1. Inspect and pin

1. Check existing dependency policy, license approval, build system, target, and
   toolchain.
2. Inspect https://github.com/adel-mamin/amast/releases and the upstream
   changelog; select a **specific** release with a reproducible source/asset
   checksum or commit hash. Do not assume the version listed at skill creation
   remains latest.
3. Prefer the project's established dependency method. If vendoring source,
   record provenance and version and preserve the MIT license notice.
4. Read the **selected release's** README, headers, configuration, API docs, and
   examples. If a release differs from current documentation, prioritize
   matching code and documentation for the selected version.

## 2. Source layout as described by the upstream README

The upstream README describes copying the following files from a release:

- `amast.h`
- `amast_config.h`
- `amast.c`
- **one of** `amast_preemptive.c` or `amast_cooperative.c`

For features requiring platform porting, it mentions a port such as
`amast_posix.c` or `amast_libuv.c`. These are examples of supported ports, **not
a claim that every embedded target has a ready-made port**. Verify whether the
selected modules need platform adaptation and what the release actually ships;
implement and review an appropriate port when required.

For the library's own tests, the README also mentions `amast_test.h`,
`amast_test.c`, optional `Makefile`, and `make test`. Verify the test
arrangement in the selected release before invoking it.

**Important:** The list above is based on the upstream README, not on a
downloaded copy of a release archive. Always inspect the chosen release and its
build/configuration instructions; do not assume this list is exhaustive or
applicable to every version.

## 3. Integration sequence

1. Identify the smallest AMAST modules and the required scheduler/port.
2. Add source files and include paths to the existing build. Compile C code as
   C99; if integrating with C++, verify `extern "C"` handling and build/link
   compatibility rather than presuming it.
3. Set `amast_config.h` options based on the release's documented configuration
   and platform requirements; avoid guessed macros.
4. For AO: determine the cooperative vs preemptive execution model; initialize
   the platform abstraction and AO global runtime as required by that port;
   provide critical-section callbacks, queue storage, task stacks, and pub/sub
   storage when needed.
5. Keep port callbacks and timer/ticker behavior aligned with the target's
   clock/RTOS/interrupt semantics. Confirm whether each API is safe from ISR
   context or cross-thread calls.
6. Build a minimal FSM/HSM or AO end-to-end example and confirm event handling,
   transitions, shutdown, and error paths.
7. Integrate remaining features only after basic behavior works.

## 4. Correctness and safety review

- Events: Is every posted event and its payload alive until consumption? Who
  owns and releases it? Are stack-scoped objects ever queued asynchronously?
- Queues: What is the maximum burst, behavior on full queue, and priority
  inversion or starvation risk?
- Timers: Which timebase and overflow/cancellation semantics does the port use?
  Are timer events disarmed before owner destruction?
- Scheduling: Are AO callbacks nonblocking, and are concurrency/critical
  sections appropriate to actual platform conditions?
- State machines: Are all handled/unhandled events and superstate fallbacks
  correct? Are entry/exit actions idempotent as needed?
- Build/release: Is the dependency pinned, license retained, regression tested,
  and source identifiable?

## 5. Report results

State: chosen release/hash; AMAST modules; cooperative/preemptive mode and
platform port where applicable; source/config/build changes; tests actually run;
unresolved target-specific verification.

## Upstream sources

- Repository and README: https://github.com/adel-mamin/amast
- Releases: https://github.com/adel-mamin/amast/releases
- Changelog: https://github.com/adel-mamin/amast/blob/main/CHANGELOG.md
- Documentation: https://amast.readthedocs.io/
- Public API: https://amast.readthedocs.io/api.html
