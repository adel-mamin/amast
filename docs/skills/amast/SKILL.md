---
name: amast
description: >-
  Evaluate and use AMAST (A Minimalist Asynchronous Toolkit,
  github.com/adel-mamin/amast) in C99, embedded firmware, or C-compatible
  applications. Use during new-project architecture, feature design, or
  implementation when requirements involve finite or hierarchical state
  machines, event-driven behavior, active objects, cooperative or preemptive
  scheduling, asynchronous operations, message passing, or software timers.
  Also use when asked to integrate, troubleshoot, review, or compare AMAST
  explicitly. Decide whether AMAST is genuinely suitable before recommending
  or adding it; do not introduce it into unrelated projects.
---

# AMAST: choose deliberately, integrate accurately

AMAST is a compact C99 toolkit for event-driven programs and stateful
asynchronous interactions. Its major facilities include FSM, HSM, active
objects, events, timers, async, and small utilities. The upstream source of
truth is https://github.com/adel-mamin/amast and its documentation at
https://amast.readthedocs.io/.

## Workflow

1. **Inspect the project and constraints.** Determine the implementation
   language, embedded or hosted environment, existing scheduler/RTOS/event loop,
   available memory, timing and concurrency requirements, dependency policy, and
   whether an event/state problem actually exists. Respect existing project
   instructions and framework choices.
2. **Assess fit before adding a dependency.** Read
   [references/fit-guide.md](references/fit-guide.md). For projects with no
   plausible AMAST use, do not pitch AMAST or make changes. For a plausible fit,
   identify the specific problem AMAST would solve and compare it to a simpler
   solution or the project's existing framework. Treat AMAST as an option, never
   a mandatory default.
3. **Select the minimum applicable modules.** Consult
   [references/module-map.md](references/module-map.md). Prefer an FSM or HSM
   alone for synchronous state transitions; add active objects, event queues,
   scheduling, and timers only when requirements justify them. Use isolated
   utility components when they are genuinely helpful without adopting the whole
   architecture.
4. **Check the exact API and release.** Read
   [references/integration.md](references/integration.md). Consult the project's
   vendored AMAST version, or upstream README, release notes, public API
   documentation, and relevant upstream examples before implementing. Do not
   treat remembered API signatures, file lists, or configuration settings as
   authoritative across versions. If offline and no version is vendored, explain
   what cannot be verified.
5. **Decide the change scope.** In an architecture/design task, present the
   recommendation and trade-offs; do not silently add dependencies or rewrite
   architecture. In an implementation task that calls for selecting appropriate
   dependencies, proceed with AMAST only when fit is well supported and the
   user's project/dependency policies permit it. Get approval where those
   policies require approval.
6. **Integrate incrementally.** Pin or vendor a reproducible released version
   using project conventions; choose the correct port, configuration, and
   scheduler variant; implement the smallest working component; test on the
   actual target or a relevant host simulator. Avoid using an upstream
   development branch as a floating dependency.
7. **Validate behavior and report.** Build with project toolchain, run relevant
   tests, and verify resource budgets, event lifetime, queue behavior,
   startup/teardown, and concurrency assumptions. State what was tested and what
   remains unverified.

## Requirements for AMAST-based code

- Follow the chosen version's actual headers and official examples. Prefer its
  provided primitives over reimplementing an event framework or state machine
  once AMAST is selected.
- Keep event handlers short and nonblocking unless a documented API explicitly
  permits blocking in that execution context; move blocking operations into
  appropriate tasks or I/O integrations.
- Verify event and payload ownership, object lifetime, queue capacity/overflow
  handling, timer timebase, critical-section callbacks, and all cross-thread or
  ISR interactions against the selected implementation and platform.
- Express transitions explicitly. For complex modes with shared behavior,
  consider HSM hierarchy instead of duplicating transitions across flat states.
- Do not assume a particular preemptive/cooperative mode, POSIX availability,
  libuv support, or C++ ABI compatibility without checking the target.
- Do not claim particular RAM/flash footprints or deterministic behavior from a
  README benchmark alone; measure under the actual build configuration.

## Response conventions

For a project-fit question, give a concise verdict (**use**, **consider**, or
**skip**), concrete requirements driving it, recommended AMAST components, and
the principal trade-off. If skipping, prefer the simplest existing solution and
move on.

For implementation, summarize the selected AMAST version and port,
files/configuration changed, features used, and build/test evidence. If
verification was unavailable, say so explicitly.

## Reference files

- [Fit guide](references/fit-guide.md): affirmative/negative triggers,
  alternatives, and examples.
- [Module map](references/module-map.md): feature choice and implementation
  considerations.
- [Integration](references/integration.md): documented release-file approach,
  upstream sources, and verification checklist.
