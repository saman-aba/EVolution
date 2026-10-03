# ADR 0037 — Application Composition Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution contains reusable architectural capabilities:

```text
Core
Processing
Domain
Analysis
Storage
Ingestion
Interfaces
```

These capabilities do not by themselves define what a particular executable should do.

An actual application must compose them into a purposeful workflow.

For example:

```text
Poker Analysis Application
    ↓
Poker Domain
    ↓
Event Processing
    ↓
Measurements
    ↓
Analysis
    ↓
Storage
```

Another application may use the same infrastructure for:

```text
Market Analysis
Simulation
Dataset Processing
Protocol Analysis
```

The architecture therefore needs an explicit boundary for application-specific composition and policy.

## Decision

EVolution treats the **Application** as the composition boundary that assembles reusable capabilities into a concrete executable workflow.

Conceptually:

```text
External Interface
        ↓
    Application
        ↓
 ┌──────┼────────┐
 ↓      ↓        ↓
Domain Processing Storage
        ↓
     Analysis
        ↓
      Core
```

The Application determines:

```text
what the executable does
which capabilities it uses
how those capabilities are composed
which configuration is required
which interfaces are exposed
which storage is selected
which policies apply
```

The Application does not redefine the meaning of Core concepts.

## Application vs Library

A library provides reusable capabilities.

An application composes those capabilities for a particular purpose.

For example:

```text
Library:
    evolution_processing

Application:
    evolution_poker_analyzer
```

The application may depend on the library without the library depending on the application.

## Application Responsibilities

An application may own:

```text
composition
workflow
application policy
configuration resolution
component selection
graph construction
lifecycle coordination
storage selection
interface assembly
execution environment setup
application-level recovery policy
```

These responsibilities must remain explicit.

## Application Is Not a Domain

A domain defines meaning.

An application defines purpose and workflow.

For example:

```text
Poker Domain:
    Hand
    PlayerAction
    TableState
    VPIP metric

Poker Analysis Application:
    import hand histories
    build processing graph
    calculate metrics
    persist results
    expose query interface
```

The Poker domain can therefore be reused by multiple applications.

## Application Policy

Application policy determines how available capabilities are used.

Examples:

```text
which processor to select
which storage backend to use
whether a component is optional
whether retry is permitted
whether degraded operation is acceptable
which interface to expose
```

Policy should not be hidden inside generic infrastructure.

## Application Configuration

Application configuration determines how a particular application instance is assembled.

Conceptually:

```text
Application Configuration
        ↓
Component Configuration
        ↓
Effective Configuration
```

Application configuration may reference:

```text
domain
processor
analysis
storage
interface
graph
execution
resource
supervision
```

but each component retains ownership of its own semantic configuration.

## Configuration Ownership

The application may resolve configuration sources and compose configuration.

It should not arbitrarily modify the internal configuration semantics of a component.

For example:

```text
Application:
    selects processor configuration

Processor:
    validates processor configuration
```

## Application Lifecycle

An application has a lifecycle distinct from processor lifecycle.

Conceptually:

```text
CREATED
    ↓
CONFIGURED
    ↓
INITIALIZED
    ↓
RUNNING
    ↓
STOPPING
    ↓
STOPPED
```

Failure may produce:

```text
FAILED
```

The exact public application lifecycle API is deferred.

## Application Initialization

Initialization may include:

```text
resolve configuration
validate configuration
discover extensions
construct components
construct processing graph
validate graph
initialize storage
initialize processors
initialize interfaces
```

The order must respect dependencies.

## Application Activation

An application should not expose an externally usable operation before required components are ready.

Conceptually:

```text
Configuration
    ↓
Validation
    ↓
Construction
    ↓
Initialization
    ↓
Graph Validation
    ↓
Processor Activation
    ↓
Interface Activation
    ↓
Application Running
```

The exact ordering depends on the application.

## Application Shutdown

Application shutdown coordinates:

```text
interfaces
applications operations
processing graphs
processors
storage
external dependencies
```

Shutdown should follow the relevant lifecycle contracts.

For example:

```text
stop accepting new external work
        ↓
stop/cancel processing
        ↓
flush required state
        ↓
persist required information
        ↓
release resources
        ↓
close interfaces
```

The exact sequence depends on dependencies and application semantics.

## Application and Processing Graph

A processing graph is a reusable execution structure.

The application decides:

```text
which graph to construct
which nodes to include
which implementations to select
which configuration to use
when to activate it
```

The graph itself owns graph-level execution semantics.

The application must not bypass graph contracts to manipulate processor internals.

