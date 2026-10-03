# ADR 0032 — Serialization and Wire Representation

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution objects exist in multiple representations:

```text
External Data
    ↓
Serialized / Wire Representation
    ↓
In-Memory Representation
    ↓
Processing
    ↓
In-Memory Result
    ↓
Serialized / Persisted Representation
```

Serialization may be required for:

* persistence
* inter-process communication
* networking
* checkpoints
* replay
* caching
* configuration
* external interfaces
* import/export
* testing
* debugging

The in-memory C++ representation should not become implicitly coupled to a particular byte format.

A C++ object layout is not a stable wire representation.

The architecture therefore needs an explicit serialization boundary.

## Decision

EVolution treats **serialization as a translation between semantic objects and external representations**.

Conceptually:

```text
Semantic Object
      ↓
Serialization
      ↓
Representation
      ↓
Transport / Storage
```

and:

```text
Representation
      ↓
Deserialization
      ↓
Semantic Object
```

Serialization is not merely copying memory.

It must preserve the semantic information required by the object's contract.

## Serialization vs Object Representation

The following are distinct:

```text
Logical Object
    ↓
In-memory C++ representation
    ↓
Serialized representation
    ↓
Transport/storage representation
```

No layer should assume that the others are identical.

In particular:

> EVolution must not use C++ object memory layout as a portable serialization format.

## Serialization Boundary

Serialization belongs at explicit boundaries such as:

```text
Storage
IPC
Network
External Interface
Checkpoint
Cache
Import / Export
```

Core semantic types should not depend on a specific serialization library.

## Semantic Preservation

A serializer must preserve all fields required by the semantic contract of the serialized object.

For example, an Event may require preservation of:

```text
id
timestamp
timestamp status
type
source
sequence
payload
```

A serializer must not silently discard semantically required information.

## Optional Information

Optional fields may be omitted only when the object's contract defines an equivalent default or absence semantics.

For example:

```text
missing
unknown
empty
zero
default
```

must not be treated as interchangeable without an explicit contract.

## Identity

Logical identity must survive serialization when the object contract requires persistent identity.

For example:

```text
EventId
AnalysisId
MetricId
```

must not be replaced by:

```text
memory address
container position
serialization order
```

Serialization may use a different physical identifier internally, but the logical identity must remain recoverable.

## Version

Object identity and version are distinct.

A serialized representation may contain:

```text
object identity
object version
schema version
```

where required.

A schema version identifies the representation contract.

It does not automatically identify the logical object's version.

## Schema Version

A serialized format that may evolve should have an explicit schema/version mechanism.

Conceptually:

```text
Representation
{
    schema_version
    payload
}
```

The exact encoding is deferred.

## Schema Evolution

Serialization formats should be designed with evolution in mind.

Potential changes include:

```text
add field
remove field
rename field
change representation
change semantics
```

These are not equivalent.

Adding an optional field may be backward-compatible.

Changing the meaning of an existing field is a semantic change even if its byte representation remains unchanged.

## Compatibility

Compatibility must be distinguished between:

```text
Representation compatibility
Semantic compatibility
```

A representation may be parseable while no longer representing the same semantic contract.

Therefore successful deserialization does not by itself prove semantic compatibility.

## Deserialization Validation

Deserialization must validate the representation before producing a valid semantic object.

Conceptually:

```text
Bytes
  ↓
Parse
  ↓
Structural Validation
  ↓
Semantic Validation
  ↓
Object
```

Malformed or incompatible data must not silently become valid domain objects.

## Serialization Errors

Serialization/deserialization operations use the `Result<T>` error model.

Possible failures include:

```text
InvalidInput
Unsupported
ProcessingFailure
ResourceExhausted
InternalFailure
```

The exact error code depends on the operation.

Malformed external data is normally an input/validation failure rather than an internal failure.

## Partial Deserialization

A partially parsed representation must not automatically become a valid semantic object.

For example:

```text
header parsed
payload incomplete
```

must not be exposed as a complete Event unless the contract explicitly supports partial objects.

## Unknown Fields

