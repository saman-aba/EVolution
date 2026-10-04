# ADR 0045: External Dependency Model

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution treats external systems, libraries, services, datasets, runtimes, and resources as **explicit dependencies** rather than hidden environmental assumptions.

An external dependency is anything outside a component's ownership boundary that can materially affect its operation or result.

Conceptually:

```text
Component
    ↓
Dependency Contract
    ↓
External Dependency
```

The dependency may be:

* an in-process library
* operating-system facility
* filesystem
* database
* network service
* external API
* message broker
* dataset
* model
* hardware capability
* runtime service
* credential/secret provider
* another EVolution application

The architecture must distinguish:

```text
Dependency Definition
Dependency Configuration
Dependency Availability
Dependency Runtime State
Dependency Failure
Dependency Version
```

These concepts must not be collapsed into a single "dependency exists" condition.

---

## 1. Explicit Dependencies

A component must declare the external capabilities it requires to perform its contract.

Dependencies must not be obtained implicitly from unrelated global state.

For example:

```text
Processor
    ├── configuration
    ├── clock
    ├── randomness source
    └── external data provider
```

must be represented through explicit contracts where those dependencies materially affect behavior.

The implementation mechanism may vary, but the semantic dependency must remain visible.

---

## 2. Dependency Categories

External dependencies are broadly classified as:

```text
Execution Dependency
Data Dependency
Storage Dependency
Network Dependency
Library Dependency
Runtime Dependency
Resource Dependency
Security Dependency
External Service
Hardware Capability
```

These categories are descriptive rather than exhaustive.

A dependency may belong to multiple categories.

For example:

```text
Database
    → Storage Dependency
    → Network Dependency
    → External Service
```

---

## 3. Required vs Optional Dependencies

Dependencies must be explicitly classified as:

```text
REQUIRED
OPTIONAL
CONDITIONAL
```

### Required

The component cannot fulfill its contract without the dependency.

Failure to make the dependency available prevents normal operation.

### Optional

The component can operate without the dependency while preserving its defined semantics.

Optional dependencies must not silently change the meaning of successful results.

### Conditional

The dependency is required only under a specific configuration or execution mode.

For example:

```text
LIVE mode
    → requires external market feed

REPLAY mode
    → uses historical input
```

The effective dependency set therefore depends on explicit configuration and execution context.

---

## 4. Dependency Contract

A dependency should be represented through a semantic contract rather than directly exposing its implementation.

Conceptually:

```text
Dependency
{
    identity
    capability
    version?
    configuration
    availability
    health?
}
```

A component should depend on:

```text
Capability / Contract
```

rather than:

```text
Concrete implementation
```

where practical.

This allows implementations to be replaced without changing the consuming component's semantic contract.

---

## 5. Dependency Identity

Dependency identity is distinct from:

* object identity
* processor identity
* process identity
* connection identity
* version
* configuration
* runtime instance

A dependency may have:

```text
Logical Dependency Identity
Implementation Identity
Runtime Instance Identity
```

These must not be conflated.

For example:

```text
MarketDataProvider
    implementation = ProviderA
    version = 3.2
    runtime instance = connection-17
```

The logical dependency remains the same even when its runtime instance changes.

---

## 6. Dependency Version

Dependency versions are relevant when behavior can change between versions.

A dependency version may therefore participate in reproducibility:

```text
Input
+
Configuration
+
Component Version
+
Dependency Versions
+
Relevant Execution Context
→
Result
```

Not every dependency requires version capture.

A dependency version is relevant when changing it can materially alter:

* processing behavior
* analytical results
* serialization
* numerical results
* external data
* recovery behavior

Purely incidental implementation details need not become part of semantic provenance.

---

## 7. Dependency Configuration

Dependency configuration is distinct from component configuration.

For example:

```text
Analysis Processor Configuration
    ≠
Database Connection Configuration
```

The application may compose these configurations, but ownership remains explicit.

A component must not silently read unrelated dependency configuration from global state.

Configuration values that materially affect results must be identifiable for reproducibility.

---

## 8. Availability vs Configuration

A dependency may be correctly configured but unavailable.

These are different conditions:

```text
Configuration:
    "Use database X"

Availability:
    "Database X is currently unreachable"
```

Configuration validation should determine whether the requested dependency configuration is valid.

Runtime availability determines whether the dependency can currently be used.

Therefore:

```text
Valid Configuration
    ≠
Available Dependency
```

---

## 9. Dependency Health

Availability and health are also distinct.

A dependency may be reachable but unable to fulfill its contract.

Conceptually:

```text
UNAVAILABLE
AVAILABLE
DEGRADED
HEALTHY
FAILED
UNKNOWN
```

These states are operational information.

They must not automatically become analytical data.

Dependency health may be exposed through the observability model and may influence application/supervision policy where explicitly configured.

---

## 10. Dependency Failure

