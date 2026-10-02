# ADR 0026 — Processing Recovery Strategy

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution distinguishes:

```text
Operation Failure
Processor Failure
Graph Failure
Execution Backend Failure
Storage Failure
```

It also supports:

```text
Retry
Cancellation
Checkpointing
Replay
Recovery
```

An error describes what went wrong, but does not determine what the system should do next.

A processing failure may therefore have several possible responses:

```text
retry
restart
restore
replay
skip
degrade
stop
fail
manual intervention
```

These responses have different semantic consequences.

Recovery strategy must therefore be explicit and must remain separate from the generic `Error` model.

## Decision

EVolution separates:

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

The `Error` identifies the failure.

A recovery policy determines whether and how processing should continue.

No universal recovery action is implied by an error category or error code.

## Recovery Policy

A recovery policy conceptually defines:

```text
RecoveryPolicy
{
    failure_scope
    eligible_errors
    action
    retry_limit?
    retry_delay?
    checkpoint_strategy?
    replay_strategy?
    escalation?
}
```

This is a conceptual model only.

The exact C++ representation is deferred.

## Recovery Scope

Recovery may apply at different scopes:

```text
operation
processor
connection
graph
application
process
```

A failure at one scope must not automatically escalate to every larger scope.

For example:

```text
Processor operation fails
```

does not necessarily imply:

```text
Processor fails
```

and:

```text
Processor fails
```

does not necessarily imply:

```text
Graph fails
```

## Operation-Level Recovery

An individual operation may fail while the processor remains active.

Possible response:

```text
operation failure
    ↓
retry
    ↓
success
```

or:

```text
operation failure
    ↓
report failure
    ↓
continue with next input
```

The processor contract determines whether this is valid.

## Processor-Level Recovery

A processor may become unable to continue normally.

Examples:

```text
persistent internal state failure
required resource unavailable
fatal processor dependency failure
```

Possible recovery:

```text
restart
restore checkpoint
reinitialize
disable processor
fail graph
```

The processor's contract determines which actions are safe.

## Connection-Level Recovery

A graph connection may fail independently.

Examples:

```text
queue corruption
persistent transport failure
storage-backed buffer unavailable
```

Recovery may involve:

```text
reconnect
recreate queue
restore buffered work
pause upstream
fail downstream
```

The graph must preserve the declared delivery semantics.

## Graph-Level Recovery

A graph may become unable to operate consistently.

Examples:

```text
required processor failed
topology invariant violated
graph state cannot be reconstructed
```

Possible responses include:

```text
restart graph
restore graph checkpoint
replay graph inputs
stop graph
require intervention
```

Graph recovery must not silently change topology or processor semantics.

## Application-Level Recovery

An application may decide to:

```text
restart graph
switch configuration
disable optional capability
notify operator
terminate process
```

These are application policies rather than generic Core behavior.

## Recovery Actions

The architecture recognizes several conceptual recovery actions.

### RETRY

Repeat the failed operation.

```text
Failure
  ↓
Retry
  ↓
Operation
```

Retry is appropriate only when the operation contract permits repeated execution.

It can produce duplicate effects.

### RESTART

Terminate and recreate a processor or execution component.

```text
Failed Instance
      ↓
Stop / Destroy
      ↓
Create New Instance
      ↓
Initialize
      ↓
Activate
```

Restart does not automatically restore prior state.

### RESTORE

Load a compatible checkpoint.

```text
Checkpoint
    ↓
Restore
    ↓
Processor State
```

Restore alone does not necessarily resume processing.

### REPLAY

Reprocess retained historical inputs.

```text
Historical Input
    ↓
Processor
    ↓
Reconstructed State
```

Replay may occur from:

```text
beginning
checkpoint position
specific offset
specific event
```

according to the processor contract.

### SKIP

Discard a failed input and continue.

This is a potentially lossy recovery action.

It must never occur implicitly when the processing contract requires lossless processing.

### DEGRADE

Continue processing with reduced functionality or resolution.

Examples:

```text
reduced analytical resolution
optional processor disabled
less expensive processing mode
```

Degradation must be explicitly supported.

### PAUSE

Temporarily stop admitting or processing work while preserving recoverable state.

```text
ACTIVE
  ↓
PAUSED
```