Whether unknown fields are accepted depends on the format and compatibility contract.

Conceptually, a reader may support:

```text
REJECT_UNKNOWN
IGNORE_UNKNOWN
PRESERVE_UNKNOWN
```

No universal policy is selected.

If unknown fields contain information required for future round-tripping, discarding them may prevent lossless conversion.

## Unknown Enum Values

Enum-like fields require explicit handling.

Possible policies include:

```text
reject
preserve unknown representation
map to explicit UNKNOWN value
```

A parser must not silently map an unknown semantic value to an unrelated known value.

## Required Fields

Required fields must be explicitly defined by the semantic contract.

A serializer must not rely on:

```text
zero initialization
empty string
null pointer
```

as accidental representations of missing required information.

## Null and Absence

Serialized formats may distinguish:

```text
absent
null
empty
zero
unknown
```

EVolution semantic contracts must define which distinctions matter.

Serialization must preserve distinctions that affect interpretation.

## Temporal Serialization

Temporal information follows the Time Model.

Absolute timestamps should use an unambiguous UTC representation.

Temporal knowledge state must be preserved:

```text
KNOWN
ESTIMATED
UNKNOWN
```

For example:

```text
event_time = T
status = ESTIMATED
```

must not deserialize as:

```text
event_time = T
status = KNOWN
```

unless an explicit semantic transformation justifies that change.

## Duration Serialization

Durations represent elapsed time.

They should not be serialized as timezone-dependent calendar values.

The representation must preserve sufficient precision for the contract.

## Time Precision

Serialization must not silently reduce temporal precision when that precision is semantically relevant.

For example:

```text
microseconds
```

must not silently become:

```text
seconds
```

if the object contract requires microsecond precision.

## Numeric Values

Numeric serialization must define:

```text
type
width
signedness
precision
unit
```

where relevant.

A raw number without semantic unit information may be ambiguous.

## Floating-Point Serialization

Floating-point values require explicit representation semantics.

The system must distinguish:

```text
exact integer
floating-point approximation
decimal representation
```

when the distinction matters.

Serialization must not claim exact numerical reproduction when the representation cannot provide it.

## NaN and Infinity

If floating-point values can contain:

```text
NaN
+Infinity
-Infinity
```

the serialization format must define whether and how they are represented.

Unsupported special values must result in explicit failure rather than silent conversion.

## Text Encoding

Text serialization must define its encoding.

Unicode-capable external representations should use an explicit encoding rather than relying on the host locale.

The Core must not depend on the host machine's locale for semantic string interpretation.

## Binary Data

Binary payloads should remain binary when the serialization format supports them.

Encoding binary data as text may introduce:

```text
size overhead
copying
encoding cost
```

and should be an explicit representation choice.

## Endianness

Portable binary formats must define byte order.

The architecture must not assume that sender and receiver have identical host endianness.

## Alignment and Padding

C++ structure padding and alignment are implementation details.

They must not become implicit wire-format semantics.

## Memory Layout

The serialized representation must not depend on:

```text
pointer values
vtable layout
padding bytes
allocator state
object addresses
container implementation
```

## Pointers

Raw pointers and references must not be serialized as object identity.

A pointer may only be serialized if the representation explicitly defines it as an external address/handle with appropriate semantics.

Ordinary EVolution object serialization should use logical identities instead.

## Containers

C++ container memory layout is not a wire contract.

For example:

```text
std::vector
std::unordered_map
std::string
```

may have implementation-specific representations.

Serialization should represent their semantic contents rather than their memory layout.

## Ownership

Serialization does not transfer ownership of the original in-memory object.

For example:

```text
serialize(object)
```

does not imply:

```text
object destroyed
```

or:

```text
serialized bytes own object
```

The serialized representation has its own lifetime.

## Deserialization Ownership

Deserialization normally creates a new in-memory representation with its own ownership.

Conceptually:

```text
bytes
  ↓
new object
```

The resulting object's lifetime is independent of the input byte buffer unless the API explicitly defines a zero-copy representation.

## Zero-Copy Deserialization

