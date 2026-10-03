# ADR 0002: Build System

## Status

Accepted

## Date

2026-10-03

## Context

EVolution is expected to grow beyond a single executable.

The project may eventually contain:

```text
Core libraries
Processing components
Domain modules
Analysis modules
Storage backends
Ingestion adapters
Applications
Tools
Tests
Examples
Optional extensions
```

The build system therefore needs to support modular compilation and testing from the beginning.

The project also needs to support:

```text
Linux development
multiple build configurations
debugging
sanitizers
unit tests
integration tests
packaging
installation
external dependencies
optional components
```

The build system should remain separate from the architectural model.

---

# Decision

EVolution will use **CMake** as its primary build system.

CMake will generate the native build system required by the selected development environment.

The project will use CMake's target-based model rather than treating the repository as a collection of global compiler flags and source files.

---

# 1. Target-Based Architecture

Each meaningful build component should be represented as a CMake target.

Conceptually:

```text
Core Library
     ↓
Processing Library
     ↓
Domain Library
     ↓
Application
```

For example:

```text
evolution_core
evolution_processing
evolution_poker
evolution_analysis
evolution_storage
evolution_cli
```

These names are conceptual and may change.

The important principle is:

> Dependencies should be expressed between targets rather than through global build configuration.

---

# 2. Dependency Visibility

CMake dependency visibility should reflect the architectural dependency model.

Conceptually:

```text
target_link_libraries(
    evolution_processing
    PRIVATE/PUBLIC
    evolution_core
)
```

The exact visibility should reflect whether the dependency is part of the target's public interface.

The build graph should therefore make architectural dependencies visible.

---

# 3. Build Graph

The build graph should approximately correspond to:

```text
                     Application
                          │
               ┌──────────┼──────────┐
               ↓          ↓          ↓
             Domain     Analysis   Interface
               │          │
               └────┬─────┘
                    ↓
                Processing
                    ↓
                   Core
                    ↑
                    │
                 Storage
```

Not every target must depend directly on every other target.

The build graph should prevent accidental dependencies.

---

# 4. Core as an Independent Target

The Core should be buildable independently.

Conceptually:

```text
cmake
   ↓
evolution_core
```

It should not require:

```text
Poker
GUI
Database
HTTP
CLI
```

just to compile.

This is an important architectural test.

---

# 5. Domain Targets

Each domain should be independently represented where practical.

For example:

```text
evolution_domain_poker
```

may depend on:

```text
evolution_core
evolution_processing
```

but Core must not depend on Poker.

A future domain could therefore be added without modifying the existing domain targets.

---

# 6. Analysis Targets

Analysis components may be organized according to their actual coupling.

For example:

```text
evolution_analysis
```

could contain generic analytical algorithms.

Domain-specific analysis may instead belong to:

```text
evolution_domain_poker
```

or a separate target.

The build system should not force every analytical algorithm into one giant library.

---

# 7. Storage Targets

Storage interfaces and implementations should be separated where useful.

Conceptually:

```text
evolution_storage
        ↑
   ┌────┴────┐
   │         │
 file       SQL
 backend   backend
```

A storage implementation should not force unrelated storage dependencies into Core.

---

# 8. Optional Components

Not every component needs to be built in every configuration.

Examples:

```text
GUI
Python bindings
optional database backend
experimental analysis
benchmark suite
developer tools
```

CMake options may control optional components.

For example:

```text
EVOLUTION_BUILD_TESTS
EVOLUTION_BUILD_TOOLS
EVOLUTION_BUILD_EXAMPLES
```

Exact option names are not fixed by this ADR.

---

# 9. Default Build

The default build should produce a useful development configuration without requiring optional external systems.

The initial default should prioritize:

```text
Core
Processing
primary domain
tests
basic tools
```

Optional dependencies should not unnecessarily prevent a developer from building the Core.

---

# 10. Build Types

The project should support at least:

```text
Debug
Release
```

Additional configurations may later include:

```text
RelWithDebInfo
MinSizeRel
Sanitized
Profiling
Benchmark
```

The exact configurations are implementation details.

---

# 11. Debug Builds

Debug builds should prioritize:

```text
debuggability
assertions
diagnostic information
developer iteration
```

They do not need to represent production performance.

---

# 12. Release Builds

Release builds should prioritize:

