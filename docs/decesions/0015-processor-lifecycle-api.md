# ADR 0015 — Processor Lifecycle API

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0014 defines the processor lifecycle states and distinguishes:

* orderly stop
* cooperative cancellation
* immediate abort
* operation failure
* lifecycle failure

The lifecycle state machine alone does not define how callers interact with a processor.

The API must establish:

* which lifecycle operations exist
* which states permit each operation
* what repeated operations mean
* how invalid operations are reported
* whether lifecycle operations are requests or completed operations
* how callers observe lifecycle completion
* how concurrent lifecycle requests are resolved

The exact C++ representation is intentionally deferred.

## Decision

EVolution defines the following semantic lifecycle operations:

```text
configure
initialize
activate
stop
cancel
abort
```

Lifecycle operations are explicit API operations and must not be triggered implicitly by unrelated processing operations.

The lifecycle API follows these principles:

1. Lifecycle state transitions are explicit.
2. Invalid lifecycle operations return explicit errors.
3. Repeated compatible termination requests are safe and well-defined.
4. `stop`, `cancel`, and `abort` retain their distinct semantics.
5. Lifecycle requests are distinguished from lifecycle completion.
6. A caller must have an explicit way to observe completion.
7. Concurrent lifecycle requests resolve according to deterministic precedence rules.
8. Lifecycle operations must not depend on hidden global state.
9. Exact synchronization and C++ API representation remain deferred.

## Lifecycle Operations

### `configure`

Moves:

```text
CREATED → CONFIGURED
```

Requirements:

* configuration must be valid
* effective configuration becomes fixed for the processor instance
* initialization has not yet occurred
* normal processing is not permitted

Invalid examples:

```text
CONFIGURED → configure
ACTIVE     → configure
STOPPING   → configure
STOPPED    → configure
FAILED     → configure
```

These return an `InvalidState` error unless a future reconfiguration mechanism explicitly defines otherwise.

Configuration validation failure does not produce `CONFIGURED`.

Conceptually:

```text
configure(config)
    ↓
Result<void>
```

Possible outcomes:

```text
success
Error(InvalidConfiguration)
Error(InvalidState)
```

## `initialize`

Moves:

```text
CONFIGURED → INITIALIZED
```

Initialization may:

* allocate resources
* construct internal execution state
* restore required state
* prepare external dependencies
* validate runtime capabilities
* prepare processor-specific execution structures

Initialization must not begin normal processing.

If initialization fails:

```text
CONFIGURED → FAILED
```

Conceptually:

```text
initialize()
    ↓
Result<void>
```

## `activate`

Moves:

```text
INITIALIZED → ACTIVE
```

Activation means the processor is ready to accept normal processing input.

Activation failure results in:

```text
INITIALIZED → FAILED
```

A processor must never report successful activation while remaining partially active.

Conceptually:

```text
activate()
    ↓
Result<void>
```

## `stop`

`stop` requests orderly shutdown.

From:

```text
ACTIVE → STOPPING
```

The processor:

* stops admitting new normal work
* drains already-admitted work
* performs required finalization
* reaches `STOPPED` when shutdown completes

The request and completion are distinct concepts:

```text
stop request
    ↓
STOPPING
    ↓
shutdown completes
    ↓
STOPPED
```

A successful `stop` operation therefore means that the shutdown procedure has completed successfully.

If shutdown fails:

```text
STOPPING → FAILED
```

and the operation reports the corresponding error.

## `cancel`

`cancel` requests cooperative early termination.

From:

```text
ACTIVE → STOPPING
```

but with cancellation semantics rather than drain semantics.

The processor:

* rejects new normal input
* requests termination of admitted work
* does not guarantee completion of admitted work
* performs required cleanup
* reaches `STOPPED` when cancellation completes

Conceptually:

```text
cancel request
    ↓
STOPPING
    ↓
cancellation completes
    ↓
STOPPED
```

Cancellation itself is not a processor failure.

If cancellation cleanup fails:

```text
STOPPING → FAILED
```

## `abort`

`abort` requests immediate termination.

From an active processor:

```text
ACTIVE → FAILED
```

It does not wait for orderly completion.

It does not guarantee:

* completion of admitted work
* final output
* checkpointing
* flushing
* normal finalization

Basic resource safety remains mandatory.

