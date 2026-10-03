# ADR 0031 — Data Ownership and Lifetime Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processes data through multiple architectural boundaries:

```text
Input
  ↓
Envelope
  ↓
Queue
  ↓
Processor
  ↓
Output
  ↓
Graph
  ↓
Storage / Application
```

The system may eventually process:

* events
* states
* measurements
* time series
* patterns
* analyses
* processor state
* queue entries
* batches
* graph execution state
* storage records

These objects may be passed between components, retained temporarily, persisted, reconstructed, or processed concurrently.

Without an explicit ownership model, implementation can easily develop:

* dangling references
* accidental shared mutable state
* unclear destruction responsibility
* unnecessary copying
* hidden lifetime dependencies
* unsafe asynchronous processing
* ownership cycles
* incorrect assumptions about queue or processor lifetime

The architecture therefore needs a common ownership and lifetime model before concrete C++ interfaces are designed.

## Decision

EVolution uses **explicit ownership semantics**.

Every component boundary must make clear:

```text
Who owns the object?
Who may retain it?
Who may mutate it?
How long is it valid?
Can ownership transfer?
Can multiple components observe it?
```

Ownership is part of an interface contract.

The architecture does not require one universal ownership mechanism for every object.

## Ownership Categories

Conceptually, EVolution distinguishes:

```text
OWNED
BORROWED
SHARED
IMMUTABLE
TRANSFERRED
```

These describe semantic ownership/lifetime relationships rather than mandatory C++ types.

## Owned Data

An owner controls the lifetime of an object.

Conceptually:

```text
Owner
  └── Object
```

The object remains valid for as long as required by the owner's contract.

Owned data may be represented using ordinary value semantics or an owning resource handle.

## Borrowed Data

A borrowed object remains owned by another component.

Conceptually:

```text
Owner
  │
  └── Object
       ↑
       │ borrowed access
    Consumer
```

The consumer must not outlive the ownership guarantee.

Borrowed access must never silently extend the object's lifetime.

## Shared Observation

Multiple components may need to inspect the same immutable object.

Conceptually:

```text
            ┌── Consumer A
Owner ─ Object
            └── Consumer B
```

Shared observation is allowed where useful, but shared mutable ownership is not the default.

## Shared Mutable State

Shared mutable state creates synchronization and reasoning complexity.

Therefore:

> Shared mutable ownership must be explicit and justified by the component contract.

It must not arise accidentally from convenience.

## Immutable Data

Immutable data is particularly useful across processing boundaries.

For example:

```text
Event
Measurement
Pattern
Analysis
```

may often be treated as immutable after construction.

Consumers can safely observe immutable data without requiring coordinated mutation.

The exact mutability of each concept remains governed by its individual contract.

## Event Ownership

Events represent historical information.

Once an Event has entered the analytical pipeline, processors should normally treat it as immutable input.

Conceptually:

```text
Ingestion
   ↓
Event
   ↓
Processor A
   ↓
Processor B
```

Processor A must not modify the Event in a way that silently changes what Processor B observes.

If an event must be corrected or transformed, the transformation should produce a new semantic object according to the relevant domain contract.

## State Ownership

State is different from Event data.

A stateful processor owns or controls the mutable execution state required by its processing contract.

Conceptually:

```text
Processor
   └── State
```

Other components should not directly mutate processor state.

State persistence is handled through Storage contracts without transferring semantic ownership of state to the storage implementation.

## Processor State

Processor state may be:

```text
in-memory
checkpointed
reconstructed
partition-local
graph-managed
```

The processor remains responsible for the semantic meaning of its state.

Storage owns the persistence mechanism, not the meaning of the state.

## Configuration Ownership

A component receives effective configuration as input to its behavior.

After successful configuration, the component owns or safely retains the configuration representation required by its contract.

Configuration should be treated as logically immutable during normal processing.

A component must not modify a caller-owned configuration object unexpectedly.

## Execution Context Ownership

Execution context is runtime information supplied to processing.

A processor may borrow context for the duration of an operation or retain explicitly defined immutable context information.

It must not retain references to temporary caller-owned context objects beyond their contract.

## Envelope Ownership

Processor envelopes transport payloads and processing metadata.

The ownership contract must define whether:

```text
InputEnvelope<T>
```

is:

```text
copied
moved
borrowed
shared immutably
```

The exact C++ representation is deferred.

An envelope must not contain dangling references to its payload or metadata.

## Queue Ownership

A queue that admits work becomes responsible for retaining the admitted work according to its contract.

Conceptually:

```text
Producer
   ↓
Admission
   ↓
Queue owns admitted item
   ↓
Consumer
```

