# ADR 0024 — Processing Delivery Semantics

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processing separates:

```text
Admission
    ↓
Queue / Buffer
    ↓
Scheduling
    ↓
Execution
    ↓
Completion
```

Previous decisions intentionally do not assume exactly-once execution.

A processing system may experience:

* retries
* crashes
* cancellation
* queue recovery
* worker failure
* partial completion
* duplicate delivery
* lost work
* persistent spill/recovery

Therefore the architecture must define what guarantees a processor or connection provides about whether an admitted item is delivered and executed.

## Decision

EVolution explicitly distinguishes **delivery semantics** from **processing semantics**.

A connection or processing boundary may declare one of the following conceptual guarantees:

```text
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

These are contracts, not properties that EVolution assumes universally.

The default architecture does **not** promise exactly-once processing.

Where no stronger guarantee is explicitly declared, callers must not assume exactly-once execution.

## Delivery vs Execution

Delivery means:

> The processing system makes an input available to the destination processor according to the connection contract.

Execution means:

> The processor actually performs its processing operation.

These are distinct.

Conceptually:

```text
Input
  ↓
Delivered
  ↓
Executed
  ↓
Completed
```

A delivered item may fail during execution.

Therefore:

```text
delivery success
```

does not imply:

```text
processing success
```

## Completion

A processor operation is considered completed only when its operation contract reaches one of:

```text
SUCCESS
FAILURE
CANCELLED
```

This follows the processor execution model.

A queue removing an item does not by itself prove successful processing.

## At-Most-Once

Under `AT_MOST_ONCE` semantics:

```text
an item is delivered/executed no more than once
```

The system may lose an item before or during execution.

Conceptually:

```text
Input
  ↓
attempt delivery
  ↓
failure
  ↓
no retry
```

This may be appropriate where duplicate execution is more harmful than loss.

The architecture does not prescribe where this semantic is appropriate.

## At-Least-Once

Under `AT_LEAST_ONCE` semantics:

```text
an admitted item is retried until successful delivery/completion
```

depending on the precise connection contract.

This can produce duplicate execution.

For example:

```text
Input X
   ↓
execute X
   ↓
worker fails before completion is recorded
   ↓
retry X
   ↓
execute X again
```

Therefore processors consuming at-least-once inputs must tolerate duplicate execution when required.

## Exactly-Once

`EXACTLY_ONCE` means that the externally observable processing effect occurs exactly once according to a precisely defined contract.

This is stronger than:

```text
delivered once
```

or:

```text
processor invoked once
```

For exactly-once semantics, the architecture would need to define:

```text
identity
commit boundary
failure recovery
duplicate suppression
state persistence
output atomicity
```

EVolution does not provide universal exactly-once semantics at this stage.

A component may implement exactly-once behavior later when its complete contract supports it.

## Invocation Count vs Effect Count

These concepts must remain separate.

A processor may be invoked twice:

```text
execute(X)
execute(X)
```

while producing one externally visible effect if the processor is idempotent or deduplicates by identity.

Conversely, a processor invoked once may produce multiple effects.

Therefore:

```text
invocation count
```

is not equivalent to:

```text
effect count
```

## Idempotency

A processor may be declared **idempotent** with respect to a defined input identity and effect boundary.

Conceptually:

```text
process(X)
process(X)
```

produces the same externally relevant result/effect as:

```text
process(X)
```

under the processor's declared semantics.

Idempotency is not automatically guaranteed by:

```text
const correctness
statelessness
pure-looking code
```

It must be established by the processor contract.

## Idempotency Key

When duplicate suppression is required, the processor may use a logical identity or explicit idempotency key.

Conceptually:

```text
Idempotency Key
{
    scope
    identifier
}
```

The key must have stable semantics.

It must not depend on:

```text
memory address
thread identity
queue position
execution time
```

unless explicitly part of the contract.

## Logical Identity and Duplicate Detection

If an input has a stable logical identity:

```text
EventId
MeasurementId
AnalysisId
```

that identity may be used for duplicate detection.

However:

```text
same identity
```

does not universally mean:

```text
same processing attempt
```

or:

```text
same payload
```

The relevant object contract defines equality and identity semantics.

## Retries

Retries are execution policy rather than delivery semantics alone.

A system may retry:

```text
Input X
   ↓
