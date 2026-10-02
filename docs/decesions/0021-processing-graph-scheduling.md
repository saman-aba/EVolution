# ADR 0021 — Processing Graph Scheduling

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0019 defines the Processing Graph.

ADR 0020 defines graph validation.

The next architectural concern is scheduling: determining **which processor operation may execute, when it may execute, and what dependencies must be satisfied**.

Scheduling must remain separate from processor semantics and from the concrete execution mechanism.

For example:

```text
A → B → C
```

defines dependencies, but does not require:

```text
thread A → thread B → thread C
```

Likewise, a graph may contain independent branches:

```text
        → B →
A →             D
        → C →
```

and B and C may be independently executable.

## Decision

EVolution defines graph scheduling as the mechanism that determines **when an admitted processor operation becomes eligible for execution according to graph dependencies, processor contracts, lifecycle state, and available input**.

Scheduling determines **eligibility**, not the physical execution mechanism.

The scheduler may eventually use:

```text
threads
thread pools
event loops
coroutines
processes
hardware queues
distributed workers
```

without changing the graph's semantic scheduling model.

## Scheduling vs Execution

The architecture explicitly separates:

```text
Scheduling
    ↓
Which work is eligible?

Execution
    ↓
How is eligible work physically executed?
```

For example:

```text
A → B
```

After A produces a valid output:

```text
B becomes eligible
```

The scheduler may then submit B to:

```text
worker thread
executor
event loop
process
```

The processor itself should not need to know which mechanism was selected.

## Scheduling Unit

The scheduling unit is a processor operation.

Conceptually:

```text
Work Item
{
    processor_node
    input
    configuration
    execution_context
}
```

A work item is eligible when the processor contract and graph dependencies permit processing.

The exact C++ representation is deferred.

## Work States

Graph scheduling conceptually distinguishes:

```text
SUBMITTED
ADMITTED
ELIGIBLE
RUNNING
COMPLETED
FAILED
CANCELLED
```

These states describe processing work, not processor lifecycle.

For example:

```text
Processor = ACTIVE
Work      = ELIGIBLE
```

is valid.

A processor may remain `ACTIVE` after an individual work item fails.

## Submission vs Scheduling

Submission does not imply immediate execution.

```text
submit
  ↓
admission
  ↓
eligible
  ↓
scheduled
  ↓
running
```

This distinction follows the work-admission model.

A rejected work item never becomes eligible.

## Dependency Readiness

A processor becomes eligible only when its required input dependencies are satisfied.

For:

```text
A → B
```

B cannot execute before the relevant input from A is available.

For:

```text
A → B
A → C
```

B and C may independently become eligible.

For:

```text
A → D
B → D
```

D's eligibility depends on the input contract declared by D.

The scheduler must not invent synchronization semantics.

## Single-Input Processor

For a processor requiring one input:

```text
A → B
```

B becomes eligible when:

```text
valid input exists
processor is ACTIVE
admission permits execution
required dependencies are satisfied
```

## Multi-Input Processor

For:

```text
A ──→
      \
       → D
      /
B ──→
```

eligibility depends on D's declared input semantics.

Possible contracts include:

```text
ANY
ALL
CORRELATED
SYNCHRONIZED
WINDOWED
```

These are semantic categories rather than mandatory API names.

The scheduler must obtain this requirement from the processor/connection contract.

## Fan-In Scheduling

The scheduler must not assume that the first available input is sufficient.

For example, if D requires:

```text
A(i) + B(i)
```

then:

```text
A(i)
```

alone does not make D eligible.

The scheduler may need to retain A(i) until the corresponding B(i) arrives.

The buffering mechanism is an implementation concern, but the required correlation semantics are part of the graph contract.

## Fan-Out Scheduling

For:

```text
A
├──→ B
└──→ C
```

A's output may make both B and C eligible independently.

One destination becoming blocked must not implicitly prevent another destination unless the graph's delivery semantics require coupled behavior.

## Ordering

Scheduling must respect declared ordering requirements.

Possible ordering contracts include:

```text
NONE
INPUT_ORDER
SEQUENCE_ORDER
EVENT_TIME_ORDER
PARTITION_ORDER
```

These are conceptual categories.

Ordering must not be inferred from:

```text
thread execution order
queue insertion timing
completion timing
memory layout
```

## Processing Sequence vs Execution Order

The architecture distinguishes:

```text
domain sequence
processing sequence
scheduling order
execution order
completion order
```

Example:

```text
Input 1
Input 2
Input 3
```

may be scheduled:

```text
1
2
3
```

but complete:

```text
2
1
3
```

This is valid for an unordered processor.

A processor requiring ordered completion must declare that requirement.

## Event-Time Ordering

Event-time ordering is distinct from execution order.

An input stream may arrive:

```text
event_time:
10:00
10:02
10:01
```

The scheduler must not automatically reorder it.

If a processor requires event-time ordering, the graph must provide an explicit ordering/reordering mechanism or the processor must reject unsupported ordering.

This follows the temporal model.

## Partitioned Scheduling