```text
optimization
deployment behavior
realistic performance
```

Release configuration must remain reproducible through explicit build configuration.

---

# 13. Sanitizers

The build system should support common sanitizers where the compiler and platform permit them.

Potentially:

```text
AddressSanitizer
UndefinedBehaviorSanitizer
LeakSanitizer
ThreadSanitizer
```

Not every sanitizer is compatible with every configuration.

Sanitizer support should therefore be an explicit build configuration rather than globally enabled.

---

# 14. Compiler Warnings

The project should enable a meaningful warning baseline.

Warnings should be treated as part of development quality rather than cosmetic output.

However, warning policies should be scoped appropriately.

For example:

```text
EVolution source
    strict warnings

Third-party dependency
    external warning policy
```

The project should avoid modifying compiler warning behavior of external dependencies unnecessarily.

---

# 15. Compiler Independence

The architecture should not depend on one compiler-specific extension unless explicitly justified.

The initial development environment may prioritize a particular compiler, but the source should avoid unnecessary compiler lock-in.

Possible compilers include:

```text
GCC
Clang
```

The exact supported compiler matrix will be defined separately.

---

# 16. C++ Standard

The project will select a modern C++ standard appropriate for the supported toolchains.

The exact minimum standard is intentionally left for a separate decision.

The selected standard should remain stable rather than continuously tracking the newest language revision.

The decision should consider:

```text
compiler support
standard library support
distribution availability
language features actually required
long-term maintenance
```

---

# 17. External Dependencies

External dependencies should be represented explicitly in the build graph.

A dependency should not be silently required because some unrelated library happens to install it.

Dependencies should have:

```text
known purpose
version requirements
configuration requirements
license considerations
platform considerations
```

where relevant.

---

# 18. Dependency Minimization

The project should avoid introducing a library merely to solve a trivial problem.

Before adding a dependency, consider:

```text
Can the standard library solve it?
Is the functionality central to EVolution?
Does the dependency introduce significant transitive dependencies?
Does it constrain deployment?
Does it create ABI or licensing concerns?
```

This does not mean avoiding dependencies at all costs.

A well-maintained dependency can be preferable to maintaining complex infrastructure internally.

---

# 19. Fetching Dependencies

The project should distinguish between:

```text
dependency discovery
dependency acquisition
dependency usage
```

CMake should not automatically download arbitrary dependencies during every normal build unless explicitly configured to do so.

This is important for:

```text
offline builds
reproducibility
controlled deployments
packaging
security
```

The exact dependency acquisition strategy will be decided later.

---

# 20. Tests as Build Targets

Tests should be first-class CMake targets.

Conceptually:

```text
evolution_core
     ↓
evolution_core_tests
```

and:

```text
evolution_processing
     ↓
evolution_processing_tests
```

Tests should depend only on the components they actually exercise.

---

# 21. Test Categories

The build system should eventually distinguish at least:

```text
Unit Tests
Integration Tests
System Tests
Benchmarks
```

These have different execution and dependency requirements.

For example:

```text
Unit Test
    → no database required

Integration Test
    → multiple modules

System Test
    → complete application

Benchmark
    → performance measurement
```

---

# 22. CTest

CTest will be used as the test execution layer associated with CMake.

This provides a common mechanism for:

```text
running tests
filtering tests
grouping tests
integrating with CI
```

The use of CTest does not dictate which test framework individual tests use.

---

# 23. Test Framework

The exact unit-test framework is not selected by this ADR.

Candidates may include:

```text
GoogleTest
Catch2
doctest
```

The choice should be made separately based on:

```text
features
build integration
performance
developer experience
dependency cost
```

---

# 24. Benchmarks

Performance-sensitive components should have dedicated benchmarks rather than relying on application runtime measurements alone.

Potential benchmark targets include:

```text
event ingestion
event dispatch
state projection
aggregation
time-series operations
pattern detection
serialization
storage
```

Benchmarks should measure actual workloads.

Performance requirements should not be invented merely because a component appears low-level.

---

# 25. Installation

CMake installation rules should eventually support installing:

```text
libraries
headers
applications
configuration
documentation
```

where appropriate.

The project should distinguish:

```text
build tree
install tree
source tree
```

rather than assuming they are identical.

---

# 26. Packaging

Packaging should be possible independently of the build system.