A mandatory public `PAUSED` lifecycle state is not introduced by this ADR.

Pause may instead be implemented as an application/graph execution policy.

### STOP

Terminate processing orderly.

This follows the existing lifecycle semantics:

```text
ACTIVE
  ↓
STOPPING
  ↓
STOPPED
```

### FAIL

Declare that processing cannot continue under the current execution context.

```text
ACTIVE
  ↓
FAILED
```

Failure is not itself a recovery action, but may be the terminal result of recovery attempts.

### MANUAL INTERVENTION

Require an operator/application to decide what happens next.

This is appropriate when automatic recovery could risk data corruption or semantic inconsistency.

## Recovery Action vs Error

The following distinction is mandatory:

```text
Error:
    What went wrong?

Recovery Policy:
    What should be attempted?

Recovery Result:
    What happened while attempting recovery?
```

For example:

```text
Error(StorageUnavailable)
```

does not mean:

```text
retry forever
```

nor:

```text
fail immediately
```

The recovery policy determines the response.

## Retry Limits

Automatic retries must be bounded unless the component explicitly defines another safe mechanism.

Conceptually:

```text
retry_count
maximum_attempts
```

An unbounded retry loop can prevent:

```text
shutdown
failure propagation
resource release
operator intervention
```

from occurring.

## Retry Delay

A recovery policy may define:

```text
immediate
fixed delay
exponential backoff
external scheduling
```

The exact algorithm is not selected.

Retry delay may depend on:

```text
error
resource availability
attempt count
external system state
```

## Retry and Idempotency

Before retrying an operation, the recovery policy must account for the operation's duplicate-execution semantics.

For example:

```text
side effect succeeded
acknowledgement lost
retry
```

may produce a duplicate side effect.

Therefore retry safety must be part of the processor/operation contract.

## Retryable vs Recoverable

These concepts are distinct.

```text
Retryable
    = repeating the same operation may succeed.

Recoverable
    = the larger component can continue or resume after the failure.
```

A failure may be:

```text
retryable but processor-fatal
```

or:

```text
non-retryable but processor-recoverable
```

depending on the recovery mechanism.

## Restart vs Retry

Retry repeats an operation.

Restart recreates the processing component.

Therefore:

```text
retry:
    same processor instance

restart:
    new processor instance
```

A restart may reset transient state while preserving recoverable persistent state.

## Restore vs Restart

Restore answers:

> Which state should the new processor instance start with?

Restart answers:

> Should a new processor instance be created?

They are independent decisions.

A recovery strategy may therefore be:

```text
restart
    +
restore checkpoint
    +
replay remaining input
```

## Recovery Composition

Recovery actions may be composed.

Example:

```text
Failure
  ↓
Retry × 3
  ↓
Restart
  ↓
Restore checkpoint
  ↓
Replay
  ↓
Activate
```

The complete sequence must be explicitly defined.

Recovery must not recursively trigger unlimited recovery actions without bounded policy.

## Escalation

Recovery may escalate:

```text
operation retry
      ↓
processor restart
      ↓
checkpoint restore
      ↓
graph recovery
      ↓
application intervention
```

The exact escalation chain is component-specific.

An escalation must preserve semantic guarantees.

## Recovery and Lifecycle

Recovery interacts with the processor lifecycle.

A failed processor cannot accept normal processing work.

A conceptual recovery sequence may be:

```text
FAILED
  ↓
Recovery Preparation
  ↓
Initialize / Restore
  ↓
ACTIVE
```

The existing public lifecycle state model does not require a new `RECOVERING` state.

Recovery may instead be an operation performed before a processor is activated again.

## Recovery from ACTIVE Failure

A processor may fail due to an unrecoverable operation or internal condition:

```text
ACTIVE
  ↓
FAILED
```

If the application chooses restart:

```text
FAILED
  ↓
new instance
  ↓
CONFIGURED
  ↓
INITIALIZED
  ↓
ACTIVE
```

The new instance must not silently inherit old state unless explicitly restored.

## Recovery from STOPPED

A stopped processor may be started again according to its lifecycle contract.

If state must persist across the stop/start boundary, it must be explicitly restored.

A normal stop does not automatically imply checkpoint creation unless the processor contract requires it.

## Recovery from Cancellation

