# ADR 0027 — Processing Supervision Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processors have explicit lifecycle states:

```text
CREATED
CONFIGURED
INITIALIZED
ACTIVE
STOPPING
STOPPED
FAILED
```

Processing failures, processor failures, graph failures, and recovery actions are distinct concerns.

Previous decisions establish that:

* processors report operation failures through `Result<T>`
* unrecoverable processor conditions transition the processor to `FAILED`
* recovery policy determines whether retry, restart, restore, replay, or another action is appropriate
* processors do not control the entire processing graph
* applications may define higher-level recovery policy
* lifecycle operations are explicit
* execution backends execute work but do not own semantic recovery policy

A system therefore needs a component that can observe processing components and coordinate lifecycle/recovery without embedding supervision logic inside every processor.

## Decision

EVolution introduces a conceptual **Supervisor** responsibility.

A Supervisor observes one or more processing components and coordinates their lifecycle and recovery according to an explicit supervision policy.

Conceptually:

```text
Processor
    ↓
Status / Error / Lifecycle Result
    ↓
Supervisor
    ↓
Recovery Policy
    ↓
Lifecycle / Recovery Action
```

The Supervisor does **not** own processor semantics.

It coordinates the processor according to contracts defined elsewhere.

## Supervisor Responsibilities

A Supervisor may:

* observe processor lifecycle state
* observe operation failures
* detect processor failure
* coordinate recovery
* enforce recovery limits
* escalate failures
* coordinate graph-level recovery
* coordinate orderly shutdown
* expose supervision state
* record material recovery decisions

A Supervisor does not:

* define domain semantics
* modify processor outputs
* reinterpret analytical results
* silently change processor configuration
* bypass processor lifecycle contracts
* implement processor-specific algorithms
* become the processor's hidden global state

## Supervision Scope

Supervision may exist at several levels:

```text
Operation
    ↓
Processor
    ↓
Connection
    ↓
Graph
    ↓
Application
```

A supervisor may supervise one or more objects at a particular scope.

The architecture does not require one universal supervisor for the entire system.

## Processor Supervision

A processor supervisor observes:

```text
processor lifecycle
operation failures
resource conditions
recovery attempts
recovery results
```

For example:

```text
ACTIVE
  ↓
processor failure
  ↓
Supervisor
  ↓
Recovery Policy
  ↓
Restart + Restore
  ↓
ACTIVE
```

The processor itself remains responsible for implementing its processing semantics.

## Graph Supervision

A graph supervisor coordinates multiple processors and connections.

For example:

```text
Graph
├── Processor A
├── Processor B
└── Processor C
```

If B fails:

```text
B → FAILED
     ↓
Graph Supervisor
     ↓
evaluate graph policy
```

The supervisor may:

```text
restart B
restore B
stop dependent nodes
pause graph
fail graph
```

depending on the graph contract.

## Supervisor Does Not Automatically Escalate

A local failure must not automatically propagate to the highest scope.

For example:

```text
operation failure
```

does not imply:

```text
processor failure
```

and:

```text
processor failure
```

does not imply:

```text
graph failure
```

Supervision policy determines escalation.

## Supervision Policy

A conceptual supervision policy is:

```text
SupervisionPolicy
{
    scope
    observed_conditions
    recovery_actions
    retry_limits
    escalation_rules
    shutdown_behavior
}
```

The exact representation is deferred.

Policies must be explicit rather than hidden in supervisor implementation.

## Failure Classification

Supervision may inspect:

```text
Error.category
Error.code
recoverability
processor state
graph state
resource state
```

It must not reinterpret domain meaning merely to make a recovery decision.

For example:

```text
Error(StorageUnavailable)
```

may trigger a storage recovery policy without becoming a domain-level event.

## Recovery Coordination

A supervisor may coordinate a recovery sequence:

```text
Failure
  ↓
Classify
  ↓
Select Policy
  ↓
Stop / Cancel if required
  ↓
Restore / Restart
  ↓
Validate
  ↓
Activate
  ↓
Observe
```

