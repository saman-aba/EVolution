# ADR 0001: Initial Implementation Language

## Status

Accepted

## Date

2026-10-03

## Context

EVolution is a generic event-driven analytical platform with the following architectural requirements:

* generic Core abstractions
* domain-specific extensions
* explicit module boundaries
* composable processors
* analytical algorithms
* stateful and stateless processing
* streaming and batch execution
* provenance and identity
* potentially large volumes of historical data
* deterministic replay
* extensibility
* long-term maintainability
* ability to expose functionality to other languages or processes later

The initial implementation language must therefore support both low-level control and sufficiently expressive abstractions for a growing analytical system.

The project should not select a language merely because a particular component is easier to implement in it.

---

# Decision

EVolution will initially be implemented primarily in **C++**.

The initial language target will be a modern, standardized C++ version supported by the project's toolchain.

The exact minimum language version will be decided separately.

C remains an important interoperability language and may be used for:

```text
C-compatible interfaces
low-level libraries
existing C libraries
performance-critical components where appropriate
external integration
```

However, C is not the primary implementation language of the EVolution architecture.

---

# Rationale

## 1. EVolution Is Larger Than a Collection of Algorithms

The system contains many interacting concepts:

```text
Event
State
Measurement
TimeSeries
Aggregation
Pattern
Analysis
Context
Provenance
Identity
Processor
ProcessingGraph
Storage
Domain
```

These concepts have relationships, ownership, lifecycle, and contracts.

C can represent all of these.

However, representing them consistently requires manually constructing many mechanisms that C++ provides directly through the language.

The architecture therefore benefits from stronger abstraction mechanisms without requiring a managed runtime.

---

## 2. Systems-Level Control Remains Important

EVolution is not intended to be a high-level scripting application.

Important requirements include:

```text
memory ownership
allocation behavior
data layout
copying
moving
lifetime
I/O
concurrency
CPU efficiency
cache behavior
binary interfaces
```

C++ provides direct control over these areas while allowing higher-level abstractions when they are useful.

This is important because the platform may eventually process substantial event streams and analytical datasets.

---

## 3. RAII and Resource Ownership

EVolution will interact with resources such as:

```text
files
memory
threads
locks
sockets
storage handles
external libraries
temporary processing resources
```

Resource ownership should be explicit.

C++ RAII provides a language-level mechanism for associating resource lifetime with object lifetime.

This can reduce the number of manual cleanup paths that must be maintained.

The architectural principle remains:

> Resource ownership must be explicit regardless of language.

C++ provides a strong mechanism for implementing that principle.

---

# 4. Generic Contracts

The architecture relies heavily on explicit contracts.

For example:

```text
Processor
Analyzer
Storage
Ingestion
Interface
```

These contracts can be represented naturally using C++ abstractions.

This does not mean every abstraction must become a deep class hierarchy.

The architecture explicitly rejects unnecessary inheritance-based design.

The preferred principle is:

> Use the simplest language mechanism that expresses the required contract.

Possible mechanisms include:

```text
structs
free functions
templates
concepts
composition
function objects
interfaces
type erasure
```

The specific mechanism will be selected per component.

---

# 5. Composition Over Framework-Like Inheritance

Choosing C++ does not imply that EVolution should become an object-oriented framework.

For example, this is not an architectural requirement:

```text
class BaseProcessor
    ↓
class MeasurementProcessor
    ↓
class PokerMeasurementProcessor
```

A processor may instead be represented using:

```text
data + functions
```

or:

```text
concepts + templates
```

or:

```text
explicit interfaces
```

depending on the problem.

The architecture defines the contract, not the implementation style.

---

# 6. Value Semantics

Many EVolution objects naturally behave as values:

```text
Event
Measurement
Identity
Context
Provenance
Configuration
Analysis Result
```

C++ provides strong support for value semantics while still allowing explicit control over copying and allocation.

This is useful for immutable or effectively immutable analytical data.

---

# 7. Performance Without a Separate High-Level Runtime

A major goal is to avoid requiring a managed runtime merely to execute the Core.

C++ allows the system to operate close to the operating system and hardware while still providing abstractions needed by the architecture.

This is particularly relevant for:

```text
large event streams
high-frequency processing
replay
large analytical datasets
parallel processing
low-latency ingestion
```

Performance is not assumed to be important everywhere.

Instead:

> Performance-sensitive boundaries should remain measurable and optimizable without changing the semantic architecture.

---

# 8. Existing Ecosystem

C++ also provides access to a mature ecosystem for:

```text
numerical computing
serialization
networking
concurrency
storage
testing
profiling
parallelism
system integration
```

The architecture should not become dependent on any particular library merely because the language is C++.

Libraries will be selected later according to explicit requirements.

---

# 9. C Interoperability

C remains important.

Many systems libraries and existing components expose C APIs.

The architecture should therefore allow:

```text
C Library
    ↓
C++ Adapter
    ↓
EVolution
```

where appropriate.

Similarly, selected EVolution functionality may eventually expose a C-compatible API:

```text
Other Language
      ↓
C ABI
      ↓
EVolution
```

This can provide an interoperability boundary without requiring the internal architecture to be written in C.

---

# 10. Scripting Interoperability

The choice of C++ does not determine the future scripting language.

Potential future integrations could include:

```text
Python
Lua
other scripting/runtime environments
```

The intended architecture is:

```text
Script
   ↓
Explicit EVolution API
   ↓
EVolution
```

rather than exposing arbitrary internal C++ implementation details.

Therefore scripting remains an extension concern rather than a Core-language decision.

---

# 11. Binary Boundaries

C++ implementation details should not automatically become public ABI.

Internal APIs may freely use appropriate C++ types.

Stable external boundaries should use explicitly defined interfaces.

For example:

```text
Internal:
    C++ API

External:
    C ABI
    RPC
    serialized protocol
    command interface
```

depending on the integration requirement.

This prevents ABI stability requirements from unnecessarily constraining the internal architecture.

---

# 12. C vs C++

The decision is not based on the claim that C++ is universally superior to C.

Both languages can implement the architecture.

The distinction is primarily about the cost of expressing the architecture.

C provides:

```text
simple data representation
predictable low-level behavior
minimal language machinery
excellent interoperability
```

C++ additionally provides:

```text
RAII
value semantics
templates
generic programming
stronger type abstraction
move semantics
standard containers
algorithms
resource-management abstractions
```

For the size and extensibility of EVolution, these capabilities are considered useful enough to justify C++ as the primary language.

---

# 13. C++ Usage Restrictions

Choosing C++ does not mean using every feature of C++.

The project should initially avoid unnecessary complexity such as:

```text
deep inheritance hierarchies
complex template metaprogramming
global mutable state
uncontrolled exceptions
macro-heavy frameworks
unnecessary RTTI
implicit ownership
over-engineered abstractions
```

The project should favor:

```text
clear ownership
small interfaces
composition
value semantics
explicit dependencies
simple data structures
measurable performance
testable components
```

---

# 14. Exceptions

Exception usage remains an implementation policy rather than an architectural requirement.

Some subsystems may benefit from exceptions for exceptional control flow.

Other subsystems may require explicit error values because of:

```text
performance
C interoperability
real-time constraints
error propagation requirements
```

The project will define an explicit error-handling policy separately.

No assumption is made here that all errors must use exceptions or that exceptions are prohibited.

---

# 15. Memory Management

The language decision does not imply a single allocation strategy.

Different components may use:

```text
stack allocation
standard allocation
object pools
arenas
custom allocators
memory-mapped storage
zero-copy representations
```

where justified.

The architecture should not prematurely optimize memory management.

Memory strategy should follow actual workload measurements.

---

# 16. Concurrency

C++ provides concurrency primitives, but the architecture does not yet select a concurrency model.

The system may eventually use:

```text
threads
thread pools
event loops
work queues
parallel algorithms
processes
```

or combinations of these.

Concurrency remains a separate architectural decision.

The logical processing model must remain independent of the execution mechanism.

---

# 17. Implementation Principle

The language should serve the architecture rather than redefine it.

Therefore:

```text
Architecture
    ↓
Contracts
    ↓
C++ Implementation
```

not:

```text
C++ Language Feature
    ↓
Architectural Concept
```

For example, the existence of C++ classes does not mean every architectural object must be a class.

---

# 18. Consequences

## Positive

The decision provides:

* low-level control
* strong resource-management mechanisms
* efficient value types
* generic programming
* mature systems ecosystem
* good C interoperability
* flexibility for both high-performance and high-level components

It also allows the Core to remain relatively close to the machine without sacrificing abstraction.

## Negative

C++ introduces additional complexity compared with C.

Potential problems include:

```text
large language surface
complex type systems
template complexity
ABI considerations
multiple valid design styles
easy misuse of abstractions
```

These risks must be controlled through project conventions and review.

---

# 19. What This Decision Does Not Decide

This ADR does not decide:

```text
C++ version
build system
compiler
standard library implementation
GUI framework
database
serialization format
threading model
plugin mechanism
scripting language
network framework
IPC mechanism
deployment model
```

Each significant choice should be evaluated independently.

---

# 20. Future Reconsideration

This decision may be revisited if practical requirements demonstrate that C++ creates unacceptable constraints.

Possible reasons could include:

```text
target platform restrictions
required ABI compatibility
specialized runtime requirements
deployment constraints
performance characteristics
integration requirements
```

Such a change would require a new ADR rather than silently changing the implementation policy.

---

# 21. Decision Summary

```text
Primary implementation language:
    C++

C:
    supported as an interoperability and systems-level language

Architecture:
    language-independent

Design style:
    composition and explicit contracts

Primary priorities:
    correctness
    explicit ownership
    modularity
    testability
    performance where measured
    long-term extensibility
```

The fundamental decision is:

> **EVolution will use modern C++ as its primary implementation language while preserving C interoperability and avoiding unnecessary dependence on C++-specific implementation details at architectural boundaries.**
