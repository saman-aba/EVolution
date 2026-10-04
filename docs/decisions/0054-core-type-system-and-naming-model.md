# ADR 0054: Core Type System and Naming Model

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution Core will use a **strongly typed, value-oriented C++ type system**.

Types should make semantically invalid combinations difficult to express while avoiding unnecessary abstraction.

The Core API will use:

* a single project namespace
* nested semantic namespaces where useful
* strong types for semantically distinct identifiers and values
* explicit ownership
* value semantics by default
* `std::chrono` for temporal primitives
* explicit `Result<T>` for expected failure
* composition rather than universal inheritance

The type system is intended to encode architectural distinctions already established by the ADRs.

---

# 1. Project Namespace

The primary namespace is:

```cpp
namespace evolution
{
}
```

All EVolution-owned C++ types should reside within this namespace or an explicitly documented nested namespace.

The global namespace must not be used for EVolution types.

---

# 2. Nested Namespaces

Nested namespaces may be used to make ownership visible.

Conceptually:

```cpp
evolution::identity
evolution::time
evolution::error
evolution::configuration
evolution::context
evolution::provenance
evolution::event
evolution::state
evolution::measurement
evolution::processing
```

The exact namespace structure may evolve if the physical module boundaries change, but namespace ownership should remain aligned with architectural responsibility.

---

# 3. Namespace Does Not Define Dependency

A namespace is a naming boundary, not automatically a dependency boundary.

For example:

```cpp
evolution::event
```

may depend on:

```cpp
evolution::identity
evolution::time
```

without implying that every type under `evolution::event` may depend on every other Core namespace.

Architectural dependency rules remain authoritative.

---

# 4. Type Ownership

Every major public type should have a clear conceptual owner.

Examples:

```text
Id                 → Identity
Timestamp          → Time
Error              → Error
Result<T>          → Error
Configuration      → Configuration
ExecutionContext   → Context
Provenance         → Provenance
Event              → Event
Measurement        → Measurement
Projection         → State
Processor Contract → Processing
```

A type should not be placed in a namespace merely because another component happens to use it.

---

# 5. Struct vs Class

The distinction between `struct` and `class` should communicate the intended abstraction.

Use `struct` primarily for simple value aggregates whose public representation is itself meaningful.

Use `class` when the type requires:

* invariants
* controlled construction
* encapsulation
* lifecycle management
* opaque implementation
* behavior requiring controlled state

This is a guideline rather than an absolute language rule.

---

# 6. Value Semantics

Core value types should use ordinary C++ value semantics wherever practical.

For a value type:

```text
Copy
Move
Destroy
```

should have predictable behavior.

Examples include:

```text
Id
TimeRange
Error
Configuration values
Temporal information
Provenance values
```

A type should not require heap allocation or shared ownership merely to behave as a normal value.

---

# 7. Strong Types

Semantically distinct values should not be represented by interchangeable primitive types when confusion would be meaningful.

For example, the API should avoid treating all of these as plain integers:

```text
Event ID
Metric ID
Analysis ID
Sequence Number
Port Number
Count
Duration
```

Strong types should be introduced when they prevent meaningful classes of mistakes.

---

# 8. Strong Types vs Excessive Wrappers

Not every primitive requires a wrapper.

For example, creating a dedicated type for every internal loop counter would add noise without improving architecture.

The criterion is semantic distinction.

A type should be strong when:

1. Its meaning differs from another value.
2. Accidentally interchanging the values could produce an invalid result.
3. The distinction is useful at an API boundary.

---

# 9. Logical Identity

Identity types follow the model:

```cpp
template <typename Tag>
class Id;
```

Conceptually:

```cpp
using EventId = Id<EventTag>;
using MetricId = Id<MetricTag>;
using AnalysisId = Id<AnalysisTag>;
```

`EventId` must not implicitly convert to `MetricId`.

---

# 10. Identity Representation

The physical representation of `Id<Tag>` remains intentionally unspecified.

Possible implementations include:

```text
Integer
UUID
Random identifier
Monotonic identifier
Content-derived identifier
Domain-generated identifier
```

The identity contract must remain independent of the chosen representation.

---

# 11. Identifier Validity

An identifier should have an explicit valid/invalid state only if the object lifecycle requires one.

The implementation must avoid ambiguous sentinel values such as:

```text
-1
0
UINT64_MAX
```

unless the identifier contract explicitly defines such a value.

Invalid identity should not be confused with a legitimate identifier.

---

# 12. Identity Comparison

Identity types should support equality comparison.

Ordering should only be provided when ordering has semantic or useful deterministic meaning.

