# ADR 0062: Context and Provenance API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent **Context** and **Provenance** as separate first-class concepts.

```text
Context
    → describes the circumstances in which an object exists or was observed

Provenance
    → describes where an object came from and how it was produced
```

They may both reference the same objects, but they answer different questions and must not be collapsed into a universal metadata structure.

The Core provides generic structures and mechanisms.

Domains define domain-specific context semantics.

Processing, ingestion, storage, and analysis components contribute provenance information according to their contracts.

---

# 1. Context

Context describes the circumstances relevant to interpreting an object or operation.

Examples include:

```text
player
table
session
market
instrument
environment
execution mode
```

Context does not itself determine the meaning of the object.

---

# 2. Provenance

Provenance describes origin and derivation.

For example:

```text
Measurement
    ← derived from Events E1..E100
    ← produced by Processor P1
    ← using Configuration C5
    ← algorithm version 3
```

Provenance answers:

```text
Where did this come from?
How was it produced?
```

---

# 3. Context vs Provenance

These concepts must remain separate.

```text
Context:
    table = T1
    stakes = 1/2

Provenance:
    source = hand_history_42
    processor = HandParser v3
```

The table and stakes describe circumstances.

The parser describes origin/derivation.

---

# 4. Contextual Entity

Context may reference entities.

Conceptually:

```text
Entity
{
    id
    type
    attributes
}
```

The exact entity model is domain-specific.

Core should not define a universal domain entity hierarchy.

---

# 5. Entity Identity

Contextual entities should use strongly typed logical identities where available.

Examples:

```text
PlayerId
TableId
SessionId
MarketId
InstrumentId
```

A generic string must not silently replace a semantic identity.

---

# 6. Context Scope

Context may be associated with a scope.

Examples:

```text
application
graph
processor
operation
event
state
measurement
analysis
decision
```

The scope must be explicit where ambiguity would affect interpretation.

---

# 7. Contextual Dimensions

Context may contain dimensions relevant to interpretation.

For example:

```text
game_type = NLHE
stakes = 1/2
table_size = 6
```

Dimensions used as analytical grouping should remain distinguishable from incidental runtime context.

---

# 8. Context vs Measurement Dimensions

Measurement dimensions answer:

```text
Along which semantic dimensions is this measurement identified?
```

Context answers:

```text
Under what circumstances does this object exist?
```

A domain may intentionally use the same value for both, but this relationship must be explicit.

---

# 9. Static Context

Some context remains stable for a period.

Examples:

```text
game type
instrument
deployment environment
table configuration
```

Static context may be referenced rather than duplicated into every object.

---

# 10. Dynamic Context

Other context changes over time.

Examples:

```text
active player set
market regime
network conditions
resource availability
application mode
```

Dynamic context must have explicit temporal semantics when historical correctness depends on when it was valid.

---

# 11. Context Validity

Context may have a validity interval.

Conceptually:

```text
Context
{
    ...
    validity
}
```

For example:

```text
table configuration
    valid from T1
    until T2
```

This prevents current context from being incorrectly applied to historical data.

---

# 12. Historical Context

When analyzing historical objects, the relevant historical context should be recoverable when it materially affects interpretation.

Current context must not silently replace historical context.

---

# 13. Context Snapshot

A component may capture a context snapshot for reproducibility.

Conceptually:

```text
ContextSnapshot
{
    context
    validity
    identity?
}
```

The exact implementation is deferred.

---

# 14. Execution Context

Execution Context is a specialized runtime context already defined by the Configuration Model.

It may contain:

```text
mode
run identity
clock
randomness
resource limits
dependency versions
cancellation
security context
```

Execution Context must not be confused with domain Context.

---

# 15. Domain Context vs Execution Context

For example:

```text
Domain Context:
    table = T1
    stakes = 1/2

Execution Context:
    mode = REPLAY
    run_id = R42
    clock = replay_clock
```

The two may coexist.

Neither should be represented as an unrestricted universal map.

---

# 16. Relevant Context

Not every runtime condition is analytically relevant.

For reproducibility, only context that can materially affect semantic output must be captured as relevant execution/context information.

Incidental information should not unnecessarily become part of analytical identity or provenance.

---

# 17. Context Projection

Context may be projected from broader scopes into narrower operations.

Conceptually:

```text
Application Context
        ↓
Graph Context
        ↓
Processor Context
        ↓
Operation Context
```

Each component receives only the context relevant to its contract.

---

# 18. Context Inheritance

Context inheritance must not be implicit.

A child component should receive explicitly defined context.

If values conflict, the contract must define the resolution rule.

Silent inheritance from global state is prohibited.

---

# 19. Context Mutability

Context used to interpret a historical object should be logically immutable.

Changing context should produce a new context representation or new validity interval rather than silently mutating historical meaning.

---

# 20. Context Ownership

Context ownership follows the Ownership Model.

A component may:

```text
borrow
own
copy
reference
```

context according to its contract.

Async operations must not retain borrowed context beyond its declared lifetime.

---

# 21. Context Identity

Context may have an identity when it represents a persistent or reusable logical object.

