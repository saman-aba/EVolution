# ADR 0004 — Source Tree and Target Structure

**Status:** Accepted
**Date:** 2026-10-03

## Context

The architectural documents define several logical modules:

* Core
* Processing
* Domain
* Analysis
* Storage
* Ingestion
* Application
* Interface

The repository now needs a physical source-tree structure that reflects these boundaries.

The structure must support:

* independent development of components
* explicit CMake dependencies
* multiple domains
* multiple applications
* optional implementations
* unit and integration testing
* AI-assisted development
* future growth without turning the repository into one large source tree

The physical directory structure should represent architectural ownership, but it must not prematurely dictate runtime deployment, threading, plugin loading, or storage technology.

## Decision

EVolution will use a **module-oriented source tree** in which major architectural boundaries have corresponding top-level directories and CMake targets.

The initial structure is:

```text
evolution/
├── CMakeLists.txt
├── README.md
│
├── docs/
│   ├── architecture/
│   ├── concepts/
│   └── decisions/
│
├── core/
├── processing/
├── domains/
│   └── poker/
├── analysis/
├── storage/
├── ingestion/
├── applications/
├── interfaces/
│
├── tests/
├── tools/
│
└── cmake/
```

The exact internal file layout of each module remains the responsibility of that module.

## Architectural Mapping

The physical tree maps to the logical architecture as follows:

```text
core/           → Generic EVolution concepts and infrastructure
processing/     → Processing graph and execution mechanisms
domains/        → Domain-specific semantics
analysis/       → Analytical algorithms and methods
storage/        → Persistence implementations and abstractions
ingestion/      → External-data ingestion and normalization
applications/   → Executable workflows
interfaces/     → External interaction boundaries
tests/          → Cross-module and system tests
tools/          → Developer/operator utilities
cmake/          → Reusable CMake infrastructure
```

The mapping is intentional:

```text
Physical boundary
        ↓
Architectural boundary
        ↓
CMake target boundary
```

These three boundaries should normally align.

## Core

`core/` contains generic concepts and mechanisms defined by EVolution's conceptual model.

Examples include:

```text
Event
State
Measurement
Metric
TimeSeries
Aggregation
Pattern
Analysis
Context
Provenance
Identity
```

It may also contain generic supporting facilities required by those concepts.

Core must remain domain-independent.

For example:

```text
core/
└── include/
    └── evolution/
        └── core/
```

The exact header/source organization is intentionally not fixed yet.

Core must not depend on:

```text
domains/
applications/
interfaces/
poker-specific code
database implementations
GUI frameworks
HTTP frameworks
```

## Processing

`processing/` contains mechanisms for executing transformations and processing graphs.

Responsibilities include concepts such as:

* processors
* processing graphs
* dependencies
* execution modes
* streaming execution
* batch execution
* replay
* incremental processing
* windows
* execution state
* processing failures

Processing may depend on Core.

Processing must not contain Poker-specific semantics.

For example:

```text
processing/
└── include/
    └── evolution/
        └── processing/
```

## Domains

`domains/` contains domain-specific meaning.

The first domain is Poker:

```text
domains/
└── poker/
```

A domain may define:

* domain entities
* domain events
* domain state
* domain metrics
* domain patterns
* domain-specific analysis
* domain policies

For example:

```text
domains/
└── poker/
    ├── include/
    ├── src/
    └── CMakeLists.txt
```

Additional domains should be added beside Poker rather than placed inside Poker:

```text
domains/
├── poker/
├── crypto/
├── simulation/
└── ...
```

A domain must not require another unrelated domain merely because both use the same Core concepts.

## Analysis

`analysis/` contains reusable analytical mechanisms.

This directory is distinct from domain definitions.

For example, generic analytical mechanisms may include:

* statistical analysis
* anomaly detection
* correlation
* regression
* clustering
* pattern detection
* forecasting
* time-series transformations

Domain-specific analysis may remain inside the corresponding domain when its semantics are inseparable from that domain.

The boundary is therefore:

```text
Generic analytical method
        ↓
analysis/

Domain-specific interpretation
        ↓
domains/<domain>/
```

The exact split can be decided when real analytical components are implemented.

## Storage

`storage/` contains persistence abstractions and implementations.

Potential future implementations include:

```text
storage/
├── memory/
├── filesystem/
├── sql/
├── key_value/
└── ...
```

These are possibilities, not commitments.

The project must not create directories for storage technologies before they are actually needed.

Storage implementations must not become the source of domain semantics.

For example, a database adapter may know how to persist a `Measurement`, but it must not decide what a Poker measurement means.

## Ingestion

`ingestion/` contains mechanisms that transform external representations into EVolution's internal model.

Examples may eventually include:

```text
ingestion/
├── file/
├── network/
├── replay/
└── ...
```

Again, directories should be introduced when the corresponding implementation exists.

Ingestion may depend on domain definitions when an external representation must be translated into domain-specific events.

The important boundary is:

```text
External representation
        ↓
Ingestion
        ↓
EVolution representation
```

## Applications

`applications/` contains executable compositions of EVolution capabilities.

An application determines how components are assembled for a particular purpose.

For example:

```text
applications/
└── evolution-cli/
```

Later there may be:

```text
applications/
├── evolution-cli/
├── poker-analyzer/
├── replay-tool/
└── ...
```

An application may select:

* domain
* processing graph
* analysis
* storage
* ingestion
* interfaces
* configuration

Applications are therefore composition roots rather than generic libraries.