A processor declared `PARTITIONED` under ADR 0017 may be scheduled concurrently across independent partitions.

Example:

```text
Partition A → processor instance
Partition B → processor instance
Partition C → processor instance
```

The scheduler must preserve the processor's declared partition semantics.

Within a partition, the processor's ordering contract remains authoritative.

## Serial Scheduling

For `SERIAL` processors:

```text
Work A
Work B
Work C
```

the scheduler must ensure that no two processing operations execute simultaneously.

This does not require a dedicated thread.

It is a semantic concurrency guarantee.

## Concurrent Scheduling

For `CONCURRENT` processors, multiple operations may become eligible and execute simultaneously.

The scheduler must still respect:

```text
input contract
state safety
resource limits
ordering requirements
cancellation
lifecycle state
```

Concurrent eligibility does not imply unlimited concurrency.

## Scheduling Capacity

Scheduling must respect resource and admission constraints.

A processor may have limits such as:

```text
maximum in-flight operations
memory capacity
connection capacity
partition capacity
```

When capacity is unavailable, work remains:

```text
waiting
```

or is handled according to the configured admission policy.

The scheduler must not create unlimited runnable work.

## Backpressure

Scheduling and backpressure interact as follows:

```text
upstream output
      ↓
connection admission
      ↓
downstream eligibility
      ↓
scheduler
```

A downstream processor that cannot accept more work may prevent upstream admission.

This interaction must follow the explicit admission policy from ADR 0018.

## Fairness

When multiple independent work items are eligible:

```text
B
C
D
```

the scheduler may need a selection policy.

Possible policies include:

```text
FIFO
priority
partition fairness
deadline
weighted scheduling
```

No universal policy is selected by this ADR.

However, scheduling policy must not violate correctness constraints.

For example, a priority policy must not reorder data when a processor requires strict input ordering.

## Starvation

A scheduling implementation should define whether eligible work can remain indefinitely unexecuted.

If fairness is required by a processor or graph contract, the scheduler must provide the corresponding guarantee.

Starvation is an execution-system concern unless it changes the semantic contract.

## Deadlock

The default DAG model significantly reduces graph dependency cycles.

However, deadlock can still arise from:

```text
resource exhaustion
blocking dependencies
bounded queues
external resources
multi-input waiting
```

The scheduler must not assume that an acyclic graph is automatically deadlock-free.

Deadlock detection/prevention mechanisms remain execution-system concerns.

## Scheduling and Lifecycle

Scheduling is constrained by graph and processor lifecycle.

Normal work admission:

```text
Graph ACTIVE
Processor ACTIVE
        ↓
work may become eligible
```

During:

```text
STOPPING
```

no new normal work is admitted.

Already-admitted work follows the shutdown policy:

```text
STOP  → drain
CANCEL → cooperative termination
ABORT → no completion guarantee
```

The scheduler must therefore distinguish:

```text
not yet admitted
admitted but waiting
running
```

during shutdown.

## Scheduling and Cancellation

Cancellation may occur while work is:

```text
waiting
eligible
running
```

Waiting/eligible work may be cancelled before execution.

Running work requires cooperative cancellation according to the processor contract.

Cancellation must not be represented as an ordinary processing failure.

## Scheduling and Failure

If a work item fails:

```text
Processor
    ↓
Result<Error>
```

the scheduler records the operation outcome and applies graph failure policy.

A local operation failure does not automatically mean:

```text
Processor → FAILED
```

or:

```text
Graph → FAILED
```

unless the relevant contract requires it.

## Scheduling and Processor Failure

If a processor transitions to:

```text
FAILED
```

the scheduler must prevent new work from being admitted to that processor.

Already-running work follows the lifecycle contract.

Dependent nodes may then:

```text
continue
wait
fail
cancel
```

according to graph policy and their dependency contracts.

## Scheduling and Graph Failure

If the graph transitions to:

```text
FAILED
```

normal scheduling must stop.

No new work should be admitted.

Existing work follows the graph failure/shutdown policy.

The graph must not report successful completion while required work remains unresolved.

## Scheduling and Provenance

Scheduling itself does not normally become domain provenance.

However, execution metadata may need to record:

```text
graph execution identity
processor node
processor version
input identity
configuration
relevant execution context
```

Scheduling order should only be recorded when it materially affects reproducibility or diagnosis.

A scheduler must not add arbitrary execution metadata to domain objects.

## Deterministic Scheduling

A graph may declare deterministic behavior.

If scheduling order can affect results, deterministic scheduling becomes part of the reproducibility contract.

For example:

```text
A
├──→ B
└──→ C
```

If B and C produce mergeable results, scheduling order may not matter.

If they update order-sensitive state, scheduling order may matter.

The graph contract must make this distinction explicit.

## Scheduling vs Graph Topology

Topology defines:

```text
who depends on whom
```

Scheduling determines:

```text
which currently-ready work executes next
```

The scheduler must not modify topology.

Changing topology requires graph mutation/revalidation under ADR 0019/0020.

## Scheduling vs Queue Implementation