However, not every context value requires an independent identity.

Context identity must not be confused with:

```text
RunId
EventId
MeasurementId
TraceId
CorrelationId
```

---

# 22. Context Serialization

If context crosses a persistence or process boundary, its semantic fields must be serialized explicitly.

The serialization must preserve distinctions such as:

```text
known
unknown
missing
estimated
```

when those distinctions are meaningful.

---

# 23. Provenance Structure

Provenance should conceptually contain:

```text
Provenance
{
    source
    inputs
    transformation
    configuration
    version
    timestamp
    environment?
}
```

The exact representation remains implementation-defined.

---

# 24. Provenance Source

A source identifies where information originated.

Examples:

```text
file
database
external API
sensor
network feed
manual input
event stream
external application
```

Source identity is separate from the identity of the derived object.

---

# 25. Source Identity

An external source may have its own identity.

For example:

```text
external_record_id
file_id
feed_id
message_id
```

These must not automatically become EVolution logical IDs.

---

# 26. Provenance Inputs

A derived object may reference the objects from which it was produced.

For example:

```text
Analysis
    inputs:
        MeasurementId M1
        MeasurementId M2
        PatternId P1
```

Input references should use logical identities where available.

---

# 27. Provenance Transformation

Provenance should identify the transformation responsible for producing a derived object.

Examples:

```text
normalization
projection
aggregation
pattern detection
analysis
serialization
conversion
```

The transformation identity should be distinguishable from the output object's identity.

---

# 28. Algorithm Identity

Where an algorithm materially affects a result, provenance should identify the algorithm/version.

For example:

```text
algorithm = volatility-estimator
version = 3
```

Algorithm identity is not necessarily the same as Processor identity.

---

# 29. Processor Identity

A Processor instance may participate in provenance.

However:

```text
ProcessorId
```

identifies the execution component or logical processor instance.

The algorithm/version identifies the transformation semantics.

They should not automatically be merged.

---

# 30. Configuration Provenance

Derived results should identify the effective configuration when configuration materially affects the result.

For example:

```text
configuration = C42
```

The original configuration source is separate from the effective semantic configuration.

---

# 31. Configuration Source

Configuration may originate from:

```text
file
command line
environment
API
application defaults
generated configuration
```

Source provenance and effective configuration must remain distinct.

---

# 32. Component Version

A result may depend on a component version.

For example:

```text
projection_version
processor_version
analysis_version
```

The relevant version should be recorded when required for interpretation or reproducibility.

---

# 33. Timestamp in Provenance

Provenance timestamps describe provenance events such as:

```text
created
processed
persisted
derived
```

They must not be confused with:

```text
event time
measurement time
domain validity time
```

---

# 34. Environment

Some results depend on execution environment.

Examples:

```text
library version
dependency version
hardware capability
numeric backend
external service version
```

Environment information should only be captured when it materially affects semantics or reproducibility.

---

# 35. Provenance Graph

Provenance may form a graph.

```text
E1 ─┐
E2 ─┼→ Measurement M1 ─→ Analysis A1
E3 ─┘
```

The graph may contain:

```text
source
input
transformation
output
```

relationships.

---

# 36. Forward Provenance

Forward traversal answers:

```text
What was derived from this object?
```

For example:

```text
Event E1
    ↓
Measurement M1
    ↓
Pattern P1
    ↓
Analysis A1
```

---

# 37. Backward Provenance

Backward traversal answers:

```text
What produced this object?
```

For example:

```text
Analysis A1
    ↓
Pattern P1
    ↓
Measurement M1
    ↓
Events E1..E100
```

Both directions may be valuable for debugging and reproducibility.

---

# 38. Provenance Granularity

Not every transformation requires references to every internal operation.

The provenance contract should capture enough information to establish semantic origin without unnecessarily recording implementation noise.

---

# 39. Provenance vs Audit

Provenance describes analytical origin and derivation.

Audit describes accountability and security-relevant activity.

For example:

```text
Provenance:
    Analysis A came from Measurement M.

Audit:
    User X requested Analysis A.
```

They are separate concerns.

---

# 40. Provenance vs Observability

Operational telemetry is not analytical provenance.

For example:

```text
log:
    processor started

provenance:
    measurement derived using processor version 4
```

Logs and traces may help diagnose provenance but do not replace it.

---

# 41. Provenance vs Identity

Identity answers:

```text
What object is this?
```

Provenance answers:

```text
Where did this object come from?
```

An object's identity must not encode its entire provenance.

---

# 42. Provenance Immutability

Once a historical derived object has been persisted, its provenance should be logically immutable.

If provenance information is corrected, the correction must itself be explicit.

Historical provenance must not silently change.

---

# 43. Provenance Ownership

A component producing a derived object is responsible for providing the provenance information required by its contract.

Higher layers may enrich provenance but should not silently discard required information.

---

# 44. Provenance Enrichment

Provenance enrichment should produce a semantically equivalent provenance value with additional information.

For example:

```text
existing provenance
    +
execution environment
```

The original derivation meaning must remain intact.

---

# 45. Provenance Translation

When information crosses a boundary, provenance may be translated.

