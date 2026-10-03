# ADR 0043 — Runtime and Deployment Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution defines processors, processing graphs, applications, interfaces, storage, ingestion, analysis, supervision, recovery, and resource semantics.

The architecture has intentionally avoided deciding how these components are physically executed.

A concrete deployment may run EVolution as:

```text
single process
multiple processes
multiple services
containers
virtual machines
multiple machines
embedded execution
library integration
```

The semantic architecture should remain independent of these choices where possible.

A runtime model is nevertheless required to define the boundary between:

```text
logical architecture
```

and:

```text
physical execution
```

## Decision

EVolution separates **logical component architecture** from **runtime deployment architecture**.

A logical component may execute:

```text
in-process
out-of-process
locally
remotely
```

provided its required contract is preserved.

The runtime/deployment model is responsible for determining:

```text
process boundaries
component placement
resource allocation
communication mechanisms
startup
shutdown
restart
environment
configuration delivery
```

without redefining the semantics of Core, Domain, Processing, Analysis, or Storage.

## Logical vs Physical Architecture

The logical architecture describes:

```text
Core
Processing
Domain
Analysis
Storage
Ingestion
Application
Interface
```

The physical architecture describes where those components execute.

For example:

```text
Logical:

Application
    ├── Processing Graph
    ├── Storage
    └── Interface


Possible physical deployment:

Process A:
    Application
    Processing Graph

Process B:
    Storage

Process C:
    Interface
```

The physical decomposition is not automatically a semantic decomposition.

## Runtime Instance

A runtime instance is a concrete execution of a logical component.

Conceptually:

```text
Runtime Instance
{
    logical_identity
    component_version
    configuration
    execution_context
    resources
    lifecycle
}
```

The exact representation is deferred.

## Process Boundary

A process boundary creates an additional isolation and communication boundary.

It may provide:

```text
memory isolation
failure isolation
resource isolation
security isolation
independent lifecycle
independent deployment
```

but also introduces:

```text
serialization
communication overhead
failure modes
coordination
```

Process boundaries must therefore be introduced deliberately.

## In-Process Components

Components may execute in the same process.

Benefits may include:

```text
low communication overhead
shared address space
direct function calls
simple ownership
low serialization cost
```

Risks include:

```text
shared failure domain
shared memory
coupled lifecycle
resource contention
```

No architectural rule requires every module to become a separate process.

## Out-of-Process Components

Components may execute in separate processes when required by:

```text
isolation
security
resource control
independent lifecycle
deployment
failure containment
```

The communication boundary must preserve the relevant component contract.

## Remote Components

A component may execute on another machine.

This introduces:

```text
network failure
latency
partial failure
connection lifecycle
serialization
authentication
authorization
delivery semantics
```

These must be represented explicitly rather than hidden behind the assumption that a remote call behaves like a local call.

## Local vs Remote Semantics

A logical API should not silently assume:

```text
zero latency
reliable delivery
shared memory
instant completion
```

if the component may later become remote.

However, the architecture should not artificially impose distributed-system complexity on components that have no requirement for remote execution.

## Deployment Unit

A deployment unit is a set of components deployed and managed together.

Examples:

```text
application process
container
service
VM
host-level installation
```

Deployment units are operational concepts rather than Core analytical objects.

## Component Placement

Applications or deployment configuration may determine component placement.

For example:

```text
Application
    ↓
Deployment Configuration
    ↓
Component Placement
```

Placement must satisfy:

```text
resource requirements
security requirements
communication requirements
lifecycle requirements
availability requirements
```

where applicable.

## Placement Does Not Change Meaning

Moving a component:

```text
same process
    ↓
different process
```

must not silently change its semantic behavior.

If physical placement changes semantics, that behavior must be represented explicitly in the relevant contract.

## Communication Boundary

When components cross a physical boundary, communication requires:

```text
serialization
transport
delivery
identity
lifecycle
failure handling
```

The communication mechanism remains replaceable.

## Local Communication

Possible local mechanisms include:

```text
function call
shared memory
queue
IPC
Unix socket
pipe
```

No specific mechanism is selected.

## Remote Communication

Possible mechanisms include:

```text
TCP
HTTP
RPC
messaging
custom protocol
```

No specific mechanism is selected.

## Communication Is Not Automatically an API

A transport mechanism does not define the semantic application API.

For example:

```text
HTTP
```

is a transport/interface technology.

The underlying application operation remains the semantic contract.

## Runtime Communication and Delivery

Communication across a runtime boundary must define delivery semantics where relevant.

For example:

```text
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

according to ADR 0024.

Remote communication does not automatically provide exactly-once semantics.

## Network Failure

A remote dependency may become:

```text
unavailable
slow
partitioned
partially responsive
```

The caller must distinguish:

```text
operation failed
operation timed out
operation was cancelled
operation completion is unknown
```

where relevant.

## Unknown Completion

A particularly important distributed condition is:

```text
request sent
    ↓
connection lost
    ↓
completion unknown
```

The caller must not automatically assume:

```text
operation did not execute
```

or:

```text
operation succeeded
```

without an appropriate contract.

This interacts with idempotency and recovery.

## Runtime and Application Operations

Application Operations remain the semantic boundary even when execution is remote.

Conceptually:

```text
External Request
    ↓
Interface
    ↓
Application Operation
    ↓
Runtime
    ↓
Component
```

The transport must not redefine the operation's meaning.

## Runtime and Processor Operations

A processor operation may execute:

```text
same process
different process
different machine
```

if its contract supports that execution model.

The processor semantics remain unchanged.

## Runtime and Processing Graphs

A processing graph may be deployed:

```text
single process
multiple processes
multiple hosts
```

provided graph semantics remain valid.

For example:

```text
Graph

A → B → C
```

could execute as:

```text
Process 1:
    A

Process 2:
    B

Process 3:
    C
```

without changing the logical graph.

## Graph Partitioning

Physical partitioning may introduce communication boundaries.

The deployment must preserve:

```text
ordering
delivery
backpressure
identity
provenance
cancellation
failure
```

according to the graph contract.

## Graph Failure Domains

Physical deployment may determine failure containment.

For example:

```text
A and B same process
```

means a process crash may affect both.

Whereas:

```text
A and B separate processes
```

may allow independent recovery.

Failure-domain behavior is a deployment property.

## Runtime Supervision

Supervisors may operate at:

```text
processor
graph
application
process
deployment
```

levels.

A deployment supervisor may restart a failed process without changing the processor's semantic recovery policy.

## Process Restart

Process restart is distinct from processor restart.

For example:

```text
Process crash
    ↓
Supervisor restarts process
    ↓
Application reconstructs processors
    ↓
Processor recovery
```

The processor may restore from checkpoint or replay input according to its recovery contract.

## Process Crash

A process crash provides no guarantee of orderly shutdown.

Therefore:

```text
STOP
```

and:

```text
process termination/crash
```

must remain distinct.

Recovery must rely on persisted state and retained inputs where required.

## Graceful Shutdown

Deployment shutdown should coordinate:

```text
application
interfaces
processing graphs
processors
storage
external dependencies
```

according to lifecycle contracts.

The conceptual sequence is:

```text
Stop accepting new external work
        ↓
Stop admitting new processing work
        ↓
Drain/cancel according to policy
        ↓
Persist required state
        ↓
Release resources
        ↓
Terminate runtime
```

## Forced Termination

Forced process termination may leave:

```text
incomplete operations
unflushed buffers
unfinished checkpoints
unknown external effects
```

Recovery must account for these possibilities.

## Runtime Health

Deployment systems may monitor:

```text
process health
component health
resource health
dependency health
```

Health remains distinct from:

```text
processor lifecycle
operation result
domain state
```

## Liveness

Liveness asks whether a runtime component continues making progress.

A component may be:

```text
alive
but unhealthy
```

or:

```text
healthy
but temporarily idle
```

Therefore liveness and health should not be conflated.

## Readiness

Readiness indicates whether a component is prepared to accept work.

For example:

```text
STARTING
    ↓
INITIALIZING
    ↓
READY
    ↓
ACTIVE
```

Readiness is an operational concept and should not replace the processor lifecycle model.

## Runtime Startup

Application startup should follow the Application Composition Model.

Conceptually:

```text
Process Start
    ↓