Potential future package formats include:

```text
Debian package
RPM
tar archive
container image
```

The initial package format is not selected by this ADR.

CMake should provide the installation metadata required by whichever packaging mechanism is selected later.

---

# 27. Exported Targets

If EVolution becomes a reusable library, consumers should be able to depend on exported CMake targets rather than manually reproducing:

```text
include paths
library paths
compiler flags
linker flags
```

Conceptually:

```text
find_package(EVolution)

target_link_libraries(
    application
    EVolution::Core
)
```

The exact package and target naming is deferred.

---

# 28. Generated Code

Some EVolution components may eventually use generated source code.

Examples could include:

```text
serialization
schemas
protocol descriptions
bindings
```

Generated files should have explicit ownership.

The build should distinguish:

```text
source-controlled input
```

from:

```text
generated output
```

Generated files should not silently modify the source tree during ordinary builds unless explicitly intended.

---

# 29. Build Reproducibility

A developer should be able to reconstruct a build from:

```text
source revision
build configuration
compiler/toolchain
dependency versions
```

as far as practical.

Build configuration that affects program behavior should be identifiable.

This supports the provenance requirements defined earlier.

---

# 30. Build Configuration vs Runtime Configuration

These must remain distinct.

Build configuration determines things such as:

```text
compiled components
platform support
optional features
compiler settings
```

Runtime configuration determines things such as:

```text
processing graph
domain configuration
analysis parameters
storage location
input sources
```

Changing an analysis threshold should not require recompiling the application unless the architecture explicitly requires compile-time configuration.

---

# 31. Source Layout Independence

The build system should support modular targets without forcing the final source directory layout prematurely.

For example, these are possible:

```text
core/
processing/
domains/
analysis/
```

but the final directory structure may differ.

The build system should follow the architectural boundaries rather than dictate them.

---

# 32. CI Compatibility

The build should eventually support automated CI stages such as:

```text
configure
build
unit tests
integration tests
sanitizers
static analysis
package
```

The CI system itself is not selected by this ADR.

The important requirement is that the build can be executed non-interactively.

---

# 33. Static Analysis

The project should eventually support tools such as:

```text
clang-tidy
clang-format
cppcheck
compiler diagnostics
```

The exact tool set is deferred.

Static analysis should be integrated as development tooling rather than embedded into runtime behavior.

---

# 34. Formatting

Formatting should be automated and reproducible.

The exact formatting policy is deferred, but the project should eventually define:

```text
indentation
line length
braces
naming
include ordering
header organization
```

A formatter should enforce the mechanical parts of the policy.

---

# 35. Build Directory

Build artifacts should remain outside the source tree when practical.

Conceptually:

```text
evolution/
├── CMakeLists.txt
├── src/
├── include/
├── tests/
└── ...

evolution-build/
├── ...
```

This keeps generated build artifacts separate from source files and simplifies clean builds.

---

# 36. Architectural Validation Through the Build

The build system should help enforce architecture.

For example:

```text
Core target
    must not link Poker

Poker target
    may link Core

Application
    may compose multiple modules
```

Therefore, an incorrect architectural dependency can become a build error rather than merely a documentation violation.

This is an important role of the build system.

---

# 37. Decision Consequences

## Positive

Using CMake provides:

* cross-platform build generation
* target-based dependency management
* integration with CTest
* support for multiple compilers
* installation support
* packaging integration
* mature IDE/editor integration
* explicit build graphs

It also fits naturally with the chosen C++ implementation language.

## Negative

CMake itself has significant complexity.

Potential problems include:

```text
complex configuration files
platform-specific behavior
dependency-management complexity
long configuration times in large projects
```

These should be controlled by keeping the build scripts modular and avoiding unnecessary CMake abstraction layers.

---

# 38. Decision Summary

```text
Build system:
    CMake

Build model:
    target-based

Testing:
    CTest integration

Configurations:
    Debug / Release initially

Quality tooling:
    sanitizers and static analysis supported

Dependencies:
    explicit and controlled

Installation:
    supported through CMake

Packaging:
    deferred

Compiler matrix:
    deferred

C++ standard:
    separate ADR
```

The fundamental decision is:

> **EVolution will use CMake as its target-based build system, with CTest integration and explicit build targets corresponding to architectural components.**
