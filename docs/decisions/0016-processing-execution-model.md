# ADR 0016 — Processing Execution Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

Previous decisions define what a processor is, what its lifecycle means, and how callers interact with that lifecycle.

They do not yet define how processing work is executed.

EVolution needs processors to support different execution styles:

* single inputs
* batches
* streams
* stateful processing
* incremental processing
* potentially long-running processing

The architecture must also distinguish:

```text
processor semantics
```

from:

```text
execution mechanism
```

A processor should define what work means without requiring the architecture to commit prematurely to a specific thread model, executor, event loop, or asynchronous framework.

## Decision

EVolution adopts an **execution-model-neutral processor architecture**.

A processor defines:

```text
input
configuration
execution context
state
    ↓
processing operation
    ↓
output
updated state
```

but does not inherently define which thread, executor, event loop, or operating-system mechanism performs that work.

Execution is modeled around explicit **processing operations**.

The execution model distinguishes:

1. work admission
2. processing
3. output production
4. state update
5. completion
6. cancellation
7. failure

The exact scheduling and concurrency mechanisms remain separate architectural decisions.

## Processing Operation

A processing operation represents one logically bounded unit of processor work.

Conceptually:

```text
Processing Operation
{
    input
    configuration
    execution_context
    state
    processing
    output
    updated_state
    outcome
}
```

An operation has one terminal outcome:

```text
SUCCESS
FAILURE
CANCELLED
```

This follows the operation semantics defined in ADR 0014.

## Work Admission

A processor must explicitly determine when submitted work becomes **admitted**.

The distinction is important during shutdown.

```text
submitted
    ↓
admitted
    ↓
processing
    ↓
completed
```

Work submitted while the processor is:

```text
CREATED
CONFIGURED
INITIALIZED
STOPPING
STOPPED
FAILED
```

is not automatically admitted as normal processing work.

Normal work admission is permitted only while:

```text
ACTIVE
```

Once a termination request prevents further admission:

```text
ACTIVE → STOPPING
```

new normal work must be rejected.

## Admission vs Processing

Submission and processing are distinct concepts.

A processor may accept work before immediately executing it.

For example:

```text
submit
   ↓
admitted
   ↓
queued
   ↓
processing
   ↓
completed
```

Therefore:

> **Admission means the processor has accepted responsibility for the work; processing means execution of that work has begun.**

This distinction is required for correct stop and cancellation semantics.

## Processing Granularity

A processor must declare the granularity at which it operates.

Possible forms include:

```text
Single item
Batch
Stream
Window
Session
Graph-level operation
```

The processor contract determines which forms are supported.

For example:

```text
Processor<Event>
```

may process one event at a time:

```text
Event → Measurement
```

while another processor may operate on:

```text
Batch<Event> → Batch<Measurement>
```

A batch is an execution/input representation.

It is not automatically an aggregation.

## Stateless Execution

A stateless processor can conceptually be represented as:

```text
Output = F(Input, Configuration, ExecutionContext)
```

The result does not depend on mutable processor history.

Such processors are easier to:

* parallelize
* replay
* retry
* test
* partition

where their contracts permit these operations.

## Stateful Execution

A stateful processor has persistent execution state:

```text
Stateₙ₊₁ = F(Stateₙ, Inputₙ, Configuration, ExecutionContext)
```

and:

```text
Outputₙ = G(Stateₙ, Inputₙ)
```

State may represent:

* windows
* accumulated measurements
* partial aggregation
* pattern detector state
* domain projections
* incremental analysis state

State is distinct from:

* configuration
* input
* output
* lifecycle state
* provenance

## State Ownership

The processor owns the logical execution semantics of its state.

The storage system owns persistence mechanisms.

Therefore:

```text
Processor
    defines state semantics
        ↓
Storage
    provides persistence mechanism
```

A processor must not require a particular storage technology merely because it is stateful.

## Incremental Execution

Processors may support incremental execution.

Instead of:

```text
all input
    ↓
recompute everything
```

an incremental processor may perform:

```text
previous state + new input
    ↓
updated state + new output
```

Incremental behavior must be part of the processor contract.

It must not silently change the meaning of the result.

## Batch Execution

Batch processing operates over an explicitly defined collection of inputs.

Conceptually:

```text
Batch<Input>
    ↓
Processor
    ↓
Batch<Output>
```

The contract must define:

* batch size semantics
* ordering
* partial failure behavior
* output cardinality
* state update semantics
* cancellation behavior

A batch processor must not assume that a batch is equivalent to an aggregation.

## Streaming Execution

A streaming processor consumes a potentially unbounded sequence:

```text
Input₁
Input₂
Input₃
...
```

and may produce:

```text
Output₁
Output₂
Output₃
...
```

Streaming processors may maintain state between inputs.

The contract must define:

* ordering requirements
* admission
* buffering
* output timing
* end-of-stream behavior
* cancellation
* failure behavior

## End of Input