## Application Graph Construction

Graph construction should conceptually follow:

```text
Application Configuration
        ↓
Extension Selection
        ↓
Graph Definition
        ↓
Graph Validation
        ↓
Graph Initialization
        ↓
Graph Activation
```

Invalid graphs must not become active.

## Application and Storage

The application selects storage implementations appropriate for its workflow.

For example:

```text
Development:
    In-memory storage

Production:
    Persistent storage
```

The application may choose between them without changing domain semantics.

## Source of Truth

The application may define which persisted representation is authoritative for a workflow.

For example:

```text
Raw Events:
    authoritative

Measurements:
    derived/materialized
```

The domain and storage contracts must remain consistent with this decision.

## Application and Ingestion

Applications select ingestion mechanisms.

For example:

```text
File
    ↓
Parser
    ↓
Normalizer
    ↓
Domain Events
```

or:

```text
Network
    ↓
Decoder
    ↓
Normalizer
    ↓
Domain Events
```

The application composes the pipeline.

Ingestion components remain responsible for translating external representations into internal representations.

## Application and Analysis

Applications select which analytical capabilities to execute.

For example:

```text
Events
    ↓
State
    ↓
Measurements
    ↓
Pattern Detection
    ↓
Analysis
```

The application may choose the workflow without changing the meaning of those analytical concepts.

## Application and Interfaces

An application may expose one or more interfaces:

```text
CLI
HTTP
gRPC
IPC
Library
```

Multiple interfaces may expose the same application operation.

The application should not contain transport-specific parsing or response formatting.

## Application API

The application should expose semantic operations.

Examples:

```text
import_dataset(...)
run_analysis(...)
start_processing(...)
stop_processing(...)
query_measurements(...)
retrieve_analysis(...)
```

These are illustrative.

The exact API is deferred.

## Application Commands

An application operation may represent an explicit command:

```text
StartProcessing
StopProcessing
ImportDataset
RunAnalysis
```

Commands request behavior.

They are not automatically domain events.

## Application Events

An application may observe or produce operational events such as:

```text
application started
processing completed
analysis generated
```

These are distinct from domain events unless the domain explicitly defines them as such.

## Application State

Application state may include:

```text
active workflows
running graphs
loaded configuration
component references
application lifecycle
```

This is distinct from domain state.

For example:

```text
Application State:
    graph is ACTIVE

Poker State:
    player has 120 BB
```

These must not be conflated.

## Application Execution Context

An application establishes relevant execution context.

This may include:

```text
run identity
execution mode
time source
randomness source
resource limits
external dependency versions
```

Components receive the relevant subset explicitly.

The application must not hide materially relevant execution conditions in global state.

## Application Run Identity

Each application execution may have a run identity.

A run identity is distinct from:

```text
object identity
processor identity
graph identity
configuration identity
extension identity
```

It can be used to correlate execution records and provenance.

## Application Provenance

The application should be able to identify the composition used to produce a result.

Conceptually:

```text
Application
    ↓
Graph
    ↓
Processors
    ↓
Configuration
    ↓
Input
    ↓
Result
```

This information may become part of provenance when required for reproducibility.

## Application Reproducibility

An application execution may be reproducible when the necessary information is retained:

```text
Input
+
Effective Configuration
+
Component Versions
+
Graph Definition
+
Relevant Execution Context
+
Required State
```

The application is responsible for ensuring the relevant composition can be reconstructed.

## Application Supervision

Applications may create and configure supervisors.

For example:

```text
Application
    ↓
Graph Supervisor
    ↓
Processors
```

The application decides the supervision policy.

The supervisor itself remains responsible for enforcing that policy.

## Application Recovery Policy

Application-level recovery may determine what happens when lower-level recovery is insufficient.

Examples:

```text
restart component
restore graph
replay dataset
enter degraded mode
stop application
require manual intervention
```

The application does not override processor contracts.

## Optional Components

Applications may classify components as:

```text
REQUIRED
OPTIONAL
```

For optional components, the application may define degraded behavior.

For example:

```text
Primary processing:
    required

Telemetry exporter:
    optional
```

Optionality must be explicit.

## Application Dependency Graph

An application may have dependencies such as:

```text
Interface
    ↓
Application Service
    ↓
Processing Graph
    ↓
Storage
```

Dependencies should be explicit.

Circular dependencies should be avoided unless a component contract explicitly supports them.

## Application Composition vs Processing Composition

These are different levels.

```text
Application Composition:
    which components/workflows make up the application

Processing Composition:
    how processors are connected into a processing graph
```