Each operation remains subject to the processor lifecycle contract.

## Supervisor and Lifecycle API

Supervision uses the explicit lifecycle operations:

```text
configure
initialize
activate
stop
cancel
abort
```

It does not directly mutate lifecycle state.

For example, the supervisor requests:

```text
processor.stop()
```

rather than modifying:

```text
processor.state = STOPPED
```

## Supervisor and Recovery

Recovery may require multiple lifecycle operations.

For example:

```text
FAILED
   ↓
create replacement
   ↓
configure
   ↓
initialize
   ↓
restore checkpoint
   ↓
activate
```

The exact sequence depends on the recovery contract.

The supervisor coordinates the sequence but does not implement state restoration semantics.

## Recovery Attempt Limits

A supervisor should support bounded recovery attempts.

Conceptually:

```text
attempt 1
attempt 2
attempt 3
    ↓
escalate
```

Unbounded automatic recovery is not the default.

An exhaustion condition may produce:

```text
REQUIRES_INTERVENTION
```

or another explicitly defined terminal policy.

## Recovery Storm Prevention

A supervisor must avoid uncontrolled recovery loops.

For example:

```text
restart
  ↓
failure
  ↓
restart
  ↓
failure
  ↓
restart
  ...
```

Possible controls include:

```text
attempt limits
backoff
cooldown
circuit breaking
dependency coordination
manual intervention
```

The architecture does not select a specific mechanism.

## Recovery Cooldown

A supervisor may temporarily delay repeated recovery attempts.

Conceptually:

```text
FAILED
  ↓
RECOVERY
  ↓
FAILED
  ↓
COOLDOWN
  ↓
RECOVERY
```

Cooldown is a supervision policy, not a processor lifecycle state.

## Dependency-Aware Supervision

Processing graphs contain dependencies.

For:

```text
A → B → C
```

if A fails, B may become unable to operate even though B itself has no internal failure.

The supervisor should distinguish:

```text
B's own failure
```

from:

```text
B cannot operate because A is unavailable
```

This prevents incorrect failure attribution.

## Dependent Processor Handling

When an upstream processor fails, downstream processors may:

```text
continue
pause
drain
cancel
fail
```

depending on their contracts.

The supervisor must not assume that all downstream processors should immediately fail.

## Optional Dependencies

A graph may contain optional capabilities.

For example:

```text
Core Analysis
      |
      +---- Optional Pattern Detector
```

Failure of the optional component may allow the graph to continue in degraded mode.

Whether this is permitted must be explicit in graph configuration/contracts.

## Required Dependencies

If a processor is required for graph correctness:

```text
A → B
```

and A cannot operate, B may be unable to continue correctly.

The graph contract can therefore declare dependency criticality.

Conceptually:

```text
Dependency
{
    source
    destination
    required
}
```

The exact representation is deferred.

## Supervision and Degradation

A supervisor may coordinate explicit degraded operation.

For example:

```text
FULL
  ↓
optional processor unavailable
  ↓
DEGRADED
```

However, degradation must not silently alter analytical meaning.

If outputs are materially different, the degraded condition must be represented in relevant context/provenance.

## Supervisor and Backpressure

A failed or paused processor can stop consuming input.

This can cause upstream backpressure:

```text
A → B
    ↓
  stopped
    ↓
A reaches capacity
```

The supervisor should coordinate this situation according to graph and connection policies.

It must not bypass queue capacity by silently creating unbounded buffers.

## Supervisor and Queues

The supervisor may coordinate queue actions such as:

```text
drain
pause
resume
recover
discard
rebuild
```

only through explicit queue/connection contracts.

It does not directly manipulate queue internals.

## Supervisor and Persistent Queues

Persistent queues can survive processor failure.

A supervisor may therefore:

```text
restore processor
    ↓
resume queue consumption
```

while preserving queued work.

Delivery semantics remain governed by the connection contract.

## Supervisor and Unknown Work

If processor failure leaves work in an unknown state:

```text
running
  ↓
process crash
  ↓
completion unknown
```

the supervisor must follow the declared delivery/recovery contract.