Zero-copy parsing may be used as an optimization.

For example:

```text
serialized buffer
    ↓
borrowed view
```

is valid only while the backing buffer remains alive.

The semantic object must not outlive its required storage.

## Zero-Copy and Mutation

A zero-copy view into serialized data should normally be immutable unless the format explicitly supports safe mutation.

Mutating serialized backing storage can invalidate multiple views and must therefore be explicit.

## Canonical Representation

Some use cases require a canonical representation.

Examples:

```text
content hashing
digital signatures
deduplication
reproducibility
cache keys
```

Canonicalization means semantically equivalent objects produce a defined representation.

The exact canonicalization rules are deferred.

## Semantic Equality vs Serialized Equality

Two objects can be semantically equivalent while having different serialized representations.

For example:

```text
field ordering
optional default fields
encoding details
```

may differ.

Therefore:

```text
serialized bytes equal
```

is not automatically equivalent to:

```text
logical objects equal
```

unless the representation is explicitly canonical.

## Hashing

Hashes may be calculated over:

```text
serialized bytes
logical content
canonical representation
```

These are different concepts.

A content hash should not automatically become the object's logical identity.

## Serialization and Identity

The Identity Model remains authoritative.

Serialization must preserve or reconstruct logical identity where required.

A serializer must not create a new logical identity merely because a new in-memory object was constructed during deserialization.

## Serialization and Provenance

Provenance may need to identify:

```text
source representation
schema version
serializer version
transformation
```

when these affect interpretation or reproducibility.

Serialization itself does not automatically become analytical provenance.

## Serialization and Configuration

Configuration may be serialized for:

```text
persistence
replay
checkpointing
reproducibility
external APIs
```

The serialized configuration must represent the effective configuration where that is what the consumer needs.

A source file path alone is not sufficient to identify effective configuration.

## Serialization and Execution Context

Execution context may need serialization when it is part of a checkpoint or reproducibility record.

Incidental runtime details should not automatically be serialized.

## Serialization and State

State serialization must preserve enough information to reconstruct a valid state according to the processor contract.

A state representation should identify the semantic version of the state when compatibility matters.

## Checkpoint Serialization

Checkpoint serialization is more restrictive than ordinary object serialization.

A checkpoint may need:

```text
processor version
graph version
configuration identity
state
input position
execution metadata
```

as established by the recovery model.

A state snapshot alone is not necessarily a valid checkpoint.

## Serialization and Recovery

Recovery must validate that serialized state is compatible with:

```text
processor
configuration
graph
schema
version
```

before restoring it.

Incompatible state must not be silently interpreted under a different semantic contract.

## Migration

When serialized data changes between versions, migration may be required.

Conceptually:

```text
Old Representation
       ↓
Migration
       ↓
Current Representation
       ↓
Object
```

Migration is an explicit transformation.

It must not silently alter semantic meaning.

## Migration Provenance

When migration materially changes a representation, provenance should be capable of identifying:

```text
source schema/version
migration version
target schema/version
```

where required.

## Backward Compatibility

Readers may support older representations where explicitly implemented.

A newer reader must not assume that every older representation is compatible.

Compatibility must be defined per schema/version.

## Forward Compatibility

A reader encountering a newer representation may:

```text
reject
ignore compatible unknown information
preserve unknown information
```

according to the format contract.

Forward compatibility must not silently discard semantically required information.

## Serialization Security

Deserialization is an input boundary and must be treated as untrusted where data originates externally.

It must protect against:

```text
malformed input
resource exhaustion
unexpected sizes
invalid nesting
integer overflow
invalid references
unexpected types
```

The exact security mechanisms are implementation-specific.

## Resource Limits

Deserializers must respect explicit resource limits.

Examples:

```text
maximum message size
maximum nesting depth
maximum collection size
maximum string size
maximum decompression size
```

Malformed input must not cause uncontrolled resource consumption.

## Compression

Compression is a transport/storage concern rather than semantic serialization.

Conceptually:

```text
Object
 ↓
Serialization
 ↓
Compression
 ↓
Transport / Storage
```

