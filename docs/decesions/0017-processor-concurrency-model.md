# ADR 0017 — Processor Concurrency Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0016 defines processing as independent from its execution mechanism.

A processor may be:

* stateless or stateful
* single-item or batch-oriented
* streaming
* incremental
* windowed

The next architectural question is how multiple processing operations may execute concurrently.

Concurrency affects:

* processor state
* input ordering
* output ordering
* lifecycle operations
* cancellation
* backpressure
* determinism
* resource usage
* graph execution

The architecture must provide useful concurrency without making every processor implicitly thread-safe or forcing a particular scheduler.

## Decision

EVolution adopts **explicit concurrency semantics at the processor contract level**.

A processor must declare its concurrency guarantees rather than callers assuming that concurrent invocation is safe.

The concurrency model distinguishes:

```text
Concurrency capability
    ↓
How many operations may execute simultaneously

Ordering guarantee
    ↓
Whether concurrent operations have ordering constraints

State isolation
    ↓
How concurrent operations interact with processor state
```

The default processor assumption is:

> **A processor is not concurrently invocable unless its contract explicitly permits concurrent execution.**

The execution system may provide concurrency, but it must respect the processor's declared concurrency contract.

## Concurrency Modes

A processor may conceptually declare one of these modes:

```text
SERIAL
CONCURRENT
PARTITIONED
```

### SERIAL

At most one processing operation executes against the processor at a time.

```text
Operation A
    ↓
Operation B
    ↓
Operation C
```

No overlapping processor operations are permitted.

This is the default mode.

A serial processor may still run inside a multithreaded application; the concurrency guarantee concerns the processor's processing semantics, not the entire application.

### CONCURRENT

Multiple processing operations may execute simultaneously:

```text
Operation A ─────────→
Operation B ───────→
Operation C ───────────→
```

The processor contract must define:

* state safety
* ordering semantics
* output semantics
* determinism
* lifecycle interaction

Concurrent execution does not automatically imply output ordering.

### PARTITIONED

Processing is concurrent across independent partitions while remaining ordered or serialized within each partition.

Conceptually:

```text
Partition A:
    A1 → A2 → A3

Partition B:
    B1 → B2 → B3

Partition C:
    C1 → C2 → C3
```

Partitions may execute concurrently:

```text
A ───────────────→
B ───────────────→
C ───────────────→
```

This model is useful for stateful processing where state can be isolated by a key or scope.

Examples may include:

```text
player_id
table_id
session_id
instrument_id
partition_id
```

Partition identity must be part of the processor contract.

## Default Concurrency

Unless explicitly declared otherwise:

```text
Processor concurrency = SERIAL
```

This conservative default prevents callers from accidentally introducing races into stateful processors.

A processor becomes concurrently invocable only when its contract explicitly establishes the required guarantees.

## Stateless Processors

Stateless processors are often suitable for concurrent execution:

```text
Input A → Processor → Output A
Input B → Processor → Output B
Input C → Processor → Output C
```

However, statelessness alone does not automatically establish concurrent safety.

The processor may still depend on:

* non-thread-safe external libraries
* shared resources
* mutable configuration
* execution context
* external side effects

Therefore:

> **Statelessness is not itself a concurrency guarantee.**

The processor contract must explicitly declare concurrent support.

## Stateful Processors

Stateful processors require explicit concurrency semantics.

For:

```text
Stateₙ₊₁ = F(Stateₙ, Inputₙ)
```

concurrent operations may conflict if they modify the same state.

Therefore a stateful processor must choose a strategy such as:

```text
SERIAL
PARTITIONED
CONCURRENT with synchronized state
```

The architecture does not require one universal implementation.

## Partitioned State

Partitioning allows independent state to execute concurrently.

Conceptually:

```text
State
 ├── Partition A
 ├── Partition B
 └── Partition C
```

Operations targeting different partitions may execute concurrently:

```text
A1 ─────→ A2 ─────→ A3

B1 ─────────→ B2 ─────→ B3
```

while operations targeting the same partition retain the declared ordering:

```text
A1 → A2 → A3
```

Partitioning must be deterministic when the processor requires deterministic replay.

The partition key must not depend on unstable properties such as:

* memory address
* container position
* thread identity
* scheduling order

## Ordering

Concurrency and ordering are separate concepts.

A concurrently executable processor may have:

```text
unordered output
```

or:

```text
ordered output
```

or:

```text
partition-local ordering
```

The contract must explicitly define which applies.

For example:

```text
Input:
A B C

Execution:
B C A

Output:
B C A
```

