# ADR 0053: Core Foundation Structure

**Status:** Accepted
**Date:** 2026-10-04

## Decision

The EVolution Core will be implemented as a small, dependency-light foundation containing the generic types and mechanisms required by the rest of the architecture.

Core will be divided conceptually into foundational layers rather than implemented as one large collection of unrelated utilities.

The initial structure is:

```text
core/
├── foundation/
├── identity/
├── time/
├── error/
├── configuration/
├── context/
├── provenance/
├── event/
├── state/
├── measurement/
└── processing/
```

The exact physical file layout may evolve, but these ownership boundaries should remain visible.

---

# 1. Core Responsibility

Core provides generic mechanisms required by multiple architectural layers.

Core owns concepts that are meaningful without knowing a specific domain.

Examples:

```text
Identity
Time
Error
Result
Configuration mechanisms
Execution Context mechanisms
Provenance
Event structure
State mechanisms
Measurement structure
Processor contracts
```

Core must not contain:

```text
Poker rules
Trading rules
HTTP semantics
Database schemas
GUI concepts
Application workflows
Domain-specific metrics
Domain-specific analysis
```

---

# 2. Core Dependency Rule

Core dependencies must remain minimal.

Conceptually:

```text
foundation
    ↓
identity / time / error / configuration
    ↓
context / provenance
    ↓
event / state / measurement
    ↓
processing contracts
```

Higher-level Core concepts may depend on lower-level Core concepts where necessary.

Core components must not create circular dependencies merely for convenience.

---

# 3. Foundation

`foundation/` contains genuinely generic infrastructure that does not belong to another semantic area.

Possible contents include:

```text
Common type utilities
Strong utility types
Generic validation mechanisms
Basic type traits
Non-domain helper facilities
```

Foundation must remain deliberately small.

A utility should not be placed here merely because its owner is unclear.

---

# 4. Identity

`identity/` owns logical identity mechanisms.

The central abstraction is conceptually:

```cpp
template <typename Tag>
class Id;
```

Examples:

```cpp
using EventId = Id<EventTag>;
using AnalysisId = Id<AnalysisTag>;
using MetricId = Id<MetricTag>;
```

Identity generation remains separate from identity representation.

The Core must not assume a specific physical identifier algorithm yet.

---

# 5. Time

`time/` owns temporal primitives and temporal semantics.

It should provide mechanisms for representing:

```text
Absolute Time
Duration
Temporal Information
Temporal Status
Time Ranges
```

The temporal model must support:

```text
KNOWN
ESTIMATED
UNKNOWN
```

where required.

The implementation should use `std::chrono` rather than defining a competing general-purpose time system.

---

# 6. Error

`error/` owns:

```text
Error
ErrorCode
ErrorCategory
Result<T>
```

The error model follows ADR 0005–0009.

Errors are:

* value-owned
* immutable after construction
* explicit
* machine-identifiable
* independent of originating operation lifetime

Expected operational failure must be represented through `Result<T>`.

---

# 7. Result

`Result<T>` provides expected-like semantics.

Conceptually:

```text
Result<T>
    ├── Value
    └── Error
```

and:

```text
Result<void>
    ├── Success
    └── Error
```

The initial implementation must target C++20.

The rest of Core must depend on the project abstraction rather than directly on `std::expected`.

This preserves the option of changing the underlying implementation later.

---

# 8. Configuration

`configuration/` provides mechanisms for representing resolved configuration.

It should distinguish:

```text
Raw Configuration
Resolved Configuration
Effective Configuration
```

The Core configuration mechanism should remain generic.

Domain-specific configuration schemas belong outside generic Core.

Configuration values must not become a universal dynamic object merely for convenience.

---

# 9. Context

`context/` represents execution conditions that are materially relevant to processing.

It should provide mechanisms for concepts such as:

```text
Execution Mode
Run Identity
Time Source
Randomness Source
Cancellation
Relevant Runtime Conditions
```

Context must remain bounded and explicit.

It must not become:

```text
Context = map<string, any>
```

for arbitrary information.

---

# 10. Provenance

`provenance/` provides mechanisms for representing origin and derivation.

Conceptually:

```text
Provenance
{
    source
    inputs
    transformation
    configuration
    version
    timestamp
}
```

The implementation should reference logical identities rather than object addresses.

Provenance should be usable by derived objects without forcing the Core to understand the meaning of those objects.

---

# 11. Event

`event/` owns the generic structure of an Event.

Conceptually:

```text
Event
{
    id
    timestamp
    type
    source
    sequence
    payload
}
```

The payload representation remains subject to the implementation design.

Core defines the event structure.

Domains define event meaning.

---

# 12. Event Immutability

Historical events should be immutable after construction.

Corrections must not silently mutate an existing historical event.

If a correction is required, it must use an explicit correction/replacement mechanism defined by the relevant domain or processing contract.

---

# 13. State

