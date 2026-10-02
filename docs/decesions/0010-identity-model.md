# ADR 0010 — Identity Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution contains many kinds of logical objects:

```text
Event
State
Measurement
Time Series
Pattern
Analysis
Decision
Context Entity
Metric
Processor
Configuration
Provenance
```

These objects need a consistent way to refer to one another.

The conceptual Identity model was defined in `docs/concepts/identity.md`, but the technical identity strategy has not yet been decided.

Without an explicit model, individual components may independently introduce incompatible identifiers such as:

```text
uint64_t
UUID
string
pointer
database primary key
hash
```

This would make references, provenance, persistence, serialization, and cross-module communication inconsistent.

The identity model must therefore provide a common semantic foundation without forcing every object to use the same physical representation.

## Decision

EVolution will use **strongly typed logical identifiers for objects that require stable identity**.

An identifier represents the logical identity of an object and is independent of:

* memory address
* storage location
* database row ID
* object lifetime
* serialization format
* version
* current in-memory representation

Conceptually:

```cpp
template <typename Tag>
class Id;
```

Specific identifiers may then be represented conceptually as:

```cpp
using EventId = Id<EventTag>;
using AnalysisId = Id<AnalysisTag>;
using MetricId = Id<MetricTag>;
```

The exact implementation of `Id<Tag>` remains an implementation detail.

## Strong Typing

Identifiers for semantically different object types must not be implicitly interchangeable.

For example:

```cpp
EventId event_id;
AnalysisId analysis_id;
```

must not be usable interchangeably merely because their underlying representation happens to be the same.

Conceptually:

```cpp
load_event(analysis_id);       // invalid
load_analysis(event_id);       // invalid
```

This prevents accidental mixing of identifiers at compile time.

The exact mechanism used to implement strong typing is deferred.

## Logical Identity vs Representation

An identifier identifies a logical object.

It does not identify its physical representation.

For example:

```text
Event
  logical identity: EventId(123)

  representations:
    in-memory object
    serialized object
    database record
    cached object
```

All of these may represent the same logical event.

Therefore:

```text
memory address ≠ identity
database row ID ≠ necessarily identity
serialization position ≠ identity
```

A storage implementation may maintain its own internal identifiers, but those identifiers must not automatically become EVolution logical identities.

## Identity and Equality

Identity and equality are separate concepts.

Two objects may have:

```text
same identity
different representation
```

when they represent different versions or representations of the same logical object.

Conversely:

```text
different identities
equal values
```

may be possible when two independently created objects contain equivalent data.

Therefore value equality must not automatically be interpreted as identity equality.

The exact equality semantics of each object type are defined by that object's contract.

## Identity and Version

Identity and version represent different concepts.

For example:

```text
EventId = 42
Version = 3
```

means:

```text
logical object: 42
representation/version: 3
```

A new version does not automatically imply a new logical identity.

Whether a particular object is mutable/versioned/immutable is defined by its own contract.

For immutable objects, a correction may instead produce a new logical object and establish a provenance relationship between the two.

## Identity Scope

An identifier has a defined scope.

The minimum required scope is the object type and the identity domain in which the identifier is meaningful.

Conceptually:

```text
EventId(123)
```

does not necessarily imply that `123` is globally unique across:

```text
Analysis
Measurement
Metric
State
```

Strong typing prevents these categories from being confused.

Domain-specific identity namespaces may also exist where required.

For example:

```text
PokerPlayerId
NetworkNodeId
InstrumentId
```

may have domain-specific semantics while remaining separate from generic Core identifiers.

## Generated Identity

Objects requiring generated identity must obtain that identity from an explicit identity-generation mechanism.

Identity generation must not depend on:

```text
memory address
process-local pointer value
container position
iteration order
```

unless an object explicitly defines such a value as non-persistent local identity.

For persistent or cross-boundary objects, identity generation must provide the uniqueness guarantees required by that object's contract.

The exact generation algorithm is deferred.

Possible implementations include:

```text
monotonic integer
random identifier
UUID
ULID
domain-generated identifier
content-derived identifier
```

No specific representation is mandated by this ADR.

## Content-Derived Identity

A hash or content-derived identifier may be useful for:

* deduplication
* immutable content addressing
* cache keys
* reproducibility
* integrity verification

However, content-derived identity must not be assumed to be the universal identity mechanism.

Two logically distinct objects may contain identical data.

Therefore:

```text
content equality ≠ logical identity
```

unless the specific object contract explicitly defines identity as content-derived.

## Local and Persistent Identity

Not every internal object requires persistent identity.

A temporary implementation object may be identified by:

```text
object lifetime
local handle
container position
```

when it never crosses a relevant architectural boundary.

Persistent, externally referenced, or provenance-relevant objects should have explicit logical identity.

Therefore identity is a property of an object's contract rather than a requirement that every C++ object carry an ID.

## Identity and Provenance

Provenance references logical identities rather than physical object addresses.

For example:

```text
AnalysisId
    ↓ derived from
MeasurementId
    ↓ derived from
EventId
```

This allows provenance to remain meaningful after objects are:

* serialized
* reloaded
* moved between processes
* stored in different backends
* reconstructed from historical data

Physical storage identifiers may be recorded separately when useful for diagnostics or storage mapping.

## Identity and Storage

Storage implementations may have their own identifiers.

For example:

```text
EVolution EventId
        ↓
Storage record ID
```

These are not automatically the same identifier.

A storage backend may use:

```text
database primary key
file offset
object key
row identifier
internal handle
```

internally.

The storage implementation is responsible for maintaining the mapping between physical storage identity and EVolution logical identity.

## Identity and Serialization