or the reverse during reading.

Compression does not change object identity.

## Encryption

Encryption is a security/transport/storage concern.

Conceptually:

```text
Object
 ↓
Serialization
 ↓
Encryption
 ↓
Transport / Storage
```

Encryption must not be confused with serialization.

## Authentication and Integrity

Message authentication or integrity mechanisms may wrap serialized representations.

These are separate from semantic serialization.

## Network Wire Formats

Network protocols may define their own wire representations.

An EVolution internal object should not automatically become a network protocol merely because it can be serialized.

External protocol adapters belong at the appropriate Interface or Ingestion boundary.

## IPC Formats

Inter-process communication may use serialized objects.

The IPC contract should define:

```text
schema
version
ownership
framing
message boundaries
failure behavior
```

The serialization layer should remain independent of the IPC transport where practical.

## Message Framing

A byte stream does not inherently contain message boundaries.

Transport protocols using streams may require explicit framing:

```text
length
delimiter
fixed-size header
record structure
```

Framing is distinct from serialization semantics.

## Persistence Formats

Storage may use serialization for persistence.

The Storage Model remains responsible for:

```text
retention
durability
queryability
source-of-truth semantics
```

Serialization only defines representation.

## Import and Export

External import/export formats may intentionally differ from internal representation.

For example:

```text
External Format
    ↓
Ingestion Adapter
    ↓
Internal Semantic Object
```

and:

```text
Internal Semantic Object
    ↓
Export Adapter
    ↓
External Format
```

No requirement exists for internal and external formats to be identical.

## Domain-Specific Serialization

Domain objects may require domain-specific serializers.

For example:

```text
PokerHand
MarketTick
NetworkEvent
```

may have different representations.

The generic Core serialization mechanism should not encode domain semantics.

## Core Serialization Responsibility

Core may provide generic mechanisms for:

```text
identity
time
basic values
envelopes
generic metadata
```

but must not become coupled to domain-specific schemas.

## Versioned Domain Schemas

Domain-specific serialized objects should own their schema evolution.

A Poker representation may evolve independently from a Crypto representation.

The generic serialization infrastructure should support this without requiring domain knowledge.

## Serialization and C++ ABI

A serialized representation should remain independent of the compiler's ABI.

Changing:

```text
compiler
standard library
build mode
platform
```

must not automatically invalidate the semantic serialization format unless the format contract explicitly permits it.

## Serialization and C ABI

C ABI interfaces may expose serialized buffers.

The ABI must explicitly define:

```text
buffer ownership
length
alignment requirements
encoding
lifetime
destruction
```

## Streaming Serialization

Large objects may be serialized incrementally.

For example:

```text
Object
 ↓
Encoder
 ↓
chunks
 ↓
Transport
```

The streaming contract must distinguish:

```text
not started
in progress
complete
failed
cancelled
```

A partial serialization must not automatically be interpreted as a complete object.

## Serialization Cancellation

Long serialization/deserialization operations may support cancellation.

Cancellation must remain distinct from malformed input or serialization failure.

## Serialization Determinism

If serialized bytes participate in:

```text
hashing
signatures
reproducibility
deduplication
```

the serializer may need deterministic/canonical output.

Otherwise semantically equivalent objects may produce different byte representations.

## Serialization and Reproducibility

A deterministic analytical result does not automatically imply deterministic serialization.

If reproducibility requires byte-identical output, canonical serialization must be explicitly defined.

Otherwise semantic equivalence remains the default target.

## Testing Serialization

Serialization implementations should test:

```text
object → serialize → deserialize → equivalent object
```

and where required:

```text
object → serialize
same object → serialize
    ↓
identical canonical representation
```

Additional tests should cover:

```text
invalid data
truncated data
unknown fields
version mismatch
boundary values
large values
temporal states
identity preservation
resource limits
```

## Round-Trip Semantics

A valid round trip should preserve the fields guaranteed by the object's contract.

Conceptually:

```text
Object A
  ↓ serialize
Representation
  ↓ deserialize
Object B
```

