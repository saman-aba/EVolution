# ADR 0042 — API and Versioning Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution contains many explicit contracts:

```text
Core types
Processor contracts
Processing graphs
Storage interfaces
Application operations
Extensions
Serialization formats
External interfaces
```

These contracts will evolve as the system develops.

Without an explicit versioning model, changes can silently break:

```text
existing data
stored checkpoints
processing graphs
extensions
applications
external clients
reproducibility
```

EVolution therefore needs to distinguish different kinds of compatibility and define when a change requires a new version.

## Decision

EVolution treats versioning as a **contract evolution mechanism**.

Versioning is applied according to the thing being versioned.

The architecture distinguishes:

```text
API version
Schema version
Component version
Algorithm version
Configuration version
Graph definition version
Extension version
Application version
Data version
```

These versions are related but are not interchangeable.

No single global version number is sufficient to describe the entire system.

## API

An API is a contract through which one component interacts with another.

Examples include:

```text
Processor API
Storage API
Application API
Interface API
Extension API
Library API
```

An API includes more than function signatures.

It may define:

```text
inputs
outputs
errors
ownership
lifetime
ordering
temporal semantics
identity
side effects
concurrency
cancellation
delivery
resource requirements
```

Therefore, a source-compatible change may still be a semantic breaking change.

## API Compatibility

Compatibility must be evaluated according to the contract.

Conceptually:

```text
Compatible
    ↓
Existing valid consumers continue to behave correctly

Incompatible
    ↓
Existing valid consumers may fail or change meaning
```

Compatibility includes semantic compatibility, not merely compilation.

## API Compatibility Dimensions

Possible dimensions include:

```text
Source compatibility
Binary compatibility
Wire compatibility
Data compatibility
Behavioral compatibility
Semantic compatibility
```

A change may preserve one while breaking another.

For example:

```text
same serialized format
+
different interpretation
```

is not semantically compatible.

## Source Compatibility

Source compatibility means existing source code can be compiled against the changed API.

This matters primarily for:

```text
C++ libraries
C libraries
development SDKs
```

Source compatibility is not guaranteed merely because semantics remain similar.

## Binary Compatibility

Binary compatibility concerns already-built consumers.

C++ binary compatibility can depend on:

```text
compiler
standard library
ABI
layout
calling convention
```

EVolution does not assume arbitrary C++ ABI stability.

Binary compatibility requirements are deployment/API-specific.

## Wire Compatibility

Wire compatibility means an external representation can continue to be exchanged between versions.

Wire compatibility follows serialization and interface contracts.

A wire-compatible representation may still contain semantically incompatible data.

## Data Compatibility

Data compatibility concerns whether stored data can still be correctly interpreted by a newer implementation.

Examples:

```text
events
measurements
time series
analysis results
checkpoints
configuration
```

Stored data must not be silently interpreted using incompatible semantics.

## Behavioral Compatibility

Behavioral compatibility means an existing consumer can continue to rely on the documented behavior.

A function signature may remain unchanged while its behavior changes.

That can still be an API-breaking change.

## Semantic Compatibility

Semantic compatibility asks:

> Does the new version preserve the meaning expected by existing consumers?

This is the primary architectural compatibility concern.

## Contract Changes

Changes should be classified according to their effect.

Conceptually:

```text
Non-breaking
Compatible extension
Behavioral change
Semantic change
Breaking change
```

The exact release policy is deferred.

## Adding Optional Information

Adding optional information may be compatible if:

```text
old consumers can ignore it
new semantics are not imposed on old consumers
serialization rules permit it
```

However, adding a field can still be breaking if it changes:

```text
requiredness
default behavior
ordering
resource requirements
```

## Removing Information

Removing a previously required field or capability is generally a contract change.

The impact depends on whether consumers rely on it.

## Changing Defaults

Changing a default is a semantic change even if no API signature changes.

For example:

