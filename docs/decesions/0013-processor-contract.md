## Processor Input/Output Envelope

A processor boundary should distinguish the **semantic payload** being processed from the **information required to transport, identify, and interpret that processing item at the processor boundary**.

EVolution therefore defines a conceptual **processing envelope**.

```text
Envelope
{
    payload
    identity?
    context?
    sequence?
    temporal information?
    provenance?
}
```

The exact C++ representation is deferred.

The envelope is not intended to replace the underlying EVolution concepts such as Event, State, Measurement, or Analysis.

Instead:

```text
Payload
    → what is being processed

Envelope
    → information required to process that payload correctly
```

### Basic Envelope

Conceptually:

```text
InputEnvelope<T>
{
    payload: T
    metadata
}
```

and:

```text
OutputEnvelope<T>
{
    payload: T
    metadata
}
```

The payload remains the primary semantic object.

For example:

```text
InputEnvelope<Event>
{
    payload:
        PlayerAction

    metadata:
        processing context
        delivery information
}
```

The envelope must not duplicate information that already belongs semantically to the payload.

For example, an `Event` already has:

```text
event_id
event_time
event_type
source
sequence
payload
```

The processor envelope should not create a second unrelated `event_time` field.

### Envelope Metadata

Envelope metadata is limited to information required at the processing boundary.

Potential categories include:

```text
identity
processing context
delivery information
correlation information
provenance references
execution metadata
```

Not every envelope contains every category.

A processor contract defines which metadata is required.

### Payload

The payload is the object being processed.

Examples include:

```text
Event
State
Measurement
TimeSeries
AggregationResult
Pattern
Analysis
domain-specific object
```

The envelope does not change the semantic type of the payload.

For example:

```text
Envelope<Measurement>
```

still represents a `Measurement`.

The envelope is a processing boundary representation, not a new analytical concept.

### Envelope Identity

An envelope may require an identity when the processing system needs to distinguish processing items.

This identity is distinct from the logical identity of the payload.

For example:

```text
Envelope identity:
    identifies this processing item

Payload identity:
    identifies the underlying logical object
```

These may be the same in simple cases, but they must not be assumed to be identical.

An envelope identity must follow the Identity Model defined in ADR 0010.

### Correlation

Processing may require a way to associate related inputs and outputs.

For example:

```text
Input A ─────┐
Input B ─────┼──→ Processor ──→ Output C
Input D ─────┘
```

A processor may need to preserve or create correlation information allowing:

```text
Output C
    ↓
related inputs A, B, D
```

to be identified.

Correlation is not necessarily logical identity.

It is processing metadata describing relationships between processing items.

### Sequence

An envelope may carry processing sequence information when the processing infrastructure needs it.

This is distinct from domain sequence information.

For example:

```text
Domain Event:
    sequence = 1842

Processing Envelope:
    delivery sequence = 73
```

These represent different concepts.

Domain sequence belongs to the domain object.

Processing sequence belongs to the processing mechanism.

Neither should silently replace the other.

### Temporal Information

Envelope-level temporal information may be present when required by the processing infrastructure.

Examples include:

```text
event time
ingestion time
processing time
```

However, temporal semantics defined by the payload remain authoritative for the payload.

The envelope must not silently overwrite them.

For example:

```text
Event:
    event_time = T1

Envelope:
    ingestion_time = T2
```

is valid because:

```text
T1 ≠ T2
```

and the timestamps have different meanings.

If the same temporal concept appears at both levels, the contract must explicitly define which value is authoritative.

### Processing Context

An envelope may carry references to relevant processing context.

For example:

```text
run_id
processing mode
partition
source stream
```

The envelope should reference execution context rather than duplicate the entire context.

Conceptually:

```text
Envelope
    ↓
Execution Context reference
```

This keeps execution context separate from the payload.

### Provenance

An envelope may carry provenance information needed during processing.

For example:

```text
input object identity
source identity
upstream processor identity
```

However, the envelope is not itself the provenance model.

Provenance remains a first-class EVolution concept.

The envelope may carry provenance references required for propagation or construction of the resulting provenance.

### Batch Envelope

A processor may receive multiple processing items.

A batch should therefore be representable without changing the meaning of an individual payload.

Conceptually:

```text
BatchEnvelope<T>
{
    items: [Envelope<T>, ...]
    batch metadata
}
```

A batch has its own boundary and may have:

```text
batch identity
batch size
batch sequence
batch provenance
```

where required.

The batch itself is not automatically a domain aggregation.

For example:

```text
100 Events
```

is not an aggregation merely because they are transported together.

### Single Item vs Batch

The processor contract must specify whether it consumes:

```text
single item
batch
stream
window
```

These are execution/input forms rather than different semantic meanings.

For example:

```text
Processor A:
    Envelope<Event>
```

may process one event at a time.

Another processor may consume:

```text
BatchEnvelope<Event>
```

without changing what an `Event` means.

### Envelope and Ownership

The envelope defines semantic containment, not necessarily a particular C++ ownership mechanism.

The implementation may use:

```text
value
move
reference
view
shared ownership
other controlled mechanism
```

