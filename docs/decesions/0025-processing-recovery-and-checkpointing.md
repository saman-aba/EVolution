# ADR 0025 — Processing Recovery and Checkpointing

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processing may involve:

```text
Events
  ↓
Processing Graph
  ↓
Stateful Processors
  ↓
Derived Outputs
```

Execution may be interrupted by:

* process termination
* machine failure
* processor failure
* backend failure
* storage failure
* explicit cancellation
* deployment restart
* resource exhaustion

Previous decisions distinguish:

```text
processor state
execution state
queue state
graph state
persistent data
```

They also establish that:

* queues may be persistent
* processing may be at-least-once
* duplicate execution may occur
* exactly-once is not assumed
* processor state may affect results
* snapshots/checkpoints are storage mechanisms, not automatically authoritative historical truth

The architecture therefore needs an explicit model for recovering processing after interruption.

## Decision

EVolution supports **explicit recovery and checkpointing** for processors and processing graphs.

Checkpointing is the process of persisting sufficient execution state to allow processing to resume from a defined recovery point.

Recovery reconstructs executable processing state from:

```text
checkpoint
+
retained input
+
graph definition
+
effective configuration
+
processor version
+
relevant execution context
```

Conceptually:

```text id="0n3f4a"
Checkpoint
    +
Historical Inputs
    +
Graph Definition
    +
Configuration
    +
Processor Version
    ↓
Recovery
    ↓
Resumed Processing
```

Checkpointing is optional for processors that can safely reconstruct their state from retained inputs.

## Recovery Point

A checkpoint identifies a logical point in processing history.

Conceptually:

```text id="k3r0dx"
Recovery Point
{
    graph
    processor
    input position?
    state
    configuration identity
    processor version
    execution context
}
```

The exact representation is deferred.

A recovery point must be meaningful relative to the processor's input and state semantics.

## Checkpoint vs Snapshot

The architecture distinguishes:

```text
Snapshot
```

from:

```text
Checkpoint
```

A snapshot is a persisted representation of state.

A checkpoint is a recovery boundary that establishes:

> This state can be used as a starting point for resuming processing under the associated execution contract.

A snapshot becomes a checkpoint only when its relationship to processing progress is explicitly defined.

## Checkpoint Scope

Checkpoints may exist at different scopes:

```text id="4w2b1k"
processor
partition
graph
execution
batch
```

The scope must be explicit.

A graph checkpoint does not automatically mean that every processor has reached the same logical input position.

## Processor Checkpoint

A stateful processor may checkpoint:

```text id="s2f7be"
processor state
input position
relevant buffered data
configuration identity
processor version
```

For example:

```text id="v6m5i3"
Processor State
    +
last processed sequence
    +
buffered inputs
```

may be required for correct recovery.

## Stateless Processor Recovery

A stateless processor may require no persistent processor state.

Its state can be reconstructed by replaying retained inputs:

```text id="h6d4f1"
Inputs
   ↓
Processor
   ↓
State/output
```

In such cases, checkpointing may be unnecessary.

This is an optimization decision rather than a requirement.

## Stateful Processor Recovery

For:

```text id="2m4q8n"
Stateₙ₊₁ = F(Stateₙ, Inputₙ)
```

recovery requires either:

```text id="p9f9b7"
checkpointed state
```

or:

```text id="2i3w3j"
replay from a known earlier point
```

or a combination.

## Replay-Based Recovery

A processor may recover by replaying historical input:

```text id="m0k0o7"
Checkpoint at N
      ↓
Input N+1
Input N+2
Input N+3
...
      ↓
Recovered State
```

This can reduce checkpoint frequency at the cost of replay work.

## Checkpoint-Based Recovery

A processor may instead restore a checkpoint:

```text id="q6j0o9"
Checkpoint at N
      ↓
Restore State
      ↓
Process N+1 onward
```

This reduces replay cost but requires correct state persistence.

## Hybrid Recovery

A common strategy is:

```text id="0e7h6y"
Checkpoint at N
      ↓
Restore state
      ↓
Replay N+1 ... M
      ↓
Current state
```

The architecture supports this model.

## Input Position

Recovery requires knowing what input has been incorporated into state.

The position may be represented by:

```text id="zq9p7w"
sequence
event identity
offset
timestamp
partition position
checkpoint token
```

The representation depends on the input source.

A timestamp alone is generally insufficient because multiple inputs may share the same timestamp.

## Input Position vs Event Identity

If events have logical identity:

```text id="9n1xg0"
EventId
```

that identity may help identify processed input.

However, event identity does not necessarily define stream position.

