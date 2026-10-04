# ADR 0050: Packaging, Installation, and Release Model

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution separates **building**, **packaging**, **installation**, and **release**.

These are related but distinct lifecycle stages:

```text
Source
  ↓
Build
  ↓
Artifacts
  ↓
Package
  ↓
Install
  ↓
Runtime
  ↓
Release
```

The architecture must not assume that the build tree, installation tree, package format, or deployment environment are the same thing.

Packaging and release mechanisms must preserve the architectural contracts established by the system without making the Core dependent on a particular operating system, package manager, or deployment platform.

---

## 1. Build vs Package

A build produces implementation artifacts.

A package produces a distributable unit containing the artifacts and metadata required by a target installation environment.

Therefore:

```text
Build Artifact
    ≠
Package
```

A build may produce:

* libraries
* executables
* headers
* generated files
* tests
* tools
* documentation

A package may contain only the subset required for a particular distribution.

---

## 2. Installation

Installation places artifacts into a target environment according to an explicit layout.

Installation may include:

* libraries
* executables
* headers
* configuration templates
* schemas
* resources
* service definitions
* documentation
* metadata

Installation must not require the source tree.

The installed system must have a clear distinction between:

```text
Application Files
Configuration
Runtime State
Persistent Data
Logs / Observability Data
```

These categories must not be mixed merely because the target filesystem permits it.

---

## 3. Source Tree vs Build Tree vs Install Tree

The architecture distinguishes:

```text
Source Tree
    → project source and documentation

Build Tree
    → compiler/build/generated artifacts

Install Tree
    → runtime/development artifacts intended for consumers
```

Generated build files must not become required source artifacts unless explicitly generated and versioned.

The source tree must remain independently understandable and reproducible.

---

## 4. Runtime Data

Runtime-generated information must not normally be written into the installation tree.

Examples include:

* databases
* checkpoints
* caches
* PID files
* sockets
* runtime state
* logs
* temporary files

These belong to explicit runtime/storage locations.

Conceptually:

```text
Install Tree
    ↓
Immutable application artifacts

Runtime State
    ↓
Separate writable location
```

This allows the same installation to support multiple executions or users where appropriate.

---

## 5. Configuration Installation

Packages may provide configuration templates or defaults.

However:

```text
Installed Default
    ≠
Effective Runtime Configuration
```

The application must still resolve effective configuration according to the Configuration Model.

Package installation must not silently create hidden runtime configuration.

---

## 6. Development vs Runtime Installation

Development installations may include:

* public headers
* CMake package configuration
* static/shared libraries
* development tools
* test support

Runtime installations may contain only what is required to execute the application.

The architecture should allow these to be packaged separately where useful.

---

## 7. Public Artifacts

An artifact becomes a public development artifact only when explicitly designated.

Examples:

```text
Public API Header
Public Library
CLI Tool
Plugin/Extension Interface
Schema
Protocol Definition
```

Internal headers and implementation files must not accidentally become public API merely because they are installed.

Public API stability follows the API and Versioning Model.

---

## 8. Package Contents

A package should have an explicit manifest of its contents.

Conceptually:

```text
Package
{
    identity
    version
    target
    artifacts
    dependencies
    configuration
    metadata
}
```

The exact package metadata format remains implementation-dependent.

Packages should not contain arbitrary files merely because they happen to exist in the build tree.

---

## 9. Dependency Declaration

Runtime dependencies must be declared where required.

A package may depend on:

* system libraries
* runtime libraries
* external services
* configuration
* operating-system capabilities
* other EVolution packages

A package dependency is not automatically equivalent to a source-level C++ dependency.

For example:

```text
Build Dependency
    ≠
Runtime Dependency
    ≠
External Service Dependency
```

These must remain distinguishable.

---

## 10. Static and Dynamic Dependencies

EVolution may use statically or dynamically linked dependencies.

The packaging model must support both where implementation requires them.

The choice between static and dynamic linkage must not alter the semantic contract of the application.

Where dynamic libraries are used, the runtime package must provide or declare compatible versions according to the deployment contract.

---

## 11. Runtime Dependency Validation

Installation does not guarantee that all runtime dependencies are currently usable.

For example:

```text
Package installed
    ≠
Database available
```

Runtime dependency availability follows the External Dependency Model.

Applications must validate required runtime capabilities during initialization or at the appropriate execution boundary.

---

## 12. Versioning

