# ADR 0056: Identity API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent logical identities using a strongly typed generic `Id<Tag>` abstraction.

The identity API will distinguish:

```text
Logical Identity
Identity Generation
Identity Scope
Identity Validity
Identity Equality
Identity Serialization
```

The identity representation will remain independent of:

* memory addresses
* object lifetime
* storage keys
* database row IDs
* serialization format
* processor execution
* physical deployment

The initial implementation will provide a generic identity mechanism and domain-specific aliases.

---

# 1. Primary Identity Type

The conceptual API is:

```cpp
template <typename Tag>
class Id;
```

Example:

```cpp
struct EventTag;
struct MetricTag;
struct AnalysisTag;

using EventId = Id<EventTag>;
using MetricId = Id<MetricTag>;
using AnalysisId = Id<AnalysisTag>;
```

The tag provides compile-time semantic separation.

---

# 2. Type Separation

The following must not be implicitly interchangeable:

```cpp
EventId
MetricId
AnalysisId
ProcessorId
GraphId
RunId
```

For example:

```cpp
EventId event_id;
MetricId metric_id;
```

must not allow:

```cpp
MetricId id = event_id;
```

through implicit conversion.

---

# 3. Identity Is Not Object Address

An object's memory address is never its logical identity.

Therefore:

```cpp
&object
```

must not be used as an EVolution logical identifier.

Moving an object in memory must not change its logical identity.

---

# 4. Identity Is Not Object Lifetime

Object construction and destruction are implementation events.

Logical identity may survive:

```text
Process restart
Serialization
Deserialization
Storage migration
Replay
Caching
Object reconstruction
```

An object's C++ lifetime therefore does not define its logical identity.

---

# 5. Identity Is Not Storage Identity

Storage implementations may have their own identifiers:

```text
Database row ID
File offset
Object-store key
Storage engine internal ID
```

These are storage identities.

They must not automatically become EVolution logical identities.

A storage implementation may maintain a mapping:

```text
Logical Identity
       ↕
Storage Identity
```

---

# 6. Identity and Version

Identity and version are separate.

Conceptually:

```text
Identity = which logical object?

Version = which representation/revision?
```

For example:

```text
EventId = E123
Version = 4
```

does not mean that the version is part of the logical identity.

Whether a particular object type is versioned is defined by that object's contract.

---

# 7. Identity Scope

Every persistent identity has a scope in which uniqueness is guaranteed.

Examples:

```text
EventId
    → event identity namespace

ProcessorId
    → processor-instance identity namespace

RunId
    → execution-run namespace
```

The scope must be defined by the owner of the identity type.

An identifier must not be assumed globally unique unless its contract guarantees global uniqueness.

---

# 8. Identity Validity

The API should provide an explicit way to determine whether an identifier contains a valid identity.

Conceptually:

```cpp
id.valid()
```

An invalid identifier represents the absence of a usable identity.

It is not automatically an error.

The exact representation of an invalid ID is implementation-defined.

---

# 9. No Universal Sentinel

The implementation must not expose a magic identifier value as the universal invalid identity unless the representation contract explicitly defines it.

For example, code should not depend on:

```text
0
-1
UINT64_MAX
```

meaning "invalid" merely because of the chosen physical representation.

---

# 10. Default Construction

Whether:

```cpp
Id<Tag> id{};
```

creates an invalid identifier or is prohibited will be decided by the concrete C++ implementation.

If default construction exists, its semantics must be explicit.

A default-created identity must never accidentally be interpreted as a valid persistent identity.

---

# 11. Identity Equality

`Id<Tag>` must support equality comparison.

Two IDs are equal when they refer to the same logical identity within their identity scope.

Conceptually:

```cpp
a == b
```

means:

```text
same logical identity
```

It does not mean:

```text
same C++ object
same storage location
same version
same serialized bytes
```

---

# 12. Identity Ordering

Ordering is not inherently part of identity.

The API should not automatically provide semantic ordering merely because the physical representation can be ordered.

For example:

```text
EventId(100) < EventId(200)
```

does not imply:

```text
Event 100 happened before Event 200
```

Domain or processing sequence must provide that meaning.

An implementation may provide an ordering operation for containers, but such ordering must not be interpreted as domain ordering.

---

# 13. Hashing

Identity types should be usable as keys in hash-based containers.

Conceptually:

```cpp
std::unordered_map<EventId, Event>;
```

should be possible.

Hash equality must preserve identity equality:

```text
a == b
    ⇒
hash(a) == hash(b)
```

Hash values must not be treated as identities themselves.

---

# 14. Identity Generation

Identity generation is separate from identity representation.

Conceptually:

```text
Identity Generator
       ↓
      Id<Tag>
```

The identity type itself should not need to know how its value was generated.

