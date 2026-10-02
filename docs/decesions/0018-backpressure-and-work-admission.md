# ADR 0018 — Backpressure and Work Admission

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processors may operate concurrently, stream continuously, or receive work from multiple producers.

Therefore the system can reach a condition where:

```text
input rate > processing rate
```

Without explicit semantics, queues can grow indefinitely, memory can be exhausted, or data can be silently lost.

Backpressure must therefore be treated as an architectural concern rather than an incidental property of a queue implementation.

This decision also needs to preserve the distinction between:

```text
submission
admission
processing
completion
```

defined by the previous execution decisions.

## Decision

EVolution treats **backpressure as an explicit work-admission policy**.

A processor or processing graph must define what happens when available processing capacity is insufficient.

The system must never silently convert resource exhaustion or capacity limits into successful processing.

The conceptual flow is:

```text
Producer
   ↓
Submission
   ↓
Admission decision
   ├── Accepted
   ├── Rejected
   ├── Delayed
   └── Dropped
   ↓
Processing
   ↓
Completion
```

The architecture supports the following admission behaviors:

```text
BLOCK
BUFFER
REJECT
DROP
SAMPLE
DEGRADE
SPILL
```

No single policy is imposed globally.

The selected policy must be explicit for the relevant processing boundary.

## Submission vs Admission

Submission means:

> A producer has offered work to the processing system.

Admission means:

> The processing system has accepted responsibility for that work.

Therefore:

```text
submitted ≠ admitted
```

For example:

```text
Producer
   │
   │ submit
   ▼
Admission Boundary
   │
   ├── accepted → admitted
   └── rejected → not admitted
```

A rejected item must not be represented as successfully processed.

## Admission Contract

An admission boundary should define:

```text
capacity
policy
ordering
overflow behavior
failure behavior
cancellation behavior
```

Conceptually:

```text
Admission Policy
{
    capacity
    behavior
    ordering
    overflow
}
```

The exact C++ representation remains deferred.

## BLOCK

Under `BLOCK`, the producer waits until capacity becomes available.

```text
Producer
   │
   ▼
Admission
   │
   │ capacity unavailable
   ▼
wait
   │
   │ capacity available
   ▼
accepted
```

This provides backpressure to the producer.

Blocking must have explicit cancellation and shutdown semantics.

A blocked producer must not become permanently stuck because the processor entered:

```text
STOPPING
STOPPED
FAILED
```

## BUFFER

Under `BUFFER`, submitted work waits in an intermediate queue.

```text
Producer
   ↓
Buffer
   ↓
Processor
```

Buffering can absorb temporary differences between production and processing rates.

However:

> **Buffering does not eliminate overload; it only delays it.**

Therefore buffers must have defined capacity or resource limits.

Unbounded buffering is not the default architecture.

## REJECT

Under `REJECT`, work that cannot currently be admitted is explicitly rejected.

Conceptually:

```text
submit(input)
    ↓
capacity available?
    ├── yes → admitted
    └── no  → Error(ResourceExhausted)
```

This is appropriate where the producer can retry, defer, or otherwise handle rejection.

Rejected work was never admitted and therefore must not be treated as partially processed.

## DROP

Under `DROP`, work that cannot be admitted is intentionally discarded.

This is fundamentally different from rejection.

```text
REJECT:
    work remains the producer's responsibility

DROP:
    processing system intentionally discards work
```

Dropping is permitted only when the processor contract explicitly allows loss.

Dropped work must not be represented as successful processing.

Where observability matters, the system should expose information about the drop, such as:

```text
drop count
drop reason
affected scope
time range
```

These are telemetry/operational concerns rather than domain events by default.

## SAMPLE

Sampling intentionally admits only a subset of offered work.

For example:

```text
1000 inputs
    ↓ sampling policy
100 inputs
    ↓
processing
```

Sampling changes the effective dataset.

Therefore sampling must be explicit and its configuration must participate in reproducibility when it affects analytical results.

A sampled result must not silently be represented as if all input had been processed.

## DEGRADE

A processor may support a degraded execution mode when capacity is insufficient.

For example:

```text
normal processing
      ↓ overload
reduced resolution
      ↓
degraded processing
```

Degradation is domain/processor-specific.

The system must not silently degrade a processor unless the processor contract explicitly permits it.

A degraded result must preserve sufficient metadata to distinguish it from a normal result when the distinction affects interpretation.

## SPILL

A processing system may temporarily move queued work to persistent storage:

```text
Producer
   ↓
Memory
   ↓ capacity exceeded
Persistent Buffer
   ↓
Processor
```

Spilling can extend effective buffering capacity but introduces:

* storage latency
* storage failures
* persistence requirements
* recovery semantics
* ordering requirements

Spilled work remains admitted work.

Therefore it remains the processing system's responsibility until its final disposition is known.