It must not assume success or failure without evidence.

## Supervisor and Exactly-Once

Supervision does not provide exactly-once semantics.

Restarting a processor may result in:

```text
duplicate execution
```

unless the processor/connection/storage contracts explicitly prevent it.

## Supervisor and Cancellation

A supervisor may request cancellation when recovery requires stopping active work.

For example:

```text
failure
  ↓
cancel admitted work
  ↓
restore
```

Cancellation remains an intentional termination request, not an operation failure.

## Supervisor and Abort

Abort is reserved for cases where orderly termination is no longer appropriate.

For example:

```text
processor
   ↓
unsafe/unrecoverable condition
   ↓
abort
   ↓
FAILED
```

A supervisor must not use abort merely as a faster version of stop.

## Supervisor and Shutdown

Application shutdown may require:

```text
Supervisor
    ↓
stop graph
    ↓
stop processors
    ↓
drain required work
    ↓
checkpoint
    ↓
release resources
```

The supervisor coordinates this process but the lifecycle contracts determine what each processor actually does.

## Supervisor and Application

Application-level supervision may decide:

```text
restart application
stop application
switch graph
require operator intervention
```

These decisions are outside the generic processor supervisor.

The architecture therefore distinguishes:

```text
Processor Supervision
Graph Supervision
Application Supervision
```

## Supervisor and Configuration

A supervisor must not silently modify processor configuration during recovery.

If a recovery policy intentionally selects another configuration:

```text
old configuration
    ↓
explicit recovery policy
    ↓
new configuration
```

the new effective configuration must participate in provenance and reproducibility.

## Supervisor and Version Changes

Likewise, replacing a processor implementation with another version is a material change.

Recovery must validate:

```text
checkpoint compatibility
configuration compatibility
processor version
graph compatibility
```

before activation.

## Supervisor State

A supervisor may maintain execution state such as:

```text
failure count
recovery attempt count
current recovery phase
last failure
cooldown state
```

This is supervision state, not processor state.

It should not be silently exposed as domain state.

## Supervisor State Persistence

Supervision state may be persisted when required for recovery across process restarts.

For example:

```text
attempt_count = 3
```

may need persistence to prevent a process restart from accidentally resetting:

```text
maximum_attempts = 3
```

and creating an infinite recovery loop.

Whether this is required is a supervision policy decision.

## Supervisor Identity

A supervisor may have logical identity where it participates in persistent execution/provenance.

Its identity is distinct from:

```text
processor identity
graph identity
execution identity
run identity
```

A supervisor restart does not necessarily represent a new processing graph.

## Supervision Events

The supervisor may internally observe lifecycle/failure information.

However, supervision activity does not automatically become a domain `Event`.

For example:

```text
Processor restarted
```

is normally execution information.

It becomes a domain event only if the domain explicitly defines such an event.

## Provenance

Material supervision actions may participate in provenance:

```text
failure
recovery policy
recovery attempt
restart
checkpoint
replay
result
```

This allows derived results to be interpreted in the context in which they were produced.

## Observability

Supervision should expose operational information such as:

```text
processor state
failure count
recovery count
restart count
recovery duration
current recovery action
```

These are observability concerns unless explicitly promoted into analytical Measurements.

## Supervisor Failure

The supervisor itself may fail.

The architecture therefore cannot assume that supervision is an infallible global mechanism.

A higher-level application/process supervisor may eventually be responsible for supervising the supervisor.

This creates a hierarchy:

```text
Application Supervisor
        ↓
Graph Supervisor
        ↓
Processor Supervisor
        ↓
Processor
```

A deployment may omit any level that is unnecessary.

## No Mandatory Supervisor Hierarchy

EVolution does not require all systems to instantiate every supervision level.

A simple application may use:

```text
Application
    ↓
Processor
```

while a larger deployment may use:

```text
Application
    ↓
Graph Supervisor
    ↓
Processor Supervisors
    ↓
Processors
```

The conceptual responsibilities remain separate even when implemented by one component.

## Supervision and Execution Backend

