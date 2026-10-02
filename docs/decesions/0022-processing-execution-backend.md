# ADR 0022 — Processing Execution Backend

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution now distinguishes:

```text
Processor
    ↓
Processing Graph
    ↓
Validation
    ↓
Scheduling
    ↓
Execution
```

Previous decisions deliberately avoided selecting a concrete execution mechanism.

A scheduler determines that work is eligible, but something must actually invoke the processor operation.

That mechanism is the **Execution Backend**.

The execution backend must therefore be defined as an architectural boundary without prematurely committing EVolution to:

* a thread pool
* one thread per processor
* an event loop
* coroutines
* OS processes
* a distributed worker system
* a particular executor library

## Decision

EVolution introduces an explicit **Execution Backend** abstraction.

Its responsibility is to execute already-eligible processor work according to the execution contract supplied by the scheduler.

Conceptually:

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

The execution backend does **not** determine whether work is semantically eligible.

It receives work that the scheduler has already determined may execute.

## Execution Backend Responsibilities

The execution backend is responsible for:

```text
1. accepting executable work
2. invoking processor operations
3. providing the declared execution context
4. respecting execution resource limits
5. reporting completion
6. reporting execution failure
7. supporting cancellation where required
8. maintaining execution isolation
```

It is not responsible for:

```text
processor semantics
graph topology
domain interpretation
analytical decisions
graph validation
graph-level dependency semantics
```

## Conceptual Contract

The conceptual execution operation is:

```text
execute(work, execution_context)
    ↓
Result<ExecutionOutcome>
```

where:

```text
ExecutionOutcome
{
    success
    failure
    cancelled
}
```

The exact C++ interface remains deferred.

## Eligible Work

The backend must only receive work that satisfies the scheduler's admission and readiness requirements.

For example:

```text
A → B
```

If B's input is not ready, the backend must not independently decide to execute B.

The scheduler determines:

```text
B = eligible
```

and the backend performs:

```text
execute(B)
```

This prevents execution infrastructure from becoming an implicit scheduler.

## Execution Context

The execution backend provides or participates in the execution context defined by the processor contract.

Relevant context may include:

```text
execution mode
run identity
time source
randomness source
resource limits
cancellation state
external dependency versions
execution environment
```

The backend must not silently introduce materially relevant execution context.

If execution depends on something that affects reproducibility, that dependency must be explicit according to the configuration/execution-context model.

## Execution Identity

A processor operation may have an execution identity distinct from:

```text
processor identity
graph identity
input identity
output identity
run identity
```

Conceptually:

```text
Graph Run
    ↓
Execution
    ↓
Processor Node
    ↓
Input
    ↓
Output
```

Whether every operation receives a persistent identity is deferred.

Transient execution identity may be sufficient for purely local operations.

## Synchronous Execution

A backend may execute work synchronously:

```text
scheduler
   ↓
execute()
   ↓
processor
   ↓
return
```

This is a valid execution model.

It does not imply that the architecture itself is synchronous.

## Asynchronous Execution

A backend may execute work asynchronously:

```text
scheduler
   ↓
submit
   ↓
execution backend
   ↓
processor
   ↓
completion
```

The scheduler must have a defined way to observe completion.

The exact mechanism may later use:

```text
future
callback
event
polling
completion queue
coroutine
```

but this ADR does not select one.

## Execution Completion

Completion must be distinguishable from submission.

Conceptually:

```text
SUBMITTED
    ↓
RUNNING
    ↓
COMPLETED
```

Submission success means:

> The execution backend accepted the work for execution.

It does not mean:

> The processor successfully completed the operation.

Therefore the architecture must distinguish:

```text
submission result
```

from:

```text
operation result
```

## Submission Failure

The execution backend may reject work before execution begins.

Examples:

```text
resource exhausted
backend unavailable
backend shutting down
invalid execution request
```

These are execution-system failures.

They are distinct from:

```text
processor operation failure
```

where the processor actually ran and could not fulfill its contract.

## Processor Operation Failure

If the processor executes and returns:

```text
Error
```

the backend reports the operation result without automatically converting it into a backend failure.

For example:

```text
execute
  ↓
Processor
  ↓
Result<Error>
```

is a normal operation outcome.

The scheduler/graph determines what that means for future processing.

## Backend Failure

A backend itself may fail.

Examples:

```text
worker unavailable
executor shutdown
process crash
resource subsystem failure
internal execution error
```

A backend failure may prevent work from completing even though the processor itself did not return an operation error.

The architecture must preserve this distinction.

## Execution Failure Layers

The following layers are distinct:

```text
Admission Failure
    ↓
Execution Submission Failure
    ↓
Processor Operation Failure
    ↓
Backend Failure
    ↓
Processor Lifecycle Failure
    ↓
Graph Failure
```

Not every failure propagates to every layer.

Propagation is determined by the relevant contract.

## Exceptions

Execution backends may interact with C++ or third-party APIs that use exceptions.

The backend boundary may translate exceptions into `Result`/`Error` where the surrounding EVolution contract requires explicit failure.

For example:

```text
third-party API
    ↓ throws
execution boundary
    ↓
Error
    ↓
Result
```

Exceptions are therefore an implementation mechanism, not the primary EVolution operational failure model.

This follows ADR 0005 and ADR 0006.

## Cancellation

Execution backends must support cooperative cancellation where the processor contract requires it.

Conceptually:

```text
Running Work
     ↓
Cancellation Request
     ↓
Processor cancellation point
     ↓
Cancelled
```

The backend must not claim that cancellation occurred merely because a cancellation request was issued.

Cancellation completion must be observable.

## Forced Termination

The architecture distinguishes cooperative cancellation from forced termination.

A backend may eventually need mechanisms such as:

```text
thread interruption
process termination
worker kill
```

but forced termination semantics are execution-backend-specific.

A forced termination must not be represented as successful processor completion.

## Thread Safety

An execution backend must respect processor concurrency contracts.

For a `SERIAL` processor:

```text
work A
work B
```

must not execute simultaneously.

For a `CONCURRENT` processor:

```text
work A
work B
```

may execute simultaneously if the backend and resource limits permit it.

For `PARTITIONED`:

```text
partition A → concurrent
partition B → concurrent
same partition → contract-defined ordering
```

The backend implements these guarantees; it does not redefine them.

## Reentrancy

Reentrancy is distinct from thread safety.

A processor may be:

```text
thread-safe
```

while not supporting:

```text
recursive/reentrant invocation
```

The execution backend must respect the processor's declared reentrancy contract.

The default remains:

```text
not reentrant unless explicitly supported
```

## Resource Limits

Execution backends operate under finite resources.

Possible limits include:

```text
worker count
CPU capacity
memory
file descriptors
connections
process count
queue capacity
```

The backend must not silently exceed explicit resource contracts.

Resource exhaustion should produce an explicit operational outcome.

## CPU Affinity

CPU affinity is an execution optimization/constraint.

The architecture may eventually support:

```text
processor → CPU affinity
partition → CPU affinity
execution class → CPU affinity
```

but affinity is not part of processor semantic identity.

It belongs to execution context/backend configuration.

## NUMA

NUMA placement is similarly an execution concern.

A processor may eventually have resource locality requirements, but the generic processor contract must not depend on one particular NUMA topology.

## Memory Ownership

The backend must respect ownership semantics of work and processor inputs.

It must not assume that:

```text
input pointer
```

remains valid after the operation's declared lifetime.

The exact C++ ownership mechanism is deferred.

The conceptual requirement is:

> Work must remain valid for the duration of its admitted execution contract.

## Zero-Copy Execution

A backend may eventually support zero-copy execution.

For example:

```text
producer
   ↓
shared buffer
   ↓
processor
```

This is an optimization/implementation concern.

Zero-copy must not change the semantic ownership or lifetime contract.

## Process-Based Execution

A backend may execute work in separate OS processes:

```text
scheduler
   ↓
process backend
   ├── worker process
   ├── worker process
   └── worker process
```

This may provide isolation or fault containment.

However, process boundaries introduce:

```text
serialization
IPC
ownership transfer
failure detection
lifecycle management
```

These are implementation concerns and are not required by the generic execution model.

## Distributed Execution

Future execution backends may execute work on remote machines.

Conceptually:

```text
Graph Scheduler
      ↓
Distributed Backend
      ↓
Remote Worker
```

Distributed execution would require explicit semantics for:

```text
network failure
duplicate execution
delivery guarantees
remote cancellation
identity
serialization
ordering
clock differences
```

None are selected by this ADR.

## Determinism

An execution backend must not violate declared deterministic processor semantics.

For processors where execution order affects output:

```text
execution ordering
```

becomes part of the execution contract.

For processors whose operations are independent and mergeable, different execution schedules may still produce equivalent results.

The backend must not assume that parallelization is semantically neutral.

## Exceptions and Process Crashes

An unexpected exception escaping a processor boundary is not equivalent to a normal processor `Result<Error>`.

The execution backend should treat an uncaught exception according to its execution isolation contract.

Possible consequences include:

```text
operation failure
processor failure
worker failure
graph failure
process termination
```

The exact escalation depends on isolation and lifecycle policy.

## Backend Isolation

Execution backends should provide isolation appropriate to their execution model.