Load Configuration
    ↓
Resolve Environment
    ↓
Discover Components
    ↓
Construct Components
    ↓
Construct Graphs
    ↓
Validate
    ↓
Initialize
    ↓
Activate
    ↓
Ready
```

## Runtime Configuration

Runtime configuration may include:

```text
process placement
resource limits
communication endpoints
logging destinations
storage locations
security configuration
```

Component semantic configuration remains owned by the component.

## Environment vs Configuration

Deployment environment may provide:

```text
hostname
network addresses
filesystem paths
available CPU
available memory
credentials
external service endpoints
```

These are not automatically semantic configuration.

The application resolves the relevant effective configuration explicitly.

## Environment Variables

Environment variables may be used as a configuration source.

They must not become hidden global configuration.

Conceptually:

```text
Environment
    ↓
Configuration Resolution
    ↓
Effective Configuration
    ↓
Component
```

## Secrets

Runtime deployment may inject secrets through:

```text
secret manager
environment
file
IPC
secure service
```

The specific mechanism is deferred.

Secrets must follow ADR 0041 and must not leak into ordinary diagnostics.

## Resource Allocation

Deployment determines physical resource availability.

Examples:

```text
CPU
memory
storage
network
file descriptors
process limits
```

The Resource Model remains the semantic contract for component requirements.

## Resource Isolation

Deployment may enforce:

```text
CPU quotas
memory limits
process limits
container limits
storage quotas
network limits
```

These are runtime mechanisms.

## Resource Overcommit

A deployment may overcommit physical resources.

However, this does not change the semantic requirement that components must receive explicit resource outcomes.

Resource exhaustion must not silently become valid processing.

## Runtime Scheduling

The deployment runtime may schedule:

```text
processes
threads
containers
services
```

This is distinct from the EVolution Processing Scheduler.

```text
EVolution Scheduler:
    semantic processing eligibility

Runtime Scheduler:
    physical execution resources
```

These must not be conflated.

## Runtime Threads

A process may contain multiple threads.

Thread allocation is an implementation concern unless a processor contract explicitly requires:

```text
thread affinity
thread-local state
specific concurrency behavior
```

## CPU Affinity

CPU affinity may be useful for performance-sensitive deployments.

It remains a runtime/execution decision.

It must not become an implicit semantic dependency.

## NUMA

NUMA placement may affect performance.

It should not affect semantic results unless the implementation explicitly violates deterministic behavior.

NUMA policy is deferred.

## Runtime Storage

Storage may be:

```text
local
remote
shared
ephemeral
persistent
```

The Storage Model defines semantic persistence requirements.

Deployment determines physical placement.

## Runtime Network

Networking may be:

```text
local
host network
virtual network
remote network
```

Network topology is deployment configuration.

Network topology must not silently alter analytical semantics.

## Runtime Clock

Runtime environments provide clocks.

EVolution follows ADR 0011:

```text
absolute historical time:
    UTC wall clock

elapsed time:
    monotonic clock
```

Runtime timezone must not silently redefine Core temporal semantics.

## Runtime Randomness

Runtime environments may provide randomness sources.

Randomness must be explicit when it affects processing results.

The deployment environment must not silently inject nondeterminism into deterministic processing.

## Runtime Dependency Availability

An application may depend on external services.

Availability must be explicit.

A missing dependency may result in:

```text
startup failure
degraded operation
runtime failure
retry
```

according to application/supervision policy.

## Required vs Optional Dependencies

Applications should explicitly classify dependencies.

```text
Required dependency:
    application cannot fulfill required contract without it

Optional dependency:
    application can continue with defined degraded behavior
```

The absence of an optional dependency must not silently create incorrect results.

## Runtime Capability Discovery

A deployment may expose capabilities such as:

```text
CPU features
available storage
network connectivity
GPU
special instruction sets
external service versions
```

Capabilities are distinct from resource quantities.

## Capability-Dependent Components

A component may require a capability.

For example:

```text
requires hardware acceleration
```

If unavailable, initialization should return an explicit error rather than silently changing semantics unless a fallback is explicitly supported.

## Fallback Implementations

A runtime may select a fallback implementation when the architecture explicitly permits it.

For example:

```text
Accelerated Implementation
        ↓ unavailable
