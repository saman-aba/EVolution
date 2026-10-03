# ADR 0023 — Processing Queue and Buffer Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processing separates:

```text
Submission
    ↓
Admission
    ↓
Scheduling
    ↓
Execution
```

Processors and graph connections may require temporary storage for work that has been:

* submitted but not yet admitted
* admitted but waiting for dependencies
* eligible but waiting for execution
* waiting for downstream capacity
* waiting for correlated inputs
* waiting for an ordering condition

These mechanisms are commonly implemented with queues or buffers.

However, a queue is not itself the semantic processing model.

The architecture therefore needs to define what queues and buffers mean, what guarantees they provide, and how they interact with admission, scheduling, lifecycle, ordering, cancellation, and failure.

## Decision

EVolution treats queues and buffers as **bounded execution infrastructure** used to temporarily retain processing work or intermediate data.

A queue/buffer:

* does not define processor semantics
* does not define graph topology
* does not define analytical meaning
* does not automatically define ordering semantics
* does not imply unbounded capacity

Its behavior is governed by the surrounding processor, connection, scheduler, and admission contracts.

Conceptually:

```text
Producer
   ↓
Admission Boundary
   ↓
Queue / Buffer
   ↓
Scheduler
   ↓
Execution Backend
```

## Queue vs Buffer

The terms are related but conceptually distinct.

A **queue** primarily represents pending work with an ordering/removal discipline.

A **buffer** primarily represents retained data required to satisfy some processing condition.

For example:

```text
Queue:
    pending processor operations

Buffer:
    events waiting for correlation
```

The same physical data structure may implement either role, but the semantic contract must remain explicit.

## Queue Responsibilities

A processing queue may provide:

```text
ordering
bounded capacity
work retention
admission state
cancellation/removal
shutdown behavior
```

It must not implicitly provide:

```text
domain interpretation
graph topology
processor semantics
analytical aggregation
```

## Buffer Responsibilities

A processing buffer may retain information because processing cannot yet continue.

Examples:

```text
waiting for second input
waiting for event-time boundary
waiting for batch completion
waiting for downstream capacity
```

The buffer's retention condition must be explicit.

## Boundedness

Queues and buffers must be bounded by an explicit resource constraint unless an exceptional component contract explicitly permits otherwise.

Capacity may be measured in:

```text
items
bytes
memory
weighted work units
```

For example:

```text
capacity = 10,000 items
```

does not necessarily mean:

```text
10,000 × identical memory cost
```

A byte or weighted-work limit may be more appropriate.

Unbounded queues are not the default because they can convert overload into uncontrolled memory growth.

## Queue Capacity

Capacity is an admission concern.

For:

```text
Producer → Queue → Processor
```

the producer must not assume that the queue always has capacity.

When capacity is exhausted, the configured admission policy applies:

```text
BLOCK
BUFFER
REJECT
DROP
SAMPLE
DEGRADE
SPILL
```

as defined by ADR 0018.

The queue itself must not silently choose a different semantic policy.

## Queue Occupancy

Queue occupancy is execution state.

Conceptually:

```text
Queue
{
    capacity
    current_usage
}
```

The exact representation is implementation-defined.

Occupancy may be measured in:

```text
items
bytes
weighted units
```

depending on the capacity model.

## Ordering

A queue may preserve an ordering discipline, but ordering must be explicitly declared.

Possible semantics include:

```text
FIFO
priority
sequence order
partition order
unordered
```

A FIFO queue does not automatically mean that the overall processor receives domain-order data.

For example:

```text
arrival order:
    E1 E3 E2

FIFO:
    E1 E3 E2
```

does not provide:

```text
event sequence:
    E1 E2 E3
```

unless arrival order and domain order are explicitly equivalent.

## Queue Ordering vs Domain Ordering

The architecture distinguishes:

```text
queue order
processing order
execution order
completion order
domain sequence
event time
```

These must never be conflated.

A queue implementation must not silently claim stronger ordering than it actually provides.

## Priority Queues

A priority queue may be used where the scheduling contract permits priority-based execution.

Priority ordering must not override a processor's required semantic ordering.

For example, if:

```text
Processor requires sequence order
```

then:

```text
priority = execution urgency
```

cannot silently reorder sequence-sensitive work.

## Partitioned Queues

A processor using partitioned concurrency may maintain independent queues:

```text
Partition A → Queue A
Partition B → Queue B
Partition C → Queue C
```

This can enable concurrency while preserving per-partition ordering.

Partition identity must come from the processor/graph contract.

It must not depend on:

```text
thread ID
memory address
hash table iteration order
current worker
```

unless those mechanisms are explicitly part of the contract.

## Multi-Input Buffers

A multi-input processor may need to retain partial input.

Example:

```text
A(i) arrives
B(i) not yet arrived
```

The processor may retain:

```text
A(i)
```

until:

```text
B(i)
```

arrives.

This buffer must define:

```text
correlation key
retention policy
capacity
expiration
missing-input behavior
cancellation
shutdown
```

The buffer must not silently discard partial state.

## Buffer Expiration

Buffered information may require an expiration policy.

Possible conditions include:

```text
maximum age
maximum event-time distance
maximum item count
maximum memory
explicit end-of-input
```

Expiration is not automatically equivalent to processing failure.

The processor contract must define whether expiration results in:

```text
discard
partial result
error
incomplete result
timeout
cancellation
```

## Temporal Buffers

Processors dealing with event time may buffer out-of-order observations.

For example:

```text
10:00
10:02
10:01
```

A processor may temporarily retain `10:02` while waiting for potentially earlier events.

The architecture does not define a universal watermark or lateness mechanism.

If temporal reordering is required, the processor contract must define:

```text
ordering criterion
buffering condition
lateness tolerance
expiration
behavior after expiration
```

## Spill-to-Storage

A buffer may use persistent storage when in-memory capacity is insufficient.

Conceptually:

```text
Memory Buffer
     ↓ capacity reached
Persistent Spill
     ↓
Later recovery
```

This is the `SPILL` admission behavior defined by ADR 0018.

Spilled work remains logically admitted.

Therefore the system must preserve:

```text
identity
ordering requirements
provenance
recovery semantics
failure semantics
```

The exact storage technology is deferred.

## Queue Ownership

A queue must have explicit ownership semantics.

Possible ownership models include:

```text
single owner
shared ownership
producer-owned
consumer-owned
reference-counted
move-based
```

The architecture does not select a C++ implementation yet.

The semantic requirement is that a queued item must remain valid for as long as the queue may deliver it.

## Queue Item Lifetime

A queued item must not contain dangling references to temporary producer data.

For example, this is not semantically valid:

```text
producer creates temporary object
        ↓
queue stores non-owning reference
        ↓
producer object destroyed
        ↓
consumer reads invalid data
```

Queue storage must therefore satisfy the ownership contract of the item.

This follows the general value/lifetime principles established for `Error` and processing envelopes.

## Envelope Interaction

Processor queues normally carry processing envelopes rather than arbitrary hidden metadata.

Conceptually:

```text
Queue<Envelope<T>>
```

where the envelope may contain:

```text
payload
identity
processing metadata
correlation
temporal information
provenance references
```

The queue must not add a second semantic identity or timestamp without an explicit reason.

## Queue and Provenance

Queue operations normally do not create new domain provenance.

However, if queue behavior materially affects processing, execution provenance may need to record:

```text
admission policy
ordering policy
buffering policy
spill behavior
relevant execution configuration
```

For example, if:

```text
DROP
```

is enabled, the resulting analytical dataset differs from a lossless execution.

That configuration therefore participates in reproducibility.

## Queue and Backpressure

Queues are one possible implementation mechanism for backpressure.

They do not eliminate backpressure.

For example:

```text
Producer
   ↓
bounded queue
   ↓
slow processor
```

eventually reaches:

```text
queue full
```

At that point the configured admission policy must determine what happens.

A queue must never silently become an unbounded escape from backpressure.

## Blocking Queues

A blocking queue may implement:

```text
BLOCK
```

admission.

The producer waits while capacity is unavailable.

Blocking must be:

* bounded by the queue contract
* cancellable where required
* released during shutdown
* compatible with lifecycle semantics

A blocked producer must not remain indefinitely blocked after the system has entered a terminal state.