## Capacity

Capacity may be constrained by:

```text
queue size
memory
CPU
worker count
external resource limits
storage
processor-specific limits
```

Capacity does not necessarily mean number of queued items.

For example:

```text
1 large input
```

may consume substantially more resources than:

```text
100 small inputs
```

A processor may therefore use resource-based admission rather than simple item counts.

## Queue Capacity

Queues should have explicit capacity semantics.

Conceptually:

```text
Queue
{
    capacity
    current_usage
    admission_policy
}
```

Capacity may be expressed in:

```text
items
bytes
memory
weighted work units
```

The architecture does not require a single universal capacity unit.

## Memory Safety

Backpressure must protect against uncontrolled memory growth.

The following is not an acceptable implicit behavior:

```text
producer
   ↓
unbounded queue
   ↓
memory exhaustion
```

Resource exhaustion should instead result in a defined admission outcome such as:

```text
Error(ResourceExhausted)
```

or an explicitly configured loss/degradation policy.

## Ordering

Backpressure must not silently change ordering semantics.

For an order-sensitive processor:

```text
A B C D
```

cannot become:

```text
A C D B
```

merely because the system experienced overload, unless the processor contract explicitly permits reordering.

Different admission policies may have different ordering consequences and must document them.

## Fairness

When multiple producers compete for limited capacity, the admission system may need fairness semantics.

Possible policies include:

```text
FIFO
producer fairness
partition fairness
priority
weighted fairness
```

No universal fairness policy is selected by this ADR.

If fairness affects correctness or starvation, it must be part of the execution contract.

## Priority

Some processing graphs may assign priority to work.

Priority may affect:

```text
admission
queue ordering
resource allocation
```

Priority must not silently override correctness requirements such as required domain ordering.

Priority semantics are optional and remain an execution concern.

## Backpressure and Lifecycle

Backpressure must respect lifecycle state.

While:

```text
ACTIVE
```

work may be admitted according to capacity and policy.

After:

```text
ACTIVE → STOPPING
```

new normal work is rejected.

Already-admitted work remains subject to the active shutdown policy.

For orderly stop:

```text
STOPPING [DRAIN]
    ↓
admitted work drains
```

For cancellation:

```text
STOPPING [CANCEL]
    ↓
admitted work may terminate
```

For abort:

```text
ABORT
    ↓
no completion guarantee
```

## Blocked Producers During Shutdown

A producer waiting under `BLOCK` must be notified when admission becomes impossible.

For example:

```text
Producer
   ↓
waiting for capacity
   ↓
processor enters STOPPING
   ↓
wait terminates
   ↓
Error(Cancelled / InvalidState)
```

The exact error depends on the operation's semantics.

The important rule is:

> **Shutdown must not leave blocked producers waiting indefinitely for capacity that will never become available.**

## Backpressure and Cancellation

A producer waiting for admission must be cancellable where the surrounding execution model supports cancellation.

For example:

```text
submit()
   ↓
waiting for capacity
   ↓
cancellation requested
   ↓
Error(Cancelled)
```

This does not mean that the processor failed.

The distinction established in ADR 0014 remains applicable.

## Backpressure and Failure

Admission failure and processing failure are separate.

For example:

```text
input cannot be admitted
    → Error(ResourceExhausted)
```

is different from:

```text
input admitted
    ↓
processor executes
    ↓
Error(ProcessingFailure)
```

The first concerns capacity/admission.

The second concerns processing.

## Partial Failure

For batches or multiple concurrent submissions, some work may be admitted while other work is rejected.

For example:

```text
Batch:
    A → admitted
    B → admitted
    C → rejected
    D → admitted
```

The result must represent this explicitly.

The system must not report the entire batch as successful.

Partial processing semantics belong to the relevant batch/graph contract.

## Loss Policy

The architecture distinguishes:

```text
LOSSLESS
LOSSY
```

processing boundaries.

### LOSSLESS

Every admitted input must eventually have a defined disposition.

Possible dispositions include:

```text
success
failure
cancellation
```

but silent loss is not permitted.

### LOSSY

The contract explicitly allows some inputs to be:

```text
dropped
sampled
expired
```

Lossy processing must expose enough information for consumers to understand that the complete input population was not processed.

## Lossless Does Not Mean Infinite

A lossless boundary does not imply infinite buffering.

If a lossless processor cannot safely accept more work, it must apply an explicit mechanism such as:

```text
block
reject
spill
slow producer
```

rather than silently dropping work.

## Backpressure in Processing Graphs

Consider:

```text
A → B → C
```

If `C` becomes slower:

```text
C capacity ↓
```

then `B` may accumulate work.

Eventually the pressure propagates:

```text
C
↑
B
↑
A
↑
source
```

The graph execution system must define whether pressure propagates upstream, is absorbed by buffering, or results in loss/rejection.

