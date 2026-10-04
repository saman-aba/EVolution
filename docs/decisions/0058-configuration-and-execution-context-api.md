# ADR 0058: Configuration and Execution Context API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent **Configuration** and **Execution Context** as separate concepts.

```text
Configuration
    → how a component should behave

Execution Context
    → under what materially relevant runtime conditions it operates
```

Neither concept will be implemented as a hidden global environment.

Configuration is resolved and validated before use and is logically immutable while a component is active.

Execution context is explicitly supplied to components and operations when runtime conditions materially affect behavior, results, reproducibility, or lifecycle.

---

# 1. Configuration

Configuration describes intended component behavior.

Examples:

```text
processor thresholds
window sizes
storage policies
analysis parameters
resource limits
domain rules
```

Configuration is not accumulated execution state.

---

# 2. Configuration vs State

The distinction is:

```text
Configuration
    → what should the component do?

State
    → what has happened so far?
```

For example:

```text
window_size = 100
```

is configuration.

```text
processed_items = 73
```

is state.

---

# 3. Configuration vs Execution Context

The distinction is:

```text
Configuration
    → intended behavior

Execution Context
    → runtime conditions
```

For example:

```text
algorithm = X
```

is configuration.

```text
execution_mode = REPLAY
run_id = R123
```

is execution context.

---

# 4. Configuration Ownership

Configuration belongs to the component whose behavior it defines.

Conceptually:

```text
Application
    ↓
Graph
    ↓
Processor
    ↓
Domain / Dependency
```

Higher-level components may compose or override configuration according to explicit contracts, but they must not silently modify another component's semantic configuration.

---

# 5. Configuration Sources

Possible configuration sources include:

```text
File
CLI
Environment
API
Database
Programmatic construction
Defaults
```

These are configuration **sources**, not configuration semantics.

The component should receive resolved semantic configuration rather than depending directly on whichever source supplied it.

---

# 6. Configuration Resolution

Configuration resolution conceptually follows:

```text
Sources
   ↓
Precedence / Override Resolution
   ↓
Defaults
   ↓
Effective Configuration
   ↓
Validation
   ↓
Component
```

Conflicting sources must be resolved before the component receives configuration.

---

# 7. Effective Configuration

The component should operate using an effective configuration.

The effective configuration contains:

```text
Explicit values
+
Resolved overrides
+
Accepted defaults
```

Defaults must be explicit and documented.

A hidden default that materially changes behavior is not acceptable.

---

# 8. Configuration Validation

Configuration validation has two levels.

### Structural validation

Determines whether the configuration is representable.

Examples:

```text
required field exists
correct type
valid structure
```

### Semantic validation

Determines whether the configuration makes sense for the component.

Examples:

```text
window_size > 0
min_value <= max_value
required dependency configured
incompatible options rejected
```

The component owns semantic validation of its own configuration.

---

# 9. Invalid Configuration

Invalid configuration must produce an explicit:

```text
Result / Error
```

A component must not silently replace invalid configuration with a convenient value.

For example:

```text
window_size = -1
```

must not silently become:

```text
window_size = 1
```

unless the contract explicitly defines that behavior.

---

# 10. Configuration Immutability

After successful configuration/application, configuration is logically immutable.

Active processing must not silently modify its configuration.

Runtime configuration changes require an explicit future mechanism.

Possible future mechanisms include:

```text
restart
reconfigure
new processor instance
new graph version
new execution run
```

No universal dynamic reconfiguration mechanism is selected by this ADR.

---

# 11. Configuration Identity

Configuration may require logical identity when it participates in:

```text
provenance
reproducibility
persistence
comparison
execution records
```

Conceptually:

```text
ConfigurationId
```

may identify an effective configuration.

The exact identity representation follows the Identity Model.

---

# 12. Configuration Version

Configuration version and configuration identity are separate.

For example:

```text
ConfigurationId = C123
ConfigurationVersion = 4
```

may describe the same logical configuration object at a particular revision.

Whether a component configuration is versioned is determined by its contract.

---

# 13. Configuration Serialization

Configuration may need serialization for:

```text
persistence
replay
deployment
reproducibility
CLI/API interaction
```

The serialized representation is not the semantic configuration itself.