## Non-Blocking Queues

A queue may support non-blocking operations:

```text
try_push
try_pop
```

or equivalent semantics.

A failed non-blocking operation must be distinguishable from:

```text
successful admission
successful processing
```

For example:

```text
queue full
```

is not:

```text
processor rejected input
```

and neither is:

```text
processor successfully processed input
```

## Queue Rejection

If a queue cannot accept an item and the admission policy is `REJECT`, the caller receives an explicit operational result such as:

```text
Error(ResourceExhausted)
```

The item is not admitted.

This is distinct from a processor operation that was admitted and later failed.

## Queue Drop

If the graph explicitly permits `DROP`:

```text
queue full
    ↓
item discarded
```

the item must not be reported as successfully processed.

Where relevant, the system should retain enough operational information to determine that loss occurred.

Drop is intentional data loss, not an error-free success.

## Queue Sampling

Sampling may deliberately retain only part of an input stream.

For example:

```text
1000 submitted
    ↓
sample policy
    ↓
100 processed
```

Sampling changes the effective analytical dataset.

Therefore sampling configuration must participate in reproducibility.

## Queue Degradation

A processor may support degraded operation under capacity pressure.

For example:

```text
full-resolution processing
        ↓
capacity pressure
        ↓
reduced-resolution processing
```

Degradation must be explicitly supported by the processor contract.

A queue must not silently change processing resolution.

## Queue Shutdown

Queue behavior changes according to lifecycle state.

### ACTIVE

Normal admission and consumption are permitted.

### STOPPING / STOP

New normal work is rejected.

Already-admitted work may drain.

### STOPPING / CANCEL

New work is rejected.

Queued work may be discarded according to cancellation semantics.

### ABORT / FAILED

No normal completion guarantee exists.

Queued work may remain incomplete.

## Drain Semantics

Orderly shutdown with `STOP` normally means:

```text
stop admission
    ↓
process admitted queue contents
    ↓
queue becomes drained
    ↓
shutdown
```

Drain order follows the queue/processor ordering contract.

The system must not silently reorder queued work merely because shutdown has started.

## Cancellation Semantics

During cancellation:

```text
queued work
```

may be cancelled before execution.

The resulting disposition must be explicit:

```text
Cancelled
```

rather than:

```text
Failed
```

unless another failure occurred.

## Queue Flush

"Flush" must not be interpreted universally as:

```text
execute everything immediately
```

It means that buffered information reaches the next contractually defined stage.

For persistent buffers, flush may mean:

```text
persist durable state
```

For processing queues, flush may mean:

```text
drain admitted work
```

The operation must therefore be defined by the owning component.

## Queue Failure

Queue infrastructure can fail.

Examples:

```text
allocation failure
storage failure
corruption
synchronization failure
backend unavailable
```

Such failures are infrastructure failures.

They must not be represented as successful admission.

The relevant component determines whether the failure is:

```text
recoverable
retryable
processor-fatal
graph-fatal
```

according to its contract.

## Queue Fairness

When multiple producers share a queue, fairness may matter.

Possible policies include:

```text
FIFO across all producers
per-producer fairness
weighted fairness
priority
```

No universal fairness policy is selected.

Fairness must not violate ordering or correctness guarantees.

## Queue Starvation

A queue may contain work indefinitely if scheduling continually favors other work.

If starvation freedom is required, the scheduler/execution contract must provide the necessary guarantee.

The queue itself does not guarantee processor fairness unless explicitly specified.

## Queue Metrics

Operational queue information may include:

```text
occupancy
capacity
enqueue count
dequeue count
rejection count
drop count
wait duration
age of oldest item
```

These are normally observability/telemetry data.

They are not automatically EVolution analytical Measurements.

If exposed as analytical Measurements, they must explicitly enter the measurement model.

## Queue Persistence

An in-memory queue is normally execution state.

A persistent queue may become recoverable processing state.

The storage model therefore distinguishes:

```text
ephemeral queue
recoverable queue
authoritative historical data
```

A queue becoming persistent does not automatically make it the authoritative source of historical domain data.

## Queue Recovery

If a persistent queue supports recovery, it must define:

```text
what was admitted
what was completed
what was cancelled
what remains pending
```

Recovery must avoid silently duplicating or losing work unless the execution contract explicitly permits those semantics.

Exactly-once processing is not assumed.

## Duplicate Execution

A recovered or retried work item may potentially execute more than once.

Therefore processors that require stronger guarantees must explicitly define:

```text
idempotency
deduplication
transaction semantics
```

The queue model does not automatically provide exactly-once execution.

## Queue and Idempotency

A queue may provide at-least-once or at-most-once style delivery semantics depending on implementation and contract.

These semantics must be explicit.

For example:

```text
At-least-once:
    item may be delivered more than once

At-most-once:
    item is not redelivered after accepted delivery
```

No universal delivery guarantee is selected.

## Queue and Transaction Boundaries

Queue admission and processor completion are distinct events.

For example:

```text
admitted
   ↓
queued
   ↓
executed
   ↓
completed
```

Successful queue insertion must not imply successful processing.

If atomicity between storage and processing is required, that must be defined by a separate transaction/recovery design.

## Consequences

### Positive

* Queue semantics are explicit rather than hidden inside data structures.
* Resource exhaustion cannot silently become unbounded memory growth.
* Ordering semantics remain separate from FIFO implementation.
* Multi-input buffering has explicit lifecycle and expiration semantics.
* Persistent spill/recovery can be introduced without changing processor semantics.
* Lossy behavior remains visible and reproducible.

### Negative

* Queue behavior becomes a substantial part of execution contracts.
* Recovery and duplicate execution require additional design.
* Multi-input and event-time buffering can require significant memory.
* Different admission policies can produce materially different analytical results.

## Deferred Decisions

This ADR does not select:

```text id="t6p5nq"
std::queue
ring buffer
lock-free queue
mutex-protected queue
MPSC
MPMC
work-stealing queue
persistent queue technology
serialization format
memory allocator
exact delivery guarantee for all processors
distributed queue
```

## Decision Summary

```text
Queue/Buffer:
    Bounded execution infrastructure

Queue:
    Pending-work representation

Buffer:
    Temporarily retained processing data

Default:
    Explicitly bounded

Capacity:
    Items / bytes / weighted work

Ordering:
    Explicit

Admission:
    Governed by graph/connection policy

Ownership:
    Explicit

Blocking:
    Must be cancellable where applicable

Drop/Sample:
    Explicitly lossy

Spill:
    Persistent but still logically admitted

STOP:
    Drain admitted work

CANCEL:
    May discard/cancel queued work

ABORT:
    No completion guarantee

Recovery:
    Explicit semantics

Exactly-once:
    Not assumed

Duplicate execution:
    Possible unless contract prevents it
```

## Invariants

1. Queues and buffers are execution infrastructure, not domain semantics.
2. Queue capacity must be explicitly bounded unless a component contract explicitly permits otherwise.
3. Capacity exhaustion must produce a defined admission outcome.
4. Queue ordering must not be confused with domain ordering.
5. FIFO does not automatically imply event-time or sequence ordering.
6. Queue items must satisfy explicit ownership and lifetime requirements.
7. Queues must not retain dangling references to temporary producer data.
8. Multi-input buffers must define correlation and retention semantics.
9. Buffer expiration must have an explicit disposition.
10. Spill-to-storage does not automatically make spilled data the authoritative domain source.
11. Queue admission success does not imply processing success.
12. Dropped or sampled work must not be reported as successfully processed.
13. Degradation must be explicitly supported by the processor contract.
14. Blocking admission must be releasable during shutdown or cancellation.
15. Orderly stop drains admitted work according to the relevant ordering contract.
16. Cancellation may terminate queued work without treating cancellation as ordinary failure.
17. Persistent queues must define recovery semantics.
18. Exactly-once execution is not assumed.
19. Retry/recovery may produce duplicate execution unless the contract prevents it.
20. Queue implementation must not redefine processor or graph semantics.

## Invariant

> **EVolution queues and buffers provide explicitly bounded temporary retention of processing work or intermediate data; their capacity, ordering, ownership, loss, persistence, recovery, and shutdown behavior must be defined by explicit contracts rather than being accidental properties of the underlying data structure.**