A recovery contract may require both:

```text id="5zv3on"
logical identity
+
stream position
```

## Recovery and Ordering

Recovery must preserve the processor's declared ordering contract.

For a sequence-sensitive processor:

```text id="5k1t6j"
E1 → E2 → E3
```

recovery must not silently produce:

```text id="r2xqf9"
E2 → E1 → E3
```

unless the processor explicitly permits that ordering.

## Recovery and Duplicates

Recovery can produce duplicate execution.

Example:

```text id="v0q1sk"
Process E10
   ↓
state updated
   ↓
checkpoint not persisted
   ↓
crash
   ↓
restore previous checkpoint
   ↓
Process E10 again
```

Therefore recovery semantics must remain compatible with ADR 0024.

Possible solutions include:

```text id="l0kn6k"
idempotent processing
deduplication
transactional state
replay-safe processor semantics
```

No universal solution is selected.

## Recovery and Outputs

A processor may have emitted outputs before failure.

Recovery must define whether those outputs are:

```text id="0z7y5r"
retained
replayed
deduplicated
retracted
replaced
ignored
```

This is especially important when outputs have external side effects.

Output recovery is therefore part of the processor's recovery contract.

## Output Commit Boundary

A processor may conceptually have:

```text id="q1c6q4"
Input
  ↓
Process
  ↓
State Update
  ↓
Output
  ↓
Commit
```

Recovery semantics depend on where the durable boundary exists.

If state is persisted but output is not, or vice versa, recovery may produce inconsistency.

A stronger recovery contract must define the relationship between:

```text id="z8s0pm"
state commit
output commit
input acknowledgement
```

## Checkpoint Atomicity

A checkpoint may contain multiple pieces of state:

```text id="6t3s0n"
processor state
input position
buffered data
```

These must correspond to a consistent recovery point.

A checkpoint must not claim:

```text id="5hm1a0"
input position = N
```

while storing state that only includes inputs through N-1.

The exact transactional mechanism is deferred.

## Checkpoint Version

Checkpoints must identify the processor definition under which they were produced.

At minimum, material identity may include:

```text id="8x9l4v"
processor version
configuration identity
graph version
```

A newer implementation must not automatically assume that an old checkpoint is compatible.

## Checkpoint Compatibility

A processor version may declare whether it can restore an older checkpoint.

Conceptually:

```text id="x2b9c8"
Checkpoint Version
       ↓
Compatibility Check
       ↓
RESTORE
```

or:

```text id="f1w0f7"
INCOMPATIBLE
```

An incompatible checkpoint must not be silently loaded as valid state.

## Configuration Compatibility

A checkpoint created with:

```text id="y6e8ar"
configuration A
```

must not silently be restored under:

```text id="n8b0p5"
configuration B
```

if the configuration materially changes state semantics.

Recovery may instead require:

```text id="5m3k4c"
migration
recomputation
replay
```

## Checkpoint Migration

Future processor versions may provide explicit checkpoint migration:

```text id="5y5h1x"
Checkpoint v1
    ↓
Migration
    ↓
Checkpoint v2
```

Migration must be explicit and versioned.

Automatic best-effort interpretation of incompatible checkpoint state is prohibited.

## Checkpoint Provenance

A checkpoint should identify:

```text id="qz1z9v"
processor
graph
processor version
configuration
input position
creation time
execution run
```

where relevant.

Checkpoint creation time is not necessarily the same as:

```text id="q2y8m7"
event time
measurement time
processing time
```

Temporal semantics follow ADR 0011.

## Checkpoint Time

Checkpoint metadata may contain processing time:

```text id="3r7tq2"
checkpoint_created_at
```

This describes when the checkpoint was created.

It must not be interpreted as the event time of the state represented by the checkpoint.

## Checkpoint Frequency

Checkpoint frequency is an execution policy.

Possible strategies include:

```text id="1x8b9c"
every N inputs
every T duration
after significant state change
explicit request
adaptive policy
```

No universal frequency is selected.

The tradeoff is generally:

```text id="6v5g3n"
more frequent checkpoints
    → less replay
    → more persistence overhead

less frequent checkpoints
    → more replay
    → lower checkpoint overhead
```

## Checkpoint Size

Checkpoint size may vary with processor state.

Processors should avoid treating checkpoint storage as unlimited.

Large state may require:

```text id="y4z3q5"
incremental checkpoints
partitioned checkpoints
externalized state
compaction
```

These remain future implementation decisions.

## Incremental Checkpoints

A processor may persist only state changes since the previous checkpoint.

Conceptually:

```text id="4r7x2s"
Checkpoint N
   +
Delta N+1
   +
Delta N+2
```

This is an optimization.

The resulting recovery state must remain semantically equivalent to the processor's checkpoint contract.

## Checkpoint Retention

Multiple checkpoints may be retained:

```text id="d5s8c0"
N-2
N-1
N
```

Retention may support:

* rollback
* recovery from corrupted latest checkpoint
* debugging
* replay
* migration

Retention policy belongs to storage/recovery configuration.

## Corrupt Checkpoint

A checkpoint that cannot be safely decoded or validated must not be loaded as valid processor state.

Possible outcomes include:

```text id="4z5r8p"
restore older checkpoint
replay from source
fail processor
fail graph
```

according to recovery policy.

Corruption is distinct from an ordinary processor operation failure.

## Missing Checkpoint

If a required checkpoint is unavailable:

```text id="n3y1z7"
restore checkpoint
    ↓
NotFound
```

the recovery system must follow the declared recovery strategy.

Possible strategy:

```text id="j8m4p1"
replay
```

or:

```text id="e7x2c4"
fail
```

There is no universal fallback.

## Recovery and Retained Input

Replay-based recovery requires the necessary input history to remain available.

Therefore:

```text id="q4y6w8"
checkpoint retention
```

and:

```text id="u1f5m9"
input retention
```

must be compatible.

Deleting input required for recovery invalidates the corresponding recovery strategy.

## Recovery and Storage Model

Checkpoint persistence follows the Storage Model.

A checkpoint is generally:

```text id="v3x0p7"
recoverable execution state
```

rather than automatically:

```text id="r9t1a6"
authoritative historical record
```

Historical events remain authoritative when the domain defines them as such.

## Graph Checkpoints

A graph may support coordinated checkpoints.

Conceptually:

```text id="3w8v6n"
Graph
├── A checkpoint N
├── B checkpoint N
└── C checkpoint N
```

A graph checkpoint is meaningful only if the included processor states and positions form a consistent recovery boundary.

## Distributed/Parallel Checkpoints

If processors execute independently or in partitions, coordinated checkpointing may require additional mechanisms.

The generic model does not assume:

```text id="0u9q3s"
single global clock
single global sequence
single atomic storage transaction
```

Such mechanisms belong to the eventual execution/storage implementation.

## Recovery Lifecycle

Conceptually:

```text id="7e4q2j"
FAILED / STARTING
      ↓
RECOVERY PREPARATION
      ↓
LOAD CHECKPOINT
      ↓
VALIDATE COMPATIBILITY
      ↓
RESTORE STATE
      ↓
RECONSTRUCT REQUIRED BUFFERS
      ↓
REPLAY IF REQUIRED
      ↓
READY
      ↓
ACTIVE
```

The exact lifecycle states remain implementation-dependent.

The processor lifecycle states defined previously do not need to add a mandatory public `RECOVERING` state.

## Recovery Failure

Recovery can fail.

Examples:

```text id="6c9r1x"
checkpoint corrupt
incompatible checkpoint
missing input history
storage unavailable
state reconstruction failure
replay failure
```

A recovery failure may prevent the processor from becoming `ACTIVE`.

The result must use the established error model.

## Recovery and Cancellation

Recovery itself may be cancellable where the component supports it.

For example:

```text id="s6e4q0"
restore
  ↓
replay millions of inputs
  ↓
cancel
```

Cancellation must not be represented as successful recovery.

A partially reconstructed state must not be reported as a complete recovered state.

## Recovery and Abort

An abort during recovery provides no guarantee that:

```text id="a3v5x1"
state was restored
```

or:

```text id="7f6q2c"
buffers were reconstructed
```

The processor must not enter `ACTIVE` unless recovery completed successfully.

## Recovery and Determinism

If recovery uses replay, the same:

```text id="f8q3k7"
input history
processor version
configuration
relevant execution context
```

should produce the same recovered state when the processor declares deterministic behavior.

If execution context affects results, that context must be preserved or reconstruction must explicitly account for it.

## Recovery and Current Time

Recovery must not silently use current wall-clock time where historical processing semantics require the original time context.

For example:

```text id="r6x8m2"
historical event time
```

must not become:

```text id="e5p4k9"
recovery time
```

simply because replay occurs later.

Current time should be an explicit execution-context dependency.

## Recovery and Randomness

If processing uses randomness and reproducibility matters, the randomness source/state must be part of the relevant execution context.

A recovered processor must not silently receive a different random stream while claiming deterministic replay.

## Recovery and External Dependencies

A processor may depend on external systems during recovery.

Examples:

```text id="2n8c5w"
database
filesystem
network service
model artifact
configuration service
```

Material dependency versions/identities should be represented in execution context or provenance where required.

Recovery must not silently use incompatible external state.

## Recovery and Side Effects

Replay-based recovery must be especially careful with external side effects.

A replay may reproduce:

```text id="q7f4a3"
network transmission
database write
notification
external mutation
```

that already happened before the crash.

Therefore replaying a side-effecting processor requires an explicit recovery contract.

Possible strategies include:

```text id="x9h5r1"
idempotent side effects
deduplication
effect log
transactional commit
replay without external effects
```

No universal strategy is selected.

## Recovery and Provenance

Recovered outputs should remain traceable to:

```text id="b5y2w6"
original inputs
checkpoint
replay range
processor version
configuration
graph version
execution run
```

Recovery must not erase the distinction between:

```text id="4q7s2j"
original processing
```

and:

```text id="3c8v9n"
recovery/replay execution
```

## Recovery and Identity

Restoring a processor state does not create a new logical identity for the processor node.

The execution run may have a new identity.

Derived outputs may have identity semantics defined by their own contracts.

Recovery therefore distinguishes:

```text id="k2x5c9"
processor identity
graph identity
execution identity
output identity
```

## Recovery and Manual Intervention

A recovery system may eventually allow manual selection of:

```text id="6n4m8v"
checkpoint
replay range
processor version
configuration
```

Such intervention is application/operations policy.

The core recovery model must preserve the resulting configuration and provenance.

## Consequences

### Positive

* Stateful processing can survive process or machine interruption.
* Replay and checkpoint recovery have explicit semantics.
* Checkpoint compatibility becomes version-aware.
* Duplicate processing is explicitly connected to recovery.
* Historical input retention and recovery become visibly related.
* Side-effecting processors cannot silently assume replay is safe.

### Negative

* Stateful processors require additional recovery design.
* Checkpoints consume storage and processing resources.
* Replay can be expensive.
* Strong recovery guarantees may require transactions or deduplication.
* Graph-wide recovery can become complex for parallel execution.

## Deferred Decisions

This ADR does not select:

```text id="r6k3v1"
checkpoint storage technology
checkpoint serialization
snapshot format
checkpoint frequency
incremental checkpoint implementation
transaction mechanism
distributed checkpoint protocol
checkpoint compression
checkpoint encryption
automatic recovery policy
```

## Decision Summary

```text
Checkpoint:
    Explicit recovery boundary

Snapshot:
    Persisted state representation

Recovery:
    Restore checkpoint and/or replay retained input

Stateful processor:
    Must define recovery semantics

Input position:
    Explicit recovery information

Checkpoint compatibility:
    Version/configuration aware

Duplicate execution:
    Possible during recovery

Exactly-once:
    Not implied by checkpointing

Output recovery:
    Explicitly defined

Graph checkpoint:
    Requires consistent component positions

Corrupt checkpoint:
    Must not be treated as valid state

Recovery:
    Must complete before ACTIVE

Historical time:
    Preserved during replay

Current time:
    Explicit execution-context dependency
```

## Invariants

1. A checkpoint is a recovery boundary, not merely a persisted snapshot.
2. Recovery must identify the logical processing position represented by a checkpoint.
3. Checkpoint state and input position must describe a consistent recovery point.
4. Checkpoints must identify material processor/configuration/version information.
5. Incompatible checkpoints must not be silently restored.
6. Replay-based recovery must preserve the processor's declared ordering semantics.
7. Recovery may produce duplicate execution.
8. Stateful processors must explicitly address duplicate execution during recovery.
9. Recovery must not silently replay external side effects without an explicit contract.
10. Required input history must remain available for replay-based recovery.
11. Checkpoint retention and input retention must be compatible with the recovery strategy.
12. A partially restored state must not be reported as successfully recovered.
13. Recovery cancellation is distinct from recovery failure.
14. A processor must not become `ACTIVE` before required recovery completes successfully.
15. Checkpoint creation time must not be confused with the temporal meaning of the state being checkpointed.
16. Historical event time must be preserved during replay.
17. Material execution context required for deterministic recovery must be preserved.
18. Checkpoint restoration must not silently change logical processor identity.
19. Recovery provenance must distinguish original processing from recovery/replay.
20. Checkpointing does not by itself provide exactly-once processing.

## Invariant

> **EVolution treats checkpointing as an explicit recovery boundary: recovered state must correspond to a known processing position and compatible processor definition, and recovery must preserve the temporal, identity, provenance, and duplicate-execution semantics required by the processor contract.**