Serialization follows the Serialization Model.

---

# 14. Secrets

Configuration may contain sensitive values.

Secrets must not automatically become ordinary configuration data flowing through:

```text
logs
errors
metrics
traces
provenance
configuration dumps
```

Where possible, configuration should reference a secret rather than directly embedding secret material.

The concrete secret-management mechanism is deferred.

---

# 15. Configuration and Environment

The operating environment is not automatically configuration.

For example:

```text
CPU availability
current time
network availability
process ID
host identity
```

are normally execution/environment conditions.

An environment variable may be a **configuration source**, but once resolved it should become explicit semantic configuration.

---

# 16. Execution Context

Execution Context describes runtime conditions that may materially affect processing.

Conceptually:

```text
ExecutionContext
{
    execution_mode
    run_identity?
    time_source?
    randomness_source?
    cancellation?
    resource_context?
    dependency_context?
}
```

The exact representation is deferred.

---

# 17. Context Must Be Bounded

Execution Context must not become:

```text
map<string, any>
```

containing every piece of runtime information.

Only information that is relevant to the component or operation should be exposed.

This keeps contracts explicit and prevents hidden dependencies.

---

# 18. Context Projection

A higher-level context may contain information needed by multiple components.

A component should receive only the subset it requires.

Conceptually:

```text
Application Context
       ↓
Graph Context
       ↓
Processor Context
       ↓
Operation Context
```

Each layer may project a narrower context.

---

# 19. Execution Mode

Execution mode may be part of execution context.

Initial conceptual modes are:

```text
LIVE
REPLAY
BATCH
EXPERIMENT
```

Execution mode is not necessarily configuration because the same effective configuration may be executed under different modes.

---

# 20. Run Identity

A processing execution may have a logical run identity:

```text
RunId
```

Run identity is distinct from:

```text
ProcessorId
GraphId
ConfigurationId
OperationId
EventId
TraceId
```

It identifies the execution/run rather than the component or input.

---

# 21. Time Source

Components whose results depend on current time must receive an explicit time source or clock abstraction.

They must not silently depend on:

```cpp
std::chrono::system_clock::now()
```

when deterministic or reproducible behavior is required.

This permits:

```text
Live Clock
Replay Clock
Fixed Test Clock
Simulated Clock
```

without changing component semantics.

---

# 22. Randomness Source

Components requiring randomness should receive an explicit randomness source when randomness materially affects results.

The component must not silently depend on a global random generator.

This enables:

```text
Production RNG
Deterministic RNG
Seeded RNG
Simulation RNG
Test RNG
```

according to the execution contract.

---

# 23. Random Seed

A seed is not automatically sufficient to reproduce a randomized operation.

Reproducibility may also depend on:

```text
RNG algorithm
RNG version
number/order of draws
partitioning
execution ordering
configuration
```

Therefore the execution context/provenance must capture whatever randomness information is materially required.

---

# 24. Cancellation

Execution context may provide access to cancellation state.

Cancellation is an execution concern.

It must not be silently converted into domain state.

A processor may observe cancellation and return:

```text
Error(Cancelled)
```

according to the Processor Lifecycle and Execution Models.

---

# 25. Resource Context

Resource availability may be exposed through execution context when a component needs to make runtime decisions.

However:

```text
Resource Requirements
    ≠
Resource Availability
```

Configuration may define:

```text
maximum memory
maximum concurrency
resource requirement
```

while execution context may expose:

```text
currently available capacity
```

---

# 26. External Dependency Context

A component may depend on external capabilities.

Relevant runtime information may include:

```text
dependency identity
dependency version
availability
selected implementation
runtime capability
```

Only information that materially affects component behavior should enter execution context.

---

# 27. Security Context

Security information is separate from generic execution context.

Examples:

```text
principal
authorization scope
trust context
security policy
```

may be passed explicitly when required.

Security context must not become an unrestricted metadata container.

---

# 28. Interface Context

An application operation may originate from an external interface.

Relevant information can include:

```text
RequestId
CorrelationId
Principal
Deadline
Cancellation
Interface metadata
```

The interface must not pass arbitrary transport internals into Core merely because they are available.

---

# 29. Deadline vs Time