For example:

```text
Application
    ├── HTTP Interface
    ├── Storage
    ├── Poker Domain
    └── Processing Graph
            ├── Processor A
            ├── Processor B
            └── Processor C
```

The application owns the composition of the graph as a subsystem.

The graph owns processor connections and execution semantics.

## Application Resource Requirements

Applications may declare resource requirements or limits such as:

```text
memory
CPU
storage
network
file descriptors
external connections
```

These are passed into the execution/resource model.

The application does not directly allocate arbitrary resources outside those contracts.

## Application Environment

The runtime environment may contain:

```text
OS
hardware
filesystem
network
external services
environment variables
```

Environment is distinct from application configuration.

Only materially relevant environmental properties should become explicit execution context/provenance.

## Application Secrets

Applications may resolve secrets required by components.

Secrets should not be placed into:

```text
ordinary analytical data
logs
provenance
configuration dumps
error messages
```

unless explicitly required and protected.

## Application Error Handling

Application operations use the established `Result<T>` model.

Application-level errors may include:

```text
configuration failure
dependency failure
component construction failure
graph validation failure
resource failure
external interface failure
startup failure
shutdown failure
```

Errors should preserve lower-level causes when meaningful.

## Application Failure vs Component Failure

A component failure does not automatically imply application failure.

For example:

```text
Optional Exporter
    ↓
FAILED

Application:
    continues
```

Conversely:

```text
Required Storage
    ↓
FAILED

Application:
    may become FAILED
```

The escalation policy belongs to the application/supervision layer.

## Application Observability

Applications may expose operational information such as:

```text
application status
active graph count
component status
startup duration
shutdown duration
workflow count
```

These remain operational observability unless explicitly promoted into the analytical model.

## Application Interfaces and Health

An application may expose health/status through an interface.

Health indicates operational capability.

It does not replace:

```text
processor lifecycle
application state
error state
```

## Application Testing

Application tests should verify:

```text
configuration
composition
startup
shutdown
workflow execution
component selection
graph construction
failure escalation
recovery policy
interface integration
```

Individual component semantics remain tested separately.

## Application Test Isolation

Application tests should use:

```text
in-memory storage
test interfaces
deterministic execution
controlled clocks
test processors
```

where appropriate.

This allows application composition to be tested without requiring production infrastructure for every test.

## Application Packaging

An application may eventually be packaged as:

```text
executable
service
library
container
```

The packaging mechanism does not change the application composition model.

## Multiple Applications

EVolution may contain multiple applications sharing the same libraries.

For example:

```text
evolution/
    libraries
    applications/
        poker-analyzer
        event-replay
        dataset-importer
        analysis-cli
```

Applications should not become dependencies of one another merely because they share infrastructure.

Shared functionality belongs in reusable libraries/modules.

## Application Reuse

If two applications require the same capability, the capability should normally be moved to an appropriate reusable module rather than duplicated.

However, premature abstraction should be avoided when semantics are not yet sufficiently understood.

## Application-Specific Policy

Application policy may intentionally differ.

For example:

```text
Application A:
    retry failed processing

Application B:
    stop on first failure
```

Neither policy should be encoded as a universal Core rule.

## Application and Domain Policy

Domain policy may also exist.

The application selects and configures the relevant domain policy.

It should not silently reinterpret domain semantics.

## Application and Decision

Applications may consume analysis and apply explicit decision policies.

The architectural relationship remains:

```text
Analysis
    ↓
Policy
    ↓
Decision
    ↓
Action
```

The application may orchestrate this flow.

It should not silently convert analytical findings into actions merely because a result exists.

## Application Actions

Actions are external effects.

Examples:

```text
write file
send message
update external system
place application command
```

Side effects should be explicit.

## Application Side Effects

If an application performs external side effects, it must define:

```text
ownership
failure semantics
retry behavior
idempotency
delivery semantics
recovery behavior
```

where relevant.

## Application and Transactions

Application workflows may require multiple components to reach a consistent outcome.

The application may coordinate transactional semantics through component contracts.

It should not assume that unrelated storage systems share one universal transaction.

## Distributed Applications

An application may eventually span multiple processes or machines.

The application composition model remains logical.

Physical deployment is separate.

For example:

```text
Logical Application
    ↓
Process A
Process B
Process C
```

does not require the logical application architecture to expose process boundaries everywhere.

## Application and Process Boundaries

If process boundaries affect:

```text
delivery
serialization
latency
failure
ownership
recovery
```

those effects must be represented explicitly in the relevant contracts.

## Application Startup Failure

