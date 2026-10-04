# ADR 0063: Processing Envelope and Metadata API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will use a lightweight **Processing Envelope** to carry a payload together with processing metadata required at processor boundaries.

```text id="p8r2k1"
Envelope<T>
{
    payload
    identity?
    context?
    sequence?
    temporal_information?
    provenance?
}
```

The Envelope is a processing-boundary construct.

It does **not** replace Event, State, Measurement, Pattern, Analysis, or any other semantic object.

The Envelope must remain small and generic.

---

# 1. Purpose

Processors need metadata that is not necessarily part of the payload's domain semantics.

For example:

```text id="y0j8sx"
payload
+
processing sequence
+
execution context
+
transport/provenance information
```

The Envelope provides this boundary without modifying the semantic payload.

---

# 2. Payload

The payload is the primary semantic object being processed.

Examples:

```text id="1h8m3f"
Event
State
Measurement
Pattern
Analysis
domain object
batch
```

The Envelope does not interpret the payload.

---

# 3. Envelope Is Not a Universal Object

An Envelope must not become a replacement for all EVolution concepts.

For example:

```text id="k5jv8s"
Envelope<Event>
```

does not mean that Event semantics belong to Envelope.

The Event remains the Event.

---

# 4. Identity

An Envelope may carry an identity when processing requires one.

However, identity must not be duplicated unnecessarily.

For example:

```text id="4utvpg"
Event
    already has EventId

Envelope<Event>
    does not automatically require another identity
```

A separate Envelope identity is justified only when the processing boundary needs to identify the envelope itself.

---

# 5. Identity Types

The following identities remain distinct:

```text id="cn9j1s"
EventId
StateId
MeasurementId
ProcessorId
OperationId
RunId
TraceId
CorrelationId
EnvelopeId
```

An Envelope must not reuse one semantic identity to represent another.

---

# 6. Correlation

Correlation identifies related processing activity.

For example:

```text id="r9b9z5"
request
 ├── operation
 ├── processing work
 └── outputs
```

A correlation identifier is not an object identity.

It must not be used as a substitute for EventId or MeasurementId.

---

# 7. Context

An Envelope may carry relevant Context.

For example:

```text id="1q6f9f"
Envelope
    payload = Event
    context = relevant processing context
```

Context must remain semantically separate from the payload.

---

# 8. Execution Context

An Envelope may reference relevant Execution Context.

It should not duplicate the complete ExecutionContext into every Envelope unnecessarily.

Conceptually:

```text id="9r5z6y"
Envelope
    execution_context_reference
```

or another bounded representation.

The exact implementation is deferred.

---

# 9. Sequence

An Envelope may carry processing sequence information.

This sequence must not be confused with:

```text id="eqm6e7"
Event sequence
domain ordering
event time
processing time
storage order
```

Sequence semantics must be explicit.

---

# 10. Processing Sequence

A processing sequence may identify ordering within a processor or processing connection.

For example:

```text id="7w6f2x"
input 1 → sequence 1
input 2 → sequence 2
input 3 → sequence 3
```

This does not establish domain ordering.

---

# 11. Temporal Information

An Envelope may carry temporal information when processing requires it.

The preferred representation follows the Time Model:

```text id="qu9x01"
TemporalInformation
{
    time
    status
    uncertainty?
}
```

The Envelope must not invent a timestamp when the payload does not have one.

---

# 12. Event Time vs Processing Time

An Envelope carrying an Event must preserve the Event's event-time semantics.

Processing time may be separately available through Execution Context or processing metadata.

For example:

```text id="9wqk7f"
Event time:
    when the event happened

Processing time:
    when EVolution processed it
```

These must remain distinct.

---

# 13. Provenance

An Envelope may carry or reference provenance relevant to processing.

However, payload provenance and processing-boundary provenance must not be unnecessarily duplicated.

For example:

```text id="d2j9u3"
Event
    provenance = source history

Envelope
    processing provenance = processor/graph context
```

The exact merging semantics remain deferred.

---

# 14. Provenance Enrichment

A processor may enrich provenance as information flows through the processing graph.

For example:

```text id="f9k3aw"
Input
    ↓
Processor P1
    ↓
Output
```

The output provenance may identify P1 as a transformation.

The original source provenance must remain recoverable where required.

---

# 15. Payload Immutability

An Envelope does not imply payload mutability.

If the payload is an immutable Event:

```text id="o2y6qf"
Envelope<Event>
```

the Event remains immutable.

A processor must not mutate payload semantics through the Envelope.

---

# 16. Envelope Ownership

Envelope ownership follows the Data Ownership Model.

Possible states include:

```text id="c5h6qk"
owned
borrowed
moved
queued
returned
```

The exact ownership contract depends on the processor/execution API.

---

# 17. Queue Ownership

When an Envelope is admitted into a queue:

```text id="a7j0n4"
Producer
    ↓
Queue
```

the queue becomes responsible for retaining the admitted work according to the Queue Model.

The producer must not assume the Envelope remains borrowed after successful admission.

---

# 18. Async Lifetime

Asynchronous processing must not retain references to temporary Envelope storage unless the API explicitly guarantees its lifetime.

For example:

```text id="2s7z9e"
invalid:
    enqueue(reference_to_stack_object)

valid:
    enqueue(owned_or_lifetime_safe_envelope)
```

The implementation must enforce or clearly document this contract.

---

# 19. Envelope Copy and Move

Envelope should have ordinary C++ value semantics where practical.

Copying an Envelope must preserve its observable processing metadata.

Moving an Envelope transfers ownership of its owned contents.

Moved-from state follows normal C++ value semantics.

---

# 20. Envelope and Result

Processor operations may conceptually return:

```text id="5t5xqa"
Result<OutputEnvelope<T>>
```

This distinguishes:

```text id="s7p1lc"
successful output
```

from:

```text id="8f9e1x"
processing failure
```

An error must not be silently encoded as an empty Envelope.

---

# 21. Empty Payload

An empty payload is distinct from an error.

Whether an operation may produce no payload must be part of the processor contract.

For example:

```text id="j1k8n2"
0 outputs
```

is a cardinality decision, not an error representation.

---

# 22. Multiple Outputs

A Processor may produce:

```text id="4p5yqn"
zero
one
many
```

outputs.

The Envelope is therefore not inherently a one-input/one-output abstraction.

Batch or collection wrappers may be used where the processor contract requires them.

---

# 23. Batch Envelope

A batch may conceptually contain:

```text id="q6x0cs"
BatchEnvelope<T>
{
    items: Envelope<T>[]
}
```

The exact representation is deferred.

Batch semantics must explicitly define ordering, partial failure, and identity.

---

# 24. Batch Identity

A batch may have its own identity.

This does not replace the identities of its individual items.

For example:

```text id="6ux5nv"
BatchId
    ≠ EventId
```

---

# 25. Batch Ordering

If ordering matters, a batch must preserve the ordering semantics required by the contract.

Possible orderings include:

```text id="4z0a4c"
input order
event sequence
event time
partition order
none
```

The Envelope itself does not choose the ordering.

---

# 26. Envelope and Graph Connections

Processing graph connections may transport Envelopes.

A connection may apply policies involving:

```text id="f1u7t4"
admission
ordering
buffering
delivery
backpressure
cancellation
```

These policies belong to the Processing layer.

---

# 27. Envelope vs Connection

The Envelope represents one processing item.

The Connection represents a relationship between processor ports.

```text id="n1j5yq"
Envelope
    → item

Connection
    → transport relationship
```

They must remain separate.

---

# 28. Envelope vs Queue

Similarly:

```text id="k3d6w8"
Envelope
    → data/work item

Queue
    → temporary retention and ordering mechanism
```

A queue must not redefine Envelope semantics.

---

# 29. Envelope vs Event Stream

An Event Stream is a semantic sequence of Events.

A processing queue is an execution mechanism.

They are not equivalent.

```text id="q7g3r2"
Event Stream
    → domain/history concept

Queue
    → processing mechanism
```

---

# 30. Envelope and Event Stream Position

A processor may attach an input position to an Envelope when needed for replay/recovery.

For example:

```text id="f7s4y1"
stream = S1
position = 1042
```

This position is processing metadata.

It does not become part of Event identity.

---

# 31. Envelope and Storage Position

A storage location may also be carried as metadata when required.

For example:

```text id="b9j0x8"
file offset
database cursor
partition offset
```

Such information is storage/execution metadata, not semantic identity.

---

# 32. Envelope and Error Context

When a processor returns an Error, the error may reference relevant Envelope identity or processing context.

However, the Error must remain self-contained after leaving the processor.

---

# 33. Envelope and Cancellation

An Envelope does not represent cancellation.

Cancellation belongs to the processing operation/lifecycle model.

An operation may stop processing an Envelope due to cancellation, but the Envelope itself remains ordinary data/work.

---

# 34. Envelope and Deadlines

A processing operation may have a deadline.

A deadline is execution context, not payload time.

For example:

```text id="l5y4qx"
event_time = 12:00
deadline   = 12:05
```

These have completely different semantics.

---

# 35. Envelope and Security Context

Security context may be associated with an Envelope when required.