```text
default ordering
default timeout
default admission policy
default timezone
default missing-data policy
```

must be treated as configuration/behavior changes.

## Error Compatibility

Error codes are part of the machine-readable contract.

Changing:

```text
ErrorCode
ErrorCategory
```

may break consumers.

Human-readable error messages are not stable API contracts according to ADR 0006.

## Error Message Changes

Applications must not depend on diagnostic text to determine behavior.

For example:

```text
if message == "not found"
```

is not an architectural contract.

Consumers should use stable error identity.

## Ownership Compatibility

Changing ownership semantics can be a breaking API change.

For example:

```text
borrowed → owned
owned → borrowed
```

may alter lifetime and resource requirements.

Such changes require explicit compatibility analysis.

## Concurrency Compatibility

Changing concurrency guarantees can be a semantic API change.

For example:

```text
SERIAL → CONCURRENT
```

may invalidate consumers relying on:

```text
ordering
state safety
non-reentrancy
```

Likewise:

```text
CONCURRENT → SERIAL
```

may change performance/resource behavior.

## Ordering Compatibility

Ordering guarantees are part of a contract.

Changing:

```text
INPUT_ORDER
```

to:

```text
NONE
```

is potentially breaking even if the same objects are produced.

## Temporal Compatibility

Temporal semantics are part of API/data compatibility.

Changing:

```text
event time
```

to:

```text
ingestion time
```

would change meaning even if the timestamp type remains identical.

Likewise, changing:

```text
[Start, End)
```

to:

```text
(Start, End]
```

changes query semantics.

## Identity Compatibility

Changing identity semantics can invalidate:

```text
references
deduplication
provenance
storage
recovery
idempotency
```

Identity changes therefore require explicit migration semantics.

## Version Identity

A version identifies a particular definition or implementation state.

Conceptually:

```text
Version
{
    major
    minor
    patch
}
```

may be appropriate for some artifacts.

However, semantic versioning is not automatically required for every EVolution object.

The exact representation is deferred.

## Version Scope

Version numbers must have an explicit scope.

For example:

```text
ProcessorVersion
GraphVersion
SchemaVersion
```

must not be treated as interchangeable.

## Component Version

A component version identifies an implementation or release of a component.

It may affect:

```text
behavior
performance
serialization
analysis
processing
```

when the implementation changes.

## Algorithm Version

An algorithm version identifies the analytical or processing algorithm used.

For example:

```text
TrendDetector v2
```

may produce different results from:

```text
TrendDetector v1
```

even when the API is unchanged.

Algorithm version is therefore important for reproducibility.

## Configuration Version

Configuration identity identifies the configuration definition or effective configuration according to its contract.

A configuration change can change processing results without changing code.

## Effective Configuration

Reproducibility uses effective configuration rather than merely the original configuration source.

For example:

```text
config file
+
defaults
+
environment resolution
=
effective configuration
```

The effective result is what matters semantically.

## Schema Version

Schema version identifies the structure/meaning of serialized data.

Schema version is distinct from:

```text
object version
component version
application version
```

A schema may remain compatible across multiple component versions.

## Schema Evolution

Schema changes must explicitly define:

```text
read compatibility
write compatibility
migration
default behavior
unknown-field handling
```

where applicable.

## Migration

Migration transforms information from one compatible representation/version to another.

Conceptually:

```text
Version N
    ↓
Migration
    ↓
Version N+1
```

Migration must not silently change semantic meaning.

## Migration vs Reprocessing

These are distinct.

Migration:

```text
change representation/schema
```

Reprocessing:

```text
run processing/analysis again
```

A schema migration should not unexpectedly rerun analytical algorithms.

## Checkpoint Compatibility

Checkpoints are version-sensitive.

A checkpoint may depend on:

```text
processor version
graph version
configuration
schema
state representation
input position
```

An incompatible checkpoint must not be silently loaded.

## Checkpoint Migration