Once ownership has transferred to a queue, the producer must not invalidate the data required by that queued item.

## Queue Rejection

If admission is rejected:

```text
Producer → REJECT
```

the queue does not become responsible for the item.

The producer retains ownership unless the submission API explicitly defines another transfer behavior.

## Queue Drop

For an explicitly lossy queue:

```text
Producer
   ↓
Admission
   ↓
DROP
```

the queue may accept responsibility for the item and subsequently discard it according to the declared contract.

Drop must not create a dangling reference.

## Buffer Ownership

A buffer that retains intermediate data owns or safely retains that data for the duration of its retention contract.

A processor must not assume that a temporary input remains alive merely because it was previously submitted.

## Batch Ownership

A batch is a collection of processing inputs.

Ownership semantics must apply both to:

```text
Batch
```

and:

```text
Batch items
```

A batch owner must ensure that all items remain valid for the lifetime promised by the batch contract.

## Output Ownership

A processor should normally produce outputs whose lifetime is independent of temporary internal processing state.

For example:

```text
Processor
   ↓
Output
```

The caller should not receive a result that secretly references:

```text
local stack variables
temporary buffers
destroyed queue entries
processor-internal mutable state
```

unless the interface explicitly guarantees the required lifetime.

## Result Ownership

`Result<T>` owns or safely contains its returned value and error.

A successful:

```text
Result<T>
```

must not contain a dangling reference to temporary processing state.

Likewise, an `Error` follows the ownership contract established by ADR 0007.

## Error Ownership

Errors are value-owning objects.

An error returned from a component must remain valid independently of:

```text
originating operation
processor stack frame
temporary input
domain object lifetime
external resource
```

This is already established by the Error ownership ADRs.

## Views and References

Non-owning views can be useful for performance.

Examples include conceptual equivalents of:

```text
string_view
span
reference
iterator
```

However:

> A view does not transfer ownership.

Any API exposing a view must define the lifetime during which the view remains valid.

## Borrowed Views Across Asynchronous Boundaries

Borrowed references/views must not normally cross an asynchronous boundary unless their lifetime is explicitly guaranteed.

For example:

```text
submit(span)
return
```

is unsafe if the processor retains the span after the caller's storage becomes invalid.

Asynchronous processing should normally use owned or otherwise lifetime-safe data.

## Zero-Copy Processing

Zero-copy is an optimization, not an ownership model.

A processor may avoid copying data while still maintaining safe ownership.

For example:

```text
Owner
  ↓
Immutable buffer
  ↓
Borrowed view
  ↓
Processor
```

is valid only while the owner guarantees the buffer's lifetime.

## Copy vs Move

C++ implementations may use:

```text
copy
move
```

according to performance and ownership requirements.

A move transfers ownership/value state according to the object's contract.

A copy creates an independent value where copy semantics are supported.

Neither operation should silently change logical identity unless the object's contract explicitly defines copy identity semantics.

## Logical Identity vs Object Lifetime

Logical identity is independent of memory lifetime.

An object may be destroyed while the logical object remains represented elsewhere.

For example:

```text
Event object in memory
       ↓
persisted Event
       ↓
object destroyed
       ↓
Event reconstructed later
```

The reconstructed object may represent the same logical identity.

This follows the Identity Model.

## Storage Ownership

Storage implementations own their physical resources:

```text
files
database connections
buffers
indexes
transactions
```

They do not thereby become owners of the logical meaning of the stored domain object.

## Persistence and Object Lifetime

Persisting an object does not imply that the in-memory object must remain alive.

Likewise, an in-memory object does not become persistent merely because a storage implementation can access it.

These are separate concerns.

## External Resources

Objects representing external resources may own:

```text
file descriptors
sockets
database handles
memory mappings
device handles
```

when their contract requires ownership.

Resource ownership must be explicit.

The generic analytical concepts should not silently own external resources.

## RAII

C++ implementations should generally use RAII for resources with deterministic ownership.

Examples include:

```text
file descriptor
socket
memory allocation
lock
transaction
temporary resource
```

RAII is an implementation mechanism consistent with the ownership model; it does not replace the semantic ownership contract.

## Raw Pointers

Raw pointers may represent:

```text
non-owning access
optional address
interop boundary
low-level resource representation
```

but ownership must not be inferred from the pointer itself.

Owning raw pointers should not be used for ordinary EVolution object ownership.

## Smart Pointers

Smart pointers may be used when their semantics match the ownership requirement.

Conceptually:

```text
unique ownership → unique_ptr
shared ownership → shared_ptr
non-owning observation → raw pointer/reference/view
```

However, these are implementation choices rather than mandatory architectural types.