If required application initialization fails:

```text
Application
    ↓
FAILED
```

rather than exposing a partially initialized application as fully operational.

Optional components may permit degraded startup if application policy explicitly allows it.

## Partial Startup

A partially initialized system must have an explicit state.

For example:

```text
Required components:
    initialized

Optional component:
    failed

Application:
    RUNNING_DEGRADED
```

If such a state is introduced, its semantics must be explicit.

No universal degraded application state is selected by this ADR.

## Application Shutdown Failure

A shutdown operation may fail due to:

```text
flush failure
storage failure
component failure
external dependency failure
```

The application must preserve the distinction between:

```text
requested shutdown
```

and:

```text
successful shutdown
```

consistent with the lifecycle model.

## Application Configuration Snapshot

For reproducibility and diagnosis, the application should be able to identify the effective configuration used by an execution.

Secrets must be redacted or excluded according to security requirements.

## Application Version

Application version is distinct from:

```text
component version
extension version
graph version
configuration version
```

It may still participate in provenance/reproducibility when application-level behavior depends on it.

## Consequences

### Positive

* Reusable libraries remain independent of application-specific workflows.
* Application policy has a clear architectural home.
* Multiple interfaces can share application operations.
* Multiple applications can reuse the same domains and processing infrastructure.
* Graph construction and application composition remain separate concerns.
* Startup/shutdown and recovery responsibilities become explicit.
* Domain semantics do not leak into executable orchestration.

### Negative

* Adds an explicit application layer between interfaces and reusable components.
* Application composition can become complex for large workflows.
* Policy boundaries require careful documentation.
* Distributed deployment may introduce additional composition concerns.

## Deferred Decisions

This ADR does not select:

```text
application framework
dependency injection framework
service manager
process model
container model
deployment topology
application configuration file format
CLI framework
HTTP framework
application RPC mechanism
workflow engine
job scheduler
distributed orchestration system
```

These require concrete application requirements.

## Decision Summary

```text
Application:
    Concrete composition and workflow boundary

Library:
    Reusable capability

Domain:
    Semantic meaning

Processing Graph:
    Processor composition/execution semantics

Application Policy:
    How capabilities are selected and coordinated

Configuration:
    Explicit application/component inputs

Lifecycle:
    Application lifecycle distinct from processor lifecycle

Storage:
    Selected by application, implemented behind storage contracts

Interfaces:
    Adapt external interaction to application operations

Supervision:
    Configured by application, implemented by supervision layer

Recovery:
    Application may define higher-level policy

Provenance:
    Application composition recorded when material

Run Identity:
    Distinct from object/component/configuration identities

Side Effects:
    Explicit

Deployment:
    Separate concern

Physical packaging:
    Deferred
```

## Invariants

1. Applications compose reusable capabilities into concrete workflows.
2. Applications may define application-specific policy without changing Core semantics.
3. Application policy must remain explicit rather than hidden in reusable infrastructure.
4. Domains define meaning; applications define purpose and workflow.
5. Processing graphs define processor composition semantics; applications select and construct graphs.
6. Interfaces adapt external interaction to application operations.
7. Application configuration must remain explicit and validated.
8. Application configuration must not become hidden mutable global state.
9. Application lifecycle is distinct from processor lifecycle.
10. Component failure does not automatically imply application failure.
11. Application failure escalation must follow explicit policy.
12. Required and optional components must be distinguished explicitly.
13. Application startup must not expose an incompletely initialized required subsystem as fully operational.
14. Application shutdown must distinguish request acceptance from successful completion.
15. Application run identity is distinct from logical object identity.
16. Application composition that materially affects results must be represented in provenance/reproducibility information.
17. Application-side external effects must have explicit ownership, failure, retry, delivery, and idempotency semantics where relevant.
18. Application code must not bypass processor, graph, storage, or interface contracts merely for convenience.
19. Applications must not become accidental reusable-library dependencies.
20. Shared reusable behavior belongs in an appropriate library/module rather than duplicated across applications.
21. Deployment topology and application semantics are separate concerns.
22. Application observability remains distinct from analytical data.
23. Application interfaces must not redefine domain semantics.
24. Application-level decisions must remain distinct from analytical findings.
25. An application coordinates capabilities but does not redefine the meaning of the capabilities it composes.

## Invariant

> **EVolution treats the Application as the concrete composition boundary: applications select, configure, coordinate, and expose reusable capabilities according to explicit workflow and policy, while Core, Domain, Processing, Analysis, Storage, and Interface layers retain ownership of their respective semantics and contracts.**