If checkpoint migration is supported, compatibility must be explicit.

Possible outcomes:

```text
load directly
migrate
discard and reconstruct
reject
```

The recovery strategy determines which behavior applies.

## Processing Graph Version

A processing graph definition has an identity/version independent of a particular execution.

For example:

```text
Graph Definition v7
```

may be executed multiple times.

Changing:

```text
nodes
connections
processor versions
graph configuration
```

may produce a new graph definition version.

## Graph Execution Identity

A graph execution is distinct from the graph definition.

Conceptually:

```text
Graph Definition
    +
Run
    ↓
Graph Execution
```

The same graph definition may be executed repeatedly with different:

```text
input
run identity
execution context
configuration
```

where permitted.

## Extension Versioning

Extensions have explicit identity/version according to ADR 0035.

Selection records should identify the implementation/version where it affects behavior.

An application must not silently substitute an incompatible extension.

## Interface Versioning

External interfaces may expose their own versions.

Examples:

```text
CLI version
HTTP API version
RPC API version
IPC protocol version
```

Interface version is distinct from application version.

## Interface vs Application Version

Changing application behavior does not necessarily require changing the transport interface.

Conversely, changing the interface representation does not necessarily change the application's semantic operation.

These versions should remain independent.

## Application Version

An application version identifies a particular composition/workflow release.

It may depend on:

```text
domain versions
processor versions
graph definitions
extension versions
interface versions
configuration
```

Application version alone is therefore not a complete reproducibility record.

## API Stability

Not every internal interface requires long-term stability.

EVolution distinguishes:

```text
internal implementation contract
reusable library contract
extension contract
application API
external API
```

Stability expectations should be explicit.

## Experimental APIs

Experimental APIs may evolve rapidly.

They should be clearly identified so that consumers do not assume long-term compatibility.

The exact mechanism for marking experimental APIs is deferred.

## Public vs Private API

Components should distinguish interfaces intended for:

```text
internal implementation
other EVolution modules
external applications
third-party extensions
```

Private implementation details should not accidentally become stable contracts.

## Header/API Exposure

For C++ components, public headers should expose only the contract required by consumers.

Implementation details should remain private where practical.

The exact include/install structure remains governed by the source-tree/build architecture.

## C API

Where a stable C ABI is required, the C boundary should define explicitly:

```text
handles
buffers
lengths
ownership
error representation
versioning
lifetime
threading
```

C++ implementation details must not leak through the ABI.

## ABI Versioning

If a C ABI is introduced, ABI compatibility must be treated separately from source/API compatibility.

Possible strategies include:

```text
versioned entry points
ABI version field
opaque handles
explicit capability negotiation
```

No specific mechanism is selected here.

## Capability Negotiation

Some APIs may need to support different capabilities across versions.

Conceptually:

```text
Client
    ↓
Discover capabilities
    ↓
Select compatible operation
```

Capability negotiation is preferable to silently assuming support.

## Unsupported Features

When a requested feature is unavailable, the system should return an explicit unsupported/error result rather than silently approximating behavior.

## Deprecation

A contract may be deprecated before removal.

Conceptually:

```text
ACTIVE
   ↓
DEPRECATED
   ↓
REMOVED
```

Deprecation should identify:

```text
replacement
migration guidance
expected removal version
compatibility impact
```

where applicable.

## Removal

Removing a contract requires consideration of:

```text
consumers
stored data
extensions
graphs
configuration
documentation
tests
```

Removal is not merely deleting code.

## Version Pinning

Reproducible executions may need to pin:

```text
processor version
algorithm version
extension version
graph version
schema version
configuration version
```

The application/run context determines which are required.

## Version Ranges

A consumer may sometimes accept a range of compatible versions.

For example:

```text
supported:
    Processor API 2.x
```

The exact range semantics are contract-specific.

## Compatibility Matrix