Finite processing requires an explicit distinction between:

```text
no input currently available
```

and:

```text
input stream has ended
```

An empty queue does not imply end-of-stream.

If a processor supports finite streams, end-of-input must be represented explicitly.

This is important for operations such as:

```text
finalize()
flush()
close aggregation window
emit final result
```

## Output Production

Output is produced according to the processor's contract.

Possible cardinalities include:

```text
zero outputs
one output
many outputs
```

Therefore:

```text
Input → Output
```

is not assumed to be one-to-one.

Examples:

```text
Event → 0..N Patterns
Batch → 1 Analysis
Window → 1 Measurement
Event → 0 Measurements
```

Zero output is distinct from failure.

For example:

```text
successful processing
    → zero outputs
```

is valid when the processor contract allows it.

## Processing Completion

A processing operation is complete when its contract has reached a terminal outcome:

```text
SUCCESS
FAILURE
CANCELLED
```

Completion means the processor has finished the operation itself.

It does not necessarily mean the entire processor lifecycle has ended.

For example:

```text
process(event)
    → SUCCESS

processor
    → ACTIVE
```

This distinction allows a processor to perform many independent operations during its active lifetime.

## Failure During Processing

Processing failure is represented through `Result<T>`.

Conceptually:

```text
process(input)
    ↓
Result<Output>
```

Possible outcomes:

```text
Value(output)

Error(InvalidInput)

Error(ProcessingFailure)

Error(StorageUnavailable)

Error(Cancelled)
```

An individual processing failure does not automatically terminate the processor.

The processor contract determines whether the failure is:

```text
local
recoverable
fatal
```

An unrecoverable failure may transition:

```text
ACTIVE → FAILED
```

## Cancellation During Processing

Cancellation is cooperative.

A processor must define cancellation points appropriate to its processing granularity.

Possible cancellation points include:

```text
between inputs
between batches
between windows
during blocking waits
during long-running computation
```

The processor must not claim immediate cancellation if its implementation cannot safely observe cancellation immediately.

If cancellation is observed before successful completion:

```text
process(input)
    ↓
Error(Cancelled)
```

If an operation has already committed successful completion, a later cancellation request must not retroactively change that result.

## Determinism

Where a processor declares itself deterministic:

```text
same input
+
same configuration
+
same relevant execution context
+
same processor version
```

must produce semantically equivalent results.

Execution scheduling alone must not change the analytical meaning of a deterministic processor.

If scheduling affects the result, that dependency must be explicit.

## Ordering

A processor must explicitly declare whether input ordering matters.

Possible semantics include:

```text
Order-sensitive
Order-insensitive
Partially ordered
Sequence-aware
Timestamp-aware
```

The processor must not silently assume ordering that its input contract does not guarantee.

Domain sequence and processing sequence remain distinct.

## Out-of-Order Input

Processors must explicitly define their behavior when input arrives outside the expected ordering.

Possible policies include:

```text
Accept
Reject
Buffer
Reorder
Process independently
Correct later
```

No generic policy is imposed by Core.

For example, an event-time processor may require:

```text
Event time ordering
```

while another processor may only require:

```text
arrival ordering
```

## Backpressure Boundary

This ADR defines backpressure as an execution concern but does not select its implementation.

A processor may encounter:

```text
input rate > processing rate
```

The execution system must eventually define what happens.

Possible policies include:

```text
buffer
block
slow producer
drop
reject
sample
degrade
spill to storage
```

These are not interchangeable.

The selected policy must be explicit in the relevant processing contract.

No default loss policy is established by this ADR.

## Resource Limits

Processors may have resource requirements or limits such as:

```text
memory
CPU
queue capacity
storage
external connections
execution time
```

Resource limits are execution context or configuration depending on their semantics.

Exceeding a limit must produce an explicit outcome.

For example:

```text
Error(ResourceExhausted)
```

The processor must not silently produce incomplete analytical data merely because an execution resource was exhausted.

## Side Effects

A processor should preferably be side-effect free when its purpose is analytical transformation.

When side effects are required, they must be explicit in the processor contract.

Examples:

```text
write storage
emit event
send external request
modify external state
```

The execution model must not assume that every processor is pure.

However, side effects must not be hidden behind an apparently pure analytical operation.

## Reentrancy

A processor contract must define whether concurrent invocation is supported.

The default architectural assumption is:

> **A processor must not be assumed to be reentrant or concurrently invocable unless its contract explicitly states that it is.**

This does not prohibit future parallel processors.

It prevents callers from assuming thread safety without an explicit guarantee.

## Concurrency

This ADR intentionally does not define:

* number of worker threads
* thread-per-processor
* thread-per-input
* thread pools
* work stealing
* CPU affinity
* NUMA behavior
* lock strategy
* atomic strategy
* coroutine usage
* executor implementation
* process vs thread isolation

These belong to a later concurrency/execution decision.

The processor contract must instead describe the **semantic concurrency guarantees** required by the processor.