Backpressure therefore belongs to graph execution as well as individual processors.

## Graph-Level Policy

A processing graph may define different policies at different boundaries.

For example:

```text
Input
   ↓
Parser
   ↓ BLOCK
Normalizer
   ↓ BUFFER
Analyzer
   ↓ REJECT
Storage
```

The architecture does not require one policy for the entire graph.

However, incompatible policies must be explicitly resolved.

## Backpressure and Analytical Correctness

Backpressure policy can change analytical results.

For example:

```text
DROP
```

may produce a different measurement than:

```text
BLOCK
```

Therefore any policy that can change the input population must be treated as a meaningful execution parameter.

If it materially affects results, it must participate in configuration/provenance.

## Time-Based Expiration

Queued work may have an expiration condition.

For example:

```text
input admitted
    ↓
waits too long
    ↓
expires
```

Expiration is not automatically equivalent to cancellation or failure.

If supported, it must have explicit semantics such as:

```text
Error(Cancelled)
Error(Expired)
Dropped
```

The exact representation is deferred until the execution/error model is extended if necessary.

## Backpressure and Replay

Replay should not silently use different admission semantics from live processing if reproducibility requires equivalent results.

For example:

```text
LIVE:
    DROP under overload

REPLAY:
    process everything
```

can produce different analytical outputs.

The difference must therefore be explicit in execution context/configuration when relevant.

## Backpressure and Determinism

A deterministic processor must not become nondeterministic merely because capacity is limited.

If a lossy or scheduling-dependent policy affects results, that policy is part of the effective execution configuration.

For reproducible analysis:

```text
Input
+
Configuration
+
Processor Version
+
Relevant Execution Context
+
Admission/Backpressure Policy
```

must determine the result where that policy materially affects processing.

## Consequences

### Positive

* Memory exhaustion from unbounded queues is avoided as an implicit behavior.
* Loss is explicit rather than accidental.
* Submission and admission have clear semantics.
* Shutdown interacts correctly with queued and blocked work.
* Processing graphs can propagate or absorb backpressure intentionally.
* Analytical reproducibility can account for capacity-related behavior.

### Negative

* Every processing boundary may require an explicit capacity/loss policy.
* Lossy systems require additional metadata and observability.
* Graph-level backpressure can become complex.
* Blocking can propagate latency upstream.
* Persistent spilling introduces storage and recovery complexity.

## Deferred Decisions

This ADR does not select:

```text
queue implementation
ring buffer
mutex-protected queue
lock-free queue
channel
executor
thread pool
network transport
persistent queue technology
```

It also does not define a universal:

```text
capacity value
fairness algorithm
priority algorithm
retry mechanism
```

These belong to later implementation or execution decisions.

## Decision Summary

```text
Backpressure:
    Explicit work-admission policy

Submission:
    Producer offers work

Admission:
    Processor accepts responsibility

Normal admission:
    ACTIVE only

Supported policies:
    BLOCK
    BUFFER
    REJECT
    DROP
    SAMPLE
    DEGRADE
    SPILL

Default:
    No universal global policy

Queues:
    Must have defined capacity semantics

Unbounded buffering:
    Not the default

Loss:
    Must be explicit

Lossless:
    No silent loss

Shutdown:
    No new normal admission after STOPPING

Blocked producers:
    Must not remain indefinitely blocked during shutdown

Partial admission:
    Must be explicitly represented

Analytical impact:
    Material admission policies participate in reproducibility

Implementation:
    Deferred
```

## Invariants

1. Submission and admission are distinct.
2. A processor assumes responsibility for work only after admission.
3. Only an `ACTIVE` processor admits normal work.
4. Backpressure behavior must be explicitly defined.
5. Unbounded buffering is not an implicit default.
6. Capacity exhaustion must not silently become successful processing.
7. Rejected work is not admitted.
8. Dropped work is not successful processing.
9. Lossy behavior must be explicitly permitted.
10. Lossless processing must not silently discard admitted work.
11. Blocking producers must be released when shutdown makes admission impossible.
12. Cancellation of blocked admission must be distinguishable from processor failure.
13. Backpressure must respect ordering requirements.
14. Backpressure policy may be different at different graph boundaries.
15. Material backpressure behavior is part of reproducibility.
16. Already-admitted work remains subject to lifecycle shutdown semantics.
17. New normal work is not admitted after `STOPPING`.
18. Partial admission must not be represented as complete batch success.
19. Resource exhaustion must have an explicit disposition.
20. Backpressure is an execution concern and must not redefine domain semantics.

## Invariant

> **EVolution treats backpressure as an explicit work-admission policy: capacity limits must produce a defined outcome rather than silent loss or unbounded resource growth, and any admission behavior that materially affects processing results must be explicit and reproducible.**
