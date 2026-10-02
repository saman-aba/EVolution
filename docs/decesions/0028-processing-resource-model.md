# ADR 0028 — Processing Resource Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processing consumes physical and logical resources:

```text
CPU
Memory
Storage
Network
File descriptors
Queue capacity
Execution slots
External service capacity
```

Previous decisions establish:

* processors may declare resource requirements
* queues are bounded
* backpressure limits work admission
* execution backends provide physical execution
* concurrency is part of the processor contract
* recovery consumes resources
* scheduling must respect capacity
* resource exhaustion is an explicit error condition

The architecture therefore needs a common conceptual model for describing resource requirements, limits, availability, allocation, and exhaustion.

## Decision

EVolution treats **resource requirements and resource availability as explicit execution concerns**.

A processor or processing graph may declare resource requirements and limits where they materially affect whether work can execute.

Conceptually:

```text id="j0xw9k"
Processor
    ↓
Resource Requirements
    ↓
Scheduler / Execution System
    ↓
Resource Availability
    ↓
Admission / Execution
```

Resource management must not change the semantic meaning of a processor.

## Resource Categories

The architecture recognizes several resource categories:

```text id="8s1n6h"
CPU
Memory
Storage
Network
Execution Slots
Queue Capacity
File Descriptors
External Capacity
```

This list is extensible.

A domain-specific processor may require additional resources without modifying Core semantics.

## Resource Requirement

A processor may declare:

```text id="6d8w0v"
ResourceRequirement
{
    resource
    minimum?
    preferred?
    maximum?
    unit
}
```

This is conceptual.

The exact representation is deferred.

Examples:

```text id="b4q0x6"
CPU:
    minimum = 1 core

Memory:
    maximum = 512 MiB

Execution slots:
    required = 1
```

## Requirement vs Limit

A requirement describes what processing needs.

A limit describes what processing is allowed to consume.

These are different:

```text id="r1j7k4"
Requirement:
    needs at least X

Limit:
    may consume at most Y
```

A processor may therefore have:

```text id="9z8r0x"
minimum CPU
preferred CPU
maximum CPU
```

where meaningful.

## Resource Availability

Resource availability describes what the current execution environment can provide.

Conceptually:

```text id="b5g3v2"
ResourceAvailability
{
    available
    reserved
    capacity
}
```

Availability is runtime context, not processor configuration.

For example:

```text id="j8x2d7"
Configuration:
    max_memory = 512 MiB

Environment:
    available_memory = 256 MiB
```

These must not be conflated.

## Resource Configuration vs Execution Context

A configured limit is part of component behavior.

Runtime availability belongs to execution context.

Therefore:

```text id="7q5c9w"
Configuration:
    maximum memory = 512 MiB

Execution Context:
    currently available memory = 300 MiB
```

The processor must behave according to both.

## Resource Allocation

The execution system may reserve resources before admitting work.

Conceptually:

```text id="h0k6e4"
Requested Resources
      ↓
Availability Check
      ↓
Reserve
      ↓
Admit
      ↓
Execute
      ↓
Release
```

Not all resources require explicit reservation.

The mechanism depends on the resource type.

## Admission and Resources

Resource availability may participate in work admission.

For example:

```text id="3z0h4j"
Memory capacity exhausted
       ↓
REJECT
       ↓
Error(ResourceExhausted)
```

or:

```text id="q4x8p2"
capacity unavailable
       ↓
BLOCK
       ↓
wait for resource
```

according to the applicable admission policy.

## Resource Exhaustion

Resource exhaustion is an explicit condition.

It must not silently result in:

```text id="c6t9r2"
partial processing
corrupted output
unbounded allocation
silent input loss
```

The component must follow its declared failure/admission semantics.

## Resource Exhaustion vs Processing Failure

These remain distinct.

```text id="y7k2m4"
Resource exhaustion:
    insufficient execution capacity

Processing failure:
    processor could not fulfill its operation contract
```

Resource exhaustion may cause processing failure, but the underlying condition remains identifiable.

## Resource Exhaustion and Backpressure

Backpressure is one possible response to limited resources.

Conceptually:

```text id="z9c1x5"
resource pressure
    ↓
admission control
    ↓
BLOCK / BUFFER / REJECT / ...
```

Resource limits therefore integrate with ADR 0018 rather than bypassing it.

## Memory

Memory is particularly important because uncontrolled allocation can terminate the entire process.

Memory requirements may include:

```text id="e2q7w1"
processor state
input buffers
output buffers
queue capacity
temporary allocations
cache
checkpoint state
```