It must not be automatically copied into analytical payloads.

Sensitive information must follow the Security and Secrets Model.

---

# 36. Envelope and Authentication

Authentication information belongs to the interface/security boundary.

If downstream processing requires a principal or authorization context, the relevant security context may be explicitly propagated.

Raw credentials must never become normal Envelope metadata.

---

# 37. Envelope and Configuration

Configuration is not Envelope metadata.

A processor's behavior is determined by its explicit Configuration.

An Envelope may identify the relevant configuration/version through provenance or execution context when needed.

---

# 38. Envelope and State

A State payload remains a State.

For example:

```text id="m2q7cw"
Envelope<State>
```

does not turn State into processing metadata.

---

# 39. Envelope and Measurement

Similarly:

```text id="d9v8te"
Envelope<Measurement>
```

contains a Measurement payload.

Metric identity, value, scope, and temporal semantics remain Measurement semantics.

---

# 40. Envelope and Analysis

An Analysis may be transported as a payload:

```text id="q8k2t1"
Envelope<Analysis>
```

The analysis retains its own evidence and provenance.

The Envelope only adds processing metadata required by the boundary.

---

# 41. Metadata Duplication

The implementation should avoid storing the same semantic information in both payload and Envelope.

For example, if Event already contains:

```text id="3c1q8f"
EventId
EventType
Event time
```

the Envelope should not independently redefine these fields.

---

# 42. Metadata Precedence

If the same information appears in both payload and Envelope, the processor contract must define which is authoritative.

The preferred rule is:

```text id="5x2m0e"
semantic payload fields
    → authoritative for payload semantics

Envelope metadata
    → authoritative only for processing metadata
```

Silent conflict is invalid.

---

# 43. Envelope Validation

Envelope validation should occur at the processing boundary.

Validation may include:

```text id="6h3x9m"
payload presence
metadata compatibility
identity validity
sequence validity
temporal validity
provenance validity
context compatibility
```

Semantic validation of the payload remains owned by the payload/domain contract.

---

# 44. Envelope Compatibility

A processor input contract should specify which metadata it requires.

For example:

```text id="f4s7z1"
Processor A requires:
    Event payload
    event time
    source identity

Processor B requires:
    Measurement payload
    metric identity
    processing sequence
```

A generic Envelope does not imply that every field is required.

---

# 45. Optional Metadata

Metadata should be optional when not semantically required.

This prevents every processing item from carrying unnecessary information.

For example:

```text id="j6r2vb"
temporary internal value
    → may require no identity/provenance

persistent analytical result
    → may require identity/provenance
```

---

# 46. Metadata Requirements

Processor contracts should explicitly identify required metadata.

Conceptually:

```text id="2p7j3e"
Input Contract
{
    payload type
    required metadata
    optional metadata
    ordering
    cardinality
    ownership
}
```

---

# 47. Envelope Transformation

A processor may:

```text id="m6x1r4"
preserve
add
remove
transform
```

processing metadata according to its contract.

It must not remove metadata required for downstream semantic correctness.

---

# 48. Provenance Transformation

A transformation processor should normally preserve upstream provenance while adding its own derivation information.

For example:

```text id="k9z2ds"
Source
   ↓
Processor A
   ↓
Processor B
```

Result provenance should permit reconstruction of:

```text id="x0s6by"
Source → A → B
```

when required.

---

# 49. Context Transformation

A processor may narrow or enrich Context.

For example:

```text id="u3j5wq"
Application Context
        ↓
Processor Context
        ↓
Operation Context
```

Context projection must be explicit.

---

# 50. Envelope Identity Generation

If an Envelope requires its own identity, identity generation follows the Identity Model.

The mechanism is not selected by this ADR.

The generated identity must not replace the payload's logical identity.

---

# 51. Envelope Persistence

An Envelope normally represents transient processing state.

It should not automatically become persistent analytical data.

If an Envelope is persisted for:

```text id="t4y8bc"
recovery
queue spill
checkpoint
```

the persistence contract must identify what parts are required for recovery.

---

# 52. Recovery

Recovery may require restoring:

```text id="q2j9s8"
payload
identity
ordering position
delivery state
provenance
relevant context
```

The Recovery Model determines the exact set.

---

# 53. Envelope and Delivery Semantics

Delivery semantics apply to the processing item represented by the Envelope.

For example:

```text id="m1w5z7"
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

The Envelope itself does not guarantee any delivery semantic.

---

# 54. Envelope and Idempotency

If a processor can receive duplicate Envelopes under at-least-once delivery, it must use an explicit idempotency mechanism when required.

Possible keys may include:

```text id="v8y2d1"
EventId
OperationId
EnvelopeId
domain-defined idempotency key
```

The appropriate key is contract-specific.

---

# 55. Envelope and Determinism

Envelope metadata that affects processing semantics becomes part of the relevant input.

For deterministic processing:

```text id="m7c3qa"
Payload
+
relevant Envelope metadata
+
Configuration
+
Execution Context
+
State
```

must determine the result.

---

# 56. Envelope and Reproducibility

A replay system must preserve metadata that materially affects the original processing result.

It must not assume that payload-only replay is sufficient.

---

# 57. Minimality

The Envelope should remain intentionally small.

It should not become a dumping ground for:

```text id="v1k8nm"
logs
metrics
arbitrary metadata
UI information
database internals
network headers
credentials
debug state
```

Those concerns have separate models.

---

# 58. API Shape

The conceptual API is:

```cpp id="n2j6l4"
namespace evolution::processing
{

template<typename T>
class Envelope;

}
```

The exact representation remains deferred.

---

# 59. Typed Envelope

The payload should be strongly typed where possible:

```text id="r3z5m7"
Envelope<Event>
Envelope<Measurement>
Envelope<State>
```

rather than:

```text id="s8k1d2"
Envelope<any>
```

Type erasure may be introduced at explicit runtime/plugin boundaries if required.

---

# 60. Envelope Construction

Construction should validate metadata combinations that can be validated generically.

Conceptually:

```text id="c7m9q2"
make_envelope(payload, metadata)
    → Result<Envelope<T>>
```

Domain validation remains outside the generic Envelope constructor.

---

# 61. Envelope Inspection

Envelope metadata should be inspectable without mutating it.

For immutable payloads and metadata:

```text id="p4x8v1"
const Envelope<T>
```

should be the normal inspection form.

---

# 62. Envelope Mutation

Metadata mutation should be restricted.

Where metadata changes semantic processing history, producing a new Envelope may be preferable to silently mutating an existing one.

The exact mutability policy depends on whether the Envelope is:

```text id="g6n2w5"
in-flight mutable execution state
```

or:

```text id="h1r7c3"
historical/persisted representation
```

---

# 63. Relationship to Processor API

The Processor API should operate on typed inputs and outputs that may be represented as Envelopes.

Conceptually:

```text id="k4q8s1"
InputEnvelope
      ↓
Processor
      ↓
OutputEnvelope
```

The exact synchronous/asynchronous invocation API remains deferred.

---

# 64. Relationship to Processing Graph

Graph connections transport compatible Envelopes between ports.

The graph validates:

```text id="y5m3n8"
payload compatibility
cardinality
metadata requirements
ordering
identity
temporal requirements
provenance requirements
```

before activation where possible.

---

# 65. Relationship to Storage

Storage may persist an Envelope only when required for:

```text id="a8w1p6"
recovery
checkpoint
queue spill
execution history
```

Storage of analytical objects should generally persist the semantic payload rather than an implementation-specific transient Envelope.

---

# 66. Testing Requirements

Tests must cover:

```text id="d6j9r2"
typed payload
identity preservation
metadata preservation
metadata separation
context propagation
provenance propagation
sequence semantics
temporal information
ownership
copy/move
queue lifetime
batch behavior
delivery semantics
idempotency
cancellation interaction
recovery
serialization
validation
```

---

# 67. Deferred Decisions

This ADR does not select:

* exact Envelope C++ representation
* immutable vs mutable internal Envelope implementation
* exact metadata storage
* EnvelopeId generation
* type erasure mechanism
* batch representation
* serialization format
* queue implementation
* execution backend
* scheduler
* asynchronous API
* distributed transport
* persistent queue format

---

# Decision Summary

```text id="r8c2m1"
Envelope:
    Processing-boundary container

Payload:
    Semantic object being processed

Identity:
    Optional and semantically distinct

Context:
    Optional processing/domain context

Sequence:
    Processing ordering metadata

Temporal information:
    Preserved when required

Provenance:
    Preserved/enriched when required

Configuration:
    Separate from Envelope

Error:
    Separate from Envelope

Queue:
    Separate from Envelope

Connection:
    Separate from Envelope

Event stream:
    Separate from Envelope

Delivery guarantee:
    Not provided by Envelope

Ownership:
    Explicit

Type:
    Strongly typed where possible

Universal metadata map:
    Rejected

Semantic duplication:
    Avoided
```

## Invariant

**An Envelope carries a semantic payload together with only the processing metadata required at a boundary; it does not redefine payload semantics, replace domain objects, or become a universal metadata container, and identity, time, context, provenance, ordering, ownership, and delivery semantics remain explicitly distinguishable.**

