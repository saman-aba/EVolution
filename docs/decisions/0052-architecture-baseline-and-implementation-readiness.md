# ADR 0052: Architecture Baseline and Implementation Readiness

**Status:** Accepted
**Date:** 2026-10-04

## Decision

The EVolution architecture is considered sufficiently defined to begin implementation of its foundational infrastructure.

Implementation must proceed from the accepted architectural contracts rather than inventing lower-level semantics independently inside individual components.

This ADR establishes the current architectural baseline, identifies what is already decided, and distinguishes it from intentionally deferred implementation choices.

The purpose is not to freeze implementation permanently. It is to prevent implementation work from silently redefining architectural semantics.

---

# 1. Architectural Baseline

EVolution is an event-driven analytical platform based on progressive transformation of information:

```text
Raw Data
   ↓
Events
   ↓
State
   ↓
Measurements
   ↓
Time Series
   ↓
Aggregation
   ↓
Patterns
   ↓
Analysis
   ↓
Decision / Application
```

This is a conceptual processing model, not a mandatory linear execution pipeline.

Actual processing is represented as an explicit graph.

---

# 2. Core Principle

The most important architectural boundary is:

```text
Generic Mechanism
        ↓
Domain Meaning
        ↓
Application Purpose
```

Core mechanisms must remain independent of domain-specific meaning.

Domains define what information means.

Applications define why and how capabilities are composed.

---

# 3. Fundamental Concepts

The following concepts are established architectural concepts:

```text
Event
State
Measurement
Metric
Time Series
Aggregation
Pattern
Analysis
Decision
Context
Provenance
Identity
Error
Configuration
Execution Context
```

Each concept has an independent semantic definition.

They must not be collapsed into a universal generic object merely for implementation convenience.

---

# 4. Information Pipeline

The conceptual relationship remains:

```text
Events
  ↓
State
  ↓
Measurements
  ↓
Time Series
  ↓
Aggregation
  ↓
Patterns
  ↓
Analysis
  ↓
Decision
```

Not every application requires every stage.

For example:

```text
Events → Analysis
```

may be valid when an analysis operates directly on events.

Likewise:

```text
Measurements → Aggregation
```

may be valid without explicit state reconstruction.

The architecture therefore defines reusable semantic stages rather than requiring a fixed pipeline.

---

# 5. Processing Architecture

Processing is represented as:

```text
Processing Graph
        ↓
Scheduler
        ↓
Eligible Work
        ↓
Execution Backend
        ↓
Processor Operation
        ↓
Result
        ↓
Scheduler / Graph
```

The following responsibilities remain separate:

```text
Processor       → what work means
Graph           → how processors are composed
Scheduler       → which work is eligible
Backend         → how eligible work executes
Queue/Buffer    → temporary retention
Supervisor      → failure/recovery coordination
Resource Model  → execution constraints
Observability   → operational visibility
```

No implementation may silently merge these responsibilities merely because a particular threading or framework model makes that convenient.

---

# 6. Processor Contract

A processor conceptually performs:

```text
Input
+
Configuration
+
Execution Context
+
State
        ↓
    Processor
        ↓
Output
+
Updated State
```

Processor behavior must explicitly define where relevant:

* input contract
* output contract
* state
* configuration
* execution context
* lifecycle
* concurrency
* ordering
* admission
* cancellation
* failure
* delivery
* provenance
* determinism
* resource requirements

---

# 7. Processor Lifecycle

The lifecycle is:

```text
CREATED
   ↓
CONFIGURED
   ↓
INITIALIZED
   ↓
ACTIVE
   ↓
STOPPING
   ↓
STOPPED
```

Failure may lead to:

```text
FAILED
```

Termination semantics distinguish:

```text
STOP
CANCEL
ABORT
```

with escalation:

```text
STOP → CANCEL → ABORT
```

The processor lifecycle remains distinct from individual operation outcomes.

---

# 8. Processing Outcomes

An operation has three primary outcomes:

```text
SUCCESS
FAILURE
CANCELLED
```

These are distinct from processor lifecycle state.

For example:

```text
Operation → CANCELLED
Processor  → STOPPED
```

is valid.

Likewise:

```text
Operation → FAILURE
Processor  → ACTIVE
```

is valid when the failure is locally recoverable.

---

# 9. Concurrency Baseline

The default processor concurrency contract is:

```text
SERIAL
```

unless explicitly declared otherwise.

Supported conceptual models are:

```text
SERIAL
CONCURRENT
PARTITIONED
```

Concurrency must not be inferred from the implementation's use of threads.

Ordering, state ownership, determinism, cancellation, and resource constraints must remain explicit.

---

# 10. Work Admission

Submission and admission remain distinct.

```text
Submitted
   ↓
Admission
   ↓
Accepted / Rejected / Delayed / Dropped / Sampled / Degraded / Spilled
   ↓
Processing
```

Resource exhaustion must never silently become successful processing.

Lossy behavior must be explicit.

---

# 11. Queue and Buffer Baseline

Queues and buffers are bounded execution infrastructure.

The default assumption is:

```text
Bounded
```

rather than unlimited growth.

Capacity may be defined by:

* items
* bytes
* memory
* weighted work

Ordering and loss behavior are explicit.

Exactly-once behavior is not assumed.

---

# 12. Delivery Semantics

The architecture distinguishes:

```text
Delivery
Execution
Completion
```

Possible delivery guarantees include:

```text
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

Exactly-once requires explicit semantic support and is not a default property.

Duplicate execution must therefore be considered possible in recovery and retry scenarios.

---

# 13. Recovery

Recovery follows the separation:

```text
Failure Detection
      ↓
Error Representation
      ↓
Recovery Decision
      ↓
Recovery Action
      ↓
Result
```

Possible actions include:

```text
RETRY
RESTART
RESTORE
REPLAY
SKIP
DEGRADE
PAUSE
STOP
FAIL
MANUAL_INTERVENTION
```

An error does not itself determine which recovery action should occur.

---

# 14. Checkpointing

A checkpoint represents a recovery boundary.

Conceptually:

```text
Checkpoint
=
State
+
Input Position
+
Compatibility Information
```

Recovery may use:

```text
Checkpoint
+
Retained Input
+
Graph Definition
+
Configuration
+
Component Version
+
Relevant Execution Context
```

Checkpoint compatibility must be validated before restoration.

---

# 15. Supervision

Supervision is responsible for coordinating failure and recovery according to policy.

It does not own:

* processor semantics
* domain meaning
* hidden configuration
* graph topology
* analytical interpretation

Failure escalation must be explicit.

---

# 16. Resource Model

Resources are explicit execution constraints.

The conceptual lifecycle is:

```text
Requested
   ↓
Availability Check
   ↓
Reserve
   ↓
Admit
   ↓
Execute
   ↓
Release
```

Resource requirements and resource availability remain separate.

The physical implementation may use threads, processes, containers, operating-system facilities, or distributed resources without changing these semantics.

---

# 17. Configuration

Configuration means:

> **How should this component behave?**

Configuration is:

* explicit
* resolved
* validated
* effectively immutable after initialization

Configuration is distinct from:

```text
State
Execution Context
Resource Availability
Environment
Provenance
```

Hidden mutable global configuration is prohibited.

---

# 18. Execution Context

Execution context represents materially relevant runtime conditions.

It may include:

* execution mode
* time source
* randomness source
* run identity
* resource constraints
* external dependency versions
* relevant capabilities
* cancellation state

It is not a universal:

```text
map<string, any>
```

Only information that materially affects behavior should cross component boundaries.

---

# 19. Time Model

EVolution distinguishes:

```text
Event Time
Ingestion Time
Processing Time
Measurement Time
Duration
```

Absolute timestamps use UTC.

Temporal knowledge states are:

```text
KNOWN
ESTIMATED
UNKNOWN
```

Missing time must never silently become current time or another fabricated timestamp.

Estimated time must remain distinguishable from known time.

---

# 20. Identity

Objects requiring stable identity use strongly typed logical identifiers.

Conceptually:

```cpp
Id<EventTag>
Id<AnalysisTag>
Id<MetricTag>
```

Logical identity is independent of:

* memory address
* storage key
* object lifetime
* serialization
* container position
* implementation representation

Identity and version remain separate concepts.

---

# 21. Error Model

Expected operational failures use:

```text
Result<T>
```

with expected-like semantics.

The conceptual error model contains:

```text
Error
{
    code
    category
    diagnostic information?
    context?
    cause?
}
```

Errors are:

* value-owned
* immutable
* independently valid
* safely movable
* copyable where practical

Exceptions are not the default mechanism for ordinary operational failures.

---

# 22. Ownership

Ownership must be explicit at component boundaries.

Default assumptions are:

```text
Semantic values       → value ownership
Historical events     → immutable
Processor state       → processor-owned
Configuration         → component-owned
Queued work           → queue-owned after admission
Returned results      → caller-owned
Errors                → value-owned
Temporary views       → borrowed
Shared mutable state  → avoided by default
```

Zero-copy is an optimization, not an ownership model.

---

# 23. Serialization

Serialization is an explicit translation boundary.

C++ memory representation is never treated as a portable wire format.

Serialization must preserve, where contractually relevant:

* identity
* temporal semantics
* optional/missing/unknown state
* ownership semantics
* version information
* provenance

The serialization technology remains undecided.

---

# 24. Persistence

Persistence is represented through semantic storage interfaces.

Storage implementations must not redefine:

* identity
* temporal meaning
* ordering
* provenance
* durability semantics
* source-of-truth semantics

The architecture permits multiple storage implementations.

No specific database is required by Core.

---

# 25. Query Architecture

Queries are semantic requests for information.

They are distinct from physical storage operations.

Conceptually:

```text
Stored Data
    ↓