where the processor contract permits it.

However, lifetime requirements must be explicit.

A processor must never retain a non-owning reference beyond its permitted lifetime.

The exact ownership model is deferred to the C++ API design.

### Envelope and Errors

An envelope normally represents successful processing input or output.

Expected processing failures are represented through `Result<T>` according to ADR 0005 and ADR 0006.

Conceptually:

```text
Result<OutputEnvelope<T>>
```

rather than encoding ordinary failure as:

```text
Envelope<T>
{
    payload = error value
}
```

This keeps:

```text
successful output
```

and:

```text
processing failure
```

semantically distinct.

For batch processing, the contract may require richer partial-failure representation.

### Envelope and Empty Output

A processor may legitimately produce no output.

For example:

```text
Event
    ↓
filter
    ↓
no output
```

This is not necessarily an error.

The processor's output contract must distinguish:

```text
no output
```

from:

```text
processing failure
```

Possible conceptual representations include:

```text
Result<optional<OutputEnvelope<T>>>
```

or a stream/batch contract that naturally permits zero outputs.

The exact API depends on the processor execution model.

### Envelope Transformation

A processor normally transforms:

```text
InputEnvelope<A>
        ↓
Processor
        ↓
OutputEnvelope<B>
```

The processor may:

```text
preserve metadata
modify metadata
add metadata
remove metadata
create new metadata
```

but these operations must follow the processor contract.

In particular, a processor must not silently alter:

```text
payload identity
event time
domain sequence
provenance
```

when those values have semantic meaning.

### Envelope Metadata vs Payload Data

A useful architectural rule is:

```text
If information describes the object:
    payload/domain model

If information describes processing of the object:
    envelope/processing metadata
```

For example:

```text
Event:
    player
    action
    event_time
    source

Envelope:
    run_id
    processing sequence
    delivery information
```

This is a guideline rather than an absolute rule because some information can legitimately belong to both domain and processing layers with different semantics.

### Envelope Stability

The envelope should remain small and stable.

New processing metadata should not automatically become part of every envelope.

A field should be added to the generic envelope only when it represents a broadly applicable processing concern.

Domain-specific information belongs in the payload or domain-specific metadata.

### Conceptual Envelope Model

The resulting conceptual model is:

```text
Envelope<T>
{
    payload: T

    processing metadata:
        envelope identity?
        correlation?
        delivery sequence?
        execution-context reference?
        provenance reference?
}
```

Temporal information is included only where required by the processing contract and must preserve its defined temporal semantics.

The envelope is therefore intentionally smaller than a universal context object.

## Processor Boundary

A processor boundary can now be represented conceptually as:

```text
Input Envelope
       │
       │
       ▼
┌─────────────────┐
│    Processor    │
│                 │
│ configuration   │
│ execution ctx   │
│ processing state│
└─────────────────┘
       │
       ▼
Output Envelope
```

With:

```text
Input Envelope
    ├── Payload
    └── Processing Metadata

Processor
    ├── Configuration
    ├── Execution Context
    └── State

Output Envelope
    ├── Payload
    └── Processing Metadata
```

This separates four different concerns:

```text
Payload
    → what information is being processed

Envelope
    → how that information crosses a processor boundary

Configuration
    → how the processor should behave

Execution Context
    → under what runtime conditions it operates

State
    → what the processor has accumulated or reconstructed
```

## Envelope Rules

The following rules apply:

1. The payload remains the authoritative semantic object.
2. The envelope must not duplicate payload semantics without an explicit reason.
3. Envelope metadata describes processing rather than domain meaning.
4. Domain identity and envelope identity are distinct concepts.
5. Domain sequence and processing sequence are distinct concepts.
6. Payload temporal semantics must not be silently overwritten by envelope metadata.
7. Provenance remains a first-class concept; the envelope may carry provenance references.
8. Execution context remains separate; the envelope may reference relevant context.
9. Batch transport does not imply analytical aggregation.
10. Expected failure is represented through `Result`, not as a normal payload value.
11. Zero-output processing must remain distinguishable from failure.
12. Envelope lifetime and ownership requirements must be explicit.
13. Generic envelope metadata must remain intentionally small.
14. Domain-specific semantics belong in the payload or domain layer.

## Decision Summary

```text
Processor boundary:             Envelope<T>

Semantic payload:               First-class payload object

Envelope purpose:               Processing-boundary metadata

Domain identity:                Separate from envelope identity

Correlation:                    Supported where required

Domain sequence:                Separate from processing sequence

Temporal metadata:              Explicit semantic meaning required

Execution context:              Referenced, not duplicated

Provenance:                     Referenced, not replaced

Batch:                          Supported conceptually

Batch ≠ aggregation:            Yes

Failure representation:         Result<OutputEnvelope<T>>

Zero output ≠ failure:           Yes

Envelope size:                  Intentionally small

Ownership model:                Deferred

Exact C++ representation:       Deferred
```

## Invariant

> **A processor envelope carries a semantic payload together with only the processing metadata required to interpret and transport that payload across the processor boundary; it must not silently duplicate or redefine domain semantics.**