Shared ownership should require an actual shared-lifetime requirement.

## Value Types

Small semantic objects should preferably use value semantics where practical.

Examples may include:

```text
Identity
Error
Configuration
timestamps
durations
small metadata
```

Value semantics simplify ownership reasoning.

## Large Data

Large payloads may require specialized ownership.

Examples:

```text
large event payload
packet buffer
dataset block
serialized message
large time-series segment
```

The implementation may use:

```text
move ownership
reference-counted immutable buffers
arena ownership
memory pools
zero-copy views
```

The exact mechanism remains deferred.

## Memory Pools

Memory pools may be used for performance.

However, a pool does not change semantic ownership.

A pooled object must remain valid according to its public ownership contract even if its physical allocation comes from a pool.

## Arena Allocation

Arena allocation can simplify bulk lifetime management.

However, objects that escape an arena's lifetime must not retain references into destroyed arena storage.

Arena use must therefore be compatible with processor, queue, and asynchronous lifetimes.

## Thread Safety

Ownership and thread safety are separate concerns.

An owned object is not automatically thread-safe.

A shared object is not automatically unsafe.

The component contract must define:

```text
ownership
mutation
concurrent access
```

separately.

## Immutable Sharing

Immutable objects may safely be shared across concurrent consumers when their representation and lifetime support it.

This can reduce copying while avoiding shared mutable state.

## Mutable State Transfer

Mutable state may be transferred between execution contexts if ownership transfer is explicit.

For example:

```text
Worker A
   ↓ move
Worker B
```

After transfer, Worker A must no longer use the transferred state except as allowed by the moved-from contract.

## Processor Concurrency

Ownership must remain compatible with the processor's concurrency mode.

### SERIAL

Processor-owned mutable state can remain exclusively owned by the processor.

### CONCURRENT

Shared state requires explicit synchronization or another safe ownership strategy.

### PARTITIONED

State can be independently owned per partition where the contract defines partition isolation.

## Partition Ownership

For partitioned processing:

```text
Partition A → State A
Partition B → State B
Partition C → State C
```

ownership should make partition isolation explicit where possible.

A partition must not accidentally mutate another partition's state.

## Graph Ownership

A processing graph owns or otherwise controls the lifetime of its node instances and connections according to its graph contract.

Conceptually:

```text
Graph
 ├── Node A
 ├── Node B
 ├── Node C
 └── Connections
```

Nodes should not assume that their graph outlives them unless the graph contract guarantees it.

## Scheduler Ownership

The scheduler may own scheduling records and admitted work references.

It must not become the owner of processor semantic state merely because it schedules the processor.

## Execution Backend Ownership

The execution backend may own an execution task while it is running.

It does not automatically own:

```text
processor
graph
domain state
logical input
```

unless explicitly defined.

## Callback Lifetime

If callback-based APIs are eventually used, the callback contract must define:

```text
who owns callback state
how long callback remains valid
whether callback may be invoked concurrently
whether callback may outlive submission
how cancellation affects callback invocation
```

No callback lifetime may be inferred implicitly.

## Futures and Asynchronous Results

If asynchronous results are used, the result object must keep all required output/error state alive until the consumer retrieves it or the contract otherwise completes.

An asynchronous operation must not return a reference to a temporary stack object.

## Cancellation and Ownership

Cancellation must not cause premature destruction of objects still required by admitted work.

For example:

```text
STOPPING / CANCEL
```

may discard queued work according to the queue contract, but any resources required to safely terminate currently running work must remain valid.

## Abort and Ownership

Abort provides no completion guarantee, but resource safety remains required.

An abort must not intentionally create use-after-free behavior.

The execution mechanism may forcibly terminate work, but ownership contracts must still protect process integrity.

## Shutdown

During orderly shutdown:

```text
STOP
```

ownership should follow:

```text
admitted work
    ↓
drain
    ↓
completion
    ↓
release
```

During cancellation:

```text
CANCEL
```

queued work may be discarded according to contract, while currently running operations receive cooperative cancellation.

## Destruction Order

Components with dependencies must not be destroyed while dependent operations can still access them.

Conceptually:

```text
Stop accepting work
    ↓
stop scheduling
    ↓
drain/cancel work
    ↓
stop processors
    ↓
release queues/buffers
    ↓
release external resources
```

The exact destruction sequence is implementation-specific but must respect dependency lifetimes.

## Ownership Cycles

Ownership cycles can prevent destruction.

The architecture should avoid unnecessary cyclic ownership such as:

```text
Processor → Graph
Graph → Processor
```

where both strongly own each other.

References back to an owning component should normally be non-owning unless a genuine shared lifetime is required.