failure
   ↓
retry
```

but the retry policy must define:

```text
maximum attempts
delay
backoff
retryable errors
cancellation interaction
shutdown interaction
duplicate handling
```

No universal retry policy is selected by this ADR.

## Retryable Errors

An error may be classified as retryable, but that does not require a caller to retry it.

For example:

```text
Error(ResourceExhausted)
```

may be retryable under one execution policy and terminal under another.

The Error model therefore remains separate from retry policy.

## Retry and Cancellation

Cancellation must prevent new retry attempts where the cancellation contract requires it.

For example:

```text
attempt 1
   ↓
failure
   ↓
cancellation requested
   ↓
no attempt 2
```

A retry already running may require cooperative cancellation.

Cancellation remains distinct from retry failure.

## Retry and Shutdown

During orderly stop:

```text
STOP
```

the system should not implicitly create unlimited new retries.

Retry behavior during shutdown must be defined by the graph/processor execution policy.

A draining system must have a bounded interpretation of:

```text
"finish admitted work"
```

when work repeatedly fails and retries.

## Duplicate Processing

Duplicates can originate from:

```text
retry
worker failure
queue recovery
network retransmission
producer retry
persistent-buffer recovery
manual replay
```

The processing architecture must not assume that duplicates indicate malformed input.

Duplicate handling is a contract concern.

## Duplicate Input vs Duplicate Execution

These are distinct:

```text
Duplicate Input:
    producer intentionally or accidentally submits the same logical object twice.

Duplicate Execution:
    one admitted input is processed more than once.
```

A processor may need to handle either or both.

## Replay

Replay intentionally processes historical inputs again.

Therefore replay is not automatically a duplicate-execution failure.

For example:

```text
Historical Event E
    ↓
Live processing
```

and later:

```text
Historical Event E
    ↓
Replay
```

are separate execution contexts.

Replay identity and run identity distinguish these executions.

## Reprocessing

Reprocessing may intentionally recompute outputs from the same historical inputs using:

```text
new processor version
new configuration
new algorithm
```

Therefore identical input identity does not imply identical output identity across different processing definitions.

Provenance must identify the relevant:

```text
processor version
configuration
execution context
graph version
```

## Output Identity

An output produced by processing must have identity semantics appropriate to its object type.

A repeated execution may produce:

```text
same logical output
```

or:

```text
new derived output
```

depending on the output contract.

The processing infrastructure must not universally assume one model.

## Side Effects

Delivery semantics become more important when processors perform external side effects.

Examples:

```text
write database record
send network message
publish event
modify external resource
```

A retry can repeat the side effect.

Therefore side-effecting processors must explicitly define:

```text
idempotency
transaction boundary
deduplication
commit semantics
failure recovery
```

## Pure Processing

Pure or side-effect-free processors simplify retry semantics.

Conceptually:

```text
Output = F(Input, Configuration, Context)
```

If the same operation is repeated under the same deterministic conditions, the output may be reproduced without external side effects.

This does not automatically provide exactly-once semantics, but it can make duplicate execution harmless at the semantic level.

## State Updates

Stateful processors require special care.

For:

```text
Stateₙ₊₁ = F(Stateₙ, Input)
```

executing the same input twice may produce:

```text
Stateₙ₊₂
```

that differs from the state produced by one execution.

Therefore stateful processors receiving at-least-once delivery must explicitly address duplicate processing.

Possible mechanisms include:

```text
deduplication
idempotent state transition
transactional checkpointing
sequence validation
version checks
```

No universal mechanism is selected.

## Sequence-Based Deduplication

A processor may use sequence information to detect duplicates.

For example:

```text
Input:
    sequence = 100