A release may contain multiple independently versioned artifacts.

For example:

```text
Application Version
Core Version
Domain Version
Processor Version
Schema Version
Extension Version
Configuration Version
```

A package version must not be treated as the universal semantic version of all included components.

The API and Versioning Model governs compatibility.

---

## 13. Release Identity

A release should have a unique release identity.

Conceptually:

```text
Release
{
    version
    source revision
    build identity
    artifact identities
    dependency information
    metadata
}
```

The exact representation is deferred.

Release identity is distinct from:

```text
RunId
ComponentId
PackageId
GraphId
ConfigurationId
```

---

## 14. Reproducible Builds

The packaging and release process should support reproducibility where practical.

A reproducible build should identify materially relevant:

* source revision
* compiler/toolchain
* build configuration
* dependency versions
* generated artifacts
* packaging configuration

The system must distinguish:

```text
Build Reproducibility
    ≠
Processing Reproducibility
```

A reproducible binary does not automatically make analytical results reproducible.

---

## 15. Build Metadata

Build metadata may be embedded into artifacts when useful.

Examples include:

```text
Version
Source Revision
Build Type
Build Timestamp
Compiler Information
Dependency Information
```

Build metadata must not silently become application semantic configuration.

A build timestamp, for example, should not affect analytical behavior unless explicitly intended.

---

## 16. Release Artifacts

A release may contain multiple artifact types:

```text
Libraries
Executables
Packages
Headers
Schemas
Documentation
Examples
Debug Symbols
Checksums / Signatures
```

Not every artifact must be distributed to every target.

The release process should identify artifact relationships explicitly.

---

## 17. Debug and Symbol Artifacts

Debug symbols may be distributed separately from runtime packages.

Separating them allows:

```text
Runtime Package
    +
Debug Information
```

without requiring production systems to install development artifacts.

The exact symbol packaging mechanism is deferred.

---

## 18. Documentation

Documentation may be distributed independently or as part of a release.

Documentation describing public contracts should correspond to the released version of those contracts.

Generated API documentation must not be treated as the sole architectural specification.

The `docs/` architecture and decision documents remain source-controlled project artifacts.

---

## 19. Database and Persistent Data

Installing a new software version must not automatically imply destructive modification of persistent data.

Persistent data has its own:

* identity
* schema/version
* migration rules
* retention
* provenance

A software release may require migration, but migration must be explicit.

Conceptually:

```text
Old Data Version
      ↓
Migration
      ↓
New Data Version
```

Migration must not silently reinterpret incompatible historical information.

---

## 20. Configuration Migration

Configuration versions are distinct from application versions.

A new release may require configuration migration:

```text
Configuration v1
      ↓
Migration
      ↓
Configuration v2
```

The migration must preserve intended semantics or explicitly report incompatibility.

Missing configuration must not silently become an unrelated default merely to make startup succeed.

---

## 21. Upgrade and Downgrade

Upgrades and downgrades are distinct operations.

An upgrade may require:

* binary replacement
* configuration migration
* data migration
* checkpoint migration
* extension compatibility changes

Downgrade may not be possible after an irreversible data migration.

The package/release system must not imply downgrade safety unless explicitly supported.

---

## 22. Installation Atomicity

Installation should avoid leaving the system in an ambiguous partially-installed state.

Where the target environment supports it, installation should conceptually follow:

```text
Prepare
  ↓
Validate
  ↓
Install
  ↓
Activate
```

Failure during installation should produce an explicit installation failure.

The exact atomic installation mechanism is deployment-specific.

---

## 23. Application Activation

Installing files does not automatically mean the application is active.

Conceptually:

```text
Installed
    ↓
Configured
    ↓
Initialized
    ↓
Running
```

Activation may be performed manually or by an external runtime/deployment system.

The package itself should not assume ownership of the entire host lifecycle.

---

## 24. Service Integration

An application may be integrated with a service manager.

Possible service concerns include:

* startup
* shutdown
* restart
* environment
* resource limits
* logs
* readiness
* health

These are runtime/deployment concerns.

The application must still expose its own lifecycle semantics independently of a specific service manager.

---

## 25. Multiple Deployment Targets

EVolution may eventually target:

```text
Developer Workstation
Test Environment
Single Server
Container
Virtual Machine
Embedded / Restricted Environment
Cluster
Remote Execution Environment
```

Packaging should allow target-specific artifacts without changing Core semantics.