## Interfaces

`interfaces/` contains external interaction mechanisms.

Possible interfaces include:

* CLI
* HTTP
* gRPC
* IPC
* scripting bindings

No particular interface technology is selected by this ADR.

The important boundary is:

```text
External user/system
        ↓
Interface
        ↓
Application / domain capabilities
```

Interfaces should not contain the actual analytical algorithms.

## Tests

Testing is treated as a first-class part of the architecture.

The initial structure is:

```text
tests/
├── unit/
├── integration/
├── system/
└── benchmarks/
```

The exact distribution of tests may evolve.

### Unit Tests

Test individual components with minimal dependencies.

### Integration Tests

Test interactions between architectural components.

### System Tests

Test complete application-level behavior.

### Benchmarks

Measure performance-sensitive components without treating benchmarks as ordinary correctness tests.

Tests should normally depend on the component they test rather than bypassing its public contract.

## Tools

`tools/` contains developer or operational utilities that are not part of the primary EVolution runtime architecture.

Examples may include:

* data inspection utilities
* development generators
* debugging tools
* migration utilities
* benchmark helpers

A tool should not become a hidden dependency of the core system.

## CMake Target Structure

Each major reusable component should normally correspond to a CMake target.

Conceptually:

```text
evolution_core
evolution_processing
evolution_analysis
evolution_storage
evolution_ingestion
evolution_poker
```

Applications should be executable targets:

```text
evolution_cli
poker_analyzer
...
```

The exact target names can change if a better naming convention is established.

Dependencies should be expressed explicitly.

For example:

```text
evolution_processing
        ↓
evolution_core

evolution_poker
        ↓
evolution_core
        ↓
evolution_analysis (when required)

poker_analyzer
        ↓
evolution_poker
        ↓
evolution_processing
        ↓
evolution_core
```

A dependency must correspond to an actual architectural requirement.

## Public and Private Source

Reusable modules should distinguish public interfaces from implementation details.

Conceptually:

```text
module/
├── include/
│   └── evolution/
│       └── module/
├── src/
└── CMakeLists.txt
```

Public headers belong under `include/`.

Implementation files belong under `src/`.

Not every module must expose a public API. Internal components may remain private to their target.

This structure is a default rather than an absolute requirement.

## Header Dependency Rule

Including a header creates a dependency.

Therefore:

> **Header dependencies should follow the same architectural direction as library dependencies.**

In particular, Core headers must never include domain headers.

This prevents accidental architectural coupling such as:

```text
core/event.hpp
        ↓
poker/hand.hpp
```

Instead:

```text
poker/hand_event.hpp
        ↓
core/event.hpp
```

The generic abstraction remains below the domain-specific interpretation.

## Dependency Direction

The preferred dependency direction is:

```text
                Applications
                     ↓
        ┌────────────┴────────────┐
        ↓                         ↓
     Domain                    Interfaces
        ↓
     Analysis
        ↓
   Processing
        ↓
      Core
```

Storage and ingestion attach through explicit interfaces where appropriate:

```text
              Applications
              /    |     \
             ↓     ↓      ↓
        Domain  Ingestion Storage
             \    |      /
              Processing
                   ↓
                 Core
```

The exact dependency graph of an implementation may differ, but dependencies must not violate the architectural boundaries defined in the previous ADRs.

## Optional Components

Not every component must be built by every user.

CMake options may eventually control optional components such as:

```text
Poker domain
Database storage
Network ingestion
Specific interfaces
Developer tools
Benchmarks
```

Core should remain buildable without optional domain or application components.

## Repository Growth

The repository should grow by adding modules rather than accumulating unrelated source files at the root.

Avoid structures such as:

```text
src/
├── event.cpp
├── poker.cpp
├── postgres.cpp
├── http.cpp
├── analyzer.cpp
├── ...
```

when those files belong to different architectural responsibilities.

The source tree should make ownership visible.

## What This ADR Does Not Decide

This decision does not establish:

* exact class hierarchy
* exact header names
* namespace naming beyond general structure
* threading model
* process model
* plugin mechanism
* dynamic library policy
* serialization format
* database technology
* GUI framework
* network framework
* IPC mechanism
* scripting language
* packaging format
* installation layout
* deployment topology

Those decisions remain separate.

## Consequences

### Positive

* The repository reflects the architectural model.
* CMake targets can enforce module boundaries.
* Domains can evolve independently.
* Applications remain composition roots.
* Core remains reusable.
* AI agents can be assigned work against explicit module boundaries.
* New domains do not require restructuring the entire repository.
* Implementation details remain separated from public contracts.

### Negative

* Small components may initially require more directories and CMake targets.
* Some functionality will require deciding which architectural boundary owns it.
* Strict boundaries can occasionally require adapters instead of direct dependencies.

These costs are intentional because architectural ambiguity becomes more expensive as the project grows.

## Initial Source Tree

The initial repository therefore becomes:

```text
evolution/
├── CMakeLists.txt
├── README.md
│
├── docs/
│   ├── architecture/
│   ├── concepts/
│   └── decisions/
│
├── core/
├── processing/
├── domains/
│   └── poker/
├── analysis/
├── storage/
├── ingestion/
├── applications/
├── interfaces/
│
├── tests/
├── tools/
└── cmake/
```

Directories that have no implementation yet may remain empty or be introduced when their first component is created.

## Invariant

> **The physical source tree and CMake target structure should make architectural ownership and dependency direction visible rather than hiding them behind a flat implementation tree.**