The numeric representation of an identifier must not automatically become domain ordering.

For example:

```text
EventId 100
EventId 200
```

does not imply that Event 100 occurred before Event 200.

---

# 13. Identity Hashing

Identity types should eventually support use in hash-based containers.

Hashability is an implementation facility.

Hash ordering must not be treated as semantic ordering.

---

# 14. Time Types

Time types use the C++ standard `<chrono>` facilities.

Absolute time is conceptually:

```cpp
std::chrono::system_clock::time_point
```

Duration is represented using:

```cpp
std::chrono::duration
```

Monotonic timing uses the appropriate monotonic clock rather than wall-clock timestamps.

---

# 15. Timestamp Aliases

EVolution may define project-level aliases around `std::chrono` to establish consistent semantic names.

For example:

```cpp
using TimePoint = ...;
using Duration = ...;
```

The aliases must not hide important semantic differences.

An event timestamp and a processing timestamp may use the same underlying physical type while remaining distinct semantic fields.

---

# 16. Temporal Information

Temporal knowledge requires more than a timestamp.

Conceptually:

```text
TemporalInformation
{
    timestamp?
    status
    uncertainty?
    provenance?
}
```

with:

```text
KNOWN
ESTIMATED
UNKNOWN
```

The exact class representation remains an implementation detail.

---

# 17. Time Range

Time intervals should have explicit boundary semantics.

The default interval representation is:

```text
[start, end)
```

meaning:

```text
start ≤ t < end
```

This avoids ambiguity when adjacent ranges meet.

---

# 18. Duration vs Timestamp

A duration represents elapsed time.

A timestamp represents a position on a time scale.

They must not be interchangeable.

For example:

```text
Duration(5 seconds)
```

does not identify a point in time.

---

# 19. Units

Where a value has an important physical or semantic unit, the type or surrounding contract should make the unit explicit.

Examples:

```text
milliseconds
seconds
bytes
events
hands
USD
BB
packets
```

The Core does not initially require a universal physical-units framework.

---

# 20. Numeric Types

The type system should prefer explicit numeric types when width or signedness matters.

For example:

```cpp
std::uint64_t
std::int64_t
```

may be preferable to an unspecified integer type for persisted or serialized values.

However, fixed-width types should not be used mechanically where the exact width has no semantic relevance.

---

# 21. Floating-Point Values

Floating-point values must not automatically imply exact equality.

APIs involving floating-point results should document relevant:

* precision
* tolerance
* NaN behavior
* infinity behavior
* rounding
* aggregation behavior

Bitwise equality is not the default definition of analytical equality.

---

# 22. Strings

The project should use standard C++ string types unless a contract requires otherwise.

Typical choices:

```cpp
std::string
std::string_view
```

Ownership must remain explicit.

`std::string_view` is borrowed and must not be retained beyond its lifetime contract.

---

# 23. Byte Data

Binary data should use an explicit byte representation.

The preferred conceptual type is:

```cpp
std::byte
```

with an owning container such as:

```cpp
std::vector<std::byte>
```

where ownership is required.

A byte buffer must not be interpreted as text unless an encoding contract explicitly says so.

---

# 24. Spans and Views

Views such as:

```cpp
std::span<T>
std::string_view
```

represent borrowed access.

They do not transfer ownership.

A view must not outlive the underlying storage.

Asynchronous or queued processing should normally use owned/lifetime-safe data unless the contract explicitly guarantees the referenced storage lifetime.

---

# 25. Optional Values

`std::optional<T>` represents a valid absence of a value.

It must not be used as a generic replacement for errors.

The distinction is:

```text
optional<T>
    → value or intentionally no value

Result<T>
    → successful value or operation failure
```

---

# 26. Result Values

`Result<T>` represents operation success or failure.

Conceptually:

```text
Result<T>
├── value
└── error
```

A function returning `Result<T>` must not use a sentinel value to represent operational failure.

---

# 27. Result and Optional

Nested forms such as:

```cpp
Result<std::optional<T>>
```

are allowed when three states genuinely exist:

```text
Success + Value
Success + No Value
Failure
```

They should not be introduced merely because the API has not clearly distinguished “not found” from failure.

---

# 28. Error Codes

Errors use stable machine-readable codes.

Codes should be represented by a strongly typed mechanism rather than raw strings.

Conceptually:

```cpp
enum class ErrorCode;
```

The exact enumeration organization may later be divided by module.

---

# 29. Error Categories

Error category and error code remain separate.

Conceptually:

```text
Category:
    Validation
    Configuration
    Input
    State
    Processing
    Storage
    Resource
    External
    Unsupported
    Cancellation
    Internal
```