`state/` provides mechanisms for state reconstruction and representation.

Core should provide generic processor/projection mechanisms.

Domain modules define what the state actually means.

Conceptually:

```text
Initial State + Event
        ↓
    Projection
        ↓
    New State
```

Core must not contain domain-specific state transition logic.

---

# 14. Measurement

`measurement/` owns the generic representation of measurements.

Conceptually:

```text
Measurement
{
    metric
    value
    scope
    dimensions
    time_start
    time_end
    provenance
}
```

The Core may provide generic metric identity and measurement structures.

It must not define domain-specific metric meaning.

---

# 15. Processing Contracts

Core may provide the fundamental contracts required by the Processing layer.

Examples include:

```text
Processor identity
Input / output contracts
Processor configuration reference
Processing result
Processor state abstraction
Execution context reference
```

However, the operational implementation of:

```text
Scheduler
Executor
Queue
Thread Pool
Supervisor
Resource Manager
```

does not belong in the initial Core foundation.

Those mechanisms belong to Processing.

---

# 16. Core vs Processing

The distinction is:

```text
Core
    → Defines what a Processor is

Processing
    → Defines how processors are composed and executed
```

For example, Core may define a processor contract while Processing implements:

```text
Processing Graph
Scheduler
Execution Backend
Queue
Backpressure
Recovery
Supervision
```

This prevents Core from becoming coupled to one execution model.

---

# 17. Dependency Direction

The intended dependency direction inside Core is approximately:

```text
Foundation
   ↓
Identity / Time / Error
   ↓
Configuration / Context / Provenance
   ↓
Event / State / Measurement
   ↓
Core Processing Contracts
```

No component should depend upward merely to reuse an unrelated utility.

If a lower-level component needs functionality owned by a higher-level component, the dependency should be reconsidered rather than introducing a cycle.

---

# 18. Public vs Internal Core

Core itself has public and internal surfaces.

Not every Core header is automatically public.

The initial implementation should distinguish:

```text
Public Core Contract
Internal Core Implementation
```

This is particularly important for:

* `Result`
* `Error`
* `Id`
* time types
* processor contracts
* serialization helpers

Public exposure must be intentional.

---

# 19. Core Data Types

Core should prefer simple value-oriented types where appropriate.

Examples:

```text
Identifier
Timestamp
Duration
TimeRange
Error
Result
Configuration Value
Context Value
Provenance Reference
```

Large polymorphic hierarchies should not be introduced merely because the architecture contains many conceptual objects.

---

# 20. Ownership

Core types must follow the ownership model established by ADR 0031.

In particular:

```text
Value types → value ownership
Views → borrowed
Errors → owned
Historical information → immutable
```

A Core type must not silently retain references to temporary caller data.

---

# 21. Allocation

The Core foundation should avoid unnecessary dynamic allocation for small, frequently used values.

However, allocation avoidance must not produce obscure ownership models or premature custom memory infrastructure.

The first implementation should favor:

```text
Simple value semantics
Standard containers
RAII
Measurable optimization
```

Custom allocators, arenas, pools, and intrusive memory management remain deferred.

---

# 22. Exceptions

Core follows the project's Result/Error model.

Expected operational failures must use `Result<T>`.

Exceptions may still occur internally where permitted by ADR 0005, but ordinary callers must not need exception handling to distinguish normal operational failure.

Core should avoid exceptions as the primary control-flow mechanism.

---

# 23. Thread Safety

Core types should not automatically become internally synchronized.

Thread safety is part of the contract of the specific type.

For example:

```text
Immutable value
    → naturally shareable

Mutable state
    → synchronization contract required
```

Adding a mutex to every Core object is explicitly rejected as a general design strategy.

---

# 24. Serialization

Core types should be designed so that serialization can be added without making their in-memory representation the wire format.

Serialization belongs behind an explicit boundary.

The initial Core implementation should not require a particular serialization framework.

---

# 25. Testing

Each Core subsystem should have focused tests.

Conceptually:

```text
core/
    foundation/   → unit tests
    identity/     → identity contract tests
    time/         → temporal contract tests
    error/        → Result/Error tests
    configuration/→ configuration tests
    context/      → context tests
    provenance/   → provenance tests
    event/        → Event contract tests
    state/        → projection/state tests
    measurement/  → measurement tests
```

Tests should validate semantic contracts rather than internal implementation details.

---

# 26. CMake Targets

The initial Core implementation should expose a primary target:

```text
evolution_core
```

Internal source organization may remain subdivided without creating a separate CMake target for every directory.

Additional targets should be introduced only when an actual architectural or build dependency boundary justifies them.

---

# 27. Dependency Policy

`evolution_core` should initially depend only on:

```text
C++20 standard library
```

plus explicitly justified lightweight dependencies if later required.

Core should not require:

```text
Qt
Boost
database client
HTTP library
GUI toolkit
network framework
external runtime
```