Where multiple versions interact, compatibility may be represented explicitly.

For example:

```text
Producer Version
        ×
Consumer Version
        ↓
Compatibility
```

No universal matrix format is selected.

## Version and Provenance

When a version materially affects a derived object, provenance should record it.

For example:

```text
Analysis
    ↓
Algorithm v3
    ↓
Configuration v8
    ↓
Input Event Set v12
```

## Version and Identity

Version does not replace identity.

For example:

```text
AnalysisId = A
Version = 4
```

identifies a version of an analysis object rather than changing the meaning of the logical identity model.

The exact copy/version semantics remain object-specific.

## Version and Reproducibility

A version is useful for reproducibility only if it identifies the relevant behavior.

A release number alone may be insufficient if:

```text
configuration
external dependencies
runtime context
input data
```

also affect results.

## Version and Serialization

Serialized data should identify the schema/version information required to interpret it.

The serializer implementation version need not be identical to the schema version.

## Version and Storage

Storage implementations may have physical schema versions.

These are distinct from semantic data schema versions.

A database migration may occur without changing the semantic object schema.

## Version and Security

Security policy versions may matter when authorization behavior affects an operation.

Security versioning remains separate from API and domain versioning.

## Version and Observability

Operational telemetry schemas may evolve independently from analytical data schemas.

Observability compatibility should not silently become analytical compatibility.

## Version and Configuration Defaults

Changing a default may require a configuration version change or another explicit mechanism if it changes semantic behavior.

A configuration file that omits a field may therefore behave differently under different application/component versions.

This difference must be reproducible.

## Version and Current Time

Current time is not a version.

Temporal information follows ADR 0011.

## Version and Randomness

Random seeds are not version identifiers.

Randomness source/version may be part of execution context where required.

## Version and Source Data

A source dataset may have its own version.

For example:

```text
Dataset v12
```

is distinct from:

```text
Parser v4
```

and:

```text
Application v7
```

All may matter for reproducibility.

## Versioned Dependencies

External dependencies can affect behavior.

Where materially relevant, reproducibility may require recording:

```text
dependency identity
dependency version
configuration
relevant capabilities
```

## Compatibility and Testing

Version compatibility must be tested where contracts require it.

Tests may include:

```text
old producer → new consumer
new producer → old consumer
old data → new reader
new data → old reader
old checkpoint → new processor
```

where supported.

## Contract Tests

Contract tests should verify that implementations continue to satisfy the API semantics across versions.

## Golden Data

Versioned golden data can verify compatibility across schema/parser/algorithm changes.

## Breaking Changes

A change is breaking when an existing valid consumer, stored artifact, or processing result can no longer be correctly used according to its previous contract.

Examples include:

```text
removed required field
changed identity semantics
changed ordering guarantee
changed temporal interpretation
changed error code semantics
changed ownership/lifetime
changed concurrency guarantee
incompatible serialized representation
```

## Non-Breaking Extensions

Examples may include:

```text
new optional field
new optional capability
additional diagnostic metadata
new independent implementation
```

provided existing semantics remain intact.

## Semantic Breaking Changes

A change can be breaking without changing syntax.

Examples:

```text
same API
+
different timezone interpretation
```

or:

```text
same processor API
+
different duplicate-handling semantics
```

These require explicit versioning/migration decisions.

## Versioning and Documentation

Material contract changes must update:

```text
ADR
API documentation
schema documentation
compatibility information
tests
migration guidance
```

where applicable.

## Versioning and AI-Assisted Development

Because EVolution is expected to use automated/AI-assisted implementation heavily, architectural contracts must be explicit enough that generated code can distinguish:

```text
API contract
implementation detail
versioned behavior
deferred decision
```

AI-generated implementation must not silently introduce a new contract version by changing behavior.

## Consequences

### Positive