A scheduler may use:

```text
queue
priority queue
ring buffer
work stealing
event loop
channel
```

but these are implementation mechanisms.

The semantic scheduling contract remains:

```text
dependencies
eligibility
ordering
concurrency
capacity
lifecycle
cancellation
```

## Scheduler Responsibilities

The scheduler is responsible for:

```text
1. determine eligibility
2. respect graph dependencies
3. respect processor concurrency contracts
4. respect ordering contracts
5. respect admission/capacity constraints
6. react to lifecycle changes
7. coordinate cancellation
8. submit eligible work to execution
9. report scheduling-level failures
```

It is not responsible for:

```text
processor business logic
domain interpretation
analytical meaning
storage semantics
application policy
```

## Execution Backend Boundary

The scheduler submits eligible work to an execution backend.

Conceptually:

```text
Graph
  ↓
Scheduler
  ↓
Execution Backend
  ↓
Processor Operation
```

The execution backend may later be:

```text
single-threaded
thread pool
event loop
process pool
distributed worker system
```

The graph architecture must remain independent of this choice.

## Scheduler State

A scheduler may maintain execution state such as:

```text
ready work
waiting dependencies
in-flight work
completed work
cancelled work
failed work
capacity usage
```

This is execution state, not domain state.

Persistence of scheduler state is a separate storage/recovery decision.

## Scheduling Observability

Scheduling may expose operational information such as:

```text
queued work
in-flight work
execution latency
admission rejection
processor utilization
```

These are telemetry/observability concerns and are not automatically analytical Measurements.

If such information becomes part of EVolution's analytical data model, it must explicitly enter through the normal measurement/event mechanisms.

## Scheduling Modes

The conceptual processing modes remain:

```text
LIVE
REPLAY
BATCH
EXPERIMENT
```

The scheduler may use different execution strategies for these modes while preserving processor and graph semantics.

For example:

```text
LIVE
    → latency-oriented scheduling

REPLAY
    → deterministic ordering where required

BATCH
    → throughput-oriented scheduling

EXPERIMENT
    → controlled/reproducible scheduling
```

These are examples of execution concerns, not prescribed policies.

## Consequences

### Positive

* Graph dependencies are separated from physical execution.
* Scheduling can evolve without changing processor contracts.
* Ordering and concurrency remain explicit.
* Fan-in and fan-out become schedulable concepts.
* Backpressure integrates naturally with scheduling.
* Lifecycle and cancellation semantics remain consistent.
* Different execution backends can support the same graph model.

### Negative

* Scheduling becomes a distinct subsystem with significant semantics.
* Multi-input processors require explicit readiness rules.
* Deterministic execution may require additional coordination.
* Resource limits and fairness complicate scheduling.

## Deferred Decisions

This ADR does not select:

```text
thread pool
executor implementation
scheduler algorithm
queue implementation
work-stealing
event loop
coroutines
CPU affinity
NUMA policy
process-based execution
distributed scheduling
priority policy
deadline scheduling
```

## Decision Summary

```text
Scheduling:
    Determines eligible graph work

Execution:
    Performs eligible work

Scheduling unit:
    Processor operation

Eligibility:
    Depends on inputs, dependencies, lifecycle,
    admission, concurrency and ordering contracts

Default topology:
    DAG

Serial processor:
    At most one concurrent operation

Concurrent processor:
    Multiple operations permitted by contract

Partitioned processor:
    Concurrency across declared partitions

Ordering:
    Explicit contract

Fan-in:
    Explicit readiness semantics

Fan-out:
    Independent destination eligibility

Backpressure:
    Admission constrains scheduling

Cancellation:
    Distinct from failure

Processor failure:
    Does not automatically imply graph failure

Graph failure:
    Stops normal scheduling

Execution backend:
    Replaceable implementation concern
```

## Invariants

1. Scheduling determines eligibility; execution determines physical execution.
2. Scheduling must respect graph dependencies.
3. Scheduling must respect processor concurrency contracts.
4. Scheduling must respect declared ordering semantics.
5. Submission does not imply execution.
6. Admission does not imply immediate execution.
7. Work cannot become eligible after its processor or graph has stopped accepting normal work.
8. Fan-in readiness must follow the destination contract.
9. Fan-out must not silently couple independent destinations.
10. Scheduling must not create unbounded runnable work.
11. Backpressure constraints must be respected.
12. Cancellation is distinct from operation failure.
13. Processor operation failure does not automatically fail the processor or graph.
14. A failed processor must not receive new work.
15. A failed graph must not admit new normal work.
16. Scheduling must not modify graph topology.
17. Scheduling mechanisms must not redefine processor semantics.
18. Execution order must not be confused with domain sequence or event time.
19. If scheduling order materially affects deterministic results, it must be part of the execution contract.
20. The physical execution backend remains replaceable.

## Invariant

> **EVolution scheduling determines which graph operations are eligible according to explicit dependencies, admission, lifecycle, ordering, and concurrency contracts, while the physical mechanism used to execute those operations remains an independent implementation concern.**