---

# 15. Generation Strategies

Possible generation strategies include:

```text
Monotonic integer
Random identifier
UUID
ULID
Domain-generated identifier
Content-derived identifier
```

No single universal strategy is selected by this ADR.

Different identity domains may eventually require different generation strategies.

---

# 16. Generated vs Natural Identity

Some domain objects may have naturally supplied identifiers.

For example:

```text
External source record ID
Transaction ID
Hand ID
Market trade ID
```

An external identifier may become an EVolution logical identity only if the domain explicitly defines that relationship.

Otherwise:

```text
External Identity
       ≠
EVolution Identity
```

by default.

---

# 17. Composite Identity

Some domain objects may naturally require multiple fields to identify them.

For example:

```text
(source, external_id)
```

may form a unique identity within an ingestion domain.

Composite identity is allowed.

The composition must be explicitly defined by the owning domain.

---

# 18. Content-Derived Identity

A content hash may be used as an identity when the object contract explicitly defines identity as content-derived.

However:

```text
content equality
```

does not automatically mean:

```text
logical identity
```

Therefore hashes must not be treated as universal identities.

---

# 19. Identity Generation and Replay

Replay requires special care.

Reprocessing an existing historical object should normally preserve its logical identity when the replay represents the same historical object.

For example:

```text
Event E123
    ↓ replay
Event E123
```

rather than silently generating:

```text
Event E987
```

unless the replay operation explicitly creates a new logical object.

---

# 20. Replay Execution Identity

The replay itself has a separate identity.

For example:

```text
EventId = E123
RunId   = R456
```

The event identity identifies the event.

The run identity identifies the execution that processed it.

These must never be conflated.

---

# 21. Derived Object Identity

A derived object may receive a new logical identity.

For example:

```text
Events
   ↓
Measurement M123
```

The measurement identity identifies the measurement.

Its provenance records that it was derived from particular events.

The measurement ID should not simply be treated as an event ID.

---

# 22. Deterministic Derived Identity

A derived object may use deterministic identity generation when the object contract requires stable identity across recomputation.

For example:

```text
same logical input
+
same semantic derivation
→
same derived identity
```

This is not required universally.

If recomputation represents a new analytical result/version/run, a new identity may be appropriate.

---

# 23. Identity and Provenance

Provenance should reference logical IDs.

Example:

```text
Measurement M42
    provenance:
        Event E1
        Event E2
        Processor P7
        Run R9
```

The provenance relationship is independent of object addresses and storage locations.

---

# 24. Identity and Corrections

A correction must explicitly define identity semantics.

Possible models include:

```text
Same identity + new version
```

or:

```text
New identity
    +
relation to previous identity
```

The appropriate model depends on the object's historical semantics.

Corrections must not silently create ambiguity about which logical object is authoritative.

---

# 25. Identity and Deletion

Deleting a stored representation does not necessarily mean that the logical identity never existed.

If historical provenance refers to an object that has been removed due to retention policy, the identity may remain in provenance even if the object itself is no longer retrievable.

Retention semantics are defined by the Storage Model.

---

# 26. Identity and Caching

Cache keys may use logical identity.

However:

```text
logical identity
```

and:

```text
cache key
```

are distinct concepts.

A cache may additionally require:

```text
version
configuration
query parameters
execution context
```

to distinguish cached representations.

---

# 27. Identity and Serialization

Serialization must preserve logical identity where the serialized object represents the same logical object.

For example:

```text
Object
   ↓ serialize
Bytes
   ↓ deserialize
Object'
```

should preserve:

```text
Object.id == Object'.id
```

when the serialization contract represents the same logical object.

---

# 28. Identity and Schema Version

Schema version is not identity.

For example:

```text
EventId = E123
SchemaVersion = 2
```

does not create a different event merely because the serialization schema changed.

---

# 29. Identity and Configuration

Configuration identity is separate from component identity.

For example:

```text
ProcessorId      = P123
ConfigurationId  = C456
```

The processor identifies the logical processor instance.

The configuration identifies the effective configuration when configuration identity is required.

---

# 30. Identity and Extension Selection

Extension implementation identity is distinct from the identity of an instantiated component.

Conceptually:

```text
ExtensionDefinition
    ↓ selected
ExtensionInstance
```

Therefore:

```text
ExtensionId
    ≠
ProcessorId
```

even when the processor is implemented by that extension.

---

# 31. Identity and Execution

Execution identity is distinct from all semantic object identities.

The system may need:

```text
RunId
ExecutionId
OperationId
RequestId
TraceId
SpanId
```

These identify different things.

They must not be collapsed into one universal identifier.

---

# 32. Identity and Correlation

Correlation is a relationship between related operations or objects.

A correlation identifier does not replace logical identity.

