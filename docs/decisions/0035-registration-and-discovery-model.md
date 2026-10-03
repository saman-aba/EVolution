# ADR 0035 — Registration and Discovery Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution is intended to grow through extensions.

Examples include:

```text
Processors
Analysis algorithms
Pattern detectors
Domains
Storage implementations
Serializers
Ingestion adapters
Interfaces
Applications
```

The Extension Model establishes that new behavior should be added through explicit contracts rather than by modifying generic Core semantics.

A mechanism is therefore required for an application or processing graph to identify available implementations and construct the ones it requires.

This introduces two related but distinct concepts:

```text
Registration
    ↓
Discovery
    ↓
Selection
    ↓
Construction
```

The architecture must support this without prematurely requiring:

```text
dynamic shared libraries
runtime code loading
a plugin directory
reflection
global registries
```

## Decision

EVolution uses an **explicit registration and discovery model**.

Extensions expose descriptive metadata and construction capabilities through explicit interfaces.

Registration and discovery are architectural concepts.

They do not imply a particular physical plugin mechanism.

Conceptually:

```text
Extension Implementation
        ↓
     Register
        ↓
Registry / Catalog
        ↓
    Discover
        ↓
    Select
        ↓
    Construct
        ↓
  Extension Instance
```

## Registration

Registration makes an extension available to a particular EVolution execution environment.

Registration may happen:

```text
at compile time
during application initialization
through static registration
through explicit application code
through dynamic loading
through a remote/service mechanism
```

The architecture does not require one universal mechanism.

## Discovery

Discovery answers:

> Which extensions are available and what capabilities do they provide?

Discovery must not automatically instantiate every available extension.

Conceptually:

```text
Discovery
    ↓
Metadata
    ↓
Capability information
```

rather than:

```text
Discovery
    ↓
Construct everything
```

## Selection

Selection determines which registered implementation satisfies an application's explicit requirements.

For example:

```text
Required:
    Processor type = "rolling_average"
    input = Measurement
    output = Measurement
```

Discovery may identify several compatible implementations.

Selection is an application/graph/configuration concern, not a Core semantic decision.

## Construction

Construction creates an instance from a selected extension definition.

Conceptually:

```text
Extension Definition
        +
Configuration
        +
Execution Context
        ↓
     Instance
```

Construction may fail and therefore follows the established `Result<T>` model.

## Extension Definition

An extension should expose metadata sufficient for discovery.

Conceptually:

```text
ExtensionDefinition
{
    identity
    type
    version
    capabilities
    configuration_schema?
    compatibility?
}
```

The exact C++ representation is deferred.

## Extension Identity

Extension identity must be stable and distinct from instance identity.

For example:

```text
Extension:
    rolling-average-v1

Instance:
    processor-node-17
```

These are different identities.

An extension identity identifies the implementation capability.

An instance identity identifies a particular constructed object.

## Extension Version

The extension version identifies the implementation/version used.

This matters for:

```text
provenance
reproducibility
checkpoint compatibility
configuration compatibility
migration
```

The exact versioning scheme is deferred.

## Capability Description

An extension should be able to describe relevant capabilities.

Examples:

```text
Processor:
    accepts Measurement
    produces Measurement
    supports streaming
    supports batch
    concurrency = SERIAL

Storage:
    supports append
    supports range query
    supports durable writes

Serializer:
    supports schema version 3
    supports streaming
```

Capability metadata must describe actual supported behavior rather than aspirational features.

## Capability vs Configuration

Capabilities describe what an implementation can support.

Configuration describes how a particular instance should behave.

For example:

```text
Capability:
    supports batch processing

Configuration:
    batch size = 1000
```

These must remain distinct.

## Capability vs Runtime Availability

A capability may exist but currently be unavailable.

For example:

```text
Capability:
    supports durable storage

Runtime:
    disk unavailable
```

Discovery describes capability.

Runtime validation determines whether the capability can actually be used in the current environment.

## Registration Scope

Registration may be scoped to:

```text
process
application
graph
execution
component
```

There is no requirement for a single global registry shared by the entire operating system.

## Global Registry

A global mutable singleton registry is not the default architecture.

Such a registry can introduce:

```text
hidden dependencies
initialization-order problems
test contamination
global mutable state
lifetime ambiguity
```

Explicit registry ownership is preferred.

## Registry Ownership

A registry should have a clear owner.

Conceptually:

```text
Application
    owns
ExtensionRegistry
```

or:

```text
Graph Definition
    references
Extension Catalog
```

depending on the final application model.

The exact ownership mechanism is deferred.

## Registry Lifetime

Registry lifetime must be explicit.