If abort is requested while orderly shutdown is already occurring:

```text
STOPPING → FAILED
```

This allows external supervision to escalate a shutdown that is taking too long or cannot safely complete.

## Lifecycle Operation Preconditions

The conceptual operation matrix is:

| State         | configure | initialize | activate | stop | cancel |            abort |
| ------------- | --------: | ---------: | -------: | ---: | -----: | ---------------: |
| `CREATED`     |         ✓ |          — |        — |    — |      — | policy-dependent |
| `CONFIGURED`  |         — |          ✓ |        — |    — |      — | policy-dependent |
| `INITIALIZED` |         — |          — |        ✓ |    — |      — | policy-dependent |
| `ACTIVE`      |         — |          — |        — |    ✓ |      ✓ |                ✓ |
| `STOPPING`    |         — |          — |        — |   ✓* |     ✓* |                ✓ |
| `STOPPED`     |         — |          — |        — |   ✓* |     ✓* |                — |
| `FAILED`      |         — |          — |        — |    — |      — |                — |

`✓*` means the operation may be accepted as an idempotent/already-completed request rather than initiating another lifecycle transition.

The exact behavior is defined below.

## Repeated Termination Requests

Termination operations are different from configuration and activation operations.

A repeated termination request must not accidentally restart or mutate the processor.

### Repeated `stop`

If:

```text
ACTIVE → stop → STOPPING
```

then another `stop` request means:

> continue/request orderly shutdown.

It does not create another shutdown operation.

If shutdown has already completed:

```text
STOPPED → stop
```

the operation may be treated as successfully already satisfied.

Conceptually:

```text
stop()
    on ACTIVE   → begin orderly shutdown
    on STOPPING → maintain/request orderly shutdown
    on STOPPED  → success / already stopped
```

### Repeated `cancel`

If:

```text
ACTIVE → cancel → STOPPING
```

another cancellation request reinforces the same cancellation request.

If:

```text
STOPPING → STOPPED
```

a later cancellation request may be treated as already completed.

Conceptually:

```text
cancel()
    on ACTIVE   → begin cancellation
    on STOPPING → maintain/request cancellation
    on STOPPED  → success / already stopped
```

### `abort`

Abort is terminal.

If:

```text
FAILED → abort
```

there is no further lifecycle transition.

A repeated abort may be treated as already terminated, but the API must not imply that a new abort operation occurred.

The exact return-value representation is deferred.

## Stop vs Cancel When Already Stopping

A processor must retain the shutdown mode once shutdown has begun.

For example:

```text
ACTIVE
   │
   │ stop
   ▼
STOPPING [DRAIN]
```

A subsequent:

```text
cancel()
```

does not silently convert the existing operation into an unrelated state transition.

However, cancellation may be used to **escalate an orderly stop into cooperative cancellation**:

```text
STOPPING [DRAIN]
        │
        │ cancel
        ▼
STOPPING [CANCEL]
```

This is an explicit semantic escalation.

The reverse is not allowed:

```text
STOPPING [CANCEL]
        │
        │ stop
        ▼
STOPPING [DRAIN]    ← not allowed
```

Once work has been allowed to terminate early, a later `stop()` cannot retroactively restore the guarantee that all admitted work will complete.

Therefore:

> **Shutdown semantics may become less permissive, but may not become more permissive after termination has begun.**

## Abort During Stop or Cancel

Abort may always escalate an active shutdown:

```text
STOPPING [DRAIN]
       │
       │ abort
       ▼
FAILED
```

or:

```text
STOPPING [CANCEL]
       │
       │ abort
       ▼
FAILED
```

This provides the termination escalation path:

```text
DRAIN
  ↓
CANCEL
  ↓
ABORT
```

## Operation Completion

A lifecycle request and its completion are conceptually distinct.

For example:

```text
stop()
```

initiates:

```text
ACTIVE → STOPPING
```

but shutdown may require additional work before:

```text
STOPPING → STOPPED
```

The lifecycle API therefore requires a semantic distinction between:

```text
request accepted
```

and:

```text
requested lifecycle transition completed
```

A caller must not assume that observing `STOPPING` means the processor is already stopped.

The exact mechanism for waiting or observing completion is deferred.

Possible future mechanisms include:

```text
wait()
future
condition/event
callback
polling state
event stream
```