For example:

```text
external source provenance
        ↓
ingestion provenance
        ↓
internal event provenance
```

Translation must preserve source identity and historical meaning where possible.

---

# 46. Provenance and Corrections

A correction must preserve traceability.

For example:

```text
Original Measurement
        ↓
Correction
        ↓
Corrected Measurement
```

The corrected object should not simply overwrite the original without a semantic record when historical traceability is required.

---

# 47. Provenance and Replay

Replay may reproduce a result without creating a new historical origin.

The distinction between:

```text
original derivation
```

and:

```text
reproduction run
```

must remain explicit.

---

# 48. Run Identity

A replay run may have:

```text
RunId = R42
```

This does not replace the provenance of the original result.

Run identity identifies an execution.

Provenance identifies derivation.

---

# 49. Reproducibility

For a reproducible result:

```text
Inputs
+
Effective Configuration
+
Algorithm/Component Version
+
Relevant Context
+
Required State
```

must be sufficient to reproduce the result according to the Determinism Model.

Provenance should make these dependencies discoverable where required.

---

# 50. External Dependencies

If an external dependency materially affects a result, provenance should identify the relevant dependency/version or captured input.

Examples:

```text
market feed version
external API response
database snapshot
reference dataset
library version
```

---

# 51. Dynamic External Data

Live external data should not be assumed reproducible merely because its source is recorded.

When reproducibility matters, the relevant input may need to be:

```text
captured
versioned
snapshotted
recorded
replayed through a deterministic substitute
```

The External Dependency Model governs the exact mechanism.

---

# 52. Provenance and Serialization

Serialization itself may be part of provenance when representation affects downstream semantics.

However, ordinary serialization/deserialization should not automatically create a new analytical derivation.

---

# 53. Provenance and Storage

Persistence may record provenance.

Storage metadata such as:

```text
row ID
file offset
database key
```

must not automatically become analytical provenance.

Storage identity and logical provenance remain separate.

---

# 54. Context API Shape

The initial conceptual API should resemble:

```cpp
namespace evolution::context
{

class Context;

}
```

The exact representation of entities, dimensions, environment, and validity remains deferred.

---

# 55. Provenance API Shape

The initial conceptual API should resemble:

```cpp
namespace evolution::provenance
{

class Provenance;

}
```

Provenance should be a value-oriented object rather than an implicit property stored in a global registry.

---

# 56. Context Attachment

Core objects may optionally carry context when their contract requires it.

Examples:

```text
Event
State
Measurement
Pattern
Analysis
Decision
```

Context must not be forced onto every object merely because the generic representation permits it.

---

# 57. Provenance Attachment

Derived objects should carry or reference provenance when their contract requires traceability.

Examples:

```text
Measurement
Pattern
Analysis
Snapshot
Derived State
```

Primitive temporary values do not necessarily require persistent provenance.

---

# 58. No Universal Metadata Bag

EVolution will not introduce a generic:

```text
map<string, any>
```

as the universal mechanism for Context or Provenance.

Such a structure makes semantic contracts invisible and encourages unrelated concepts to become arbitrary metadata.

---

# 59. Extensibility

Context and Provenance must remain extensible without requiring Core to understand every domain-specific field.

Domain-specific structures may be introduced through explicit extension mechanisms.

Extension must preserve semantic typing.

---

# 60. Testing Requirements

Tests must cover:

```text
Context identity
Context scope
Context validity
Static context
Dynamic context
Historical context
Context projection
Context ownership
Context serialization

Provenance source
Provenance inputs
Transformation identity
Algorithm version
Configuration identity
Component version
Environment dependencies
Forward provenance
Backward provenance
Provenance immutability
Provenance enrichment
Provenance translation
Replay provenance
```

---

# 61. Deferred Decisions

This ADR does not select:

* exact Context representation
* exact Provenance representation
* universal entity model
* dimension storage
* provenance graph storage
* provenance query engine
* provenance serialization format
* context registry
* provenance database
* audit implementation
* observability implementation
* distributed provenance protocol
* automatic provenance collection mechanism

---

# Decision Summary

```text
Context:
    Circumstances and scope surrounding an object or operation

Provenance:
    Origin and derivation history

Domain Context:
    Domain-specific circumstances

Execution Context:
    Runtime conditions affecting execution

Context validity:
    Explicit when historical correctness matters

Context inheritance:
    Explicit

Provenance inputs:
    References to contributing objects

Provenance transformation:
    Transformation responsible for derivation

Algorithm version:
    Captured when materially relevant

Configuration:
    Effective configuration captured when materially relevant

RunId:
    Identifies an execution, not historical origin

Audit:
    Separate

Observability:
    Separate

Identity:
    Separate

Universal metadata map:
    Rejected

Provenance mutation:
    Rejected for historical objects

Context mutation:
    Must not silently alter historical meaning
```

## Invariant

**Context describes the circumstances and scope in which an object or operation exists, while Provenance describes its origin and derivation; both are explicit, value-oriented, and semantically typed, and neither is replaced by a universal metadata map, object identity, audit trail, or operational telemetry.**