The system should avoid assuming that total configured limits can always be satisfied simultaneously.

## Memory Limits

A processor may declare a maximum memory budget where required.

If the processor cannot operate within the configured limit, initialization or execution should fail explicitly.

A processor must not silently exceed a declared hard limit.

## Queue Memory

Queue capacity and processor memory are related but distinct.

For example:

```text id="v3m8k0"
Queue:
    maximum 10000 items

Processor:
    maximum 512 MiB
```

A queue implementation must account for the actual memory cost of retained envelopes/items rather than assuming item count alone represents capacity.

## Weighted Capacity

Some resources are better represented by weighted units.

For example:

```text id="a8y6c2"
small item = weight 1
large item = weight 20
```

Admission may therefore use:

```text id="s5d3f7"
capacity = weighted units
```

This extends the queue/backpressure model.

## CPU

CPU requirements may be expressed conceptually as:

```text id="t9w4e3"
minimum capacity
preferred capacity
maximum capacity
```

A processor must not assume that a requested number of CPU units corresponds to a particular operating-system thread or hardware core unless the execution environment explicitly defines that mapping.

## CPU Affinity

CPU affinity is an execution optimization/constraint.

The generic resource model does not require:

```text id="k1c8v5"
specific CPU
NUMA node
core ID
thread ID
```

Such requirements may be expressed by an execution backend when necessary.

## Execution Slots

A processor may require a bounded number of simultaneous execution slots.

For example:

```text id="r4m7p2"
maximum concurrent operations = 4
```

This interacts with the processor concurrency contract.

A resource limit must not silently contradict the declared concurrency mode.

## Concurrency vs CPU

These are distinct.

For example:

```text id="z2f9q1"
CONCURRENT
maximum 4 operations
```

does not imply:

```text id="x5d0n8"
4 CPU cores
```

Likewise:

```text id="k7s3v9"
1 CPU core
```

does not necessarily imply serial semantics.

The execution backend determines how available CPU resources are mapped to execution.

## Storage

Storage resources may include:

```text id="u8c6x3"
capacity
throughput
latency
IOPS
connections
```

A processor may require storage for:

```text id="e0n4q7"
checkpoint
spill
temporary data
persistent state
```

Storage availability is runtime information.

## Storage Capacity and Retention

Storage capacity must not silently change retention semantics.

If required storage is unavailable, the system must explicitly:

```text id="r7h2p5"
block
reject
spill elsewhere
degrade
fail
```

according to policy.

It must not silently delete required historical data merely to remain operational.

## Network

Network resources may include:

```text id="c4m8v1"
bandwidth
connections
buffers
latency
external service availability
```

A network-dependent processor may declare relevant requirements.

Network behavior remains an execution/integration concern rather than Core domain semantics.

## External Capacity

Some processors depend on external services with finite capacity.

Examples:

```text id="w3n6j9"
database connections
API request limits
model-serving capacity
message broker capacity
```

These resources may participate in admission.

The external service itself remains outside EVolution's generic resource ownership.

## Resource Ownership

The component responsible for allocating a resource should define its ownership rules.

For example:

```text id="x7p2s8"
Execution backend:
    execution slots

Queue:
    queue storage

Storage backend:
    persistent storage

Processor:
    logical processor state
```

A processor must not assume ownership of resources controlled by another component.

## Resource Lifetime

Resources should have explicit lifetime semantics.

Conceptually:

```text id="n8c4y6"
Acquire
  ↓
Use
  ↓
Release
```

Failure and cancellation must not silently leak allocated resources.

## Resource Reservation and Cancellation

If work reserves resources and is later cancelled:

```text id="q5v7m3"
reserve
  ↓
cancel
  ↓
release
```

the resource must be released according to the cancellation contract.

Blocked resource waiters must also be released during shutdown where required.

## Resource Reservation and Failure

If processing fails after reserving resources:

```text id="f1k9d2"
reserve
  ↓
processing failure
  ↓
release
```

the resource allocation must not remain indefinitely reserved unless it represents persistent state.

## Resource Leaks

Resource leaks are execution failures even when the processor's analytical output is correct.

The execution system should therefore distinguish:

```text id="m2r8w5"
processing correctness
```

from:

```text id="d6x1p0"
resource lifecycle correctness
```

## Resource Deadlocks

Multiple resources can create deadlock conditions.

For example:

```text id="c7w4n2"
Processor A:
    holds memory
    waits for storage

Processor B:
    holds storage
    waits for memory
```

The generic architecture does not select a universal deadlock prevention strategy.