A code provides a more specific machine-readable identity.

---

# 30. Error Messages

Human-readable messages are diagnostic.

They are not machine contracts.

Code must never parse its own or another component's error message to determine behavior.

---

# 31. Error Context

Error context should use structured, bounded information.

The Core must not introduce an unrestricted:

```text
map<string, any>
```

as the default error context mechanism.

Context should contain information useful for understanding the failure without becoming an arbitrary serialization format.

---

# 32. Error Immutability

`Error` is logically immutable after construction.

Operations such as:

```cpp
error.with_context(...)
```

conceptually produce a new error value.

They must not mutate the original error.

---

# 33. Configuration Values

Configuration requires a representation capable of expressing the configuration model without forcing all application/domain configuration into Core.

The Core may eventually provide primitive configuration mechanisms such as:

```text
Scalar
Sequence
Object
Optional Value
```

but the exact configuration representation remains an implementation decision.

Domain schemas remain outside generic Core.

---

# 34. Execution Context

Execution context should be composed from explicit fields rather than a universal dynamic map.

Conceptually:

```text
ExecutionContext
{
    run_id?
    execution_mode
    time_source
    randomness_source?
    cancellation?
    resource_context?
}
```

Only materially relevant information should be included.

---

# 35. Context Lifetime

Execution context is normally scoped to the operation or execution environment that created it.

A processor must not retain borrowed context information beyond the lifetime guaranteed by the contract.

If context must survive asynchronously, ownership must be explicit.

---

# 36. Provenance References

Provenance should reference logical identities rather than memory addresses.

For example:

```text
EventId
MeasurementId
ProcessorId
ConfigurationId
```

may appear as provenance references.

A provenance record must not require access to the original object merely to identify its origin.

---

# 37. Collections

Standard containers are preferred.

Typical choices include:

```text
std::vector
std::deque
std::array
std::map
std::unordered_map
std::set
std::unordered_set
```

The choice should follow semantic and performance requirements.

A custom container requires measurable justification.

---

# 38. Ownership Containers

Ownership should normally be expressed through ordinary C++ mechanisms:

```text
Value
std::unique_ptr
std::shared_ptr
```

but the architecture does not prescribe smart pointers universally.

`std::shared_ptr` should only be used when shared lifetime is genuinely required.

Owning raw pointers are discouraged.

---

# 39. References

References and pointers should normally represent non-owning access unless the contract explicitly states otherwise.

An API must not require callers to infer ownership from:

```cpp
T*
T&
```

alone.

---

# 40. Polymorphism

Runtime polymorphism should be introduced only where substitutability is actually required.

Possible mechanisms include:

```text
Templates
Concepts
Interfaces
Type Erasure
Variants
Function Objects
```

The implementation should choose the simplest mechanism that satisfies the contract.

Inheritance is not the default architecture.

---

# 41. Inheritance

Deep inheritance hierarchies are discouraged.

The default preference is:

```text
Composition
    over
Inheritance
```

Inheritance may be appropriate for explicit behavioral contracts where runtime substitutability is required.

---

# 42. Type Erasure

Type erasure may be used where the architecture requires a runtime-independent interface without exposing concrete implementation types.

Potential examples include:

```text
Processor
Clock
Randomness Source
Storage Backend
Execution Backend
```

However, type erasure should be introduced only when a concrete requirement exists.

---

# 43. Concepts

C++20 concepts may express compile-time contracts.

They should be used where they make the API clearer or prevent invalid use.

Concepts must not become a mechanism for excessive compile-time abstraction.

---

# 44. Enums

Enums should normally use:

```cpp
enum class
```

rather than unscoped enums.

Enumerations that cross serialization or ABI boundaries require explicit representation and versioning rules.

---

# 45. Boolean Parameters

Multiple boolean parameters should be avoided when their meaning can be confused.

For example:

```cpp
process(data, true, false, true);
```

is discouraged.

A named configuration/value type should be preferred when the flags have meaningful semantics.

---

# 46. Default Arguments

Default arguments should not silently encode architectural behavior.

Defaults are appropriate when:

* the default is semantically obvious
* changing it does not create hidden behavior
* configuration/provenance requirements remain satisfied

Material processing behavior should normally be represented by explicit configuration.

---

# 47. Nullability

Pointers should not be used as ambiguous representations of:

```text
Missing value
Invalid object
Failure
Optional object
Ownership
```

The appropriate semantic mechanism should be used:

```text
optional
Result
reference/view
pointer
```

according to the contract.

---

# 48. API Naming

Names should describe semantic responsibility rather than implementation technique.

Prefer:

```text
TimeRange
Measurement
Projection
Processor
Storage
Query
```