When an object crosses a serialization boundary, its logical identity must remain stable when the object is intended to represent the same logical object.

For example:

```text
in-memory EventId(123)
        ↓ serialize
wire representation
        ↓ deserialize
EventId(123)
```

The serialized representation may differ completely from the in-memory representation.

The serialization format must not redefine logical identity.

## Identity and Object Lifetime

Destroying an in-memory object does not necessarily destroy its logical identity.

For example:

```text
load EventId(123)
      ↓
use
      ↓
destroy object
      ↓
load EventId(123) again
```

may produce two different C++ object instances representing the same logical event.

Therefore:

```text
C++ object lifetime ≠ logical object lifetime
```

## Identity and Copies

Copying an object normally preserves its logical identity only when the object's contract defines copying as another representation of the same logical object.

This must not be assumed universally.

Each major object type must explicitly define its copy semantics.

For example:

```text
copying an Error
    → same represented error value

copying an Event
    → depends on Event's object semantics

creating a new Event from another Event
    → may produce a new Event identity
```

Identity behavior therefore belongs to the object's contract.

## Identity and Corrections

Historical corrections must not silently overwrite identity semantics.

If an immutable historical object is found to be incorrect, the system should represent the correction explicitly.

Conceptually:

```text
Event A
   │
   │ correction
   ▼
Event B
```

The correction relationship should be representable through provenance or another explicit domain mechanism.

Whether a correction replaces, supersedes, invalidates, or coexists with the original object is domain-specific.

## Identity and Versioned Objects

For versioned objects:

```text
logical identity
       +
version
```

may identify a particular representation.

Conceptually:

```text
EventId = 42
Version = 1

EventId = 42
Version = 2
```

Both refer to the same logical identity while representing different versions.

An object type may instead define each correction as a new logical identity.

That choice must be explicitly documented by the object's contract.

## Identity Across Domains

Generic Core identifiers must not encode domain-specific meaning.

For example, Core should not assume:

```text
PokerPlayerId
CryptoWalletId
NetworkSubscriberId
```

are interchangeable with a generic `EntityId`.

A domain may define its own identity type where semantic constraints require it.

The domain-specific type may still use Core's generic identity mechanisms internally.

## Identifier Representation

This ADR intentionally does **not** mandate a specific physical identifier representation.

The following remain possible:

```text
uint64_t
uint128_t
UUID
ULID
string
opaque byte sequence
domain-specific representation
```

The representation must be selected according to the requirements of the object using it.

The implementation should prefer compact representations where identity volume and performance make this important.

Human-readable identifiers may be useful for diagnostics but are not required to be the canonical logical representation.

## Identifier Stability

An identifier intended to survive persistence or external communication must remain stable across:

```text
process restarts
serialization
deserialization
storage migration
in-memory reconstruction
```

An implementation must not regenerate such an identity merely because the object was reconstructed.

Temporary process-local identities are allowed where the object contract explicitly defines them as such.

## Identity Generation and Determinism

Deterministic replay does not require randomly generated identities to be reproduced automatically.

A replay system must distinguish:

```text
historical identity
newly generated execution identity
```

Historical objects must retain their original identities.

New objects created during replay may require deterministic identity generation if their identities participate in reproducible results.

The exact replay identity strategy is deferred to the processing/replay architecture.

## Identity and Concurrency

Identity generation must be safe under the execution model eventually selected by EVolution.

This ADR does not prescribe:

```text
mutex
atomic counter
thread-local generation
central allocator
distributed generator
```

The implementation must provide the uniqueness and stability guarantees required by the identity's scope.

## Identity Contract for Major Objects

Before implementation of a major object type, its contract should answer:

```text
Does it have logical identity?
What is its identity scope?
How is identity generated?
Is identity persistent?
Is identity immutable?
Does versioning exist?
What does copying mean?
What does reconstruction mean?
Can two objects have equal values but different identities?
How is identity represented across serialization?
```

This prevents individual modules from inventing incompatible identity semantics.

## Consequences

### Positive

* Different object types cannot accidentally exchange identifiers.
* Logical identity remains independent of storage and memory representation.
* Provenance can survive persistence and reconstruction.
* Storage backends remain implementation details.
* Domain-specific identity semantics remain in domains.
* Identity generation can evolve without changing the conceptual model.

### Negative

* Strongly typed identifiers introduce additional types.
* Some conversions between identifier representations require explicit code.
* Different object types may legitimately use different identity strategies.
* Distributed identity requirements may introduce additional complexity later.

These costs are intentional because identity mistakes can propagate across the entire architecture.

## Deferred Decisions

The following are intentionally left open:

* exact `Id<Tag>` implementation
* underlying integer/string/UUID representation
* UUID vs ULID vs integer generation
* distributed identity generation
* persistence-specific mapping
* serialization representation
* domain-specific ID conventions
* identity generation service
* content-addressed identity
* replay identity generation
* version implementation

These should be decided only when their concrete requirements become clear.

## Decision Summary

```text
Identity model:                 Strongly typed logical identifiers
Identifier interchangeability: Forbidden across semantic types
Logical identity:               Independent of representation
Storage identity:               Separate from logical identity
Memory address as identity:     Forbidden for persistent identity
Serialization:                  Preserves logical identity
Provenance:                     References logical identity
Content hash:                   Not universal identity
Temporary objects:              May omit persistent identity
Domain identities:              Allowed
Exact ID representation:        Deferred
ID generation algorithm:        Deferred
Version semantics:              Separate from identity
```

## Invariant

> **A logical identifier identifies an EVolution object independently of its memory address, storage representation, serialization format, and lifetime; semantically different identifier types must not be implicitly interchangeable.**
