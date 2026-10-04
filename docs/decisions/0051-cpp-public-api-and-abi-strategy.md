# ADR 0051: C++ Public API and ABI Strategy

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution distinguishes between **internal C++ interfaces**, **public C++ APIs**, and **stable external interfaces**.

The project will use modern C++20 internally and may expose C++ APIs to applications and extension implementations where appropriate.

However, EVolution will **not treat arbitrary C++ ABI compatibility as a stable architectural contract**.

Stable cross-language or long-lived binary boundaries should use an explicitly defined boundary such as:

* C ABI
* serialization protocol
* IPC/RPC protocol
* another explicitly versioned external contract

The C++ API remains the primary native programming interface unless a component requires a more stable boundary.

---

## 1. API vs ABI

API describes source-level interaction:

```text
Header
    ↓
Compiler
    ↓
C++ Program
```

ABI describes binary-level compatibility:

```text
Compiled Component
    ↔
Compiled Consumer
```

They are separate concerns.

A source-compatible change does not necessarily imply ABI compatibility.

Likewise, ABI compatibility does not guarantee semantic compatibility.

---

## 2. Internal API

Most EVolution components may use internal C++ interfaces without promising external stability.

Internal APIs may evolve as architecture and implementation develop.

Examples include:

```text
Processor internals
Graph implementation
Scheduler internals
Storage implementation
Internal helpers
Execution backend
```

Internal APIs remain governed by the architectural contracts but do not automatically become public APIs.

---

## 3. Public C++ API

A C++ API becomes public only when explicitly designated.

A public API may include:

```text
Core Types
Processor Contracts
Application APIs
Query APIs
Storage Interfaces
Extension Interfaces
Configuration Interfaces
```

Public API status should be intentional rather than inferred from header location.

---

## 4. Public API Design Principles

Public C++ APIs should favor:

* explicit ownership
* value semantics
* clear lifetimes
* strong types
* `Result<T>`
* explicit configuration
* explicit cancellation
* explicit concurrency semantics
* explicit temporal semantics
* explicit identity
* minimal hidden state

APIs should avoid requiring callers to understand internal implementation details.

---

## 5. C++20 Baseline

The public C++ API follows the project's C++20 baseline.

Public interfaces must not require language features newer than the adopted project standard.

The standard library should be preferred over proprietary abstractions where suitable.

C++23 or later features require an explicit architectural decision before becoming part of the public baseline.

---

## 6. Header Boundaries

Public headers should contain only declarations and definitions intended for consumers.

Internal implementation headers should remain private.

Conceptually:

```text
include/
    public API

src/
    implementation

internal/
    private implementation
```

The exact directory convention may evolve, but public/private ownership must remain explicit.

---

## 7. Header Dependencies

Public headers should minimize unnecessary dependencies.

A public header should not expose internal implementation types merely for convenience.

For example, a public processor contract should not require callers to include:

```text
internal scheduler
internal queue
internal executor
internal storage implementation
```

unless those types are themselves part of the public contract.

---

## 8. Pimpl and Representation Hiding

Representation hiding may be used where it provides meaningful benefits.

Possible reasons include:

* ABI stability
* reduced compile-time dependencies
* hiding implementation details
* reducing recompilation
* protecting internal structures

However, Pimpl is not mandatory.

The architectural requirement is that implementation details remain replaceable where the public contract promises such replaceability.

---

## 9. C++ ABI Stability

EVolution does not initially promise universal ABI stability across arbitrary:

* compilers
* standard libraries
* compiler versions
* build configurations
* operating systems
* architectures

A C++ binary consumer must therefore use a compatible toolchain and binary environment unless a specific component explicitly defines stronger guarantees.

---

## 10. C ABI Boundary

A C ABI may be provided when a stable binary boundary is required.

Examples:

```text
C application
Plugin boundary
Foreign-language binding
Stable system integration
Long-lived binary interface
```

A C ABI must explicitly define:

* ownership
* allocation/deallocation
* buffer length
* string encoding
* error representation
* handles
* lifetime
* threading
* cancellation
* versioning
* compatibility

The C ABI is therefore an explicit translation boundary, not merely:

```text
extern "C"
```

around arbitrary C++ classes.

---

## 11. C ABI Handles

Opaque handles may represent C++ objects across a C ABI.

Conceptually:

```cpp
typedef struct evolution_processor evolution_processor;
```

The C consumer must not depend on the internal representation of the handle.

The implementation owns the underlying object according to the C API contract.

---

## 12. C ABI Ownership

Every pointer crossing the C ABI must have explicit ownership semantics.

For example:

```text
Caller-owned input
Library-borrowed input
Library-owned output
Caller-released output
Library-released resource
```

The API must never require callers to infer ownership from:

* pointer type
* naming convention alone
* allocation implementation
* undocumented behavior

---

## 13. C ABI Errors

The C ABI must translate internal `Result<T>` / `Error` semantics into a stable C representation.

The C boundary must not expose C++ exceptions as its normal error mechanism.

Conceptually:

```text
C++ Result<T>
      ↓
C ABI translation
      ↓
status / error representation
```

Human-readable error messages remain diagnostic rather than the machine contract.

---

## 14. C ABI Versioning

A C ABI requires explicit compatibility rules.

Potential mechanisms include:

```text
API version
ABI version
Capability query
Structure size
Function availability
```

No specific mechanism is selected yet.

An ABI change must not be silently presented as compatible.

---

## 15. Serialization Boundary

Serialization is another stable external boundary.

For example:

```text
C++ Object
    ↓
Serializer
    ↓
Wire Representation
```

A consumer can therefore remain independent of the C++ object representation.

Serialization contracts follow ADR 0032.

---

## 16. Extension ABI

EVolution extensions may eventually be:

```text
Statically linked
Dynamically loaded
Out-of-process
Remote
```

The Registration and Discovery Model does not require a particular mechanism.

If extensions cross a dynamic binary boundary, the C++ ABI must not be assumed stable merely because both sides were written in C++.

A dynamic extension boundary therefore requires either:

```text
Compatible controlled C++ toolchain
```

or:

```text
Explicit stable ABI
```

such as a C ABI or protocol boundary.

---

## 17. Extension Interface Stability

An extension interface is more than a function signature.

Compatibility may depend on:

* lifecycle
* ownership
* concurrency
* cancellation
* error semantics
* configuration
* identity
* versioning
* provenance
* delivery semantics

Therefore:

```text
Same function signatures
    ≠
Semantically compatible extension
```

---

## 18. Public Types

Public value types should have well-defined:

* ownership
* lifetime
* copy semantics
* move semantics
* equality semantics
* identity semantics
* serialization behavior where relevant

A public type must not expose implementation-specific invariants accidentally.

---

## 19. Exceptions in Public C++ APIs

Public C++ APIs follow the Error Model.

Expected operational failures should normally be represented through:

```text
Result<T>
```

rather than exceptions.

Exceptions remain permitted for situations explicitly covered by the project's exception policy, such as certain construction or third-party boundaries.

A public API must document its exception behavior where exceptions are possible.

---

## 20. `noexcept`

`noexcept` is part of the C++ contract where relevant.

It must not be added merely for optimization or style.

A function should be `noexcept` only when its contract can actually guarantee non-throwing behavior.

The exact `noexcept` policy for every public type remains an implementation-level decision.

---

## 21. Templates in Public APIs

Templates may be used in public APIs when they provide meaningful type-safe abstraction.

However, public templates increase:

* compile-time dependency
* implementation exposure
* binary complexity
* error-message complexity
* compatibility surface

Therefore public templates should remain deliberate and relatively simple.

Heavy template metaprogramming is discouraged.

---

## 22. Concepts in Public APIs

C++20 concepts may be used where they materially improve interface constraints.

Concepts should express meaningful semantic requirements rather than merely make APIs appear generic.

A concept does not replace documentation of:

* ownership
* ordering
* temporal semantics
* concurrency
* error behavior
* lifetime

---

## 23. `std::string_view`, `std::span`, and Borrowed Types

Borrowed views may be used in APIs where lifetime is explicit.

For example:

```cpp
std::span<const std::byte>
std::string_view
```

must not imply ownership.

A borrowed value must not be retained beyond its contract.

Asynchronous APIs require particular care because the original storage may disappear before processing completes.

---

## 24. Asynchronous Public APIs

If EVolution exposes asynchronous operations, the API must explicitly define:

* operation lifetime
* ownership
* completion
* cancellation
* error delivery
* callback/future lifetime
* thread/executor assumptions
* synchronization
* result ownership

An asynchronous API must not require callers to infer these properties from the implementation.

The concrete asynchronous mechanism remains deferred.

---

## 25. Public Processor Contract

The Processor API should expose semantic contracts rather than execution implementation.

A public processor abstraction may eventually express:

```text
Input
Configuration
Execution Context
State
Operation
Output
Result
Lifecycle
Concurrency
Admission
Cancellation
```

It should not require callers to depend directly on:

```text
thread
mutex
queue
executor
scheduler
OS primitive
```

unless those mechanisms are intentionally part of a lower-level API.

---

## 26. Public Storage API

Storage interfaces should express persistence semantics.

For example:

```text
append
lookup
range_scan
ordered_scan
snapshot
batch
transaction
```

where supported.

They should not expose SQL statements or a specific database API as the architectural storage contract.

Physical database APIs belong behind the storage implementation boundary.

---

## 27. Public Query API

Query APIs should expose semantic queries rather than physical query syntax.

For example:

```text
FindEvents
FindMeasurements
QueryTimeSeries
GetStateSnapshot
QueryAnalysis
```

are preferable architectural concepts to exposing:

```text
execute_sql(...)
```

at the application boundary.

A SQL-based implementation may still be used internally.

---

## 28. Public Configuration API

Configuration APIs should distinguish:

```text
Raw Configuration
Resolved Configuration
Effective Configuration
```

A public API should not require consumers to know which source supplied a value in order to use the component.

Configuration validation should return structured errors.

---

## 29. ABI and Memory Allocation

The C++ ABI and C ABI must not silently assume that memory allocated by one binary can safely be released by another.

For a C boundary, allocation ownership must therefore be explicit.

Possible strategies include:

```text
Caller-provided buffer
Library-owned buffer + release function
Allocator pair
Opaque object with destructor function
```

No universal strategy is selected by this ADR.

---

## 30. ABI and Standard Library Types

Standard-library types such as:

```text
std::string
std::vector
std::unique_ptr
std::shared_ptr
std::function
```

should not cross an independently compiled C ABI boundary.

They may be used freely inside a compatible C++ ABI boundary where the toolchain contract permits them.

The external boundary must otherwise translate them into explicitly defined representations.

---

## 31. Binary Plugins

Binary plugins introduce compatibility requirements beyond source compatibility.

A future binary plugin system must define:

* ABI version
* architecture
* compiler/toolchain compatibility
* lifecycle
* ownership
* threading
* error model
* allocation
* dependency loading
* unloading
* security/trust
* extension identity
* capability discovery

Plugin unloading must not occur while objects or callbacks from the plugin remain active.

---

## 32. Symbol Visibility

Internal symbols should not automatically become public binary symbols.

Where shared libraries are used, symbol visibility should be controlled explicitly.

This reduces accidental ABI exposure and makes the public binary surface easier to reason about.

The exact compiler/platform mechanism remains deferred.

---

## 33. ABI and Versioning

ABI compatibility is one dimension of compatibility.

A release may be:

```text
Source compatible
Binary compatible
Wire compatible
Data compatible
Behaviorally compatible
Semantically compatible
```

These properties must not be conflated.

A version number alone does not establish compatibility.

---

## 34. API Evolution

Public API changes should be explicit.

Possible evolution mechanisms include:

```text
Additive change
Deprecation
Replacement
Versioned API
Adapter
Breaking release
```

Removing or changing the meaning of a public operation requires an explicit compatibility decision.

Internal implementation changes do not require public API changes when semantics remain preserved.

---

## 35. API and Provenance

When implementation version affects the semantics of a result, provenance should identify the relevant component/algorithm version.

For example:

```text
Analysis Result
    ↓
Analyzer Version
    ↓
Configuration Version
    ↓
Input Dataset
```

The ABI version itself does not normally belong in analytical provenance unless it materially affects the result.

---

## 36. API and Reproducibility

Reproducibility requires semantic implementation identity rather than merely package identity.

For example:

```text
Input
+
Configuration
+
Algorithm Version
+
Relevant Execution Context
+
Required State
```

may be sufficient.

A compiler build ID may also matter if it materially changes results, but incidental build metadata should not automatically become analytical provenance.

---

## 37. Public API Testing

Public APIs require contract tests covering:

* successful operations
* expected errors
* ownership
* lifetime
* cancellation
* concurrency guarantees
* ordering
* temporal semantics
* serialization
* version compatibility
* ABI behavior where applicable

A public API should not be considered stable merely because it compiles.

---

## 38. API Stability Levels

EVolution may classify APIs as:

```text
Internal
Experimental
Public
Stable
Deprecated
```

These levels describe compatibility expectations.

An experimental API may change without preserving compatibility.

A stable API requires explicit compatibility guarantees.

No universal release-duration guarantee is selected by this ADR.

---

## 39. C vs C++

C++ is the primary native implementation and API language.

C remains appropriate for:

* low-level libraries
* existing C dependencies
* C-compatible interfaces
* systems where C ABI is desirable
* interoperability with non-C++ consumers

C is not required for every module.

C++ is not required to expose every boundary directly.

---

## 40. Foreign-Language Bindings

Other languages may interact with EVolution through:

```text
C ABI
Serialization
IPC
RPC
Generated bindings
```

The preferred mechanism depends on the required lifetime, performance, isolation, and compatibility characteristics.

A language binding must not redefine the underlying semantic contract.

---

## 41. AI-Assisted Implementation

AI-generated code must not infer public API or ABI guarantees from implementation details.

In particular, an implementation agent must not automatically expose:

* internal classes
* internal headers
* C++ implementation types
* storage-specific types
* scheduler types
* executor types

as public APIs merely because they are technically accessible.

Public exposure must be an explicit architectural decision.

---

## 42. Deferred Decisions

This ADR does not select:

* exact public header layout
* namespace hierarchy
* symbol visibility mechanism
* Pimpl policy
* C ABI versioning scheme
* ABI stability duration
* plugin ABI
* allocator ABI
* async API mechanism
* callback vs future vs coroutine
* C++ module usage
* foreign-language binding technology
* semantic-versioning policy
* binary compatibility matrix
* compiler support matrix

These remain implementation or release decisions.

---

## Decision Summary

```text
Primary native language:        C++
C++ standard:                   C++20
Public C++ API:                 Supported
C++ ABI stability:              Not universally guaranteed
Internal C++ API:               Flexible
Stable binary boundary:         Explicit ABI/protocol required
C ABI:                          Supported where appropriate
C ABI ownership:                Explicit
C ABI errors:                   Explicit translation
Serialization boundary:         Supported
Public templates:               Deliberate
Borrowed views:                 Explicit lifetime
Async APIs:                     Explicit contract
Expected failures:              Result<T>
Internal implementation types:  Not public by default
Plugin ABI:                     Deferred
Foreign bindings:               Explicit boundary
API compatibility:              Semantic
ABI compatibility:              Separate concern
```

## Invariant

**EVolution treats its C++ API as an explicit semantic contract rather than an accidental exposure of implementation details: public interfaces define ownership, lifetime, errors, concurrency, temporal, identity, and versioning semantics, while binary compatibility is separately governed and any stable cross-language or long-lived binary boundary must use an explicitly defined ABI or protocol.**

