# ADR 0044: Process and Component Isolation Model

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution distinguishes **logical component boundaries** from **physical isolation boundaries**.

A component may be isolated at several levels:

```text
Logical Component
    ↓
In-Process Isolation
    ↓
Thread / Execution Isolation
    ↓
Process Isolation
    ↓
Host / Container Isolation
    ↓
Remote System Isolation
```

The architecture must not require a particular physical isolation mechanism. Components communicate through explicit contracts regardless of whether they execute in the same function call, thread, process, host, or remote system.

Physical isolation may be introduced for:

* failure containment
* security isolation
* resource isolation
* dependency isolation
* deployment independence
* restartability
* compatibility constraints
* operational scaling

Physical isolation must not silently change the semantic contract of the component.

---

## 1. Logical Component vs Physical Unit

A logical component is an architectural responsibility.

A physical unit is where that responsibility executes.

For example:

```text
Logical:
    Analysis Processor

Possible physical representations:
    - C++ object in the application process
    - dedicated worker thread
    - separate process
    - external service
```

These representations are implementation and deployment choices.

The logical contract remains defined by:

* inputs
* outputs
* configuration
* execution context
* state
* lifecycle
* errors
* cancellation
* delivery semantics
* ordering
* provenance
* resource requirements

A physical boundary must therefore be treated as an implementation of an existing contract rather than as a new semantic model.

---

## 2. In-Process Components

Components may execute within the same process.

In-process execution provides:

* direct function calls
* shared address space
* low communication overhead
* direct access to compatible memory
* simple object ownership relationships

However, in-process components also share:

* process failure
* address-space corruption risk
* some resource limits
* process lifetime
* runtime environment

Therefore:

```text
In-process ≠ isolated failure domain
```

A component that crashes the process can affect unrelated components regardless of their logical architectural boundaries.

---

## 3. Thread and Execution Isolation

Thread-level separation may be used to provide execution independence or concurrency.

Thread separation does **not** automatically provide:

* memory isolation
* failure isolation
* security isolation
* independent process lifecycle

Threads remain part of the same process and therefore share the process address space.

Threading must follow the Processing Concurrency Model rather than redefine it.

The architecture must not assume:

```text
one component = one thread
```

or:

```text
one thread = one component
```

unless a future implementation explicitly chooses such a mapping.

---

## 4. Process Isolation

A logical component or group of components may execute in a separate process.

A process boundary provides stronger isolation for:

* memory
* process failure
* dependency loading
* restart
* resource limits
* security permissions

But it introduces additional concerns:

* serialization
* communication failure
* process startup/shutdown
* unknown remote completion
* duplicated work
* IPC resource limits
* independent recovery
* version compatibility

Therefore a process boundary must use an explicit communication contract.

It must not rely on C++ object identity, pointers, references, or process-local memory addresses.

---

## 5. Failure Domains

Isolation boundaries create potential failure domains.

Conceptually:

```text
Operation Failure
      ↓
Processor Failure
      ↓
Component/Connection Failure
      ↓
Process Failure
      ↓
Application Failure
      ↓
Host/Runtime Failure
```

These are not automatically equivalent.

A failure should be contained at the smallest scope capable of continuing safely.

For example:

```text
processor operation fails
        ↓
processor remains ACTIVE
```

does not imply:

```text
process fails
```

Likewise:

```text
worker process fails
```

does not automatically imply:

```text
entire application fails
```

Escalation is determined by explicit supervision and application policy.

---

## 6. Lifecycle Ownership

Every physical component instance must have an explicit lifecycle owner.

The owner is responsible for coordinating:

* creation
* configuration
* initialization
* activation
* shutdown
* cancellation
* termination
* recovery where applicable

Physical process termination must not be treated as equivalent to orderly processor shutdown.

For example:

```text
Processor STOP
    → drain
    → finalize
    → checkpoint if required
    → release resources
    → STOPPED
```