## Execution Context

Execution may depend on an explicit execution context.

Relevant context can include:

```text
execution mode
time source
randomness source
resource limits
external dependency versions
run identity
cancellation state
```

Only context that materially affects processor behavior needs to participate in reproducibility.

The processor must not silently obtain materially relevant execution inputs from unrelated global state.

## Processing Modes

The processor model supports:

```text
LIVE
REPLAY
BATCH
EXPERIMENT
```

These are execution modes, not necessarily different processor implementations.

A processor may support one or multiple modes.

The mode must be explicit when it affects behavior.

## Execution Isolation

A processor should not implicitly control the execution environment of unrelated processors.

For example, one processor should not silently:

* change another processor's scheduling
* modify global configuration
* change global time
* alter unrelated processor state

Execution coordination belongs to the processing execution system.

## Processing and Lifecycle

Execution must respect lifecycle state.

```text
ACTIVE
    ↓
work may be admitted
```

When:

```text
ACTIVE → STOPPING
```

normal work admission stops.

Already-admitted work follows the shutdown mode:

```text
STOPPING [DRAIN]
    → complete admitted work

STOPPING [CANCEL]
    → cooperatively terminate admitted work
```

Abort does not guarantee completion.

## Processing Graph

A processor normally executes as part of a processing graph:

```text
Processor A
     ↓
Processor B
     ↓
Processor C
```

The execution system is responsible for coordinating graph dependencies.

Individual processors remain responsible for their own transformation semantics.

A processor must not assume that it controls the complete graph lifecycle.

## Provenance

Execution must preserve sufficient provenance to establish:

```text
input
processor
configuration
processor version
relevant execution context
output
```

For stateful processing, state lineage may also be required.

Execution metadata must not silently replace domain provenance.

## Consequences

### Positive

* Processor semantics remain independent from threading technology.
* Stateless, stateful, batch, streaming, and incremental processors share one conceptual execution model.
* Admission is clearly separated from execution.
* Lifecycle shutdown semantics can be applied consistently.
* Concurrency can be decided later without rewriting processor semantics.
* Determinism and ordering requirements become explicit.
* Zero output, failure, and cancellation remain distinguishable.

### Negative

* Later execution decisions must define the actual scheduling mechanism.
* Processor contracts need more explicit declarations.
* Backpressure and concurrency cannot be treated as implementation details.
* Some processors will require richer execution metadata.

## Deferred Decisions

The following remain intentionally undecided:

```text
Threading model
Executor model
Scheduler
Thread pools
Work stealing
Coroutines
Callbacks
Futures
Event loops
CPU affinity
NUMA
Locking
Atomic synchronization
Inter-process execution
Distributed execution
Queue implementation
Backpressure implementation
```

## Decision Summary

```text
Execution model:
    Execution-mechanism neutral

Unit of work:
    Processing Operation

Admission:
    Explicit

Normal admission:
    ACTIVE only

Operation outcomes:
    SUCCESS
    FAILURE
    CANCELLED

Processing forms:
    Single
    Batch
    Stream
    Window

Stateful processing:
    Supported

Incremental processing:
    Supported

Output cardinality:
    0..N

Ordering:
    Explicitly declared

Out-of-order input:
    Explicitly declared

Cancellation:
    Cooperative

Failure:
    Result/Error

Determinism:
    Explicitly declared

Concurrency:
    Semantic guarantees required
    Implementation deferred

Backpressure:
    Contractually explicit
    Implementation deferred

Scheduling:
    Deferred

Threading:
    Deferred
```

## Invariants

1. Processing work is explicitly admitted before the processor assumes responsibility for it.
2. Only an `ACTIVE` processor admits normal processing work.
3. Admission and execution are distinct concepts.
4. Processing operations have explicit terminal outcomes.
5. Success, failure, and cancellation remain distinguishable.
6. Zero output is not equivalent to failure.
7. Output cardinality is part of the processor contract.
8. Stateful processors explicitly define their state semantics.
9. Batch processing is not automatically aggregation.
10. Ordering requirements must be explicit.
11. Out-of-order behavior must be explicit.
12. Missing input must not be silently fabricated.
13. Cancellation is cooperative and must have meaningful cancellation points.
14. A processor must not claim stronger cancellation responsiveness than it can provide.
15. Deterministic processors must not derive different semantics merely from uncontrolled scheduling.
16. Material execution dependencies must be explicit through configuration or execution context.
17. Side effects must be explicit.
18. Concurrency guarantees must be explicit before concurrent invocation is assumed.
19. Backpressure and loss behavior must be explicit.
20. Processing execution must respect processor lifecycle state.
21. Execution mechanisms must not redefine processor semantics.

## Invariant

> **EVolution separates processing semantics from execution mechanisms: a processor defines what work means, what inputs it admits, what outputs it produces, and how state and outcomes are handled, while scheduling, threading, synchronization, and other execution mechanisms remain replaceable implementation concerns.**