## Dependency Injection

Components should normally receive dependencies through explicit interfaces/references/handles.

Dependency injection must make lifetime assumptions explicit.

A component must not silently retain a dependency beyond the lifetime guaranteed by the caller.

## Configuration and Lifetime

Configuration may be supplied during:

```text
configure()
```

and retained by the processor for its configured lifetime.

After successful configuration, the processor should not depend on the caller retaining a temporary configuration object.

## Context and Lifetime

Execution context may be:

```text
borrowed for one operation
owned by a run
shared immutably
```

according to contract.

Temporary execution context must not be retained beyond its validity period.

## Provenance and Lifetime

Provenance references should use stable logical identities or owned value representations.

Provenance must not depend on the memory address of a temporary object.

## Identity and Lifetime

An identity remains meaningful independently of the memory address of its current representation.

This permits:

```text
serialize
destroy
reload
```

without changing the logical identity.

## Serialization

Serialization must not depend on memory addresses or transient ownership relationships.

Serialized data represents logical information, not C++ object layout.

## API Boundaries

Every public API should document, explicitly or by a well-defined convention:

```text
input ownership
output ownership
borrowing duration
mutation permissions
retention permissions
thread-safety requirements
asynchronous lifetime
```

## Ownership and Error Handling

Errors returned through `Result<T>` must remain valid after the originating operation has ended.

Error ownership therefore follows the independent Error ownership ADRs.

## Ownership and Provenance

Ownership changes do not automatically create new logical identities.

For example:

```text
move Event A
```

does not mean:

```text
new Event identity
```

The object's logical identity remains governed by the Identity Model.

## Ownership and Copies

Copying an object does not have one universal identity rule.

Each object type must define whether copying means:

```text
same logical identity
new logical identity
temporary representation
```

This is especially important for persistent domain objects.

## Ownership and Persistence

A persistent object may be represented by many in-memory objects over its lifetime.

Therefore:

```text
logical identity
```

must not be confused with:

```text
memory ownership
```

## Ownership and Performance

Correct ownership semantics take priority over premature zero-copy optimization.

Performance-sensitive implementations may later introduce:

```text
move-only buffers
immutable shared buffers
memory pools
arenas
custom allocators
```

without changing the semantic ownership contract.

## Ownership and ABI

Ownership across C ABI boundaries must be explicit.

If one side allocates memory and another side releases it, the allocation/deallocation contract must be defined.

An ABI must not assume that independently built components can safely free each other's allocations without an explicit compatible ownership contract.

## C Interoperability

C interfaces should prefer explicit ownership conventions.

Examples:

```text
caller-owned input
callee-owned output
caller-provided output buffer
explicit destroy function
```

The exact C ABI conventions are deferred.

## Ownership and Exceptions

If an exception occurs during construction or transfer, RAII must preserve resource ownership and avoid leaks.

However, ordinary operational failures remain represented through `Result<T>` according to the Error Model.

## Ownership and Recovery

Checkpoint and recovery mechanisms must preserve the semantic ownership required to reconstruct processor state.

A recovered object must not depend on memory owned by the failed execution instance.

## Ownership and Replay

Replay creates a new execution instance.

Replay processing may reuse logical input identities while using different in-memory object instances.

Therefore:

```text
logical identity ≠ object instance ≠ execution instance
```

## Ownership and Caching

Caches may retain copies or immutable references to data.

A cache must not silently become the authoritative owner of historical truth merely because it retains an object.

Cache lifetime and invalidation remain separate concerns.

## Ownership and Storage

Storage may return:

```text
owned value
owned buffer
borrowed view
stream
cursor
```

depending on its API.

The caller must know which lifetime guarantees apply.

A storage API must not return a borrowed view into an internal buffer unless that lifetime is explicitly guaranteed.

## Ownership and Iterators

Iterators and cursors are non-owning traversal mechanisms unless explicitly specified otherwise.

The lifetime of the underlying collection must remain valid for their use.

## Ownership and Streaming

A streaming interface may expose data incrementally.

Each emitted item must remain valid for the period specified by the stream contract.

If consumers may retain items after the next item is emitted, the stream must provide ownership/lifetime semantics supporting that behavior.

## Ownership and Backpressure

Backpressure may determine whether ownership transfers.

For example:

```text
submission rejected
```

means the destination did not necessarily acquire ownership.

Where ownership transfers before processing, the receiving component must provide the retention guarantee defined by the admission contract.

## Ownership and Delivery Semantics

Delivery guarantees interact directly with ownership.

For example:

```text
AT_LEAST_ONCE
```

may require retaining input data long enough to support retry or redelivery.