Query
    ↓
Read Model / Result
    ↓
Application / Analysis / Interface
```

Query contracts explicitly define relevant:

* scope
* temporal semantics
* consistency
* ordering
* identity
* filtering
* pagination
* result ownership

---

# 26. Domain Boundary

Domains provide meaning.

A domain may define:

```text
Entities
Events
State
Metrics
Patterns
Analyses
Policies
Relationships
Domain Validation
Domain Ingestion
Domain Serialization
```

The generic Core must never contain domain-specific semantics.

For the initial implementation:

```text
domains/
└── poker/
```

is the first domain extension.

---

# 27. Application Boundary

Applications compose capabilities into executable workflows.

Applications determine:

* which domain is used
* which processors are composed
* which storage is selected
* which interfaces are exposed
* which policies are applied
* which execution environment is required

Applications do not redefine Core semantics.

---

# 28. Application Operations

Application operations form the boundary between external invocation and application behavior.

Primary categories:

```text
Command
Query
Long-running Operation
```

Commands request behavior.

Queries request information.

Operation identity is distinct from:

```text
RequestId
EventId
ProcessorId
GraphId
RunId
TraceId
```

---

# 29. Interface Boundary

Interfaces translate external interaction into application operations.

Possible interfaces include:

```text
CLI
HTTP
gRPC
IPC
Library
Messaging
GUI
File
```

No interface technology is architecturally required.

Interfaces must not leak transport-specific semantics into Core.

---

# 30. Ingestion

Ingestion follows:

```text
External Source
      ↓
Acquisition
      ↓
Parsing
      ↓
Structural Validation
      ↓
Normalization
      ↓
Semantic Validation
      ↓
Domain Mapping
      ↓
EVolution Information
```

Ingestion must preserve source identity, temporal knowledge, provenance, and explicit loss/deduplication semantics.

---

# 31. Security

External input is untrusted by default.

Security distinguishes:

```text
Identity
Authentication
Authorization
Trust
Secrets
Confidentiality
Integrity
Audit
Provenance
Observability
```

Secrets must not become ordinary configuration data, analytical data, or diagnostic output.

---

# 32. Observability

Observability is operational rather than analytical.

Primary signals are:

```text
Logs
Metrics
Traces
Health / Status
Events
```

Operational telemetry must not silently become domain information.

Likewise, analytical measurements must not be confused with operational metrics.

---

# 33. Versioning

EVolution has multiple version dimensions:

```text
API
ABI
Schema
Component
Algorithm
Configuration
Graph
Extension
Application
Data
```

Semantic compatibility is more important than version-number similarity.

Migration and reprocessing remain distinct operations.

---

# 34. External Dependencies

External dependencies are explicit capabilities.

Dependencies may be:

```text
Required
Optional
Conditional
```

Dependency identity, configuration, version, availability, lifetime, failure, side effects, and reproducibility implications must be explicit where relevant.

A dependency's physical implementation remains replaceable where the contract permits.

---

# 35. Runtime and Deployment

Logical architecture is independent of physical deployment.

A component may execute:

```text
In Process
Separate Thread
Separate Process
Container / Host
Remote System
```

without changing its semantic contract.

Physical boundaries may introduce additional failure, resource, communication, or security semantics, which must be modeled explicitly.

---

# 36. Packaging

Build, package, install, runtime, and release are distinct.

```text
Source
  ↓