repeat:
    sequence = 100
```

However, sequence numbers are only suitable for deduplication when the input contract defines them as stable within the relevant scope.

A sequence number must not automatically be treated as a globally unique identity.

## Exactly-Once State Transition

A processor may eventually implement an atomic boundary such as:

```text
Input
  +
State Update
  +
Output Commit
```

where all required effects become durable together.

Such semantics require explicit storage and transaction decisions.

They are not provided by the generic queue or scheduler.

## Delivery Guarantees and Graph Connections

Each graph connection may declare delivery semantics.

For:

```text
A → B
```

the connection can conceptually specify:

```text
delivery = AT_LEAST_ONCE
```

while another connection may use:

```text
delivery = AT_MOST_ONCE
```

The graph must validate compatibility between connection semantics and processor requirements.

## Delivery Guarantees and Fan-Out

For:

```text
A
├──→ B
└──→ C
```

each destination may have independent delivery semantics.

For example:

```text
A → B : AT_LEAST_ONCE
A → C : AT_MOST_ONCE
```

The source output must therefore have suitable ownership/lifetime semantics for both connections.

One destination's delivery guarantee must not silently change another's.

## Delivery Guarantees and Fan-In

For fan-in:

```text
A ──→
      \
       → D
      /
B ──→
```

D's correctness may depend on the delivery guarantees of both inputs.

If D requires each correlated input exactly once but receives at-least-once sources, D must explicitly provide duplicate handling.

Graph validation should detect incompatible requirements when they are statically identifiable.

## Delivery Guarantees and Backpressure

Backpressure affects delivery.

For example:

```text
AT_LEAST_ONCE
+
REJECT
```

does not mean:

```text
silently lose item
```

The system must retry, retain, spill, or otherwise satisfy the declared delivery contract.

If it cannot satisfy the contract, it must expose an explicit failure rather than claiming successful delivery.

## Delivery Guarantees and Drop

Explicit `DROP` conflicts with a lossless at-least-once contract for the same admitted work.

Therefore:

```text
AT_LEAST_ONCE + unconditional DROP
```

is incompatible.

A graph may still use lossy admission before an item is considered admitted.

This distinction is important:

```text
Rejected before admission
```

is different from:

```text
Admitted then silently dropped
```

## Delivery Guarantees and Sampling

Sampling similarly occurs at an admission boundary.

Items that are never admitted do not necessarily violate a downstream delivery contract.

Once an item is admitted, however, the declared delivery semantics apply.

## Admission vs Delivery

The architecture therefore distinguishes:

```text
Submission
    ↓
Admission
    ↓
Delivery
    ↓
Execution
    ↓
Completion
```

For example:

```text
REJECT
```

occurs before admission.

It does not represent a failed delivery of an admitted item.

## Failure Before Delivery

If an item is admitted but cannot be delivered:

```text
admitted
   ↓
delivery failure
```

the delivery contract determines whether the system:

```text
retry
retain
spill
fail
```

or applies another explicit mechanism.

The item must not silently disappear under a lossless contract.

## Failure After Execution

The most difficult case occurs when processing succeeds but completion is not durably recorded.

Example:

```text
execute
   ↓
external effect succeeds
   ↓
worker crashes
   ↓
completion record absent
   ↓
retry
```

The second execution may repeat the external effect.

This is one reason exactly-once semantics require an explicit commit boundary.

## Acknowledgement

A processing system may eventually use acknowledgements.

Conceptually:

```text
Delivery
   ↓
Processing
   ↓
Acknowledgement
```

An acknowledgement means that the sender/queue may apply the contractually defined post-processing disposition.

The exact acknowledgement mechanism is deferred.

## Acknowledgement and Success

An acknowledgement must not be interpreted universally as:

```text
processor returned success
```

The acknowledgement contract must explicitly define what it confirms.

Possible meanings include:

```text
received
admitted
executed
completed
durably committed
```

These are materially different.

## Durable Completion

For recoverable processing, the system may require a durable completion record before considering work complete.

Conceptually:

```text
Process
  ↓
