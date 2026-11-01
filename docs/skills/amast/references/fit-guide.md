# AMAST selection guide

Use this when deciding whether AMAST should enter an architecture or
implementation.

## Favor AMAST when several of these hold

- Code is in C99 (or an interoperability environment with a verified C
  interface) and state/event behavior is central.
- The application has modes or protocol phases driven by commands, timeouts,
  external interrupts, or messages.
- Hierarchical states would reduce duplicated transitions, guards, and
  entry/exit behaviors.
- Multiple independently progressing components need queued event delivery or
  publish/subscribe.
- There are predictable memory/CPU constraints and a small C-oriented toolkit is
  preferable to a heavyweight framework.
- The target can support the chosen module's scheduling, clocks, critical
  sections, and port requirements.

## Prefer simpler or established alternatives when

- The task is a pure function, static data transform, simple CLI, or one-off
  script with no meaningful event/state model.
- A small local `switch` is clearer than introducing a formal FSM dependency.
- The codebase already has a suitable event loop, scheduler, HSM/FSM
  implementation, or RTOS integration and replacement would create duplication
  or migration risk.
- The language is not C/C-compatible and FFI would dominate the problem.
- Portability, safety case, toolchain, license policy, or deployment constraints
  have not been established; investigate first.
- Another existing project dependency fully addresses the same requirement.

## Match the problem, not the library

- **Few simple mode transitions:** Consider a plain C switch or AMAST FSM.
  Check whether a separate abstraction is worth it.
- **Shared state behavior / nested modes:** Consider AMAST HSM. Check whether
  hierarchy and entry/exit semantics are useful.
- **Independent event-driven components:** Consider AMAST active objects and
  events. Check the scheduler, queue sizing, and synchronization requirements.
- **Timeouts or periodic callbacks:** Consider AMAST timer. Check whether
  the target already provides a timer service.
- **Async flow in C:** Consider AMAST async. Check whether its execution
  model fits the calling code.
- **Bounded buffer or list structure:** Consider an individual AMAST utility.
  Check whether an existing local implementation is already sufficient.

## Examples

**Strong:** Embedded C device controller with disconnected / pairing / connected
/ recovery states, timeout events, and asynchronous peripherals. Consider HSM,
timers, then AO if isolated event consumers are needed.

**Conditional:** Network service in C already using libuv. Check if AMAST's
libuv port and selected modules add value beyond libuv; avoid duplicate
scheduling layers.

**Weak:** A Python web service, standalone JSON formatter, or C math library
without asynchronous modes. Skip AMAST.

**Migration:** A working RTOS firmware using an established state-machine
library. Keep the existing architecture by default; propose AMAST only if there
is a concrete, measurable migration benefit.

## Decision format

- **Verdict**: Use / Consider / Skip
- **Requirements**: Which project facts support this verdict?
- **Components**: Which minimal AMAST modules would be used?
- **Alternatives**: What is the simplest workable alternative?
- **Risks**: Port, scheduling, resource, licensing, integration, or migration
  costs.
- **Action**: Propose for approval in design mode, or implement if already
  authorized by task/project policy.