over implementation-driven names such as:

```text
DataWrapper
Manager
Helper
Handler
Utils
```

unless those names accurately describe a bounded responsibility.

---

# 49. “Manager” Types

Generic classes named:

```text
Manager
Helper
Utils
Controller
Coordinator
```

should be treated with suspicion.

A type should have a specific responsibility.

For example:

```text
ProcessingGraph
Scheduler
Supervisor
Storage
QueryService
```

are preferable when those are actually the semantic responsibilities.

---

# 50. Header Naming

Headers should use names corresponding to their primary public responsibility.

Examples:

```text
id.h
result.h
error.h
clock.h
timestamp.h
event.h
measurement.h
processor.h
```

A header should not become a miscellaneous container of unrelated definitions.

---

# 51. Include Policy

Public headers should include what they require to compile correctly.

They should not rely on transitive includes.

Implementation files should include their corresponding public/private headers first where practical.

Forward declarations may be used when they meaningfully reduce dependencies and do not obscure the contract.

---

# 52. `const` Correctness

Const correctness is part of the API contract.

Read-only operations should be represented as such.

Immutable historical objects should expose read-only access.

Const correctness must not be used to imply thread safety.

---

# 53. `constexpr`

`constexpr` may be used for genuinely compile-time values and operations.

It should not be added everywhere merely as an optimization or stylistic convention.

---

# 54. RAII

Resource ownership should use RAII where appropriate.

This applies to:

* files
* sockets
* locks
* memory
* external handles
* transactions
* temporary resources

RAII does not determine the semantic ownership model; it implements it safely.

---

# 55. Error and RAII

Resource cleanup must occur independently of whether an operation succeeds, fails, or is cancelled.

An error path must not bypass resource cleanup.

---

# 56. Thread Safety of Value Types

Small immutable value types should generally be safe to copy and transfer between threads.

This does not imply that every containing object is thread-safe.

Mutable shared state requires an explicit concurrency contract.

---

# 57. Serialization of Core Types

Core types that may cross persistence or external boundaries should have serialization semantics defined separately from their C++ representation.

For example:

```text
Id
Timestamp
Duration
TemporalInformation
Error
Configuration
```

may eventually need serialization contracts.

The wire representation remains undecided.

---

# 58. ABI and Public Types

Public C++ types should avoid exposing implementation details unnecessarily.

Types that must cross a stable C ABI should be translated into explicit C representations.

C++ standard-library objects should not cross an independent C ABI boundary.

---

# 59. Testing Type Contracts

Foundational types require contract tests.

At minimum:

```text
Id:
    equality
    invalid state if applicable
    hashing
    type separation

Time:
    ordering
    duration
    range boundaries
    KNOWN/ESTIMATED/UNKNOWN

Result:
    success
    failure
    propagation

Error:
    immutability
    copy
    move
    enrichment

Views:
    lifetime assumptions

Ownership:
    copy/move/destruction
```

---

# 60. Deferred Decisions

This ADR does not decide:

* exact namespace hierarchy
* exact header directory structure
* exact identifier representation
* identifier generation algorithm
* exact `Result` implementation
* exact error storage representation
* exact configuration representation
* exact provenance representation
* exact type-erasure mechanism
* custom allocator strategy
* serialization format
* ABI layout
* module usage
* formatting conventions
* linting rules

These may be established as implementation proceeds.

---

# Decision Summary

```text
Primary namespace:              evolution
Nested semantic namespaces:     Allowed
Type style:                     Strongly typed
Default semantics:              Value-oriented
Identity:                       Id<Tag>
Time:                            std::chrono
Absolute time:                  UTC
Temporal status:                KNOWN / ESTIMATED / UNKNOWN
Ranges:                          [start, end)
Errors:                          Error + Result<T>
Optional absence:               std::optional
Expected failure:               Result<T>
Strings:                         std::string / string_view
Binary data:                     std::byte
Views:                            Borrowed
Ownership:                        Explicit
Inheritance:                      Limited
Composition:                      Preferred
Universal base object:            Rejected
Universal metadata map:          Rejected
Universal variant:               Not default
Universal manager type:          Discouraged
RAII:                             Preferred
Custom allocation:               Deferred
Serialization:                    Separate contract
ABI:                              Explicit boundary
```

## Invariant

**EVolution's Core type system must make important semantic distinctions visible in C++ without turning every value into an abstraction: logically distinct identities and concepts use strong types where useful, ownership and lifetime remain explicit, temporal and error semantics are first-class, value semantics are preferred, and universal base objects or unrestricted metadata containers are not used as substitutes for well-defined contracts.**