An extension registered in one execution environment must not accidentally remain available in another execution environment merely because a process-global object retains it.

## Static Registration

Static registration may be used where appropriate.

For example, an executable may explicitly register:

```text
Poker domain
Standard processors
In-memory storage
```

during application initialization.

This provides extension behavior without requiring dynamic loading.

## Dynamic Registration

Dynamic loading may be supported later.

Possible mechanisms include:

```text
shared libraries
runtime modules
plugin directories
service discovery
remote extension services
```

No dynamic mechanism is selected by this ADR.

## Dynamic Loading Boundary

If dynamic loading is introduced, the dynamic boundary should expose a stable ABI/API.

The extension implementation must not require the host to understand its private C++ object layout.

Possible future boundaries include:

```text
C ABI
stable C++ ABI policy
RPC
serialization-based protocol
```

The exact mechanism is deferred.

## C++ ABI

EVolution's internal C++ extension model does not automatically imply a stable cross-binary C++ ABI.

The project must not assume that arbitrary independently compiled C++ modules can exchange objects safely across compiler/library versions.

If binary compatibility becomes a requirement, it requires a separate architectural decision.

## Registration vs Linking

A statically linked implementation may be registered without being a dynamic plugin.

For example:

```text
executable
    ↓
links evolution_poker
    ↓
explicit registration
    ↓
Poker extensions available
```

Therefore:

```text
Extension
≠
Dynamic Plugin
```

## Discovery Metadata

Discovery metadata should be sufficient for selection without constructing the full implementation where practical.

For example:

```text
identity
type
version
input contracts
output contracts
capabilities
configuration requirements
```

Expensive initialization should not be required merely to inspect availability unless unavoidable.

## Type Identity

Extension type identity must be distinct from C++ RTTI type identity.

A C++ class name is not a stable architectural identifier.

For example:

```text
EvolutionProcessorRollingAverage
```

should not automatically become the persistent extension identity.

## Contract Identity

Extensions may expose the contracts they implement.

For example:

```text
Processor<Measurement → Measurement>
```

This is more meaningful architecturally than merely reporting:

```text
C++ class = RollingAverageProcessor
```

## Compatibility

Compatibility must be considered at multiple levels:

```text
extension version
contract version
configuration schema
data/schema version
checkpoint version
graph version
```

A matching extension name does not imply semantic compatibility.

## Version Compatibility

Compatibility should be explicit.

Possible relationships include:

```text
compatible
incompatible
requires migration
requires adapter
```

The exact version negotiation rules are deferred.

## Adapters

If two extensions are not semantically compatible, an explicit adapter may bridge them.

For example:

```text
Processor A
    ↓
Adapter
    ↓
Processor B
```

Discovery must not silently insert adapters.

Graph validation should make the adapter explicit.

## Configuration Schema

An extension may expose the configuration it requires.

Conceptually:

```text
Extension
    ↓
Configuration Definition
    ↓
Configuration Resolution
    ↓
Validation
```

Configuration remains explicit according to ADR 0012.

## Configuration Defaults

Extension metadata may describe defaults.

However, defaults must become part of the effective configuration.

They must not remain hidden in implementation code.

## Construction Errors

Construction may fail due to:

```text
invalid configuration
unsupported capability
missing dependency
resource exhaustion
incompatible version
invalid state
external failure
```

These are represented through `Result<T>` and the established error model.

## Construction vs Initialization

Construction and processor lifecycle initialization remain conceptually distinct.

For example:

```text
construct
    ↓
CREATED
    ↓
configure
    ↓
CONFIGURED
    ↓
initialize
    ↓
INITIALIZED
```

Registration/discovery does not automatically activate an extension.

## Registration Does Not Activate

Registering an extension means:

```text
available
```

not:

```text
active
```

An extension should not begin processing merely because it was registered.

## Discovery Does Not Initialize

Likewise, discovery should not implicitly:

```text
open files
connect databases
allocate large buffers
start threads
```

unless the discovery contract explicitly requires such behavior.

## Dependency Discovery

An extension may depend on other extensions.

For example:

```text
Analysis A
    requires
Pattern Detector B
```

Dependencies should be explicit.

A registry must not silently instantiate arbitrary transitive dependencies without a defined construction contract.

## Dependency Cycles

Extension dependencies should not create hidden initialization cycles.

If dependency cycles are supported, they require explicit lifecycle semantics.

Otherwise they should be rejected during validation.

## Extension Configuration Dependencies

An extension may require configuration from another component.

Such dependencies should be represented explicitly rather than obtained through unrelated global configuration.

## Extension Resource Requirements

An extension may declare resource requirements such as:

```text
memory
CPU
storage
external connections
```

These are capability/requirement metadata.

Actual availability is determined by the execution environment and resource model.

## Extension Lifecycle

Extension definitions are not necessarily lifecycle-managed components.

An extension definition may exist for the lifetime of a registry while its instances have independent lifecycles.

```text
Extension Definition
        │
        ├── Instance A
        ├── Instance B
        └── Instance C
```

Each processor instance follows its own lifecycle contract.

## Extension Instance Identity

Every constructed instance that requires logical identity must have its own instance identity.

Two instances created from the same extension definition are distinct objects.

## Provenance

When an extension materially contributes to a derived result, provenance should identify:

```text
extension identity
extension version
configuration
relevant execution context
```

This allows a result to be interpreted and reproduced later.

## Reproducibility

Extension selection is part of reproducibility when different implementations can produce different results.

Therefore:

```text
Extension Identity
+
Extension Version
```

may be required in the reproducibility record.

## Extension Selection and Graph Definition

A processing graph should identify the extension implementation required by each node.

Conceptually:

```text
Graph Node
{
    node_id
    extension_id
    extension_version
    configuration
}
```

This prevents a graph from silently selecting a different implementation during replay.

## Graph Validation

Before graph activation, extension references should be validated:

```text
extension exists
version compatible
required capabilities available
configuration valid
contracts compatible
resources satisfiable
```

Failure prevents activation according to the Processing Graph Model.

## Missing Extension

If a required extension cannot be found:

```text
Result<...>
    → Error
```

The graph must not silently substitute another implementation unless substitution is explicitly part of its selection policy.

## Optional Extensions

Applications may define optional components.

For example:

```text
primary analysis
optional visualization exporter
```

Failure of an optional extension may permit degraded operation if the application policy explicitly permits it.

The extension mechanism itself does not decide whether an extension is optional.

## Registration Conflicts

Registration may encounter:

```text
duplicate identity
incompatible version
conflicting capability
```

These should produce explicit errors rather than silently replacing an existing registration.

## Versioned Registration

Multiple versions of an extension may theoretically coexist:

```text
rolling-average v1
rolling-average v2
```

Whether this is permitted within one registry is an application/build decision.

If multiple versions coexist, identity and selection must distinguish them explicitly.

## Replacement

Replacing a registered extension should not silently alter existing active instances.

Existing instances retain their selected implementation.

A registry change must not mutate an already-running processor's implementation.

## Hot Replacement

Runtime replacement of active extension implementations is not supported by this ADR.

If hot replacement becomes necessary, it requires explicit lifecycle, state migration, compatibility, and provenance semantics.

## Unregistration

Unregistration should not invalidate an already-created instance unless the contract explicitly permits it.

A definition may become unavailable for future construction while existing instances continue operating.

## Thread Safety

The registry's concurrency behavior must be explicit once the C++ API is designed.

The architecture does not require:

```text
lock-free registry
global mutex
thread-local registry
```

or any particular implementation.

## Registry Mutation

Registration and unregistration are likely infrequent compared with extension use.

The implementation should not optimize prematurely around assumptions about mutation frequency.

## Discovery Queries

Discovery may support queries such as:

```text
find by identity
find by type
find by capability
find compatible version
find implementations of contract
```

The exact query language is deferred.

## Discovery vs Selection Policy

Discovery reports available information.

Selection decides what should be used.

For example:

```text
Discovery:
    A and B satisfy contract

Selection:
    application configuration chooses A
```

The registry must not silently decide application policy.

## Extension Security

Dynamic or externally supplied extensions introduce security concerns.

If arbitrary code can be loaded, the extension mechanism may become a code-execution boundary.

Security requirements for dynamic extensions require a separate decision.

## Trust Boundary

A statically linked project component and a remotely supplied extension are not necessarily equivalent trust boundaries.

Future extension mechanisms must explicitly identify:

```text
trust
authentication
authorization
integrity
sandboxing
```

where relevant.

## Remote Extensions

A remote service may implement an extension contract without being loaded into the process.

Conceptually:

```text
Application
    ↓
Extension Interface
    ↓
Remote Service
```

This is an architectural possibility, not a current requirement.

Remote extensions introduce serialization, network, failure, latency, and versioning concerns.

## Extension Metadata Serialization

If extension discovery crosses a process or network boundary, extension metadata must use an explicit serialized representation according to the Serialization Model.

C++ object layout must not become the discovery wire format.

## Application Ownership

Applications decide which extensions they require.

Core should provide mechanisms for:

```text
registration
discovery
construction
```

but must not select domain-specific functionality on behalf of the application.

