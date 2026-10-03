# ADR 0003 — C++ Language Standard

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution is primarily implemented in C++.

The project requires a language baseline that provides modern C++ facilities while remaining conservative enough for a systems-oriented project that may be built across different Linux/Windows environments and maintained for a long time.

The language standard should be independent from:

* compiler vendor
* compiler version
* standard library implementation
* build system
* operating system distribution
* specific third-party libraries

The C++ standard should also provide useful capabilities for expressing EVolution's architectural contracts without encouraging unnecessary abstraction or framework complexity.

## Decision

**EVolution uses C++20 as its minimum required C++ language standard.**

Source code must be valid standard C++20 and must not require compiler-specific language extensions.

The project may adopt individual C++23 or later facilities in the future, but doing so requires an explicit architectural decision and must not silently raise the project's language baseline.

## Rationale

### Modern language capabilities

C++20 provides important facilities that fit EVolution's architecture, including:

* concepts
* `std::span`
* improved `constexpr`
* ranges
* `std::jthread`
* improved chrono facilities
* structured concurrency-related primitives
* improved language support for generic programming

These facilities can help express contracts and data transformations without requiring large framework abstractions.

### Systems programming requirements

EVolution needs:

* deterministic resource ownership
* predictable object lifetime
* direct memory control
* value-oriented data structures
* interoperability with C
* low runtime overhead
* access to low-level operating-system facilities when required

C++20 provides these capabilities without introducing a managed runtime.

### Stability

The project should prefer a mature language baseline over continuously adopting the newest standard.

The goal is not to use every feature available in C++20. The standard provides a vocabulary from which appropriate features can be selected.

### AI-assisted development

A fixed language baseline is particularly important because EVolution will be developed with substantial AI assistance.

AI-generated code must have a clearly defined language boundary. Agents should not independently introduce newer language features simply because they are available in their generation environment.

Therefore:

> **C++20 is the architectural contract; newer language features require an explicit decision.**

## Language Usage Principles

The project should generally favor:

* RAII
* value semantics
* explicit ownership
* `std::unique_ptr` where dynamic ownership is required
* `std::shared_ptr` only where shared ownership is genuinely required
* `std::span` for non-owning contiguous views
* `std::string_view` for non-owning string views where lifetime is clear
* concepts where they make generic interfaces clearer
* `constexpr` where it provides meaningful compile-time guarantees
* standard containers and algorithms
* strong types where they prevent meaningful classes of errors
* composition over inheritance
* explicit interfaces
* simple data structures

The project should avoid using language features merely because they are available.

## Restrictions

The following are not architectural requirements merely because C++20 supports them:

* modules
* coroutines
* concepts everywhere
* ranges everywhere
* complex template metaprogramming
* deep inheritance hierarchies
* heavy operator overloading
* extensive compile-time computation
* macros as substitutes for language facilities

Each should be used only where it provides a concrete architectural or implementation benefit.

### Compiler Extensions

Portable source code should not depend on extensions such as:

```text
gnu++20
__attribute__
compiler-specific language syntax
compiler-specific built-in types
```

Compiler-specific facilities may be isolated behind explicit platform/compiler boundaries when there is a justified systems requirement.

The default project language mode should therefore represent standard C++20 rather than a compiler-specific C++ dialect.

## C and C++ Interoperability

C remains an important part of the system.

Existing C libraries, low-level system interfaces, performance-sensitive components, and C-compatible APIs may remain implemented in C.

C++ components that need to expose stable interfaces to C should use explicit C-compatible boundaries where appropriate.

For example:

```text
C++ implementation
        │
        ├── internal C++ interface
        │
        └── extern "C" boundary
                    │
                    ▼
              C consumer
```

The choice between a C and C++ implementation remains a component-level decision and does not change the project's C++ baseline.

## Standard Library

The project uses the C++ standard library as the default general-purpose library.

A third-party library should not be introduced merely to replace functionality already adequately provided by the standard library.

Third-party dependencies may still be used where they provide capabilities outside the practical scope of the standard library or provide important specialized functionality.

Dependency selection is governed separately from the language-standard decision.

## Error Handling

This ADR does **not** establish the project's complete error-handling policy.

In particular, it does not decide:

* whether exceptions are allowed
* when error codes should be used
* whether `std::expected` or an equivalent abstraction should be used
* how errors cross module boundaries
* how failures are represented in processing graphs

Those decisions belong to a separate architectural decision.

## Toolchain Requirements

The project must require a toolchain capable of compiling the required C++20 language and standard-library features.

Exact minimum versions for:

* GCC
* Clang
* MSVC, if ever required
* standard libraries

are intentionally deferred.

Toolchain compatibility should be established separately rather than encoded into this architectural decision.

## Build Configuration

The build system should request the C++20 standard explicitly.

Conceptually:

```text
C++ standard = C++20
C++ extensions = disabled
C++ standard = required
```

The exact CMake implementation belongs to the build configuration.

## Future Standard Upgrades

A future upgrade to C++23, C++26, or a later standard must consider:

1. compiler availability
2. standard-library availability
3. deployment environments
4. CI support
5. dependency compatibility
6. build reproducibility
7. benefits to the architecture
8. migration cost

A newer standard should not be adopted solely because it is newer.

When a newer standard becomes the project baseline, this ADR should be superseded by a new ADR rather than silently modified.

## Consequences

### Positive

* Modern C++ facilities are available.
* The project has a clear language boundary.
* AI-generated code has an explicit baseline.
* Compiler-specific extensions are discouraged.
* Long-term portability is improved.
* C interoperability remains straightforward.
* Future language upgrades can be deliberate.

### Negative

* Code requiring newer standards cannot be used by default.
* Some newer standard-library facilities may need project-specific alternatives.
* Developers must occasionally avoid convenient features from newer standards until the baseline is upgraded.

## Decision Summary

```text
Language:             C++20
Minimum standard:     C++20
Compiler extensions:  Disabled by default
Standard library:     C++ standard library by default
C interoperability:   Supported
C++23+:               Requires explicit architectural decision
Modules:              Not required
Coroutines:           Not required
Template complexity:  Keep justified and controlled
```

## Invariant

> **EVolution code must require no language standard newer than the project's explicitly adopted C++ baseline.**