Build
  ↓
Artifact
  ↓
Package
  ↓
Install
  ↓
Runtime
```

Runtime state must not normally be stored inside the installation tree.

No specific package format has been selected.

---

# 37. C++ API and ABI

C++ is the primary native API language.

However:

```text
C++ API
    ≠
Stable C++ ABI
```

Arbitrary C++ ABI compatibility is not assumed.

Stable binary boundaries require an explicit ABI or protocol.

A C ABI is supported where appropriate.

---

# 38. Registration and Discovery

Extensions follow:

```text
Register
   ↓
Discover
   ↓
Select
   ↓
Construct
   ↓
Initialize
   ↓
Use
```

Registration does not imply activation.

Discovery does not imply selection.

Selection is explicit application/graph policy.

---

# 39. Testing

Testing follows a layered architecture:

```text
Unit
Contract
Integration
System
End-to-End
Property
Regression
Performance
Fault / Recovery
```

Architectural contracts should be tested independently from implementation details wherever possible.

---

# 40. Architectural Dependency Direction

The baseline dependency direction is:

```text
External Systems
       ↓
   Interfaces
       ↓
  Applications
       ↓
Domain / Analysis
       ↓
  Processing
       ↓
      Core
```

Storage and external dependencies attach through explicit contracts.

The critical restriction is:

```text
Core
  ✕→ Domain
  ✕→ Application
  ✕→ Interface
  ✕→ Specific Storage Technology
```

Specific layers may depend on more generic layers, but generic layers must not depend on specific application concerns.

---

# 41. Implementation Language

EVolution uses:

```text
C++20
```

as its minimum C++ standard.

C remains supported where it provides an appropriate boundary or implementation benefit.

C++ should favor:

* RAII
* value semantics
* explicit ownership
* composition
* strong types
* standard facilities
* controlled abstraction

Complex template machinery, deep inheritance, and hidden global state remain discouraged.

---

# 42. Build System

CMake is the primary build system.

CMake targets should correspond approximately to architectural components.

CTest is the test execution integration.

The build must preserve architectural dependency direction.

Optional components should remain optional where practical.

---

# 43. Initial Source Structure

The initial physical structure is:

```text
evolution/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── architecture/
│   ├── concepts/
│   └── decisions/
├── core/
├── processing/
├── domains/
│   └── poker/
├── analysis/
├── storage/
├── ingestion/
├── applications/
├── interfaces/
├── tests/
├── tools/
└── cmake/
```

The exact internal file organization remains an implementation concern as long as architectural ownership remains visible.

---

# 44. What Is Intentionally Not Decided

The architecture deliberately leaves several implementation choices open.

These include:

```text
Database
Serialization format
GUI framework
HTTP framework
RPC framework
IPC mechanism
Thread pool
Executor
Scheduler implementation
Coroutine usage
Event loop
Queue implementation
Memory allocator
Lock-free structures
Plugin mechanism
Dynamic loading mechanism
Container technology
Service manager
Package format
Repository
Logging framework
Metrics backend
Tracing backend
Secret manager
Authentication framework
Configuration format
C++ ABI strategy details
```

These are not architectural omissions.

They are deferred decisions because the higher-level contracts do not require selecting them yet.

---

# 45. Implementation Readiness Rule

A component may begin implementation when its semantic contract is sufficiently defined.

At minimum, the component must know:

```text
What it owns
What it consumes
What it produces
What its configuration means
What state it maintains
What errors it can produce
What lifecycle it follows
What ordering it requires
What temporal semantics it uses
What identity it uses
What provenance it must preserve
What concurrency it permits
What cancellation means
What resources it requires
```

The implementation must not invent missing semantics silently.

If implementation requires a new architectural decision, that decision must be documented before the implementation establishes it as an implicit contract.

---

# 46. AI Implementation Rule

AI-assisted implementation must follow the architecture documents as normative design input.

An implementation agent must:

1. Read the relevant architecture and decision documents before modifying architectural code.
2. Preserve established contracts.
3. Avoid introducing hidden global state.
4. Avoid silently selecting deferred technologies as architectural requirements.
5. Avoid exposing implementation details as public APIs.
6. Add an ADR when implementation requires a new architectural decision.
7. Keep domain semantics outside Core.
8. Keep physical execution mechanisms behind the appropriate abstraction.
9. Add tests for newly established contracts.
10. Never resolve an architectural ambiguity by silently choosing behavior that changes system semantics.

Implementation convenience is not sufficient justification for violating an architectural boundary.

---

# 47. Architecture Change Rule

An implementation may reveal that an existing decision is insufficient.

In that case:

```text
Implementation Discovery
        ↓