## Domain Ownership

Domains define the meaning of their domain extensions.

For example:

```text
Poker
    defines poker events
    defines poker state
    defines poker metrics
    defines poker analyzers
```

The registration mechanism only exposes those capabilities.

It does not define poker semantics.

## Storage Ownership

Storage implementations register their physical capabilities.

The registry does not decide which storage is authoritative for a domain.

That is an application/configuration/architecture decision.

## Processor Ownership

Processor implementations define their own processing semantics and contracts.

Registration metadata describes those contracts.

The registry does not alter processor behavior.

## Testing

Registration/discovery should be tested for:

```text
successful registration
duplicate identity
discovery
capability matching
version matching
construction
construction failure
missing extension
configuration failure
dependency failure
```

Dynamic loading, if later introduced, requires additional integration tests.

## Static Build Testing

Static registration should be testable without requiring dynamic plugin infrastructure.

This keeps the initial architecture simple and suitable for ordinary unit/integration builds.

## Extension Contract Testing

Each extension should be tested against the contract it claims to implement.

For example:

```text
Processor Extension
    ↓
Processor Contract Tests
```

Registration tests should not replace behavioral contract tests.

## Consequences

### Positive

* Extensions can be added without modifying generic Core semantics.
* Static linking remains viable.
* Dynamic plugins remain possible later.
* Discovery and construction become explicit architectural concepts.
* Graphs can identify exact implementations needed for reproducibility.
* Registry state does not need to be a global singleton.
* Application policy remains separate from capability discovery.

### Negative

* Extension metadata introduces another conceptual layer.
* Version and compatibility handling add complexity.
* Dynamic extension mechanisms may require a stable ABI or protocol.
* Capability descriptions must remain accurate as implementations evolve.

## Deferred Decisions

This ADR does not select:

```text
dynamic shared libraries
plugin ABI
C ABI for plugins
C++ ABI policy
reflection system
global registry implementation
dependency injection framework
service discovery protocol
remote extension protocol
extension package format
security/sandbox mechanism
hot reload
runtime code replacement
```

These require separate decisions if they become necessary.

## Decision Summary

```text
Registration:
    Makes an extension available

Discovery:
    Describes available extensions/capabilities

Selection:
    Chooses an implementation according to explicit application/graph policy

Construction:
    Creates an instance

Extension identity:
    Distinct from instance identity

Version:
    Part of compatibility/provenance/reproducibility where relevant

Capability:
    What an implementation supports

Configuration:
    How a selected instance behaves

Runtime availability:
    Separate from capability

Static linking:
    Fully supported

Dynamic loading:
    Possible later, not required

Global mutable registry:
    Not the default

Graph:
    Records selected extension identity/version

Activation:
    Separate from registration

Replacement:
    Does not silently modify existing instances

Security:
    Separate concern for dynamic/remote extensions

Physical mechanism:
    Deferred
```

## Invariants

1. Registration makes an extension available; it does not activate it.
2. Discovery describes available capabilities; it does not choose application policy.
3. Selection is explicit and must not silently substitute an unrelated implementation.
4. Extension identity is distinct from extension instance identity.
5. Extension version is distinct from instance version/state.
6. Capability and configuration are separate concepts.
7. Runtime availability is distinct from declared capability.
8. Registration must not require construction or activation of every registered extension.
9. A missing required extension must produce an explicit failure.
10. Registration conflicts must not silently overwrite an existing definition.
11. Changing registry contents must not silently mutate already-created instances.
12. Extension selection that can affect results must be represented in provenance/reproducibility information.
13. Processing graphs must identify the implementation required by each node when implementation choice affects semantics.
14. Extension dependencies must be explicit.
15. Extension registration must not depend on hidden global configuration.
16. Static linking and registration must work without requiring a dynamic plugin mechanism.
17. C++ implementation type names are not stable architectural extension identities.
18. Extensions must implement explicit contracts rather than relying on registry-specific behavior.
19. Dynamic loading, if introduced, must define an explicit binary/protocol boundary.
20. The registry must not redefine domain semantics.
21. Core provides registration/discovery mechanisms but does not select domain-specific behavior.
22. Extension construction failures use the established `Result<T>` error model.
23. Existing extension instances retain their selected implementation even if registry contents later change.
24. Hot replacement is not implied by registration/discovery.
25. The physical mechanism used to register or discover extensions remains replaceable.

## Invariant

> **EVolution separates extension availability from extension selection and execution: registration exposes explicit implementations and capabilities, discovery describes what is available, and applications or processing graphs explicitly select and construct the required implementation without introducing hidden global dependencies or requiring a particular plugin mechanism.**
