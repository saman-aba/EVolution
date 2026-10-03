# ADR 0014 — Processing Lifecycle

**Status:** Accepted
**Date:** 2026-10-03

## Context

A processor has to distinguish between normal completion, requested termination, and inability to continue processing.

In particular, EVolution needs clear semantics for:

```text
stop
cancel
abort
operation failure
lifecycle failure
```

Without this distinction, an operation returning an error could incorrectly force the processor into `FAILED`, while cancellation could incorrectly be treated as an operational failure.

The lifecycle must therefore separate:

```text
operation outcome
```

from:

```text
processor lifecycle state
```

## Decision

EVolution adopts the following lifecycle and termination semantics.

### Lifecycle States

A processor has these logical states:

```text
CREATED
CONFIGURED
INITIALIZED
ACTIVE
STOPPING
STOPPED
FAILED
```

Only `ACTIVE` permits normal processing.

The normal lifecycle is:

```text
CREATED
   ↓ configure
CONFIGURED
   ↓ initialize
INITIALIZED
   ↓ activate
ACTIVE
   ↓ stop / cancel
STOPPING
   ↓ finalize
STOPPED
```

Unrecoverable lifecycle failure transitions to:

```text
FAILED
```

`STOPPED` and `FAILED` are terminal states for the processor instance.

### Stop

`stop` means **orderly shutdown**.

It:

* rejects new work
* allows already-admitted work to complete
* performs required finalization
* flushes required output
* checkpoints state where required
* releases resources
* transitions to `STOPPED`

```text
ACTIVE → STOPPING → STOPPED
```

### Cancel

`cancel` means **cooperative early termination**.

It:

* rejects new work
* requests termination of admitted work
* does not guarantee completion of admitted work
* allows the processor to perform required cleanup
* may produce incomplete or partial results
* normally transitions to `STOPPED`

```text
ACTIVE → STOPPING → STOPPED
```

Cancellation is not considered processor failure.

### Abort

`abort` means **immediate termination**.

It:

* rejects new work
* does not wait for cooperative completion
* does not guarantee completion of admitted work
* does not guarantee final output
* does not guarantee checkpointing
* still requires basic resource and object-lifetime safety
* transitions directly to `FAILED`

```text
ACTIVE → FAILED
```

Abort may also terminate an existing orderly shutdown:

```text
STOPPING → FAILED
```

### Operation Outcomes

An individual processing operation can have three semantic outcomes:

```text
SUCCESS
FAILURE
CANCELLED
```

Conceptually:

```text
process(input)
    │
    ├── success
    ├── failure
    └── cancelled
```

`CANCELLED` is represented through the Result/Error model, for example:

```text
Error(Cancelled)
```

This does **not** mean the processor has failed.

### Operation Failure

An operation failure means the operation could not fulfill its contract.

Examples include:

```text
InvalidInput
InvalidState
ProcessingFailure
StorageUnavailable
ResourceExhausted
ExternalFailure
Unsupported
```

An operation failure does not automatically transition the processor to `FAILED`.

For example:

```text
ACTIVE
   │
   │ process(invalid_input)
   ▼
Error(InvalidInput)
   │
   ▼
ACTIVE
```

If the failure makes continued processing unsafe, the processor may instead transition:

```text
ACTIVE → FAILED
```

The processor contract determines this behavior.

### Cancellation vs Failure

The distinction is normative:

```text
Cancellation
    = requested termination of processing

Operation failure
    = inability to fulfill an operation

Lifecycle failure
    = inability to continue the processor lifecycle
```

Therefore:

```text
Error(Cancelled)
    ≠
Error(ProcessingFailure)
```

and:

```text
Error(Cancelled)
    ≠
FAILED
```

Cancellation normally produces:

```text
ACTIVE → STOPPING → STOPPED
```

while an unrecoverable operation failure may produce:

```text
ACTIVE → FAILED
```

### Concurrent Cancellation and Failure

If cancellation and failure occur concurrently, the operation must preserve the condition that actually determined its termination according to its defined observation/commit semantics.

Conceptually:

```text
working
   │
   ├── cancellation observed first
   │       ↓
   │   Cancelled
   │
   └── failure observed first
           ↓
         Failure
```

Cancellation must not be used to hide an already-observed operation failure.

### Error vs Lifecycle State

An operation result and processor lifecycle state are separate.

For example:

```text
Operation:
    Error(Cancelled)

Lifecycle:
    ACTIVE → STOPPING → STOPPED
```

or:

```text
Operation:
    Error(StorageUnavailable)

Lifecycle:
    ACTIVE
```

or:

```text
Operation:
    Error(StateCorruption)

Lifecycle:
    ACTIVE → FAILED
```

The Result/Error model describes the operation outcome.

The lifecycle describes whether the processor can continue.

### Escalation

Termination may escalate when cooperative termination does not complete:

```text
STOP
  ↓
CANCEL
  ↓
ABORT
```

The semantics are:

```text
STOP
    orderly drain

CANCEL
    cooperative early termination

ABORT
    immediate termination
```

This is a termination-semantics hierarchy, not a hierarchy of error severity.

### Partial Results

Cancellation may leave partial output or state.

Such results must not silently be represented as complete results.

If partial results remain valid, their completion status must be explicit.

### Restart and Recovery

There is no implicit restart from:

```text
STOPPED
FAILED
```

Recovery from `FAILED` is external supervision policy.

A future restart mechanism may create a new processor lifecycle instance.

## Lifecycle State Machine

The authoritative state machine is:

```text
                         failure / abort
                              │
                              ▼
                           FAILED
                              ▲
                              │
                              │ abort
                              │
CREATED ──configure──→ CONFIGURED
                          │
                          │ initialize
                          ▼
                     INITIALIZED
                          │
                          │ activate
                          ▼
                        ACTIVE
                       /     \
              stop/cancel     \ abort
                     /         \
                    ▼           ▼
                STOPPING ───→ FAILED
                    │
                    │ finalize
                    ▼
                 STOPPED
```

The key semantic paths are:

```text
ACTIVE → STOPPING → STOPPED
    orderly stop

ACTIVE → STOPPING → STOPPED
    cooperative cancellation

ACTIVE → FAILED
    abort or unrecoverable lifecycle failure

STOPPING → FAILED
    abort or unrecoverable shutdown failure
```

## Consequences

### Positive

* Cancellation no longer implies processor failure.
* Operation failures no longer automatically imply lifecycle failure.
* Stop, cancel, and abort have distinct completion guarantees.
* `Result<Error>` and lifecycle state remain separate concerns.
* Supervisors can escalate termination without changing the meaning of ordinary errors.
* Partial results can be represented without pretending that processing completed normally.

### Negative

* Processor contracts must define which failures are locally recoverable.
* Cancellation semantics require explicit cancellation points.
* Concurrent cancellation/failure requires defined observation semantics.
* Supervisors must distinguish operation errors from lifecycle failures.

These costs are intentional because conflating these concepts makes processing behavior ambiguous and difficult to reproduce.

## Invariants

1. `ACTIVE` is the only normal processing state.
2. `stop` requests orderly draining.
3. `cancel` requests cooperative early termination.
4. `abort` requests immediate termination.
5. `stop` and `cancel` transition to `STOPPING`.
6. `abort` bypasses `STOPPING` when issued from `ACTIVE`.
7. `STOPPING` accepts no new normal input.
8. `STOPPED` is successful terminal state.
9. `FAILED` is unsuccessful terminal state.
10. Cancellation is not processor failure.
11. `Error(Cancelled)` does not imply `FAILED`.
12. Operation failure does not automatically imply `FAILED`.
13. Unrecoverable operation failure may transition the processor to `FAILED`.
14. Abort transitions the processor to `FAILED`.
15. Partial results must not silently appear as complete.
16. Retry remains external policy.
17. Recovery from `FAILED` remains external supervision policy.
18. Lifecycle state and operation outcome remain separate concepts.

## Decision Summary

```text
Lifecycle:
    CREATED
    CONFIGURED
    INITIALIZED
    ACTIVE
    STOPPING
    STOPPED
    FAILED

Normal processing:
    ACTIVE only

STOP:
    Orderly drain
    → STOPPING → STOPPED

CANCEL:
    Cooperative early termination
    → STOPPING → STOPPED

ABORT:
    Immediate termination
    → FAILED

Operation outcomes:
    SUCCESS
    FAILURE
    CANCELLED

Cancellation:
    Not processor failure
    May return Error(Cancelled)

Operation failure:
    May remain ACTIVE
    May cause FAILED if unrecoverable

Lifecycle failure:
    Prevents continued lifecycle
    → FAILED

Restart:
    Deferred / external policy
```

## Invariant

> **EVolution separates operation outcomes from processor lifecycle state: cancellation is an intentional termination outcome, operation failure is an inability to fulfill an operation, and only an unrecoverable condition that prevents continued processing transitions the processor to `FAILED`.**