may be valid for an unordered processor.

For an order-sensitive processor:

```text
A B C
```

must preserve the required ordering semantics even if execution is internally parallelized.

## Domain Sequence vs Execution Order

EVolution distinguishes:

```text
domain sequence
processing sequence
execution order
completion order
```

These must not be treated as interchangeable.

For example:

```text
Domain:
E1 → E2 → E3

Execution:
E1 ──────────→
E2 ─────→
E3 ─────────────→

Completion:
E2 → E1 → E3
```

The completion order does not change the domain sequence.

A processor must preserve domain ordering semantics explicitly where required.

## Determinism

Concurrency must not silently introduce nondeterminism into a processor that declares deterministic semantics.

For deterministic concurrent processing:

```text
same inputs
+
same configuration
+
same relevant execution context
+
same processor version
```

must produce semantically equivalent results regardless of scheduling.

If execution order legitimately affects results, the processor is not order-independent and must declare the relevant ordering requirement.

## Reduction and Aggregation

Some concurrent processors can safely combine independent partial results.

Conceptually:

```text
Input
 ├── Worker A → Partial A
 ├── Worker B → Partial B
 └── Worker C → Partial C

Partial A
    +
Partial B
    +
Partial C
    ↓
Final Result
```

This requires a valid merge operation.

For such processors, the contract should establish whether the operation is:

```text
associative
commutative
mergeable
order-sensitive
```

The execution system must not assume these properties merely because a processor appears aggregative.

Floating-point calculations may require particular care because mathematically equivalent reductions can produce different representations depending on execution order.

## Shared State

Shared mutable state between concurrent operations is not prohibited, but it must be explicit.

Possible approaches include:

```text
state isolation
state partitioning
serialization
synchronization
immutable shared state
atomic state
```

The architecture does not mandate a specific synchronization mechanism.

However:

> **Concurrent access to mutable state must have an explicit ownership and synchronization contract.**

## External Resources

Concurrency may also affect resources outside the processor.

Examples:

```text
database connections
files
network sockets
storage engines
external services
```

A processor's concurrency guarantee must account for relevant external-resource constraints.

A processor cannot declare unlimited concurrent execution if its required external dependency imposes a smaller safe capacity.

Resource limits belong to execution configuration/context where appropriate.

## Lifecycle Interaction

Concurrency must respect processor lifecycle state.

When:

```text
ACTIVE
```

new work may be admitted according to the processor's concurrency contract.

When:

```text
STOPPING
```

no new normal work may be admitted.

Already-admitted operations follow the shutdown mode.

For orderly stop:

```text
STOPPING [DRAIN]
    ↓
already-admitted operations complete
```

For cancellation:

```text
STOPPING [CANCEL]
    ↓
already-admitted operations may terminate cooperatively
```

Abort does not guarantee completion.

## Lifecycle Operations and Concurrent Work

Lifecycle operations must not race ambiguously with processing.

For example:

```text
Worker A:
    process(input)

Worker B:
    stop()
```

The execution system must distinguish whether `input` was admitted before the stop transition.

The semantic boundary is:

```text
admitted before shutdown
    → existing work

submitted after shutdown begins
    → rejected
```

The exact synchronization mechanism is deferred.

## Cancellation

Cancellation applies to operations independently of processor concurrency.

A concurrently executing processor may have:

```text
Operation A → Cancelled
Operation B → Success
Operation C → Success
```

during the same processor lifetime.

Cancellation of one operation must not implicitly cancel unrelated operations unless the processor contract explicitly defines group cancellation.

## Group Cancellation

Some execution environments may need to cancel multiple operations together.

Conceptually:

```text
Cancellation Scope
    ├── Operation A
    ├── Operation B
    └── Operation C
```

Group cancellation is an execution-system feature, not automatically a processor semantic.

Individual processors must not assume that cancellation of one operation cancels every operation.

## Failure Isolation

Concurrent processing should isolate local operation failures where the processor contract permits.

For example:

```text
Operation A → Success
Operation B → Error(InvalidInput)
Operation C → Success
```

does not inherently require:

```text
Processor → FAILED
```

An unrecoverable shared-state or lifecycle failure may still require:

```text
ACTIVE → FAILED
```

## Backpressure and Concurrency

Concurrency affects capacity.

If:

```text
input rate > processing capacity
```

the execution system must apply the backpressure policy defined by the relevant processor/graph contract.

Concurrency itself is not a backpressure policy.

Increasing concurrency does not guarantee that the system can safely accept unlimited work.