At minimum:

```text
processor operation failure
```

must not automatically corrupt unrelated processor state.

Stronger isolation may be provided through:

```text
thread isolation
process isolation
container isolation
remote worker isolation
```

but these are implementation choices.

## Backend Shutdown

Execution backend shutdown must coordinate with processor and graph lifecycle.

For orderly graph stop:

```text
Graph STOPPING
      ↓
Scheduler stops admission
      ↓
Backend drains admitted work
      ↓
Backend stops accepting new work
      ↓
Backend completes shutdown
      ↓
Graph STOPPED
```

For cancellation:

```text
Graph STOPPING
      ↓
Scheduler cancels eligible/waiting work
      ↓
Backend requests cancellation of running work
      ↓
Backend completes shutdown
```

For abort:

```text
Graph FAILED
      ↓
Backend termination
```

without normal completion guarantees.

## Backend Reuse

An execution backend may serve:

```text
one processor
multiple processors
one graph
multiple graphs
```

provided its isolation and concurrency contracts remain explicit.

The architecture does not require one backend instance per processor.

## Backend State

The execution backend may have execution state such as:

```text
workers
queues
resource accounting
running operations
shutdown state
```

This is infrastructure state.

It is distinct from:

```text
processor state
domain state
graph analytical state
```

## Scheduling Boundary

The scheduler and backend must have an explicit boundary.

Conceptually:

```text
Scheduler
{
    determine_eligibility()
    submit()
}

ExecutionBackend
{
    execute()
    cancel()
    observe_completion()
}
```

This is conceptual only.

The final C++ interface may differ.

The key architectural requirement is that neither component absorbs the other's semantic responsibility.

## Backend Selection

Execution backend selection may eventually depend on:

```text
processor capabilities
graph requirements
execution mode
resource availability
deployment configuration
performance requirements
isolation requirements
```

Backend selection itself is configuration/execution policy, not processor semantics.

## Consequences

### Positive

* Execution mechanism can evolve independently from graph semantics.
* Thread pools are not baked into the architecture.
* Process and distributed execution remain possible.
* Processor contracts remain independent of OS execution details.
* Cancellation and failure semantics remain explicit.
* Execution failures can be distinguished from processor failures.
* Performance optimizations such as zero-copy and CPU affinity remain possible without changing domain semantics.

### Negative

* An additional abstraction boundary is introduced.
* Completion and cancellation semantics require explicit infrastructure.
* Backend isolation and resource management can become complex.
* Distributed execution would require substantial additional contracts.

## Deferred Decisions

This ADR does not select:

```text
thread pool
executor library
event loop
coroutine framework
thread-per-task
process pool
IPC mechanism
distributed worker system
CPU affinity implementation
NUMA strategy
queue implementation
serialization mechanism
```

## Decision Summary

```text
Execution Backend:
    Executes scheduler-approved work

Scheduler:
    Determines eligibility

Backend:
    Performs execution

Submission:
    Distinct from completion

Processor result:
    Distinct from backend failure

Cancellation:
    Cooperative where required

Concurrency:
    Must respect processor contract

Resource limits:
    Explicit

Execution context:
    Explicit where materially relevant

Exceptions:
    May be translated at boundary

Process/distributed execution:
    Permitted conceptually

Concrete backend:
    Deferred
```

## Invariants

1. The execution backend executes work already deemed eligible by the scheduler.
2. The backend must not redefine graph dependency semantics.
3. Submission success is not processor-operation success.
4. Processor operation failure is distinct from backend failure.
5. Backend failure is distinct from graph failure.
6. Cancellation is distinct from failure.
7. Execution must respect processor concurrency guarantees.
8. Execution must respect processor ordering guarantees.
9. Execution must respect declared resource limits.
10. Execution must preserve required input ownership/lifetime semantics.
11. Zero-copy or other optimizations must not change semantic ownership.
12. Exceptions at execution boundaries must not silently become successful processing.
13. Backend shutdown must respect graph and processor lifecycle semantics.
14. Forced termination must not be represented as successful completion.
15. Execution context must be explicit when it materially affects results.
16. Execution backend identity is distinct from processor and graph identity.
17. The backend must not introduce hidden global execution state that changes analytical results.
18. Concrete execution mechanisms remain replaceable.
19. Process-based and distributed execution must preserve the same higher-level processing contracts if introduced.
20. The execution backend is infrastructure; processor semantics remain owned by the processor.

## Invariant

> **EVolution separates execution from scheduling: the scheduler determines which work is eligible, while an execution backend performs that work according to explicit processor, resource, lifecycle, cancellation, and ordering contracts without embedding a particular threading, process, or distributed execution mechanism into the architecture.**