No mechanism is selected by this ADR.

## Synchronous Lifecycle Operations

Configuration, initialization, and activation are conceptually completion-oriented operations.

For example:

```text
initialize()
    returns success
```

means:

```text
CONFIGURED → INITIALIZED
```

has completed.

For termination operations, the API must distinguish the request from the eventual lifecycle completion.

The exact synchronous/asynchronous API is intentionally deferred because it depends on the later execution and concurrency model.

## Invalid Lifecycle Operations

An operation that is not permitted in the current state returns:

```text
Error(InvalidState)
```

Examples:

```text
initialize() while ACTIVE
activate() while CREATED
configure() while ACTIVE
process() while STOPPING
process() while STOPPED
```

The error must identify enough context for the caller to understand that the operation was invalid.

The error must not silently change lifecycle state.

## Concurrent Lifecycle Requests

Lifecycle requests may race.

For example:

```text
Thread A: stop()
Thread B: cancel()
```

or:

```text
Thread A: stop()
Thread B: abort()
```

The lifecycle model therefore defines semantic precedence.

### Termination Precedence

The ordering is:

```text
STOP
  ↓
CANCEL
  ↓
ABORT
```

where a later stronger request may escalate an existing weaker shutdown.

Therefore:

```text
STOP + CANCEL
    → CANCEL semantics

STOP + ABORT
    → ABORT

CANCEL + ABORT
    → ABORT
```

A weaker request arriving after a stronger request must not weaken the existing lifecycle decision.

This ensures:

```text
cancel → stop
```

does not restore draining semantics.

### Concurrent Equal Requests

Multiple concurrent identical requests are logically equivalent.

For example:

```text
stop()
stop()
stop()
```

represents one logical shutdown request with multiple callers observing/requesting the same transition.

The implementation must avoid creating multiple independent lifecycle transitions.

## Operation Failure During Lifecycle Transition

Lifecycle operations themselves can fail.

For example:

```text
initialize()
    ↓
Error(ResourceExhausted)
```

may cause:

```text
CONFIGURED → FAILED
```

Similarly:

```text
stop()
    ↓
shutdown error
```

may cause:

```text
STOPPING → FAILED
```

The lifecycle operation's returned `Error` describes the operation failure.

The lifecycle state describes the resulting processor condition.

These remain separate:

```text
Result:
    Error(StorageUnavailable)

Lifecycle:
    FAILED
```

## Cancellation During Lifecycle Operations

Cancellation may also be requested while an operation is executing.

For example:

```text
initialize()
    │
    │ cancellation requested
    ▼
Error(Cancelled)
```

The processor must define whether cancellation during a lifecycle operation is supported.

If supported, the processor must still reach a valid lifecycle state.

It must never leave an externally observable ambiguous state such as:

```text
"maybe initialized"
```

Instead, the lifecycle must resolve to a defined state, normally:

```text
FAILED
```

if initialization could not complete and the processor cannot safely return to `CONFIGURED`.

The exact cancellation support of each lifecycle operation is deferred to the processor contract.

## Lifecycle State Observation

A processor has an observable lifecycle state.

Conceptually:

```text
state()
    → LifecycleState
```

The returned state must represent a valid state-machine state.

State observation must not itself trigger lifecycle transitions.

The exact synchronization and memory-ordering requirements are deferred.

## Lifecycle and Processing

Normal processing is permitted only while:

```text
state == ACTIVE
```

Therefore:

```text
CREATED
CONFIGURED
INITIALIZED
STOPPING
STOPPED
FAILED
```

do not accept normal processing.

A processing operation racing with shutdown must have explicitly defined admission semantics.

The key boundary is:

```text
work accepted before shutdown
    → belongs to shutdown policy

work submitted after shutdown begins
    → rejected
```

The exact synchronization mechanism is deferred.

## Lifecycle and Processor State

Processor runtime state is distinct from lifecycle state.

For example:

```text
Lifecycle:
    ACTIVE

Processing State:
    current window
    accumulated measurements
    pending work
```

After:

```text
stop()
```

the lifecycle may become:

```text
STOPPING
```

while processing state still exists until shutdown completes.

After:

```text
STOPPED
```

the processor's runtime state must have reached whatever persistence/disposal condition its contract specifies.

Lifecycle state therefore does not replace processor state.