* Different kinds of versioning remain distinguishable.
* Semantic compatibility receives priority over syntax-only compatibility.
* Stored data and checkpoints can evolve explicitly.
* Reproducibility can identify materially relevant implementation versions.
* External interfaces can evolve independently from internal architecture.
* Future C ABI or plugin contracts have a clear versioning foundation.

### Negative

* Multiple version dimensions increase metadata and documentation requirements.
* Compatibility analysis can become complex as the system grows.
* Migration infrastructure may be required for long-lived stored data.
* Versioning cannot completely solve semantic changes; explicit migration/testing remain necessary.

## Deferred Decisions

This ADR does not select:

```text
semantic versioning policy
version number format
release policy
API stability guarantees
C ABI strategy
ABI compatibility mechanism
wire protocol versioning format
schema registry
migration framework
deprecation duration
compatibility matrix format
```

## Decision Summary

```text
Versioning:
    Contract evolution mechanism

API Version:
    API contract

Schema Version:
    Serialized/data structure semantics

Component Version:
    Implementation/release identity

Algorithm Version:
    Processing/analytical algorithm

Configuration Version:
    Configuration definition/effective behavior

Graph Version:
    Processing graph definition

Extension Version:
    Extension implementation

Application Version:
    Concrete application composition

Data Version:
    Source/stored dataset definition

Compatibility:
    Must include semantic compatibility

Errors:
    Stable machine-readable identity

Ownership:
    Part of API compatibility

Ordering:
    Part of API compatibility

Temporal semantics:
    Part of API/data compatibility

Identity:
    Separate from version

Migration:
    Explicit transformation

Reprocessing:
    Distinct from migration

Reproducibility:
    Requires all materially relevant versions and conditions

Deprecation:
    Explicit lifecycle

Removal:
    Explicit compatibility decision
```

## Invariants

1. Versioning is scoped to an explicit contract or artifact.
2. No single global version number represents the complete state of EVolution.
3. API version, schema version, component version, algorithm version, configuration version, graph version, extension version, and application version remain distinct concepts.
4. Semantic compatibility is more important than signature compatibility.
5. A syntactically compatible change can still be semantically breaking.
6. Error codes are machine-readable API contracts.
7. Human-readable error messages are not stable machine contracts.
8. Ownership and lifetime semantics are part of API compatibility.
9. Concurrency guarantees are part of API compatibility.
10. Ordering guarantees are part of API compatibility.
11. Temporal semantics are part of API/data compatibility.
12. Identity semantics are part of compatibility and must not silently change.
13. Version does not replace logical identity.
14. Migration and reprocessing are distinct operations.
15. Incompatible checkpoints must not be silently interpreted as compatible state.
16. Stored data must not silently acquire a different semantic interpretation because implementation code changed.
17. Effective configuration is version/reproducibility relevant when it affects behavior.
18. Algorithm versions must be identifiable when algorithm changes can affect derived results.
19. Extension selection must identify the implementation/version when it affects semantics.
20. External interface versions remain distinct from application versions.
21. C++ ABI compatibility is not assumed automatically.
22. C ABI compatibility requires an explicit contract.
23. Capability availability must not be inferred solely from version numbers where explicit capability discovery is required.
24. Deprecated contracts require an explicit migration/removal path where long-term compatibility matters.
25. Version changes must not silently modify domain meaning.
26. Reproducibility requires materially relevant versions in addition to input data and configuration.
27. Observability and security versions must remain separate from analytical versions unless explicitly promoted into analytical semantics.
28. A new implementation does not automatically constitute a new semantic version if its behavior remains contract-compatible.
29. Version metadata must not be used as a substitute for provenance.
30. Versioning must remain independent of the physical storage, transport, and deployment technology.

## Invariant

> **EVolution treats versioning as explicit contract evolution: API, schema, implementation, algorithm, configuration, graph, extension, application, and data versions remain distinct, while compatibility is determined by preserved semantics rather than by syntax, representation, or version numbers alone.**