A deadline is an execution constraint.

It is not domain event time.

For example:

```text
request_deadline = T
```

does not mean:

```text
event_time = T
```

Deadline enforcement should use appropriate monotonic elapsed-time semantics where necessary.

---

# 30. Context and Provenance

Execution context and provenance are related but distinct.

```text
Execution Context
    → conditions under which work executes

Provenance
    → origin and derivation of resulting information
```

A result may record relevant execution context in its provenance.

The context itself is not automatically provenance.

---

# 31. Context and Configuration

Configuration may become part of provenance when it materially affects a result.

Execution context may also become part of provenance when runtime conditions materially affect the result.

The reproducibility model is conceptually:

```text
Inputs
+
Effective Configuration
+
Component/Algorithm Version
+
Relevant Execution Context
+
Required State
→
Result
```

---

# 32. Relevant vs Incidental Context

Not every runtime condition affects a result.

For example:

```text
CPU temperature
host hostname
process ID
```

may be operationally interesting but irrelevant to an analytical result.

Such information should not automatically become part of the semantic reproducibility record.

---

# 33. Configuration Hierarchy

The conceptual configuration hierarchy is:

```text
Application
    ↓
Graph
    ↓
Node / Processor
    ↓
Domain
    ↓
Dependency
```

Each layer owns its own semantic configuration.

A higher layer may compose lower-level configuration but must not silently reinterpret it.

---

# 34. Graph Configuration

Graph configuration controls graph-level behavior.

Examples:

```text
topology options
graph-level resource policy
admission policy
execution mode
graph-wide constraints
```

Graph configuration is distinct from processor configuration.

---

# 35. Processor Configuration

A processor receives only the configuration relevant to its own contract.

It should not depend on an application-wide configuration object containing unrelated settings.

This prevents hidden coupling.

---

# 36. Domain Configuration

Domain-specific behavior belongs to domain configuration.

For example:

```text
poker blinds
poker action rules
market session rules
```

must not be added to Core configuration semantics.

---

# 37. Dependency Configuration

External dependencies may have their own configuration.

Examples:

```text
database connection configuration
network endpoint
storage policy
external service options
```

Dependency configuration belongs to the dependency or its adapter.

It must not become hidden global configuration.

---

# 38. Configuration Inheritance

Configuration inheritance is not implicit.

If a child component inherits a value from a parent, the relationship must be explicitly defined.

For example:

```text
Graph config
    ↓ explicit inheritance
Processor config
```

must have documented precedence and override semantics.

---

# 39. Configuration Conflicts

If multiple sources or configuration layers provide incompatible values, resolution must produce an explicit outcome.

Possible outcomes include:

```text
accepted override
validation error
configuration conflict
```

The component must not silently choose an arbitrary value.

---

# 40. Runtime Configuration Changes

This ADR does not define live mutation of effective configuration.

If runtime reconfiguration is introduced later, it must define:

```text
ownership
atomicity
validation
lifecycle interaction
state compatibility
in-flight operation behavior
provenance
versioning
rollback
failure handling
```

---

# 41. Configuration and State Recovery

A checkpoint is valid only with a compatible configuration.

Recovery must verify that the configuration used to create the checkpoint is compatible with the configuration used to restore it.

An incompatible configuration must not silently reinterpret persisted state.

---

# 42. Configuration and Replay

Replay should normally use the configuration recorded or explicitly selected for the replay.

Changing configuration creates a semantically different execution even when the input events are identical.

Therefore:

```text
same inputs
+
different configuration
```

does not imply:

```text
same result
```

---

# 43. Configuration and Determinism

Configuration is one of the required inputs to deterministic processing.

For a deterministic processor:

```text
Input
+
Effective Configuration
+
Relevant Execution Context
+
Component Version
+
Required State
```

must determine the result.

---

# 44. Configuration Validation Timing

Configuration should be validated as early as practical.

The conceptual lifecycle is:

```text
UNRESOLVED
    ↓
RESOLVED
    ↓
VALIDATED
    ↓
APPLIED
    ↓
ACTIVE
```

Invalid configuration should prevent activation.

---

# 45. Configuration Failure

Configuration errors use the established `Result<T>` / `Error` model.

