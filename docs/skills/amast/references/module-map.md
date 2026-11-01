# AMAST module map

This is a navigation and decision aid, **not a substitute for the versioned
public API**. Check upstream sources and the installed release before generating
calls.

- **`fsm` — Finite state machine:** A small number of explicit modes and
  event-triggered transitions.
- **`hsm` — Hierarchical state machine:** Sub-machine support; nested states,
  shared event handling, and entry/exit logic.
- **`ao` — Active objects:** Cooperative or preemptive scheduling of
  multiple event-driven components with independent queues.
- **`event` — Event representation and utilities:** Message passing between
  stateful components.
- **`timer` — Timed events:** State timeouts, retries, and periodic activity.
- **`async` — Async/await-style patterns in C:** Long-running asynchronous
  flows that fit AMAST's API.
- **`ringbuf` — Ring buffer:** Bounded streaming or producer-consumer buffers.
- **`onesize` — Fixed-size memory allocator:** Predictable allocations of
  identically sized objects.
- **`slist`, `dlist` — Singly/doubly linked lists:** Small list primitives
  with clear ownership semantics.

## FSM / HSM usage clues from upstream README

- User events start at `AM_EVT_USER`; `AM_EVT_ENTRY`, `AM_EVT_EXIT` (and HSM
  `AM_EVT_INIT`) have state-machine significance.
- FSM examples use `struct am_fsm`, `am_fsm_init`, `am_fsm_start`,
  `am_fsm_dispatch`, and `am_fsm_tran`.
- HSM examples use `struct am_hsm`, `am_hsm_state_make`, `am_hsm_init`,
  `am_hsm_super`, `am_hsm_tran`, `am_hsm_start`, and `am_hsm_dispatch`.
- AO examples show `am_ao_init`, `am_ao_start`, event publication, timers, and
  task creation.
- Exact signatures and event semantics may change. Copy code from **the pinned
  release's** examples instead of inferring from this summary.

## Architecture guardrails

1. Define events, state ownership, and allowed transitions before writing
   callback code.
2. Decide whether to use a synchronous FSM/HSM or queued active objects; do not
   add AO merely because FSM is needed.
3. For asynchronous paths, identify which code executes in event handlers, task
   context, timer callbacks, and ISRs; verify allowed API calls in each.
4. Dimension event queues and memory pools based on burst conditions and
   recovery behavior, not merely a happy-path demonstration.
5. Account for startup order, subscriptions, cleanup, failed initialization, and
   timer cancellation.
6. Include tests for transition edges, ignored/deferred events when relevant,
   entry/exit effects, timer expiry, and queue overflow behavior.

## Authoritative references

- Overview and self-contained examples:
  https://github.com/adel-mamin/amast#what-is-it-useful-for
- Public API: https://amast.readthedocs.io/api.html
- HSM guidance and examples:
  https://github.com/adel-mamin/amast/tree/main/libs/hsm
- AO guidance: https://github.com/adel-mamin/amast/blob/main/libs/ao/README.rst
- Example applications:
  https://github.com/adel-mamin/amast/tree/main/apps/examples