Dependency failure is an external condition affecting an operation.

Examples:

* network unavailable
* database unavailable
* filesystem permission failure
* external API failure
* incompatible dependency version
* malformed dependency response
* dependency timeout
* dependency resource exhaustion

The consuming component must translate the condition into the appropriate EVolution `Error`/`Result` semantics.

The dependency failure itself does not determine recovery.

For example:

```text
ExternalFailure
```

does not automatically mean:

```text
Retry
```

Recovery remains the responsibility of the applicable recovery/supervision policy.

---

## 11. Dependency Failure Scope

Failure scope must remain explicit.

A dependency failure may affect:

```text
One Operation
    ↓
One Processor
    ↓
One Graph
    ↓
One Application
```

but must not automatically escalate through all scopes.

For example:

```text
Optional analytics service unavailable
    → analytical feature disabled
```

does not necessarily imply:

```text
Entire application FAILED
```

Required dependencies may produce stronger escalation according to application policy.

---

## 12. External Data Dependencies

External data is an important dependency category.

Examples:

* market feeds
* historical datasets
* user-provided files
* reference data
* model parameters
* external metadata

The system must distinguish:

```text
Source Identity
Dataset Identity
Dataset Version
Acquisition Time
Data Content
```

A dataset's current availability does not establish what data was used for a historical result.

For reproducibility, the relevant dataset version, snapshot, content identity, or equivalent provenance must be captured when necessary.

---

## 13. Live vs Historical Dependencies

A dependency may behave differently in different execution modes.

For example:

```text
LIVE
    External service provides current data

REPLAY
    Historical snapshot provides data

EXPERIMENT
    Synthetic provider provides data
```

These are different execution contexts even if they satisfy the same logical dependency contract.

A replay must not silently substitute live external information for historical input when reproducibility requires the original dependency state.

---

## 14. External Dependency Side Effects

Dependencies may be:

```text
READ_ONLY
STATEFUL_READ
SIDE_EFFECTING
```

A read-only dependency provides information without intentionally changing external state.

A side-effecting dependency can change external state.

Examples:

```text
Read:
    query database
    fetch market data

Side effect:
    submit order
    write external record
    send message
```

Side effects must be explicit in the component contract.

A component must not hide external side effects behind an operation that appears observational.

---

## 15. Dependency Delivery Semantics

External communication may introduce delivery uncertainty.

For side-effecting dependencies, the system may encounter:

```text
Request sent
    ↓
No response
    ↓
Did the external operation execute?
```

The architecture must not assume that a timeout means the operation did not occur.

Retrying may therefore produce duplicate external effects.

Side-effecting dependencies must define appropriate:

* idempotency
* request identity
* acknowledgement
* timeout
* retry
* deduplication
* compensation
* reconciliation

semantics where required.

Exactly-once external effects are not assumed.

---

## 16. Dependency Time

External dependencies may provide time information.

The source's temporal semantics must remain distinct from EVolution's own clocks.

For example:

```text
External Event Time
    ≠
Ingestion Time
    ≠
Processing Time
```

A dependency must not cause the system to replace unknown event time with current system time.

If an external source provides an estimated temporal value, its `ESTIMATED` status must be preserved.

---

## 17. Dependency Ownership

EVolution must distinguish:

```text
Dependency Ownership
Dependency Usage
Dependency Lifetime
```

A component may use a dependency without owning it.

For example:

```text
Application owns database connection manager
Processor uses database capability
```

The exact ownership arrangement is implementation-dependent, but the lifetime contract must be explicit.

A component must never retain a borrowed dependency beyond the lifetime guaranteed by its contract.

---

## 18. Dependency Initialization

Dependencies participate in application/component initialization.

Conceptually:

```text
Resolve Configuration
        ↓
Validate Dependency Definition
        ↓
Construct Dependency
        ↓
Check Required Capabilities
        ↓
Initialize
        ↓
Available
```

Initialization failure must be represented explicitly.

A component must not enter `ACTIVE` if a required dependency is known to be unavailable and the component's contract requires that dependency for activation.

Optional dependencies may allow activation without becoming available if degraded operation is explicitly supported.

---

## 19. Dependency Runtime Changes

Dependencies may change availability during execution.

Examples:

```text
Database connection lost
Network feed disconnected
External service degraded
Storage temporarily unavailable
```

Runtime dependency changes must not require hidden global state transitions.

The component, supervisor, or application may respond according to explicit policy.

Possible responses include:

```text
RETRY
PAUSE
RECONNECT
DEGRADE
FAIL
STOP
```

No response is universally implied.

---

## 20. Dependency Isolation

Dependencies may be isolated physically or logically.

For example:

```text
Application
    ↓
Dependency Adapter
    ↓
External Process
```

or:

```text
Application
    ↓
Library
```

The dependency contract remains the same where semantic compatibility permits.

Physical isolation follows ADR 0044.