Generic Implementation
```

The selection should be explicit and, when materially relevant, recorded in execution context/provenance.

## Runtime and Extension Discovery

Deployment may make extensions available.

However:

```text
available
```

does not mean:

```text
selected
```

and:

```text
selected
```

does not mean:

```text
active
```

This follows ADR 0035.

## Runtime and Security

Deployment may establish security boundaries through:

```text
process isolation
container isolation
network isolation
filesystem permissions
user identity
resource limits
```

Security requirements may influence placement.

The logical architecture remains security-mechanism-independent.

## Runtime and Observability

Deployment should expose operational information such as:

```text
process status
resource usage
restart count
dependency availability
health
```

These belong to observability unless intentionally promoted into analytical data.

## Runtime Logging

Runtime failures may be logged by:

```text
application
supervisor
deployment system
```

without automatically becoming domain events.

## Runtime Metrics

Operational runtime metrics may include:

```text
CPU usage
memory usage
process restarts
network usage
open files
container resource usage
```

They remain distinct from analytical measurements unless explicitly promoted.

## Runtime Tracing

Distributed runtime boundaries may create tracing spans.

Trace identity remains distinct from:

```text
EventId
OperationId
RunId
AnalysisId
```

## Deployment Artifacts

A deployment may contain:

```text
executables
shared libraries
configuration
schemas
extensions
resource files
service definitions
```

Packaging is addressed separately.

## Runtime Reproducibility

Deployment environment may affect results.

When materially relevant, reproducibility may require recording:

```text
application version
component versions
extension versions
effective configuration
relevant dependency versions
execution mode
resource constraints
capabilities
runtime environment
```

Not every host detail is relevant.

## Runtime Environment vs Reproducibility

Reproducibility does not require recording every incidental machine property.

Only conditions that materially affect semantic output need to be captured.

For example:

```text
hostname
```

normally does not affect an analytical result.

Whereas:

```text
algorithm implementation version
```

may.

## Deployment Portability

The architecture should permit deployment across different environments where contracts can be satisfied.

Examples:

```text
developer workstation
server
container
VM
cluster
```

Portability does not require identical performance or operational characteristics.

## Single-Process Deployment

A complete application should be capable of being deployed as a single process where practical.

This provides a simple deployment topology for:

```text
development
testing
small datasets
local analysis
```

without making single-process execution a permanent architectural constraint.

## Multi-Process Deployment

Applications may split components into processes when required for:

```text
isolation
scaling
security
availability
resource management
```

No universal process decomposition is required.

## Distributed Deployment

Distributed deployment may be introduced when requirements justify it.

The architecture already separates:

```text
logical processing
scheduling
execution
delivery
storage
supervision
```

which allows distributed mechanisms to be added without redefining those concepts.

## Deployment Topology

Deployment topology is configuration rather than domain semantics.

Conceptually:

```text
Deployment Definition
{
    components
    placement
    resources
    communication
    dependencies
    security
}
```

The exact representation is deferred.

## Deployment Version

A deployment definition may have its own version.

It is distinct from:

```text
application version
graph version
component version
configuration version
```

## Deployment Reconfiguration

Changing deployment placement should not require changing semantic application configuration unless the application explicitly depends on placement.

Dynamic reconfiguration is deferred.

## Rolling Changes

Distributed deployments may eventually support:

```text
rolling upgrade
blue/green deployment
canary deployment
```

These are operational deployment strategies and are not selected by this ADR.

## Runtime Upgrade

A runtime upgrade must preserve required compatibility contracts.

For example:

```text
stored checkpoint
    ↓