A target-specific package may include different dependencies or runtime integration while preserving the same logical architecture.

---

## 26. Platform Support

The initial implementation may support a subset of platforms.

Platform support should be explicitly declared.

Platform-specific code must remain behind appropriate boundaries.

For example:

```text
Generic Core
      ↓
Platform Abstraction
      ↓
Linux / Other Platform
```

Platform-specific behavior must not leak into domain semantics.

---

## 27. Package Integrity

Packages may require integrity verification.

Potential mechanisms include:

* checksums
* signatures
* trusted repositories
* verification metadata

The exact mechanism is deferred.

Package integrity is distinct from runtime authentication and application authorization.

---

## 28. Package Authenticity

Authenticity establishes that a package originates from a trusted release source.

It is distinct from:

```text
Package Integrity
Application Authentication
Runtime Authorization
```

A valid package does not imply that every runtime operation performed by the application is authorized.

---

## 29. Release Provenance

Release provenance should identify the information required to understand how a release was produced.

Potential information includes:

```text
Source Revision
Build Configuration
Toolchain
Dependency Versions
Generated Sources
Packaging Configuration
Artifact Identity
```

Release provenance may be referenced by processing provenance when the implementation version materially affects results.

---

## 30. Package Repositories

Packages may eventually be distributed through:

* local repositories
* artifact servers
* OS package repositories
* container registries
* release archives

No distribution mechanism is selected by this ADR.

Repository availability is an external dependency, not a Core assumption.

---

## 31. Testing Packages

Packaging must be tested separately from source/build correctness.

Tests should verify:

* expected files are present
* unwanted files are absent
* runtime dependencies are correct
* installation succeeds
* configuration locations are correct
* upgrade behavior
* migration behavior
* executable/library discovery
* package metadata
* clean-environment execution

A successful build is not sufficient evidence that a package is correct.

---

## 32. Release Testing

A release should be tested as an installed artifact where practical.

Conceptually:

```text
Build
  ↓
Package
  ↓
Install in clean environment
  ↓
Initialize
  ↓
Run tests
  ↓
Release validation
```

This catches problems that source-tree tests cannot detect.

---

## 33. Release and Reproducibility

A released analytical application must be identifiable sufficiently to reproduce results when required.

Relevant information may include:

```text
Application Version
Core Version
Domain Version
Processor / Algorithm Versions
Configuration
Input Dataset Version
Relevant Execution Context
Dependency Versions
```

The release itself is only one part of the reproducibility record.

---

## 34. AI-Assisted Implementation

AI-generated implementation must not silently modify packaging or release contracts.

Changes to:

* install paths
* public artifacts
* package dependencies
* versioning
* configuration locations
* runtime service integration
* migration behavior

must be treated as explicit architectural or implementation changes.

The package layout should be derived from the architecture rather than generated ad hoc by an implementation agent.

---

## 35. Deferred Decisions

This ADR does not select:

* DEB
* RPM
* tar archives
* containers
* AppImage
* Snap
* Flatpak
* package repository
* service manager
* installer framework
* signing infrastructure
* release automation platform
* semantic-versioning policy
* OS support matrix
* cross-compilation strategy
* binary distribution policy

These remain implementation, deployment, or release-engineering decisions.

---

## Decision Summary

```text
Build:                         Produces implementation artifacts
Package:                       Produces distributable artifacts
Install:                       Places artifacts into runtime environment
Release:                       Identifies a distributable software state
Source tree:                   Separate
Build tree:                    Separate
Install tree:                  Separate
Runtime data:                  Separate from installation
Configuration:                 Explicit
Runtime dependencies:          Explicit
Component versions:            Independent where required
Release identity:              Explicit
Build reproducibility:        Supported conceptually
Processing reproducibility:   Separate concern
Data migration:                Explicit
Configuration migration:      Explicit
Upgrade:                       Supported conceptually
Downgrade:                     Not assumed safe
Package integrity:             Explicit concern
Package authenticity:          Explicit concern
Service integration:           Deployment concern
Platform support:              Explicit
Package technology:            Deferred
Release automation:            Deferred
```

## Invariant

**EVolution separates build, packaging, installation, runtime, and release concerns: distributable artifacts must have explicit identity, dependencies, configuration, and compatibility semantics, while installation and deployment mechanisms remain replaceable and must not redefine the behavior or meaning of the underlying application and components.**