---

## 21. Dependency Adapters

External systems should normally be accessed through explicit adapters.

Conceptually:

```text
Domain / Application
        ↓
Dependency Contract
        ↓
Adapter
        ↓
External System
```

The adapter translates:

* external representation
* external errors
* external lifecycle
* external authentication
* external transport
* external delivery semantics

into EVolution-compatible contracts.

Core must not directly depend on external service semantics.

---

## 22. Dependency Security

Dependencies may require:

* authentication
* authorization
* credentials
* certificates
* encryption
* trust configuration
* network permissions

These concerns belong at the appropriate security/dependency boundary.

Secrets must not be embedded into:

* logical identities
* ordinary configuration diagnostics
* error messages
* logs
* metrics
* provenance
* analytical objects

unless explicitly protected by a future security mechanism.

---

## 23. Dependency Resource Requirements

Dependencies may impose resource requirements.

Examples:

```text
Database:
    connection count

Network:
    bandwidth

External API:
    request rate

Hardware:
    device availability
```

These requirements interact with the Resource Model and may participate in admission or application startup validation.

Resource availability remains runtime information.

---

## 24. Dependency Reproducibility

A result that depends on an external system may not be reproducible merely from its local input.

For example:

```text
Analysis
    ↓
External Market API
    ↓
Current market state
```

may produce different results when executed later.

If reproducibility is required, the relevant external dependency state must be captured through an appropriate mechanism such as:

* snapshot
* version
* recorded response
* immutable dataset
* deterministic substitute
* equivalent provenance reference

The architecture does not mandate one mechanism.

---

## 25. Dependency Substitution

A dependency may be replaced for:

* testing
* replay
* simulation
* experimentation
* offline operation
* fault injection

A substitute must satisfy the required dependency contract.

Substitution must not silently change semantics.

For example:

```text
Production Database
        ↓
Test In-Memory Store
```

is valid only when the test store provides the capabilities required by the contract being tested.

---

## 26. Dependency Discovery

Dependency discovery may use the Registration and Discovery Model where the dependency represents a replaceable extension.

Discovery must distinguish:

```text
Available implementation
    ≠
Selected implementation
    ≠
Constructed instance
    ≠
Currently available runtime service
```

Missing required dependencies produce explicit errors.

Silent substitution is prohibited unless the application contract explicitly defines fallback behavior.

---

## 27. Testing External Dependencies

External dependencies must be testable without requiring the production dependency in every test.

Possible strategies include:

* in-memory implementations
* deterministic fakes
* test adapters
* recorded responses
* simulators
* fault injectors
* contract-compatible substitutes

The substitute must be tested against the same dependency contract where practical.

Tests must also cover dependency failure conditions where they affect component behavior.

---

## 28. Observability

Dependency behavior may generate operational signals such as:

* availability
* latency
* failure count
* reconnect count
* request rate
* resource usage

These belong to observability unless explicitly promoted into analytical data.

Dependency operational metrics must not silently become domain measurements.

---

## 29. Dependency Graph

Applications may contain a dependency graph:

```text
Application
 ├── Processing Graph
 │    ├── Dependency A
 │    └── Dependency B
 │
 ├── Storage
 │    └── Database
 │
 └── Interface
      └── Authentication Service
```

Dependency relationships must be explicit enough to support:

* initialization ordering
* failure handling
* shutdown
* supervision
* resource accounting
* provenance
* reproducibility

Circular dependencies should be avoided unless explicitly supported by the architecture.

---

## 30. Deferred Decisions

This ADR does not select:

* dependency injection framework
* service discovery mechanism
* configuration service
* database technology
* HTTP/RPC library
* message broker
* secret manager
* credential provider
* service mesh
* package manager
* dynamic linker strategy
* external API framework
* dependency health protocol
* distributed service architecture
* retry library
* circuit breaker implementation

These remain implementation or deployment decisions.

---

## Decision Summary

```text
Dependencies:                  Explicit
Dependency contract:           Required
Required/optional/conditional: Explicit
Configuration:                 Separate from availability
Availability:                  Runtime property
Health:                        Operational property
Version:                       Captured when materially relevant
Side effects:                  Explicit
External delivery:             Not assumed exactly-once
Dependency failure:            Result/Error
Recovery:                      Explicit policy
Ownership/lifetime:            Explicit
External data provenance:      Preserved when relevant
Live vs replay:                Explicit
Substitution:                  Contract-based
Adapters:                      Preferred boundary
Secrets:                       Never implicitly exposed
Observability:                 Separate from analytical data
Physical isolation:            Governed by ADR 0044
```

## Invariant

**EVolution treats external dependencies as explicit contractual capabilities: their identity, configuration, availability, version, lifetime, failure, side effects, and reproducibility implications must be defined where relevant, while the specific library, service, database, transport, or deployment mechanism remains replaceable.**