new runtime
```

requires checkpoint compatibility according to ADR 0025.

## Failure During Upgrade

Upgrade failures must not silently produce incompatible state.

Deployment recovery must preserve storage, checkpoint, identity, and provenance semantics.

## Development Runtime

Development may use a simplified runtime.

For example:

```text
single process
in-memory storage
local configuration
```

provided it still exercises the same semantic contracts.

## Test Runtime

Tests may use:

```text
fake runtime
in-memory components
controlled resources
deterministic clocks
fault injection
```

without requiring production deployment infrastructure.

## Production Runtime

Production deployment may introduce:

```text
persistent storage
supervision
resource isolation
security controls
remote interfaces
monitoring
```

without changing the semantic contracts.

## Consequences

### Positive

* Logical architecture remains independent of deployment technology.
* Single-process development remains possible.
* Multi-process and distributed deployment remain available.
* Process and component failure domains become explicit.
* Runtime scheduling remains separate from processing scheduling.
* Security and resource isolation can evolve independently.
* Deployment-specific behavior can be captured in execution context/provenance when materially relevant.

### Negative

* Supporting multiple deployment models increases operational complexity.
* Remote execution introduces serialization, communication, and failure semantics.
* Distributed deployment requires more extensive testing.
* Runtime configuration becomes another layer that must be distinguished from semantic configuration.

## Deferred Decisions

This ADR does not select:

```text id="1wmrdy"
process architecture
service architecture
container runtime
orchestrator
VM platform
threading model
IPC mechanism
RPC framework
network protocol
service discovery
deployment tool
init/system supervisor
container image format
cloud platform
Kubernetes
Docker
systemd
```

## Decision Summary

```text
Logical Architecture:
    Independent of physical deployment

Runtime:
    Concrete execution environment

Deployment:
    Placement + resources + communication + environment

Process Boundary:
    Optional isolation boundary

In-Process:
    Supported

Out-of-Process:
    Supported

Remote Execution:
    Supported conceptually

Single Process:
    Supported

Distributed Execution:
    Possible without changing semantic architecture

Runtime Scheduler:
    Physical execution

Processing Scheduler:
    Semantic processing eligibility

Process Restart:
    Distinct from processor recovery

Health:
    Operational capability

Readiness:
    Ability to accept work

Resource Limits:
    Runtime enforcement of resource contracts

Security:
    Deployment may provide isolation

Configuration:
    Explicitly resolved

Environment:
    Separate from semantic configuration

Secrets:
    Externalized and protected

Reproducibility:
    Relevant runtime conditions captured

Physical Technology:
    Deferred
```

## Invariants

1. Logical architecture and physical deployment are distinct.
2. A logical component may execute in-process, out-of-process, or remotely when its contract permits.
3. Physical placement must not silently redefine component semantics.
4. Process boundaries are optional architectural deployment boundaries.
5. Process restart is distinct from processor recovery.
6. Process termination does not imply orderly processor shutdown.
7. Runtime scheduling is distinct from EVolution processing scheduling.
8. Runtime resource availability does not redefine processor resource requirements.
9. Runtime configuration must not become hidden semantic configuration.
10. Environment and configuration remain distinct concepts.
11. Runtime clocks must follow the established temporal model.
12. Current wall-clock time must not silently replace missing domain time.
13. Runtime randomness must not silently introduce nondeterminism into deterministic processing.
14. Remote execution must explicitly account for communication failure and unknown completion.
15. Remote communication does not imply exactly-once delivery.
16. Deployment topology must preserve required ordering, identity, delivery, provenance, and lifecycle semantics.
17. Required and optional dependencies must be explicitly distinguished.
18. Missing optional dependencies must not silently produce incorrect results.
19. Capability availability and resource quantity are distinct.
20. Fallback implementations must be explicit when they can affect semantics or reproducibility.
21. Extension availability does not imply extension selection or trust.
22. Security isolation may influence deployment placement without becoming a Core semantic.
23. Runtime health is distinct from processor lifecycle state.
24. Runtime observability is distinct from analytical data.
25. Deployment version is distinct from application, component, graph, and configuration versions.
26. Deployment changes must not silently invalidate checkpoints or stored semantic data.
27. Reproducibility requires recording runtime conditions only when they materially affect results.
28. A single-process deployment must remain possible where practical without weakening architectural contracts.
29. Distributed deployment must not be required merely because the architecture permits it.
30. Physical deployment technology remains an implementation and operational concern.

## Invariant

> **EVolution separates logical execution semantics from physical deployment: components define what they mean and what guarantees they require, while runtime and deployment determine where and how they execute, communicate, recover, and consume resources without silently redefining those semantics.**