The execution backend reports execution outcomes.

The supervisor may observe:

```text
execution failure
execution cancellation
backend failure
```

but the backend does not become the recovery policy owner.

The boundary remains:

```text
Scheduler
    ↓
Execution Backend
    ↓
Processor
    ↓
Result
    ↓
Supervisor / Graph
```

## Supervision and Scheduler

A failed processor must not continue receiving normal scheduled work.

The supervisor and scheduler must coordinate so that:

```text
FAILED processor
```

is not treated as:

```text
ELIGIBLE processor
```

Recovery must explicitly restore eligibility.

## Supervision and Graph Topology

A supervisor must not silently modify graph topology as a recovery shortcut.

For example:

```text
A → B → C
```

must not silently become:

```text
A → C
```

because B failed.

Such a topology change is a graph configuration change and requires explicit policy.

## Supervision and Dynamic Reconfiguration

Dynamic topology/configuration changes remain deferred.

A supervisor may eventually coordinate them, but this ADR does not introduce a generic dynamic reconfiguration mechanism.

## Supervision and Manual Intervention

A supervisor may transition into a state requiring external intervention when:

```text
automatic recovery exhausted
checkpoint incompatible
state corruption detected
semantic ambiguity exists
external dependency unavailable
```

The exact operator interface is outside this ADR.

## Consequences

### Positive

* Failure observation and recovery coordination have a clear architectural owner.
* Processors remain focused on processing semantics.
* Recovery policy remains explicit.
* Local failures can remain local when safe.
* Graph dependencies can be handled without making every processor graph-aware.
* Recovery loops can be bounded and observable.

### Negative

* Supervision introduces another architectural responsibility.
* Multiple supervision scopes can increase coordination complexity.
* Recovery policies can become complex for large graphs.
* The supervisor itself requires failure handling.

## Deferred Decisions

This ADR does not select:

```text
supervisor implementation
supervisor thread/executor
watchdog mechanism
process supervisor
systemd integration
distributed supervisor
health-check protocol
circuit-breaker implementation
operator interface
automatic graph reconfiguration
```

## Decision Summary

```text
Supervision:
    Explicit responsibility

Supervisor:
    Observes failures and coordinates lifecycle/recovery

Processor:
    Owns processing semantics

Graph:
    Owns topology/dependency semantics

Application:
    Owns application-level recovery policy

Recovery:
    Explicit policy

Escalation:
    Bounded and policy-driven

Retries:
    Must be bounded unless explicitly justified

Dependency failure:
    Does not automatically imply dependent failure

Exactly-once:
    Not provided by supervision

Topology changes:
    Not implicit recovery actions

Observability:
    Separate concern

Provenance:
    Material supervision actions may be recorded
```

## Invariants

1. Supervision observes and coordinates; it does not define processor semantics.
2. A processor must not silently implement its own hidden global supervision policy.
3. Operation failure does not automatically imply processor failure.
4. Processor failure does not automatically imply graph failure.
5. Recovery escalation must be explicit.
6. Automatic recovery must be bounded unless an explicit contract defines otherwise.
7. Recovery loops must not silently continue indefinitely.
8. A failed processor must not receive normal new work.
9. Recovery must explicitly restore processor eligibility.
10. Supervision must respect processor lifecycle contracts.
11. Supervision must respect delivery and recovery semantics.
12. Supervision must not silently modify processor configuration.
13. Supervision must not silently modify graph topology.
14. Dependency failure must be distinguished from internal processor failure.
15. Degraded operation must be explicitly supported.
16. Supervisor state is distinct from processor state and domain state.
17. Supervision actions do not automatically become domain events.
18. Material supervision actions should remain available through provenance/observability where required.
19. The supervisor itself is not assumed to be failure-proof.
20. A higher-level application or infrastructure component may supervise the supervisor.

## Invariant

> **EVolution separates supervision from processing: processors own processing semantics, while supervisors observe failures and coordinate lifecycle and recovery according to explicit policy without silently changing processor meaning, configuration, topology, or delivery guarantees.**