## Lifecycle and Configuration

Configuration is fixed for the processor instance after configuration succeeds.

Therefore:

```text
ACTIVE → configure()
```

is invalid.

There is no implicit:

```text
ACTIVE → CONFIGURED
```

transition.

A different configuration requires a separate lifecycle instance or a future explicit reconfiguration mechanism.

## Lifecycle and Result/Error

Lifecycle API operations use the same Result/Error model defined by ADR 0006.

Conceptually:

```text
configure() → Result<void>
initialize() → Result<void>
activate()   → Result<void>
stop()       → Result<void>
cancel()     → Result<void>
abort()      → Result<void>
```

This is a semantic model, not yet the final C++ interface.

`Result<void>` communicates operation success or failure.

Lifecycle state communicates the resulting processor condition.

## Lifecycle and Provenance

Lifecycle transitions may be recorded as execution metadata:

```text
configured
initialized
activated
stop_requested
cancel_requested
abort_requested
stopped
failed
```

These records are not automatically domain Events.

They describe processor execution rather than domain activity.

## Exact C++ API Deferred

This ADR does **not** decide:

* exact class/interface definition
* synchronous vs asynchronous implementation
* futures/promises
* callbacks
* condition variables
* atomic state representation
* mutex usage
* event-loop integration
* executor/scheduler
* thread ownership
* process ownership
* memory ordering
* virtual vs non-virtual lifecycle methods

Those decisions belong to the execution model and Processor Interface ADRs.

## Consequences

### Positive

* Lifecycle operations have explicit semantic contracts.
* Repeated termination calls have defined behavior.
* Stop/cancel escalation is explicit.
* Abort remains a distinct immediate-termination mechanism.
* Lifecycle completion is not confused with request acceptance.
* Concurrent termination requests have deterministic semantic precedence.
* Operation errors remain distinct from lifecycle state.

### Negative

* Lifecycle implementations need explicit synchronization.
* Asynchronous shutdown requires a completion-observation mechanism.
* Processors must define cancellation behavior for long-running operations.
* Concurrent lifecycle requests require coordination.

## Decision Summary

```text
Lifecycle operations:
    configure
    initialize
    activate
    stop
    cancel
    abort

Normal lifecycle:
    CREATED
      → CONFIGURED
      → INITIALIZED
      → ACTIVE

Orderly shutdown:
    ACTIVE
      → STOPPING
      → STOPPED

Cancellation:
    ACTIVE
      → STOPPING
      → STOPPED

Abort:
    ACTIVE
      → FAILED

Shutdown escalation:
    STOP
      → CANCEL
      → ABORT

Operation failure:
    Result/Error

Lifecycle state:
    Separate from operation Result

Invalid operation:
    Error(InvalidState)

Repeated stop:
    Idempotent / already requested

Repeated cancel:
    Idempotent / already requested

Repeated abort:
    Terminal / already aborted

Concurrent termination:
    Stronger termination may escalate weaker termination

Configuration:
    Fixed for processor instance

Restart:
    Deferred / external policy

Exact C++ API:
    Deferred
```

## Invariants

1. Lifecycle operations are explicit.
2. Only `ACTIVE` permits normal processing.
3. `configure`, `initialize`, and `activate` establish progressively stronger lifecycle guarantees.
4. `stop` requests orderly draining.
5. `cancel` requests cooperative early termination.
6. `abort` requests immediate termination.
7. `stop` and `cancel` enter `STOPPING`.
8. `abort` may bypass `STOPPING`.
9. A stronger termination request may escalate a weaker one.
10. A weaker termination request cannot reverse a stronger one.
11. Repeated compatible termination requests do not create additional lifecycle transitions.
12. Invalid lifecycle operations return explicit errors.
13. Lifecycle request acceptance and lifecycle completion are distinct concepts.
14. Operation failure and lifecycle failure remain distinct concepts.
15. Cancellation does not inherently mean failure.
16. Configuration cannot be changed implicitly during an active lifecycle.
17. Restart is not implicit.
18. Exact concurrency and C++ API mechanisms remain separate architectural decisions.

## Invariant

> **EVolution lifecycle operations have explicit state-dependent semantics: termination requests may escalate from stop to cancel to abort, lifecycle completion is distinct from request acceptance, and operation results remain separate from the processor's lifecycle state.**
