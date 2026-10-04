# ADR 0047: Resource and Configuration Distribution Model

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution separates **configuration**, **execution context**, and **resource availability** while defining how these concerns are distributed through the application and processing graph.

The fundamental rule is:

```text id="8z1b5f"
Configuration
    = how a component should behave

Execution Context
    = under what relevant runtime conditions it operates

Resource Availability
    = what execution capacity is currently available
```

These concepts must not be represented as one universal mutable environment object.

Configuration is resolved before component behavior begins and is normally logically immutable during execution.

Execution context may vary during execution.

Resource availability is inherently dynamic.

---

## 1. Configuration Ownership

Every configurable component owns the semantics of its configuration.

For example:

```text id="p0l4qv"
Processor
    → ProcessorConfiguration

Storage
    → StorageConfiguration

Domain
    → DomainConfiguration

Application
    → ApplicationConfiguration
```

A parent component may provide configuration to a child, but must not redefine the child's configuration semantics.

The consuming component remains responsible for validating configuration that requires its own semantic knowledge.

---

## 2. Configuration Hierarchy

Configuration may exist at several scopes:

```text id="k3n8zs"
Application Configuration
        ↓
Graph Configuration
        ↓
Node / Processor Configuration
        ↓
Domain Configuration
        ↓
Dependency Configuration
```

These scopes must remain distinct.

A higher-level configuration may select or compose lower-level configuration, but should not silently override values whose ownership belongs to another component.

---

## 3. Configuration Distribution

Configuration should flow explicitly through construction or configuration operations.

Conceptually:

```text id="5n7l9c"
Configuration Source
        ↓
Resolution
        ↓
Validation
        ↓
Effective Configuration
        ↓
Component
```

Possible sources include:

* configuration files
* command-line arguments
* environment variables
* API requests
* databases
* programmatic configuration
* application defaults

These are configuration **sources**, not different semantic configuration models.

---

## 4. Effective Configuration

A component operates using an effective configuration.

```text id="1h75vl"
Raw Sources
    ↓
Resolve
    ↓
Defaults
    ↓
Overrides
    ↓
Validate
    ↓
Effective Configuration
```

The effective configuration must be identifiable when it materially affects behavior.

A component must not behave differently because an unrelated configuration source remains accessible globally after configuration resolution.

---

## 5. Configuration Immutability

Once a component has successfully entered its configured state, its effective configuration is logically immutable.

Therefore:

```text id="m1h3r9"
ACTIVE processor
    ↓
hidden configuration mutation
```

is prohibited.

Runtime reconfiguration requires an explicit future mechanism.

Possible future mechanisms include:

* stop → reconfigure → restart
* versioned configuration
* transactional reconfiguration
* dynamic configuration operation

None is selected by this ADR.

---

## 6. Configuration Propagation Through Processing Graphs

A processing graph may contain many processors with different configurations.

The graph should therefore represent configuration explicitly:

```text id="c0o0n6"
Graph
 ├── Graph Configuration
 ├── Node A Configuration
 ├── Node B Configuration
 └── Node C Configuration
```

A processor should receive only the configuration relevant to its contract.

The graph must not require processors to access a global application configuration object.

---

## 7. Configuration vs Execution Context

Configuration and execution context must remain distinct.

For example:

```text id="9j1i4k"
Configuration:
    window_size = 100

Execution Context:
    mode = REPLAY
    run_id = R123
    clock = historical_clock
```

Configuration defines intended behavior.

Execution context describes the conditions under which that behavior executes.

Changing execution context does not necessarily mean changing configuration.

---

## 8. Execution Context Propagation

Execution context may flow through an application and processing graph.

Conceptually:

```text id="x9y0hr"
Application
    ↓
Graph
    ↓
Processor
    ↓
Operation
```

Each layer may derive a more specific context while preserving the relevant parent context.

Context should be passed explicitly rather than obtained from hidden thread-local or process-global state unless a future execution mechanism explicitly establishes such a facility as an implementation detail.

---

## 9. Context Projection

A component should receive only the context it needs.

For example:

```text id="h8t4f1"
Processor A
    needs:
        execution mode
        clock
        cancellation

Processor B
    needs:
        execution mode
        randomness source
```

This is preferable to passing an unrestricted:

```text
Context<Map<String, Any>>
```

to every component.

The architecture favors explicit semantic dependencies over universal context bags.

---

## 10. Resource Availability

Resources differ from configuration because availability changes during execution.

For example:

```text id="8y1v6x"
Configuration:
    max_memory = 1 GiB

Resource Availability:
    currently available memory = 320 MiB
```

The configured limit does not guarantee that the resource is currently available.