Exactly-once semantics may additionally require durable ownership/commit relationships.

## Ownership and Duplicate Execution

Duplicate execution must not result in unsafe sharing of mutable input.

Each execution should receive data with a valid lifetime according to the delivery contract.

## Ownership and Observability

Telemetry must not retain references to temporary processing objects merely to report them later.

Observability systems should copy or safely own the information they require.

Instrumentation must not create hidden lifetime dependencies.

## Default Ownership Rules

Unless a more specific contract says otherwise, EVolution should prefer:

```text
Semantic values:
    value ownership

Historical events:
    immutable after construction

Processor state:
    processor-owned

Configuration:
    component-owned after configuration

Queued work:
    queue-owned after admission

Returned results:
    caller-owned

Errors:
    value-owned

Temporary views:
    borrowed and explicitly lifetime-bound

Shared mutable ownership:
    avoided by default
```

These are defaults, not universal restrictions.

## Consequences

### Positive

* Lifetime dependencies become explicit.
* Asynchronous processing can be designed safely.
* Queues have clear ownership semantics.
* Processor state remains encapsulated.
* Immutable sharing can be used without requiring shared mutable ownership.
* Future zero-copy optimizations can be introduced without changing semantic contracts.
* C and external API boundaries can define explicit allocation responsibility.

### Negative

* Interfaces require more explicit lifetime documentation.
* Some high-performance paths may require more sophisticated ownership mechanisms.
* Shared immutable data may require reference-counting or equivalent lifetime management.
* Copy avoidance can make API design more complex.

## Deferred Decisions

This ADR does not select:

```text id="d2n1uy"
unique_ptr vs custom handles
shared_ptr usage policy
custom allocators
memory pools
arena allocation
reference counting implementation
buffer library
intrusive ownership
C ABI allocation convention
zero-copy implementation
lock-free memory reclamation
hazard pointers
RCU
```

## Decision Summary

```text
Ownership:
    Explicit part of API contracts

Default semantic values:
    Value ownership

Historical events:
    Immutable after construction

Processor state:
    Processor-owned

Configuration:
    Component-owned after configuration

Queue:
    Owns admitted work according to contract

Rejected submission:
    Destination does not acquire ownership by default

Borrowed views:
    Explicit lifetime required

Shared mutable ownership:
    Not the default

Immutable sharing:
    Allowed where lifetime is safe

Zero-copy:
    Optimization, not ownership semantics

Logical identity:
    Independent of memory lifetime

Persistence:
    Separate from in-memory ownership

Asynchronous boundaries:
    Borrowed temporary data not allowed without explicit lifetime guarantee

C ABI:
    Explicit allocation/deallocation contract required
```

## Invariants

1. Every cross-component data transfer has defined ownership and lifetime semantics.
2. Ownership must not be inferred solely from a pointer type or memory address.
3. Borrowed data must remain valid for the entire period in which it may be accessed.
4. Temporary borrowed data must not cross asynchronous boundaries without an explicit lifetime guarantee.
5. Queues own admitted work according to their admission contract.
6. Rejected work does not silently transfer ownership to the destination.
7. Processor-owned mutable state must not be externally mutated without an explicit contract.
8. Historical events must not be silently mutated by processors.
9. Returned `Result<T>` values must not contain dangling references to temporary processing state.
10. Errors remain independently valid after the originating operation ends.
11. Logical identity is independent of object lifetime and memory address.
12. Copying or moving an object must follow that object's defined identity semantics.
13. Shared mutable ownership is not assumed merely because multiple components access related data.
14. Immutable sharing is permitted when lifetime and thread-safety contracts support it.
15. Zero-copy optimizations must preserve the semantic ownership contract.
16. Storage ownership of physical resources does not imply ownership of domain meaning.
17. Persistence does not imply that an in-memory object remains alive.
18. Destruction must not occur while active operations can still access the destroyed resource.
19. Cancellation and shutdown must preserve resource safety for admitted/running work.
20. Recovery and replay must not depend on memory owned by a previous execution instance.
21. Observability must not create hidden lifetime dependencies on processing data.
22. C and external ABI boundaries must define allocation and deallocation responsibility explicitly.
23. Ownership semantics and concurrency semantics are separate and must both be defined where relevant.
24. An implementation may optimize ownership and allocation mechanisms, but it must not change the observable lifetime guarantees of the architectural contract.

## Invariant

> **EVolution requires ownership and lifetime to be explicit at component boundaries: data must remain valid for exactly the period promised by its contract, ownership must not be inferred from representation alone, and asynchronous, concurrent, persistent, and zero-copy execution must preserve those semantic lifetime guarantees.**