Resource acquisition order may be part of an execution contract where necessary.

## Resource Availability and Scheduling

Scheduling may consider resource availability when determining whether work is eligible.

For example:

```text id="h8q2v4"
Work:
    admitted

Dependencies:
    satisfied

Processor:
    ACTIVE

Resources:
    unavailable
```

The work may remain waiting rather than becoming runnable.

Thus:

```text id="v9m3x1"
semantic eligibility
```

and:

```text id="a4k7s5"
physical resource readiness
```

remain distinct.

## Resource Availability and Admission

Admission can also depend on resources.

The distinction is:

```text id="p5d8c2"
Admission:
    Should this work enter the processing system?

Scheduling:
    Is this admitted work ready to run?

Execution:
    Can the backend actually perform it now?
```

Resource constraints can participate in all three layers, but their semantic responsibilities remain separate.

## Resource Requirements and Graph Validation

Static resource requirements may be validated before graph activation.

For example:

```text id="g2x6r9"
Processor requires:
    4 execution slots

Environment provides:
    2
```

The graph may be known to be unsatisfiable.

However, dynamic availability must remain a runtime concern.

## Static vs Runtime Resource Validation

Static:

```text id="b8v5q3"
declared minimum
vs
known environment capacity
```

Runtime:

```text id="j6k1z4"
currently available capacity
```

Both are needed.

Static validation does not eliminate runtime resource failures.

## Resource Requirements and Concurrency

Processor concurrency contracts and resource requirements must be compatible.

For example:

```text id="e5m9p2"
CONCURRENT
maximum 16 operations
```

may require more resources than:

```text id="x1q7c8"
memory limit
execution slots
```

permit.

The execution system may therefore limit actual concurrency without changing the processor's declared semantics, provided the contract allows queued/waiting execution.

## Resource Limits and Degradation

A processor may support degraded modes.

For example:

```text id="y3r8n6"
normal:
    high resolution

degraded:
    lower resolution
```

Resource pressure may trigger degradation only if explicitly permitted.

The system must not silently change analytical resolution.

## Resource Limits and Lossy Processing

Resource pressure must not implicitly become:

```text id="p2k7v5"
DROP
SAMPLE
SKIP
```

Those are explicit admission/processing policies.

A resource limit can cause such behavior only if the configured policy explicitly permits it.

## Resource Limits and Recovery

Recovery consumes resources too.

A recovery policy must account for:

```text id="m6t4x9"
checkpoint loading
replay
temporary state
queue capacity
CPU
storage I/O
```

A system must not endlessly retry recovery while resources are exhausted.

## Resource Limits and Checkpointing

Checkpointing can itself create resource pressure.

Examples:

```text id="n4w8q1"
large checkpoint
    ↓
storage saturation
    ↓
normal processing delayed
```

Checkpoint policy must therefore be compatible with resource constraints.

## Resource Limits and Backpressure

Checkpointing, replay, and recovery work may participate in the same capacity model as ordinary processing.

Recovery must not automatically bypass:

```text id="c9x3s7"
queue limits
memory limits
storage limits
execution limits
```

unless an explicit system-level recovery contract says otherwise.

## Resource Accounting

Where resource accounting affects correctness or capacity, the system should distinguish:

```text id="r5y2k8"
requested
reserved
consumed
released
available
```

These are not necessarily identical.

For example:

```text id="a7n1q6"
requested = 100 MiB
reserved  = 128 MiB
consumed  = 96 MiB
```

may be valid depending on the resource contract.

## Resource Measurement

Resource usage may be exposed as Measurements:

```text id="e8c4v0"
memory_used
cpu_time
queue_bytes
storage_bytes
```

But resource telemetry is not automatically part of the analytical data model.

It becomes an analytical Measurement only through an explicit metric definition.

## Resource Provenance

Resource conditions that materially affect results may belong in execution context/provenance.

For example:

```text id="k6m2p9"
processing degraded because resource limit was reached
```

may be important when interpreting the result.

## Resource Isolation

A processor may require resource isolation from other processors.

Possible mechanisms include:

```text id="w1f7r4"
separate process
resource quota
execution slot reservation
memory limit
container limit
```

The generic architecture does not select the physical mechanism.

## Resource Fairness

When multiple processors compete for resources, the execution system may implement fairness or priority.

This is an execution policy.

Fairness must not violate explicit semantic ordering or correctness requirements.

No universal fairness algorithm is selected.

## Resource Priority

A processor may optionally declare priority.

Priority must not override:

```text id="q3x9v5"
correctness
dependency ordering
lifecycle restrictions
capacity safety
```

Priority is therefore an execution policy, not analytical semantics.

## Resource Starvation

A processor may remain eligible but unable to acquire required resources.

This is resource starvation.

The execution system may detect and expose it.

Starvation must not be silently reported as successful processing.

## Resource Exhaustion Error

Resource exhaustion may be represented through:

```text id="z8m4c2"
Error(ResourceExhausted)
```

where appropriate.

The error does not prescribe whether the caller should:

```text id="u6q1s3"
retry
wait
degrade
stop
```

Recovery policy remains separate.

## Resource Failure vs Resource Unavailability

These are distinct.

```text id="v4p8n0"
Resource unavailable:
    temporarily cannot obtain resource

Resource failure:
    resource subsystem cannot fulfill its contract
```

The distinction can affect recovery.

## Resource Capability

Execution environments may expose capabilities:

```text id="d2r7k5"
CPU capability
memory capability
storage capability
network capability
accelerator capability
```

A processor can require capabilities in addition to quantities.

For example:

```text id="f5y9q1"
requires GPU capability
```

does not merely mean:

```text id="s3k6m8"
requires 8 CPU units
```

Capability matching is therefore distinct from capacity accounting.

## Resource Affinity

A processor may have affinity requirements such as:

```text id="g7c2v4"
NUMA locality
device locality
storage locality
network interface
```

These are execution constraints.

They remain optional and backend-specific.

## Resource Model and Portability

The generic resource model must remain portable across:

```text id="n6x1w8"
single process
multi-threaded process
multi-process deployment
container
distributed execution
```

Resource descriptions should therefore represent semantic requirements rather than operating-system implementation details.

## Consequences

### Positive

* Resource pressure becomes explicit rather than accidental.
* Backpressure, scheduling, and execution can share a consistent capacity model.
* Resource limits can participate in graph validation.
* Recovery cannot silently bypass resource constraints.
* Resource-dependent analytical changes can be represented in provenance.

### Negative

* Accurate resource accounting can be complex.
* Physical resources do not map cleanly to universal abstract units.
* Resource contention can make execution nondeterministic unless explicitly controlled.
* Distributed resource management introduces additional complexity.

## Deferred Decisions

This ADR does not select:

```text id="c8v3p5"
thread pool
CPU affinity
NUMA strategy
memory allocator
cgroup integration
container runtime
GPU framework
resource manager
distributed scheduler
fairness algorithm
priority scheduler
```

## Decision Summary

```text id="y2n7c4"
Resource requirements:
    Explicit where materially relevant

Resource availability:
    Runtime execution context

Requirement:
    What processing needs

Limit:
    What processing may consume

Allocation:
    Explicit execution responsibility

Exhaustion:
    Explicit condition

Backpressure:
    Valid response to capacity pressure

Memory:
    Must remain bounded where limits are declared

Concurrency:
    Distinct from CPU capacity

Scheduling:
    Distinct from physical resource availability

Recovery:
    Must respect resource limits

Degradation:
    Explicit only

Resource telemetry:
    Observability unless explicitly modeled as Measurements

Physical mechanism:
    Deferred
```

## Invariants

1. Resource requirements and resource limits are distinct concepts.
2. Resource availability is runtime context, not hidden configuration.
3. Resource exhaustion must produce an explicit outcome.
4. Resource exhaustion must not silently become successful processing.
5. Resource limits must not implicitly create data loss.
6. Backpressure and resource management must remain compatible.
7. Admission, scheduling, and execution remain distinct even when all depend on resource availability.
8. A processor must not silently exceed an explicitly declared hard resource limit.
9. Resource ownership and lifetime must be explicit.
10. Cancellation and failure must not silently leak resources.
11. Resource requirements must not be confused with operating-system implementation details.
12. Concurrency limits and CPU capacity are distinct.
13. Static resource validation does not eliminate runtime resource failures.
14. Recovery work must respect applicable resource constraints.
15. Checkpointing must account for its own resource consumption.
16. Resource pressure must not implicitly trigger drop, sampling, or degradation.
17. Degradation must be explicitly supported.
18. Resource conditions that materially affect results must remain identifiable through execution context/provenance.
19. Resource telemetry is not automatically analytical data.
20. Physical resource allocation mechanisms remain replaceable implementation details.

## Invariant

> **EVolution treats resources as explicit execution constraints: requirements, limits, availability, allocation, consumption, and exhaustion must have defined semantics, while the physical mechanism used to provide resources remains an implementation concern.**