Resource availability must therefore be represented through runtime mechanisms rather than configuration.

---

## 11. Resource Requirements

Components may declare resource requirements.

Examples:

```text id="9r5w3d"
Processor
    CPU requirement
    memory requirement
    queue capacity
    storage requirement
    network requirement
```

Requirements are part of the component/execution contract.

They may be:

```text
Minimum
Preferred
Maximum
```

where useful.

The exact resource model remains governed by ADR 0028.

---

## 12. Resource Distribution

Resources may be allocated at different scopes.

```text id="r2v6as"
Application
    ↓
Graph
    ↓
Processor
    ↓
Operation
```

For example:

```text id="z5u3a1"
Application:
    8 GiB memory limit

Graph:
    4 GiB processing budget

Processor:
    512 MiB working-set limit
```

These limits must have explicit semantics.

A child limit must not silently exceed a parent limit.

---

## 13. Resource Context

Runtime resource information may be made available through execution context or explicit resource interfaces.

However:

```text id="3f1k8m"
Execution Context
    ≠
Resource Manager
```

Context may describe relevant resource conditions.

The resource subsystem remains responsible for:

* availability
* reservation
* allocation
* release
* accounting

---

## 14. Admission and Resource Allocation

Work admission must consider resource constraints where required.

Conceptually:

```text id="2q8g3y"
Submit
  ↓
Validate
  ↓
Admission
  ↓
Resource Check
  ↓
Reserve
  ↓
Execute
  ↓
Release
```

Resource exhaustion must produce an explicit result such as:

```text
Error(ResourceExhausted)
```

or an explicit admission outcome defined by the processor contract.

It must not silently become successful processing.

---

## 15. Resource Reservation Lifetime

A resource reservation must have a defined lifetime.

Conceptually:

```text id="v2p8lm"
Requested
    ↓
Reserved
    ↓
Consumed
    ↓
Released
```

If admission fails after reservation, the reservation must be released.

If cancellation occurs, resources must be released according to the cancellation contract.

If abort occurs, physical resource cleanup remains required even though semantic completion is not guaranteed.

---

## 16. Configuration and Resource Limits

Configuration may define resource limits.

For example:

```text id="h0c4sa"
queue_capacity = 10000
memory_limit = 512 MiB
max_concurrency = 8
```

These values are configuration.

The actual available resources are runtime conditions.

Therefore:

```text id="5i7l2d"
Configured Limit
    ≠
Currently Available Resource
```

Both may be needed to determine whether work can proceed.

---

## 17. Application-Level Distribution

The Application is responsible for composing configuration and resource policy.

Conceptually:

```text id="4k1v9w"
Application
 ├── resolves configuration
 ├── constructs components
 ├── establishes execution context
 ├── establishes resource policy
 └── creates processing graph
```

The Application must not bypass component-owned validation.

It provides configuration; the component determines whether that configuration is semantically valid.

---

## 18. Graph-Level Distribution

A processing graph may establish shared execution constraints.

For example:

```text id="j6z3dp"
Graph
    concurrency limit
    queue budget
    execution mode
    cancellation policy
```

Individual processors may additionally declare their own requirements.

The graph must ensure that its configuration is compatible with its nodes and connections.

Graph validation may therefore include:

* resource compatibility
* concurrency compatibility
* capacity compatibility
* dependency availability where statically knowable

---

## 19. Processor-Level Distribution

A processor receives:

```text id="7u5jcw"
Input
Configuration
Execution Context
State
```

It should not need to discover configuration or resources through unrelated global mechanisms.

A processor may request resources through an explicit execution/resource contract.

The processor should not directly control application-wide resource allocation.

---

## 20. Resource Sharing

Resources may be shared.

Examples:

```text id="n8e7k2"
Multiple processors
        ↓
Shared storage capacity

Multiple operations
        ↓
Shared execution slots
```

Shared resources require explicit accounting.

A processor must not assume exclusive access unless the contract guarantees it.

Resource sharing can affect scheduling and admission but must not silently alter semantic results.

---

## 21. Fairness and Priority

Applications may require resource prioritization.

Examples:

```text id="x6f8m4"
High-priority analysis
Low-priority background indexing
```

Priority and fairness are execution policies.

They must not alter domain meaning unless explicitly defined as part of the processing contract.

Starvation prevention, priority inheritance, scheduling algorithms, and fairness mechanisms remain implementation concerns.

---

## 22. Resource Degradation

A component may support degraded execution.

For example:

```text id="a3q9f2"
Full resolution
      ↓ resource pressure
Reduced resolution
```

Degradation must be explicitly supported by the component contract.