## Resource Bounds

Concurrency must be bounded where unbounded parallelism could exhaust resources.

Possible limits include:

```text
maximum concurrent operations
maximum partition count
queue capacity
memory budget
CPU budget
external resource capacity
```

The exact limits belong to configuration/execution context.

The architecture does not mandate a global maximum.

## Thread Safety

Thread safety is not treated as a universal property of every processor.

A processor may instead specify:

```text
concurrent invocation supported
```

or:

```text
single invocation only
```

This allows the architecture to support processors that use non-thread-safe libraries without forcing unnecessary synchronization into every component.

## Reentrancy

Reentrancy and concurrency are distinct.

A processor may be:

```text
concurrently safe
```

without supporting:

```text
recursive/reentrant invocation
```

and vice versa.

The processor contract should explicitly define reentrancy when it matters.

The default is:

```text
reentrancy not guaranteed
```

## Execution Mechanism

This ADR deliberately does not select:

* `std::thread`
* thread pools
* executors
* task systems
* coroutines
* event loops
* work stealing
* OpenMP
* TBB
* custom schedulers
* OS-specific synchronization primitives

These are implementation mechanisms.

The architecture first establishes semantic concurrency guarantees.

## C++ Interface

The final C++ processor interface must expose or encode enough information for the execution system to respect concurrency semantics.

However, this ADR does not select the exact representation.

Possible future forms include:

```text
ConcurrencyMode
ConcurrencyPolicy
ProcessorCapabilities
ExecutionContract
```

The final API must not allow callers to accidentally assume stronger concurrency guarantees than the processor provides.

## Consequences

### Positive

* Stateful processors are safe by default.
* Concurrency becomes an explicit architectural contract.
* Partitioned processing provides a path to scalable stateful execution.
* Ordering and concurrency remain separate concepts.
* Scheduling implementation can evolve independently.
* Operation-level cancellation and failure remain composable with concurrency.

### Negative

* Processor contracts become more explicit.
* Concurrent processors require more design and testing.
* Deterministic parallel processing may require additional merge/order semantics.
* Execution infrastructure must enforce processor concurrency contracts.

## Deferred Decisions

The following remain undecided:

```text
Thread pool implementation
Executor
Scheduler
Task representation
Thread affinity
CPU pinning
NUMA strategy
Coroutine usage
Lock implementation
Atomic implementation
Queue implementation
Work stealing
Process-level parallelism
Distributed execution
```

## Decision Summary

```text
Default concurrency:
    SERIAL

Explicit concurrent modes:
    SERIAL
    CONCURRENT
    PARTITIONED

Concurrent invocation:
    Must be explicitly supported

Stateful processors:
    Must explicitly define concurrency strategy

Partitioned processing:
    Supported

Ordering:
    Explicit

Domain sequence:
    Distinct from execution order

Completion order:
    Does not redefine domain order

Determinism:
    Must survive scheduling when declared

Cancellation:
    Operation-scoped by default

Failure:
    Operation-scoped where possible

Shared mutable state:
    Requires explicit ownership/synchronization semantics

Lifecycle:
    No new work after STOPPING

Resource limits:
    Explicit where relevant

Threading mechanism:
    Deferred
```

## Invariants

1. A processor is not concurrently invocable unless its contract explicitly permits it.
2. `SERIAL` is the default concurrency mode.
3. Statelessness does not automatically imply concurrency safety.
4. Stateful processors must explicitly define how concurrent access is handled.
5. Partitioned execution requires explicit partition semantics.
6. Same-partition ordering must be preserved when the processor requires it.
7. Domain sequence is distinct from execution order and completion order.
8. Concurrent execution must not silently invalidate declared determinism.
9. Shared mutable state requires explicit synchronization or isolation semantics.
10. Operation cancellation is distinct from processor-wide cancellation.
11. One operation's failure does not automatically terminate unrelated concurrent operations.
12. Lifecycle shutdown prevents admission of new normal work.
13. Already-admitted work follows the applicable stop/cancel semantics.
14. Concurrency does not itself define backpressure.
15. Resource limits remain explicit.
16. Thread safety and reentrancy are separate guarantees.
17. The execution mechanism must respect the processor's declared concurrency contract.
18. A caller must never have to infer concurrency safety from implementation details.

## Invariant

> **EVolution treats concurrency as an explicit processor contract: serial execution is the default, concurrent and partitioned execution require declared guarantees, and ordering, state ownership, cancellation, determinism, and resource constraints must remain explicit rather than being inferred from the underlying execution mechanism.**