Commit result
  ↓
Durable completion
```

Whether this is required depends on the storage/recovery contract.

## Delivery Semantics and Provenance

Provenance may record material execution semantics such as:

```text
delivery guarantee
attempt count
processor version
configuration
graph version
execution run
```

This is especially important when results may have been produced under retries or replay.

## Observability

Operational telemetry may expose:

```text
delivery attempts
duplicate executions
retry count
acknowledgement latency
failed deliveries
recovered work
```

These are normally observability concerns.

They become analytical data only when explicitly modeled as Events or Measurements.

## Default Guarantee

EVolution does not establish a universal stronger-than-default delivery guarantee.

Unless a connection/processor contract explicitly declares otherwise:

```text
exactly-once must not be assumed
```

Components must document the guarantee required for correctness.

## Consequences

### Positive

* Duplicate execution becomes an explicit architectural concern.
* Retry behavior can be reasoned about independently of error representation.
* Stateful processors are forced to address duplicate delivery.
* Exactly-once claims require explicit evidence and commit semantics.
* Replay is clearly separated from accidental duplicate execution.
* Admission loss is distinguished from post-admission loss.

### Negative

* At-least-once processing can require deduplication.
* Stronger guarantees can require persistent state and transactions.
* Side-effecting processors become substantially more complex.
* Recovery semantics must be designed explicitly.

## Deferred Decisions

This ADR does not select:

```text
acknowledgement protocol
transaction mechanism
deduplication storage
persistent queue implementation
retry algorithm
exactly-once implementation
distributed commit protocol
message broker
```

## Decision Summary

```text
Delivery semantics:
    Explicit contract

Supported conceptual guarantees:
    AT_MOST_ONCE
    AT_LEAST_ONCE
    EXACTLY_ONCE

Default:
    Exactly-once is not assumed

Delivery:
    Distinct from execution

Execution:
    Distinct from completion

Invocation count:
    Distinct from effect count

Duplicate input:
    Distinct from duplicate execution

Retries:
    Execution policy

Idempotency:
    Processor/side-effect contract

Replay:
    Separate execution context

Admission loss:
    Distinct from delivery loss

Acknowledgement:
    Explicit semantic meaning required

Durable completion:
    Required only where recovery contract demands it
```

## Invariants

1. Delivery semantics must be explicit where they materially affect correctness.
2. Exactly-once execution is never assumed implicitly.
3. Delivery success does not imply processor success.
4. Queue removal does not imply successful processing.
5. Invocation count and externally visible effect count are distinct.
6. Duplicate input and duplicate execution are distinct conditions.
7. At-least-once delivery may produce duplicate execution.
8. Processors requiring stronger guarantees must explicitly provide the required mechanism.
9. Stateful processors must account for duplicate processing when receiving at-least-once inputs.
10. Side-effecting processors must define retry and duplicate-effect semantics.
11. Replay is not automatically an execution failure caused by duplication.
12. Admission rejection is distinct from post-admission delivery failure.
13. Explicit dropping before admission does not constitute successful delivery.
14. Admitted work must satisfy its declared delivery contract or produce an explicit failure.
15. Acknowledgement semantics must be explicitly defined.
16. Durable completion is distinct from in-memory completion.
17. Delivery guarantees may differ between graph connections.
18. Fan-in processors must account for the guarantees of all relevant inputs.
19. Delivery semantics must not silently change because of queue implementation or execution backend.
20. Provenance must preserve material delivery/execution information required to reproduce or interpret derived results.

## Invariant

> **EVolution treats delivery, execution, and completion as distinct stages with explicit guarantees; duplicate execution, retry, recovery, and acknowledgement semantics must be defined rather than hidden behind an assumption of exactly-once processing.**