Examples include:

```text
InvalidConfiguration
Unsupported
InvalidInput
ExternalFailure
```

The exact error code depends on the failure.

---

# 46. Execution Context Lifetime

Execution context may be:

```text
application-scoped
graph-scoped
processor-scoped
operation-scoped
```

according to the information it contains.

A component must not retain a borrowed context beyond its promised lifetime.

If asynchronous work outlives the context, required information must be explicitly owned or otherwise lifetime-safe.

---

# 47. Context Immutability

Execution context should be treated as logically immutable for an individual operation.

Runtime systems may maintain mutable execution state elsewhere.

A processor should not silently mutate shared context in order to communicate with unrelated components.

---

# 48. Context Composition

Contexts may be composed from narrower components:

```text
Application Context
      +
Operation Context
      +
Processor Context
```

but composition must preserve explicit ownership and avoid duplicate conflicting semantics.

---

# 49. Context and Processor Contract

A processor contract must state which execution-context capabilities it requires.

For example:

```text
Processor X requires:
    execution mode
    run identity
    clock
```

while another processor may require:

```text
Processor Y requires:
    cancellation
    resource availability
```

A processor must not silently depend on unrelated context.

---

# 50. Context and Processing Graph

The graph may establish common execution context for its nodes.

However, a graph must not force every node to depend on every context field.

Context projection should keep component dependencies explicit.

---

# 51. Context and Testing

Testing should permit deterministic substitution of relevant context components.

Examples:

```text
Fake clock
Deterministic RNG
Fixed run identity
Controlled cancellation
Simulated resource availability
Recorded dependency context
```

Tests should not depend on the actual host environment when the environment is not semantically relevant.

---

# 52. Initial API Shape

The initial Core API should conceptually expose:

```cpp
namespace evolution::configuration
{

class Configuration;

}

namespace evolution::context
{

class ExecutionContext;

}
```

The exact configuration representation and context composition API remain implementation decisions.

---

# 53. Configuration API Principles

The implementation should favor:

```text
explicit values
strong types
validated construction
logical immutability
value semantics
clear ownership
```

and avoid:

```text
global mutable configuration
implicit environment lookup
unrestricted metadata maps
hidden inheritance
runtime mutation without a contract
```

---

# 54. Execution Context API Principles

The implementation should favor:

```text
explicit dependencies
bounded context
capability-oriented access
clear lifetime
immutable observation
testability
```

and avoid:

```text
global runtime state
unrestricted key/value bags
implicit current time
implicit global RNG
implicit thread-local semantic state
```

---

# 55. Deferred Decisions

This ADR does not select:

* exact configuration value representation
* configuration file format
* configuration parser
* configuration precedence syntax
* configuration serialization
* secret-management mechanism
* exact `ExecutionContext` class layout
* clock interface
* RNG interface
* cancellation-token implementation
* resource-context implementation
* security-context implementation
* dependency-context implementation
* dynamic reconfiguration mechanism
* dependency-injection framework

---

# Decision Summary

```text
Configuration:                 Intended component behavior
Execution Context:             Runtime conditions
State:                         Accumulated execution/history
Provenance:                    Origin and derivation
Resource Availability:         Current execution capacity

Configuration ownership:       Component/domain/dependency
Configuration lifetime:        Logically immutable after application
Configuration validation:      Explicit
Configuration source:          Separate from semantics
Configuration hierarchy:       Application → Graph → Node → Domain → Dependency

Execution mode:                Context
Run identity:                  Context
Current time:                  Explicit dependency
Randomness:                    Explicit dependency
Cancellation:                  Explicit dependency
Resource availability:         Explicit where required
Security context:              Separate concern
Interface context:             Projected explicitly

Global configuration:          Rejected
Universal context map:         Rejected
Implicit current time:        Rejected
Implicit global RNG:           Rejected
Hidden runtime mutation:       Rejected
```

## Invariant

**EVolution separates intended behavior from runtime conditions: configuration defines what a component should do and becomes logically immutable after validation, while execution context explicitly supplies only materially relevant runtime conditions such as execution mode, run identity, time, randomness, cancellation, resources, or dependencies; neither may be replaced by hidden global state or an unrestricted universal context object.**