For example:

```text
OperationId = O123
EventId     = E456
Correlation = C789
```

may all refer to the same processing workflow while identifying different entities.

---

# 33. Identity and Sequence

Identity and sequence are separate.

For example:

```text
EventId  = E123
Sequence = 42
```

means:

```text
E123 identifies the event
42 identifies its sequence position within the relevant sequence scope
```

Sequence numbers may repeat across different streams.

---

# 34. Identity and Time

Identity does not imply temporal ordering.

Two objects may have:

```text
EventId A
EventId B
```

with event times:

```text
B < A
```

without violating identity semantics.

Temporal ordering is provided by the time model and relevant domain/processing contract.

---

# 35. Identity and Equality

Identity equality and object equality are different.

Two representations may:

```text
have the same logical identity
```

while differing in:

```text
version
cached state
representation
storage location
```

Conversely, two objects with identical content may have different logical identities.

---

# 36. Identity Copy Semantics

Copying an object does not universally imply copying or changing its identity.

Each object type must define copy semantics.

For an immutable historical value, copying normally preserves its logical identity.

For a newly created independent object, the creation API should explicitly generate a new identity.

---

# 37. Identity Move Semantics

Moving an object must not change its logical identity.

Conceptually:

```text
Object A
    id = X

move(A → B)

Object B
    id = X
```

The implementation must not derive identity from object address.

---

# 38. Identity Lifetime

A logical identity may outlive the in-memory object carrying it.

Therefore:

```text
C++ object lifetime
    ≠
logical identity lifetime
```

This distinction is required for:

* persistence
* provenance
* replay
* caching
* distributed execution
* recovery

---

# 39. Identity Scope and Distributed Execution

If EVolution later executes across processes or hosts, logical identity must remain meaningful across those boundaries.

The identity mechanism must therefore not depend on:

```text
process-local pointer
thread ID
host address
container position
```

---

# 40. Identity Security

Identifiers should not automatically be treated as secrets.

However, some domain-specific identifiers may reveal sensitive information.

Security classification belongs to the owning domain/interface contract.

The generic Core identity mechanism must not assume that every identifier is either public or confidential.

---

# 41. Identity Generation Failure

If identity generation can fail, the generation operation must use:

```cpp
Result<Id<Tag>>
```

rather than returning an invalid ID while silently hiding the failure.

If the chosen generation mechanism is guaranteed not to fail under its contract, ordinary value return may be appropriate.

---

# 42. Identity Serialization

Identity serialization must be explicit.

A serialized identifier must preserve the identity semantics rather than merely exposing internal memory representation.

For persistent/wire representations, the encoding must be:

```text
Stable
Unambiguous
Versionable where necessary
```

The exact encoding remains deferred.

---

# 43. Identity ABI

`Id<Tag>` is primarily a C++ abstraction.

If an identifier crosses a C ABI, it must be translated into an explicitly defined C representation.

C++ template types must not be assumed to provide a stable external ABI.

---

# 44. Testing Requirements

The implementation must test:

```text
Type separation
Equality
Invalid identity semantics
Hashing
Copy
Move
Serialization round-trip
Persistence/reconstruction
Replay identity preservation
Generated identity uniqueness within declared scope
```

Where identity generation is probabilistic, testing must follow the guarantees of the chosen generation algorithm rather than attempting to prove mathematical global uniqueness through tests.

---

# 45. Deferred Decisions

This ADR does not select:

* integer vs UUID vs ULID
* identifier byte width
* text representation
* binary representation
* generation algorithm
* global vs scoped generation service
* distributed ID generator
* persistent ID allocation mechanism
* content-hash algorithm
* canonical serialization format
* C ABI representation
* database mapping strategy

Those decisions belong to implementation or later specialized ADRs.

---

# Decision Summary

```text
Primary type:                  Id<Tag>
Identity separation:           Compile-time
Memory address as identity:    Rejected
Object lifetime as identity:   Rejected
Storage ID as logical ID:      Rejected by default
Identity/version:              Separate
Identity/equality:             Distinct concepts
Identity/sequence:             Separate
Identity/time:                 Separate
Identity/correlation:          Separate
Identity/provenance:           Referenced, not replaced
Identity generation:           Separate mechanism
Content hash:                  Not universal identity
Replay:                        Preserve identity when same object
Derived objects:               May receive new identity
Hashing:                       Supported
Ordering:                      Not semantic by default
Serialization:                 Explicit
C ABI:                         Translation required
Physical representation:       Deferred
```

## Invariant

**An EVolution logical identity identifies an object independently of its memory address, object lifetime, storage representation, execution location, serialization format, and version; semantically different identity domains must remain strongly typed and must not become interchangeable merely because they share a physical representation.**