whereas:

```text
Process termination
    → execution stops
    → no completion guarantees
```

A process restart may therefore require the recovery mechanisms defined by the Processing Recovery Model.

---

## 7. State and Isolation

Processor state remains logically owned by the processor regardless of physical placement.

If a processor moves across a physical boundary:

```text
Processor
    ↓
Process A
```

to:

```text
Processor
    ↓
Process B
```

its logical state semantics must remain unchanged.

If state must survive process termination, persistence/checkpointing must provide that capability.

Process memory itself must not implicitly become authoritative persistent state.

Therefore:

```text
Process memory ≠ durable processor state
```

unless an explicit persistence contract says otherwise.

---

## 8. Communication Across Isolation Boundaries

When a component boundary crosses a physical isolation boundary, communication must use an explicit representation.

Conceptually:

```text
Component A
    ↓
Output Contract
    ↓
Serialization / Transport
    ↓
Communication Boundary
    ↓
Deserialization / Validation
    ↓
Component B
```

The boundary must preserve all semantic information required by the contract, including where applicable:

* logical identity
* temporal information
* sequence
* correlation
* provenance
* configuration/version references
* error semantics
* cancellation
* delivery semantics

Pointers and process-local addresses must never be treated as transferable semantic identity.

---

## 9. Communication Failure

Physical communication introduces failures that do not exist in direct in-process calls.

Examples include:

* unavailable destination
* connection failure
* timeout
* serialization failure
* transport failure
* peer termination
* unknown completion
* duplicate delivery

These failures must be represented through the existing error and delivery models.

A communication failure must not silently become:

```text
successful processing
```

Likewise, timeout must not automatically imply that the destination did not execute the operation.

The system may have:

```text
request sent
    ↓
unknown completion
```

and recovery must account for the declared delivery semantics.

---

## 10. Resource Isolation

Physical boundaries may be used to constrain resources.

Possible resource boundaries include:

* memory
* CPU
* storage
* file descriptors
* network capacity
* execution slots
* queue capacity

Resource limits remain governed by the Resource Model.

A physical boundary must not silently change semantic resource requirements.

For example, moving a processor into a smaller process/container must not silently turn:

```text
resource exhaustion → successful degraded result
```

unless degradation is explicitly part of its contract.

---

## 11. Security Isolation

A physical boundary may also provide a security boundary.

Examples include separation of:

* privileges
* credentials
* filesystem access
* network access
* secret access
* untrusted extensions
* external integrations

Security isolation does not replace application authorization.

For example:

```text
Process isolation
    ≠
Authorization
```

and:

```text
Container boundary
    ≠
Trust establishment
```

Security guarantees must remain explicit and must not be inferred merely from physical placement.

---

## 12. Dependency Isolation

A component may require dependencies that should not be loaded into the main application process.

Physical isolation can therefore be used to contain:

* incompatible libraries
* unstable third-party dependencies
* conflicting runtime requirements
* platform-specific components
* experimental implementations
* untrusted extensions

The dependency boundary must still preserve the component's semantic contract.

A dependency being isolated does not make its behavior automatically trustworthy or correct.

---

## 13. Recovery and Restart

Process restart and processor recovery are distinct concepts.

```text
Process Restart
    ↓
Physical execution recreated

Processor Recovery
    ↓
Logical processing state reconstructed
```

A restarted process may require:

1. configuration reconstruction
2. extension discovery
3. component construction
4. state restoration
5. checkpoint validation
6. buffer reconstruction
7. input replay
8. activation

The exact recovery sequence follows the Processing Recovery Model.

Restarting a process must not silently imply that processing state has been recovered correctly.

---

## 14. Observability

Physical isolation must remain visible to operational observability without becoming domain semantics.

Operational information may identify:

* process
* host
* worker
* execution instance
* connection
* restart
* resource boundary
* physical placement

These identities are distinct from logical domain identities.