merely to compile.

---

# 28. Compile-Time Boundary

The Core foundation should remain relatively lightweight because nearly every higher-level component may depend on it.

Public headers should therefore avoid unnecessary includes and implementation-heavy templates.

Compile-time cost is an architectural concern because Core dependencies propagate widely.

---

# 29. Error Dependency

Error handling is foundational.

Therefore:

```text
Error / Result
```

should sit below most higher-level Core concepts.

A Core API that can fail operationally should normally be able to express failure without introducing a dependency on a higher-level subsystem.

---

# 30. Time Dependency

Time primitives are similarly foundational.

Higher-level Core objects such as:

```text
Event
Measurement
Provenance
Context
```

may depend on Core temporal types.

They must not independently invent incompatible timestamp representations.

---

# 31. Identity Dependency

Logical identity is foundational to:

```text
Event
Measurement
Analysis
Provenance
Processing
```

Identity representation must therefore remain independent of storage and serialization.

---

# 32. No Universal Base Object

EVolution will not introduce a universal base class such as:

```cpp
class Object;
```

containing:

```text
id
timestamp
metadata
context
provenance
```

for every Core object.

Different concepts have different semantics.

Shared properties should be represented through explicit types or composition where appropriate.

---

# 33. No Universal Metadata Bag

The Core will not introduce a universal:

```text
Metadata = map<string, string>
```

or:

```text
Metadata = map<string, any>
```

as the default extension mechanism.

Such structures obscure ownership, semantics, validation, versioning, and reproducibility.

Structured fields should be used where the information is part of the contract.

---

# 34. No Universal Variant Object

Likewise, Core will not initially define a universal dynamically typed value such as:

```text
Value
{
    null
    bool
    integer
    float
    string
    array
    object
}
```

as the representation for every concept.

Such a type may eventually be useful for configuration or serialization, but its use must be restricted to the semantic context that requires it.

---

# 35. No Universal Base Processor Implementation

Core defines processor contracts but does not require every processor to inherit from one large abstract class.

Possible implementation styles include:

```text
Interface
Concept
Composition
Value-based processor
Type-erased processor
```

The exact C++ mechanism will be selected when implementing the processor API.

---

# 36. Initial Implementation Philosophy

The first Core implementation should optimize for:

```text
Correct semantics
Clear ownership
Small dependencies
Simple APIs
Strong compile-time checking
Good tests
Easy replacement of implementation
```

It should not optimize prematurely for:

```text
Maximum abstraction
Plugin flexibility
Distributed execution
Lock-free execution
Zero-copy everywhere
Custom allocators
ABI stability
```

---

# 37. Implementation Sequence

The recommended first implementation sequence is:

```text
1. foundation
2. identity
3. time
4. error / Result
5. configuration primitives
6. context
7. provenance
8. event
9. measurement
10. state / projection mechanisms
11. processor contract
```

Processing graph and execution mechanisms follow after these foundations are sufficiently usable.

---

# 38. Architectural Constraint

If implementing a later Core component reveals that an earlier Core component needs to know about:

```text
Poker
Database
GUI
HTTP
Specific storage
Specific scheduler
Specific thread pool
```

the dependency is considered architecturally suspicious and must be reviewed before being introduced.

---

# 39. Deferred Decisions

This ADR does not decide:

* exact namespace layout
* exact header naming
* exact class/struct choices
* exact `Id` representation
* exact `Result` implementation
* exact error storage
* exact configuration value representation
* exact context implementation
* exact provenance graph representation
* event payload representation
* state storage representation
* measurement numeric representation
* type erasure strategy
* allocator strategy
* serialization framework
* public header installation layout

Those decisions belong to implementation-level design.

---

# Decision Summary

```text
Core role:                  Generic semantic foundation
Primary language:           C++20
Primary dependency:         C++ standard library
Architecture:               Layered within Core
Ownership:                  Explicit
Errors:                     Result<T>
Identity:                   Strongly typed
Time:                       std::chrono-based
Historical Events:          Immutable
Configuration:              Explicit
Execution Context:          Explicit and bounded
Provenance:                 First-class
Processing:                 Contracts only
Scheduler:                  Processing layer
Executor:                   Processing layer
Queue:                      Processing layer
Database:                   Outside Core
GUI:                        Outside Core
HTTP/RPC:                   Outside Core
Universal Object:           Rejected
Universal Metadata Bag:     Rejected
Universal Variant:          Rejected as default
Universal Processor Base:  Not required
Custom Allocators:          Deferred
Serialization:              Separate boundary
```

## Invariant

**EVolution Core is a small, generic foundation: it owns reusable semantic types and contracts, not domain meaning or execution infrastructure. Core APIs must remain explicit about ownership, identity, time, errors, configuration, context, and provenance while avoiding universal metadata objects, universal base classes, hidden global state, and dependencies on specific domains or technologies.**