Identify Architectural Impact
        ↓
Create / Revise ADR
        ↓
Update Affected Contracts
        ↓
Implement
```

A code change must not become the de facto architecture merely because it was implemented first.

---

# 48. Implementation Order

The implementation should proceed from the most foundational contracts toward higher-level functionality.

A reasonable initial order is:

```text
1. Core value types and concepts
2. Identity
3. Time
4. Error / Result
5. Configuration
6. Context / Provenance
7. Event / State / Measurement primitives
8. Processor contracts
9. Processor lifecycle
10. Processing graph
11. Scheduling / execution abstractions
12. Storage interfaces
13. Query interfaces
14. Domain infrastructure
15. Poker domain
16. Analysis infrastructure
17. Application layer
18. Interfaces
19. Runtime / packaging integration
```

This is an implementation guideline rather than a requirement that every component be completed before another can begin.

---

# 49. Foundational Implementation Boundary

The first implementation phase should establish a minimal usable foundation rather than attempting to implement the complete platform.

The initial foundation should make it possible to express:

```text
Value
Identity
Time
Error
Result
Configuration
Context
Provenance
Event
Processor
```

and test their contracts.

The first implementation must not prematurely implement:

```text
Database
GUI
Distributed Runtime
Plugin System
Production Scheduler
Complex Analytics
```

unless a concrete requirement makes one necessary.

---

# 50. Architecture Stability

This ADR does not declare the architecture immutable.

It establishes a baseline.

Future changes should distinguish:

```text
Clarification
Extension
Refinement
Correction
Breaking Architectural Change
```

A clarification should not be treated as a new semantic behavior if the existing contract already implied it.

A genuine semantic change requires explicit documentation.

---

# 51. Architecture Completion Criteria

The architecture is considered sufficiently mature for implementation when:

* major semantic concepts have explicit definitions
* module boundaries are explicit
* dependency direction is defined
* processor semantics are defined
* lifecycle semantics are defined
* execution semantics are separated from execution mechanisms
* storage semantics are separated from storage technology
* application and interface boundaries are defined
* error and ownership models are defined
* temporal semantics are defined
* identity and provenance are defined
* configuration and execution context are distinguished
* delivery and recovery semantics are defined
* security boundaries are defined
* testing strategy is defined
* versioning boundaries are defined
* deferred implementation choices are explicitly identified

These conditions are now satisfied sufficiently to begin foundational implementation.

---

# Decision Summary

```text
Architecture status:             Ready for foundational implementation
Language:                        C++20
Build:                           CMake + CTest
Core:                            Generic
Domain semantics:                Domain-owned
Processing:                      Explicit graph
Execution:                       Separate from scheduling
Concurrency:                     Explicit contract
Default processor concurrency:  Serial
Admission:                       Explicit
Delivery:                        Explicit
Recovery:                        Explicit
Storage:                         Contract-based
Queries:                         Semantic
Configuration:                   Explicit
Execution context:               Explicit
Identity:                        Strongly typed
Time:                            Explicit
Errors:                          Result<T>
Error ownership:                 Value-owned
Serialization:                   Explicit boundary
Security:                        Cross-cutting boundary
Observability:                   Separate operational layer
Interfaces:                      Adapter-based
Applications:                    Composition boundary
Extensions:                      Explicit registration/discovery
Packaging:                       Separate from build/install/runtime
C++ ABI:                         Not universally stable
Testing:                         Layered and contract-oriented
Physical deployment:             Decoupled from logical architecture
Architecture changes:            Explicitly documented
```

## Invariant

**EVolution's accepted architecture is the baseline for implementation: implementation components must realize established semantic contracts rather than silently redefining them, and any implementation requirement that materially changes architecture, ownership, lifecycle, execution, persistence, compatibility, or domain meaning must be resolved explicitly through the architecture decision process before becoming an implicit contract.**