For example:

```text
ProcessId
ExecutionId
TraceId
ProcessorId
EventId
```

must not be treated as interchangeable.

Physical topology may be included in provenance when it materially affects reproducibility or interpretation.

---

## 15. Deployment Independence

The same logical architecture should support multiple deployment arrangements.

For example:

```text
Single Process

Application
├── Processing
├── Analysis
├── Storage
└── Interfaces
```

or:

```text
Multiple Processes

Application
├── API Process
├── Processing Process
├── Analysis Process
└── Storage Process
```

or:

```text
Remote Components

Application
├── Local Processing
├── Remote Analysis
└── Remote Storage
```

These arrangements must not require different domain semantics.

Deployment topology is therefore configuration/deployment information rather than part of the domain model.

---

## 16. Physical Boundary and Delivery Semantics

Cross-process or remote communication must explicitly select delivery semantics.

The system must not assume:

```text
IPC = exactly once
```

or:

```text
TCP = exactly once processing
```

Transport reliability and processing delivery guarantees are different concepts.

A communication channel may provide reliable byte delivery while the destination still executes an operation more than once because of retries or unknown completion.

Delivery guarantees therefore remain governed by the Processing Delivery Semantics Model.

---

## 17. Isolation and Performance

Isolation introduces overhead.

Potential costs include:

* serialization
* copying
* context switching
* IPC latency
* synchronization
* network latency
* additional buffering
* duplicated resources

These are implementation concerns unless they materially affect semantic behavior.

Performance optimization must not silently weaken:

* correctness
* ordering
* identity
* delivery guarantees
* provenance
* recovery semantics
* ownership guarantees

Zero-copy or shared-memory communication may be introduced later as an optimization with an explicit ownership/lifetime contract.

---

## 18. Isolation and Extension Model

Extensions may execute:

* inside the application process
* in an isolated process
* through an external service

The Registration and Discovery Model remains responsible for identifying and selecting the extension.

Physical isolation does not change extension identity.

If an extension's physical implementation affects behavior, its selected implementation/version must participate in provenance and reproducibility as required.

---

## 19. Default Architectural Position

EVolution does not require every component to have its own process.

The default architectural position is:

```text
Logical isolation first.
Physical isolation only when justified.
```

Physical isolation should be introduced when it provides a concrete benefit such as:

* failure containment
* security
* dependency compatibility
* resource isolation
* independent restart
* deployment requirements
* operational scaling

It should not be introduced merely because a logical module exists.

---

## 20. Deferred Decisions

This ADR does not select:

* process architecture
* executable decomposition
* container technology
* service architecture
* IPC mechanism
* RPC protocol
* shared-memory mechanism
* operating-system sandboxing
* process supervision technology
* service manager
* orchestration platform
* CPU affinity
* namespace/cgroup mechanism
* network topology
* deployment topology
* dynamic process spawning model

These are implementation and deployment decisions to be made when required.

---

## Decision Summary

```text
Logical components:             Independent of physical placement
In-process execution:           Supported
Thread isolation:               Execution isolation only
Process isolation:              Supported
Remote execution:               Supported when contracts permit
Failure isolation:              Explicit, not automatic
Lifecycle ownership:            Explicit
Process restart:                Distinct from processor recovery
State persistence:              Explicit; process memory is not durable by default
Cross-boundary communication:   Explicit contract + representation
Pointers across processes:      Not semantic identity
Resource isolation:             Explicit
Security isolation:             Explicit
Delivery guarantees:            Explicit
Observability:                  Separate from domain semantics
Deployment topology:            Separate from logical architecture
Default:                        Logical isolation first
```

## Invariant

**EVolution separates logical component boundaries from physical isolation: components retain the same semantic contracts whether executed in-process, in separate processes, or remotely, while failure, lifecycle, resource, security, communication, and recovery behavior introduced by physical boundaries must be explicit rather than accidental.**