Resource pressure must never silently change:

* sampling rate
* precision
* analytical scope
* data completeness
* ordering
* delivery guarantees

unless the degradation behavior is declared.

If degradation changes the meaning of a result, that result must be distinguishable as degraded.

---

## 23. Configuration Distribution and Security

Configuration may contain sensitive information.

Examples:

* credentials
* tokens
* private endpoints
* cryptographic material

Sensitive configuration must follow the Security and Trust Boundary Model.

The configuration model must support redaction and must not automatically expose secrets through:

* logs
* errors
* metrics
* provenance
* debugging output

---

## 24. Environment vs Configuration

The runtime environment is not automatically configuration.

For example:

```text id="a5s0qp"
Environment:
    CPU count
    hostname
    available memory
    OS version

Configuration:
    worker_limit = 8
    window_size = 100
```

An environment value becomes semantic configuration only when explicitly designated as a configuration source.

Likewise, an environment condition that materially affects results belongs in relevant execution context or provenance rather than being silently treated as configuration.

---

## 25. Configuration and Reproducibility

Material configuration participates in reproducibility.

Conceptually:

```text id="x0w4zc"
Input
+
Effective Configuration
+
Component / Algorithm Version
+
Relevant Execution Context
+
Required State
→
Result
```

Configuration provenance should distinguish:

```text id="g5w8jv"
Configured Value
Source of Value
Effective Value
```

For example:

```text
window_size:
    source = configuration file
    configured = 50
    effective = 100
    reason = explicit application override
```

The exact representation is deferred.

---

## 26. Configuration Inheritance

Inheritance should not be implicit.

A child may receive defaults from its parent, but the resulting effective configuration must be resolved explicitly.

Conceptually:

```text id="7w9c3h"
Parent Defaults
    +
Child Configuration
    ↓
Resolved Child Configuration
    ↓
Validation
    ↓
Effective Configuration
```

A component should not need to traverse an arbitrary parent configuration tree at runtime to determine its behavior.

---

## 27. Configuration Conflicts

Conflicting configuration sources must have deterministic resolution semantics.

For example:

```text id="p7g1y5"
File
Environment
CLI
API
```

may all specify the same value.

The application/configuration layer must define precedence before producing effective configuration.

A component should receive one resolved semantic configuration rather than deciding precedence between unrelated external sources.

---

## 28. Runtime Configuration Changes

Dynamic configuration is not assumed.

If introduced later, a change must define:

* who can request it
* which component owns it
* validation
* atomicity
* activation boundary
* effect on active work
* effect on state
* effect on reproducibility
* versioning
* rollback
* provenance

A configuration change must never silently alter the semantics of already-completed historical results.

---

## 29. Configuration, State, and Context

The distinctions are normative:

```text id="u1j6k3"
Configuration
    = intended component behavior

State
    = accumulated execution/history information

Execution Context
    = relevant runtime conditions

Resource Availability
    = currently available execution capacity

Provenance
    = origin and derivation of resulting information
```

For example:

```text
window_size = 100
    → Configuration

processed_count = 437
    → State

mode = REPLAY
    → Execution Context

available_memory = 512 MiB
    → Resource Availability

derived_from = event-stream-42
    → Provenance
```

None should be silently substituted for another.

---

## 30. Deferred Decisions

This ADR does not select:

* dependency injection framework
* configuration file format
* environment variable convention
* configuration service
* resource manager implementation
* scheduler/resource scheduler
* thread pool
* CPU affinity
* memory allocator
* container resource mechanism
* dynamic configuration mechanism
* configuration serialization format
* secret management implementation
* distributed configuration system
* resource quota technology

These remain implementation and deployment decisions.

---

## Decision Summary

```text id="3w8j1c"
Configuration:                 Explicit
Configuration ownership:       Component/domain
Effective configuration:       Resolved + validated
Runtime mutation:              Not implicit
Execution context:             Separate
Resource availability:         Separate
Resource requirements:         Explicit
Resource allocation:           Explicit
Resource exhaustion:           Explicit outcome
Application distribution:      Application-owned composition
Graph distribution:            Graph-owned composition
Processor distribution:        Explicit inputs/context/config
Global mutable configuration:  Prohibited
Universal context bag:         Prohibited
Implicit inheritance:          Prohibited
Secrets:                       Protected/redacted
Dynamic reconfiguration:       Deferred
```

## Invariant

**EVolution distributes configuration, execution context, and resources through explicit contracts: configuration defines intended behavior, execution context defines materially relevant runtime conditions, and resource availability defines current execution capacity; none may be replaced by hidden global state or an implicit universal context.**