must satisfy the declared equivalence relation:

```text
A ≡ B
```

The equivalence relation is object-specific.

## Serialization Failure Must Not Corrupt Semantic State

A failed serialization operation must not partially mutate the source semantic object unless the API explicitly defines such behavior.

Similarly, failed deserialization must not expose partially constructed invalid objects as successful results.

## Consequences

### Positive

* In-memory architecture remains independent of wire/storage formats.
* Schema evolution becomes explicit.
* Identity, temporal semantics, provenance, and configuration can be preserved correctly.
* External protocols can evolve independently from internal objects.
* Zero-copy and streaming optimizations remain possible.
* Security/resource validation has a defined boundary.

### Negative

* Every external representation requires explicit serialization logic.
* Schema evolution requires migration/compatibility decisions.
* Canonical serialization can add implementation complexity.
* Zero-copy deserialization requires careful lifetime management.

## Deferred Decisions

This ADR does not select:

```text
JSON
CBOR
MessagePack
Protocol Buffers
FlatBuffers
Cap'n Proto
Avro
custom binary format
serialization framework
schema registry
canonical encoding
compression algorithm
encryption format
network framing protocol
```

These require separate decisions based on concrete requirements.

## Decision Summary

```text
Serialization:
    Explicit semantic translation boundary

C++ object layout:
    Not a wire format

Identity:
    Preserved independently of representation

Version:
    Separate from logical identity

Schema evolution:
    Explicit

Deserialization:
    Parse + validate before producing semantic object

Malformed input:
    Explicit Result/Error

Temporal status:
    KNOWN / ESTIMATED / UNKNOWN preserved

Ownership:
    Independent of serialization

Zero-copy:
    Allowed as optimization with explicit lifetime

Canonical representation:
    Required only where byte-level determinism matters

Hash:
    Not automatically logical identity

Checkpoint:
    Requires more than serialized processor state

External formats:
    May differ from internal representation

Security:
    Deserialization treated as an input boundary

Resource limits:
    Explicit

Physical serialization technology:
    Deferred
```

## Invariants

1. C++ object memory layout is never implicitly treated as a portable serialization format.
2. Serialization preserves all semantic information required by the serialized object's contract.
3. Deserialization must validate input before producing a valid semantic object.
4. Malformed or incompatible data must not silently become valid analytical data.
5. Logical identity must survive serialization when persistent identity is required.
6. Logical identity must remain independent of memory address and serialized representation.
7. Object version and schema version are distinct concepts.
8. Temporal knowledge state must survive serialization without silently upgrading estimated or unknown time.
9. Missing, unknown, empty, zero, and null must remain distinct where the semantic contract requires the distinction.
10. Serialization must not depend on compiler ABI, pointer values, padding, allocator state, or container memory layout.
11. Deserialized objects must have ownership and lifetime independent of temporary input buffers unless an explicit zero-copy contract says otherwise.
12. Zero-copy representations must not outlive their backing storage.
13. Partial serialization/deserialization must not be presented as complete successful data.
14. Schema evolution must preserve semantic compatibility explicitly rather than assuming parseability implies compatibility.
15. Migration between representations is an explicit transformation.
16. Serialization failure must not silently corrupt or partially invalidate the source semantic object.
17. Serialization does not transfer ownership of the original object.
18. Content hashes and serialized representations do not automatically define logical identity.
19. Canonical serialization is required only when byte-level equality is itself a contract.
20. Checkpoint serialization must preserve the recovery information required by the Recovery Model.
21. External network, IPC, and persistence formats must remain separate from the semantic definitions of Core objects.
22. Deserialization of externally supplied data must respect explicit resource and security limits.
23. Serialization technology remains replaceable behind explicit interfaces.
24. Domain-specific serialization semantics belong to the relevant domain or boundary, not generic Core.

## Invariant

> **EVolution treats serialization as an explicit translation between semantic objects and external representations: logical meaning, identity, temporal semantics, ownership, and required provenance must be preserved according to contract, while byte layout, transport format, and serialization technology remain replaceable implementation concerns.**