Cancellation is not automatically a failure.

If a processor is cancelled:

```text
ACTIVE
  ↓
STOPPING
  ↓
STOPPED
```

a later start may simply initialize normally.

If continuation requires previous state, the processor must explicitly persist or reconstruct that state.

## Recovery from Abort

Abort provides no completion guarantee.

Therefore after:

```text
ACTIVE
  ↓
FAILED
```

the recovery strategy must assume that:

```text
state
output
buffer
checkpoint
external effect
```

may be incomplete unless independently known to be durable.

## Recovery and Partial Work

Recovery must account for work that was:

```text
submitted
admitted
queued
running
completed
acknowledged
checkpointed
```

at the time of failure.

These states must not be collapsed into a single "processed" state.

## Recovery and Queue State

If a persistent queue survives processor failure, recovery must determine whether queued items are:

```text
pending
in-flight
completed
cancelled
unknown
```

An `unknown` disposition may require replay or duplicate handling.

Queue recovery must preserve its declared delivery semantics.

## Recovery and Unknown Completion

The hardest case is:

```text
operation executes
    ↓
process fails
    ↓
completion status unknown
```

The system cannot assume either:

```text
did not execute
```

or:

```text
executed successfully
```

unless the contract provides evidence.

Recovery may therefore produce duplicate execution.

Exactly-once handling requires an explicit durable commit/acknowledgement mechanism.

## Recovery and State Consistency

A recovered processor must not combine incompatible pieces of state.

For example:

```text
checkpointed state = N
input position = N+1
```

is invalid if the state does not include N+1.

Recovery must validate state/position consistency.

## Recovery Validation

Before resuming processing, recovery should validate:

```text
checkpoint compatibility
configuration compatibility
processor version
graph compatibility
input availability
state integrity
required resources
```

Failure of these checks prevents activation.

## Recovery and Configuration Changes

Changing configuration between failure and recovery is allowed only if the processor contract permits it.

If configuration materially changes state semantics:

```text
old checkpoint
    +
new incompatible configuration
```

must not be silently accepted.

Possible responses:

```text
reject recovery
migrate state
discard state and replay
start fresh
```

## Recovery and Version Changes

A new processor implementation may be incompatible with an old checkpoint.

Recovery must explicitly determine:

```text
compatible
migratable
incompatible
```

No automatic assumption of compatibility is permitted.

## Recovery and Graph Changes

A checkpoint created under:

```text
Graph Version A
```

must not automatically be restored into:

```text
Graph Version B
```

if topology or processor contracts changed materially.

Graph migration/recomputation must be explicit.

## Recovery and Provenance

Recovery actions should preserve provenance describing:

```text
failure
recovery policy
recovery action
checkpoint
replay range
processor version
configuration
execution run
```

This allows later analysis to distinguish:

```text
original result
```

from:

```text
recovered result
```

## Recovery and Analytical Correctness

Recovery policy can affect analytical results.

For example:

```text
retry
skip
drop
degrade
replay
```

may produce different outputs.

Therefore any recovery behavior that materially changes results must be represented in relevant execution context/provenance.

## Automatic vs Manual Recovery

Recovery may be:

```text
AUTOMATIC
MANUAL
HYBRID
```

Automatic recovery is suitable where the component contract provides safe, deterministic recovery.

Manual recovery is appropriate when automatic action could cause irreversible semantic effects.

Hybrid recovery may automatically retry and then require intervention.

## Recovery Policy Ownership

Recovery policy belongs to the layer that has sufficient semantic knowledge.

Examples:

```text
Processor:
    operation-specific recovery

Graph:
    topology/dependency recovery

Application:
    workflow/business recovery

Infrastructure:
    process/machine recovery
```

Core provides generic mechanisms but does not decide application-specific recovery policy.

## Recovery Does Not Redefine Errors

Recovery may inspect:

```text
Error.code
Error.category
recoverability
context
```

but must not modify the original error merely to record the recovery decision.

Recovery results are separate information.

## Recovery Result

A recovery operation may conceptually return:

```text
RecoveryResult
{
    outcome
    attempts
    restored_from?
    replayed_range?
    resulting_state?
    error?
}
```

Possible outcomes:

```text
RECOVERED
NOT_RECOVERABLE
CANCELLED
REQUIRES_INTERVENTION
```

The exact type is deferred.

## Recovery and Observability

Recovery systems should expose operational information such as:

```text
recovery attempts
restarts
checkpoint restores
replay duration
replayed input count
failed recovery attempts
```

These are observability concerns unless explicitly modeled as analytical data.

## Recovery and Resource Limits

Recovery itself consumes resources.

Examples:

```text
memory
CPU
storage I/O
network
queue capacity
```

A recovery policy must not create uncontrolled resource consumption.

For example:

```text
automatic replay
    ↓
recovery consumes all capacity
    ↓
normal processing cannot resume
```

Recovery may therefore need explicit resource limits.

## Recovery Storms

Multiple components may fail simultaneously and independently trigger recovery.

For example:

```text
A fails → restart
B fails → restart
C fails → replay
```

The execution system must avoid an uncontrolled recovery storm.

Global coordination may eventually be required, but the generic architecture does not prescribe an implementation.

## Recovery and Backpressure

Recovery-generated replay can produce a large amount of work.

Therefore replay must participate in the same admission/backpressure model as ordinary processing unless the execution contract explicitly defines otherwise.

Recovery must not bypass capacity controls simply because the work originated from historical replay.

## Recovery and Replay Mode

Replay may run under:

```text
LIVE
REPLAY
BATCH
EXPERIMENT
```

execution modes.

Recovery must preserve the mode semantics required by the processor.

A historical replay must not accidentally use live external effects merely because the processor is being recovered.

## Consequences

### Positive

* Error handling remains separate from recovery policy.
* Recovery behavior becomes explicit and testable.
* Retry, restart, restore, and replay are no longer conflated.
* Stateful recovery can be reasoned about consistently.
* Application-specific policy remains outside Core.
* Recovery-induced analytical differences can be tracked through provenance.

### Negative

* Recovery policies add architectural complexity.
* Strong recovery guarantees may require persistent state and transactional boundaries.
* Automatic recovery can interact badly with side effects.
* Large replay operations can consume substantial resources.

## Deferred Decisions

This ADR does not select:

```text id="k9j3m7"
automatic recovery framework
retry algorithm
supervisor implementation
checkpoint storage
transaction system
distributed recovery protocol
process supervisor
service manager
recovery UI
operator control protocol
```

## Decision Summary

```text
Failure:
    Represented by Error

Recovery:
    Separate policy

Recovery scope:
    Operation / Processor / Connection / Graph / Application

Recovery actions:
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

Retry:
    May duplicate execution

Restart:
    Creates a new processor instance

Restore:
    Reconstructs from checkpoint

Replay:
    Reprocesses retained input

Recovery:
    Must respect delivery, lifecycle, ordering,
    backpressure, configuration, and provenance contracts

Automatic recovery:
    Never implicitly assumed

Recovery policy:
    Owned by the layer with sufficient semantic knowledge
```

## Invariants

1. An error describes a failure; it does not prescribe recovery.
2. Recovery policy must be explicit.
3. Recovery scope must be explicit.
4. Operation failure must not automatically become processor failure.
5. Processor failure must not automatically become graph failure.
6. Retry must account for duplicate execution.
7. Restart does not imply state restoration.
8. Restore does not imply processor restart.
9. Replay does not imply exactly-once execution.
10. Skip is explicitly lossy and must never occur implicitly.
11. Degradation must be explicitly supported by the affected processor/graph.
12. Recovery must respect lifecycle semantics.
13. Recovery must validate checkpoint, configuration, processor-version, and graph compatibility where relevant.
14. Recovery must not combine inconsistent state and input positions.
15. Recovery must account for work whose completion status is unknown.
16. Recovery must preserve declared delivery semantics.
17. Recovery-generated work must respect applicable backpressure and resource limits.
18. Recovery must preserve material provenance.
19. Recovery actions must not mutate the original `Error`.
20. Recovery policy belongs to the layer with sufficient semantic knowledge to make the decision.

## Invariant

> **EVolution separates failure from recovery: errors describe unsuccessful conditions, while explicit recovery policies determine whether processing is retried, restarted, restored, replayed, degraded, stopped, or requires intervention; no recovery action is implied solely by the existence or classification of an error.**
