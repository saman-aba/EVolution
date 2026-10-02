# ADR 0033 — Persistence Interface Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution needs to retain and retrieve information across process lifetimes.

Potential persistent information includes:

```text
Source Data
Events
State Snapshots
Checkpoints
Measurements
Time Series
Patterns
Analyses
Configuration
Provenance
Execution Records
```

The Storage Model establishes that persistence is independent of the physical storage technology.

The Serialization Model establishes that in-memory semantic objects are separate from their serialized representations.

A further boundary is required between:

```text
Semantic Object
    ↓
Serialization
    ↓
Persistence Interface
    ↓
Physical Storage
```

Without this boundary, domain and processing code could become coupled directly to database APIs, filesystem layouts, or vendor-specific storage behavior.

## Decision

EVolution uses an explicit **Persistence Interface** between semantic storage requirements and physical storage implementations.

Conceptually:

```text
Domain / Processing / Application
             ↓
      Persistence Interface
             ↓
      Storage Implementation
             ↓
   File / Database / KV / etc.
```

The persistence interface defines **what storage operation is required**.

The storage implementation defines **how that operation is physically performed**.

## Persistence vs Storage

The terms are related but distinct.

```text
Storage:
    physical mechanism for retaining information

Persistence:
    semantic requirement that information survives
    beyond the lifetime of an execution instance
```

For example:

```text
SQLite
```

is a storage technology.

```text
Persist Event
```

is a persistence requirement.

## Persistence Scope

Persistence may apply to:

```text
single object
batch
stream segment
partition
processor state
checkpoint
graph execution
dataset
```

The persistence contract must define the relevant scope.

## Source of Truth

Every persistent dataset should identify whether it is:

```text
AUTHORITATIVE
DERIVED
CACHE
RECOVERABLE
EPHEMERAL
```

according to the Storage Model.

A storage backend must not automatically become authoritative merely because it contains a copy of data.

## Logical Identity

Persistent records must use logical identity where the object contract requires it.

Physical identifiers such as:

```text
database row ID
file offset
memory address
storage key
```

must not silently replace logical identity.

A storage implementation may maintain its own internal identifier.

## Storage Key vs Logical Identity

These are distinct:

```text
Logical Identity:
    identifies the semantic object

Storage Key:
    locates the object in a particular storage implementation
```

For example:

```text
EventId = E123
```

may be stored under:

```text
partition=2026-10
offset=827341
```

The offset is not the EventId.

## Persistence Interface Responsibilities

A persistence interface may define operations such as:

```text
create
insert
get
find
update
delete
append
scan
checkpoint
restore
```

The exact operation set depends on the object/storage contract.

The interface should expose semantic operations rather than vendor-specific commands.

## CRUD Is Not Universal

Not every EVolution object should support arbitrary CRUD.

For example, historical Events may be:

```text
append-only
```

while a cache may support:

```text
insert
replace
evict
```

A checkpoint may support:

```text
write
load
delete
```

The persistence interface should therefore be designed around semantic capabilities rather than assuming universal CRUD.

## Capability-Oriented Storage

A storage implementation may advertise capabilities such as:

```text
append
lookup
range_scan
ordered_scan
transactions
atomic_commit
batch_write
batch_read
snapshots
durability
streaming
random_access
delete
```

Not every backend must support every capability.

A consumer must not assume an unsupported capability exists.

## Interface Compatibility

Before using a storage implementation, the application or storage coordinator should be able to determine whether required capabilities are available.

For example:

```text
Required:
    ordered_scan
    append
    durable_write

Backend:
    supports all three
```

Only then should the component rely on those semantics.

## Read Semantics

A read operation may conceptually produce:

```text
FOUND
NOT_FOUND
ERROR
```

These must remain distinct.

`NOT_FOUND` is not necessarily an error.

For example:

```text
Result<optional<T>>
```

may be appropriate when both:

```text
operation failure
```

and:

```text
successful absence
```

are meaningful.

The exact C++ API remains deferred.

## Write Semantics

A write operation should define:

```text
accepted
durable
committed
visible
```

semantics where relevant.

A successful in-memory write is not automatically equivalent to durable persistence.

## Durability

Durability is a storage property that must be explicit.

Conceptually:

```text
write accepted
    ↓
write persisted
    ↓
write durable
```

These may be different points.

A persistence API must not claim durability unless the underlying contract provides it.

## Visibility

A successfully written record may not immediately be visible to all readers depending on storage semantics.

Possible models include:

```text
READ_AFTER_WRITE
EVENTUAL_VISIBILITY
TRANSACTIONAL_VISIBILITY
```

The exact guarantees belong to the storage contract.

## Atomicity

Some operations may require atomicity.

For example:

```text
checkpoint state
+
input position
```

may need to represent one consistent recovery boundary.

The persistence interface must expose atomicity requirements when semantic correctness depends on them.

## Transactions

Transactions are a possible implementation mechanism.

They are not themselves the architectural semantic contract.

A component should specify:

```text
these changes must commit atomically
```

rather than:

```text
use SQL transaction
```

where possible.

## Batch Writes

Batch persistence may be required for performance.

A batch operation must define:

```text
all-or-nothing
partial success
per-item result
best-effort
```

semantics.

A failed batch must not be ambiguously represented as fully successful.

## Partial Persistence

Partial success may be valid.

For example:

```text
100 records
97 persisted
3 rejected
```

requires an explicit result describing the disposition of each relevant item.

The system must not report:

```text
batch persisted
```

without qualification.

## Append-Only Persistence

Historical event storage may use append-only semantics.

Conceptually:

```text
Event1
Event2
Event3
...
```

An append-only contract can support:

```text
replay
audit
historical reconstruction
```

without requiring mutable updates to historical records.

## Event Corrections

If an event requires correction, the persistence model should follow the Event and Provenance contracts.

The system should not silently overwrite historical truth unless the source-of-truth contract explicitly permits mutation.

Possible mechanisms include:

```text
correction event
replacement record
versioned record
superseding record
```

The exact domain semantics remain domain-owned.

## Ordering

Storage ordering is not automatically semantic ordering.

A database returning rows in physical insertion order does not guarantee domain sequence.

If order matters, the persistence interface must expose an explicit ordering key or semantic ordering guarantee.

Possible ordering information includes:

```text
sequence
timestamp
logical identity
partition position
storage offset
```

These must not be conflated.

## Range Queries

Time-series and event workloads may require range queries.

A range query must define:

```text
start
end
boundary semantics
ordering
inclusion/exclusion
```

The preferred temporal interval convention is:

```text
[start, end)
```

unless the relevant contract requires otherwise.

## Event-Time Queries

Querying by event time must use the event's temporal semantics.

It must not silently substitute:

```text
ingestion time
processing time
storage time
```

for event time.

## Unknown and Estimated Time

Persistent temporal information must preserve:

```text
KNOWN
ESTIMATED
UNKNOWN
```

status.

An event with unknown event time must not be silently inserted into a time-range query as though it had a known timestamp.

Storage may support separate handling for unknown/estimated temporal values.

## Indexes

Indexes are physical or logical query optimizations.

They must not redefine object semantics.

For example:

```text
EventId index
event_time index
type index
```

can accelerate lookup without changing Event identity or temporal meaning.

## Query Semantics

A query should specify what semantic information is requested.

For example:

```text
events between T1 and T2
```

must define whether the query means:

```text
event time
ingestion time
processing time
```

A storage backend must not guess.

## Pagination

Large query results may be paginated.

Pagination must define stable traversal semantics where results can change during iteration.

Possible mechanisms include:

```text
offset
cursor
logical sequence
storage position
```

The exact mechanism is deferred.

## Streaming Reads

A persistence interface may expose large datasets as streams rather than materializing everything in memory.

Conceptually:

```text
Storage
   ↓
Iterator / Cursor / Stream
   ↓
Processor
```

The stream must define:

```text
ordering
ownership
lifetime
cancellation
failure
completion
```

## Storage Cursors

A cursor is generally a traversal mechanism rather than a persistent object identity.

A cursor must not be assumed to remain valid across process restart unless the storage contract explicitly supports persistent cursors.

## Storage and Backpressure

Storage reads/writes may interact with processing backpressure.

For example:

```text
Processor
    ↓
Storage
    ↓
slow persistence
```

may cause upstream work to block or queue.

The persistence interface should expose enough semantics for the surrounding processing system to apply explicit admission/backpressure policy.

Storage itself must not silently convert capacity problems into successful loss.

## Storage Resource Exhaustion

Possible failures include:

```text
disk full
memory exhausted
connection limit
transaction limit
quota exceeded
```

These should be represented through the established `Result<T>` error model.

The error does not itself prescribe:

```text
retry
drop
degrade
stop
```

Recovery remains policy-driven.

## Storage Availability

Storage may become temporarily unavailable.

This is distinct from:

```text
record not found
invalid request
data corruption
permission failure
```

The error category/code should preserve the distinction where useful.

## Storage Corruption

Corrupt persistent data must not silently deserialize into valid analytical objects.

Corruption should produce an explicit error.

Recovery policy may then choose:

```text
restore
replay
skip
repair
fail
manual intervention
```

according to policy.

## Storage Version Compatibility

Persistent data may outlive the software version that created it.

Therefore persisted representations should be compatible with the Serialization Model and schema/version rules.

Incompatible data must not be silently interpreted using a different semantic contract.

## Storage Migration

Storage migration may involve:

```text
schema migration
representation migration
index migration
data transformation
```

Migration is distinct from ordinary runtime storage access.

## Migration Safety

A migration should define:

```text
source version
target version
transformation
failure behavior
rollback/recovery behavior
validation
```

where applicable.

A partially completed migration must not silently appear as a valid fully migrated dataset.

## Persistence and Provenance

Persistent derived objects should retain provenance information required by their contract.

For example:

```text
Analysis
    ↓
input identities
configuration
algorithm version
```

may need to remain available after process restart.

Storage must preserve provenance information when it is part of the persisted object's semantics.

## Persistence and Reproducibility

Persistent records required for reproduction must remain available according to retention policy.

For example, replay may require:

```text
input events
configuration
processor version
checkpoint
```

If required information has been deleted, the system must not claim complete reproducibility.

## Retention

Retention determines how long persistent information remains available.

Retention is separate from physical storage capacity.

Examples:

```text
events: 5 years
telemetry: 30 days
checkpoints: 7 days
cache: 1 hour
```

These are examples only.

No universal retention period is selected.

## Deletion

Deletion may be:

```text
physical deletion
logical deletion
expiration
tombstone
compaction
```

depending on storage semantics.

Deletion must not silently violate provenance, recovery, or source-of-truth requirements.

## Cache Persistence

A persistent cache may survive process restart.

That does not make the cache authoritative.

Cache contents may still be invalidated or reconstructed.

## Derived Data

Derived analytical objects may be persisted to improve query performance.

Because they are derivable, the architecture should retain enough information to distinguish:

```text
authoritative source
```

from:

```text
materialized derived result
```

where relevant.

## Materialized Results

Materialized measurements, time series, patterns, or analyses may be stored.

Their provenance should identify:

```text
source inputs
processor/algorithm
configuration
version
```

when required for interpretation or reproducibility.

## Storage and State

Processor state may be persisted.

The storage implementation does not define the meaning of that state.

The processor remains responsible for:

```text
state schema
state invariants
state version
compatibility
reconstruction
```

## Storage and Checkpoints

A checkpoint is a recovery boundary, not merely a serialized state record.

A valid checkpoint may require:

```text
processor state
input position
processor version
graph version
configuration identity
```

and consistency between them.

## Checkpoint Atomicity

Where required by the processor contract:

```text
State
+
Input Position
```

must correspond to the same semantic processing point.

Persisting them independently without a consistency mechanism may create an invalid recovery boundary.

## Storage Isolation

Storage implementations should not expose physical resources directly to higher architectural layers unless the interface explicitly requires them.

For example:

```text
SQL connection
file descriptor
database transaction object
```

should not become implicit dependencies of domain logic.

## Storage Transactions

A domain should express semantic atomicity requirements.

The storage implementation may satisfy those requirements using:

```text
transaction
write-ahead log
atomic rename
journal
batch commit
```

or another mechanism.

The implementation remains replaceable if semantic guarantees are preserved.

## Storage Concurrency

Storage implementations may support concurrent readers/writers.

Concurrency guarantees must be explicit.

A persistence API must define whether operations are:

```text
thread-safe
serialized
partitioned
transactionally isolated
```

where relevant.

## Storage and Processor Concurrency

A processor's concurrency model must not assume stronger storage concurrency guarantees than the storage interface provides.

For example, a `CONCURRENT` processor cannot safely issue concurrent writes if its persistence contract permits only serialized access unless an explicit coordination layer exists.

## Storage and Delivery Semantics

Persistence may participate in delivery guarantees.

For example:

```text
write durable
    ↓
acknowledge input
```

may be required for a particular at-least-once workflow.

The meaning of acknowledgement must be explicit.

## Storage and Exactly-Once

Exactly-once processing is not implied by having durable storage.

Exactly-once effects may require coordination among:

```text
input identity
state
output
acknowledgement
commit
recovery
deduplication
```

A storage backend alone does not provide universal exactly-once semantics.

## Deduplication

Storage may support deduplication based on logical identity or idempotency key.

The key must be stable and semantically defined.

Storage position or memory address must not be used as an accidental deduplication identity.

## Idempotent Writes

An idempotent persistence operation may allow repeated execution without changing the resulting logical state.

This can support recovery and at-least-once delivery.

Idempotency must be explicit rather than assumed.

## Storage Error Translation

A storage implementation may translate vendor-specific failures into EVolution error codes.

For example:

```text
vendor:
    SQLITE_BUSY

EVolution:
    Resource / External / Storage-specific code
```

The exact mapping belongs to the storage implementation/interface boundary.

Human-readable vendor messages are not stable EVolution contracts.

## Storage Observability

Storage may expose operational telemetry such as:

```text
read_latency
write_latency
queue_depth
connection_count
cache_hit_rate
storage_errors
```

These remain observability data unless explicitly modeled as analytical information.

## Storage Security

Storage implementations may require:

```text
authentication
authorization
encryption
key management
access control
```

These are storage/security concerns rather than generic analytical semantics.

Credentials must not become part of ordinary persisted domain objects unintentionally.

## Storage Secrets

Secrets required to access storage should be handled through the configuration/security boundary.

They should not be serialized into ordinary provenance or analytical records unless explicitly required and protected.

## Storage API Shape

A conceptual persistence interface may resemble:

```text
Repository<T>
{
    write(...)
    read(...)
    query(...)
    remove(...)
}
```

but this is illustrative rather than a final C++ API.

Different object categories may require different interfaces.

For example:

```text
EventStore
CheckpointStore
ConfigurationStore
AnalysisStore
```

may expose different capabilities.

## Repository vs Store

The architecture does not require one universal repository abstraction.

A generic abstraction can become too weak or too complicated if every storage workload is forced into the same interface.

Interfaces should reflect semantic requirements.

## Event Store

An Event Store may require:

```text
append
read by identity
read by sequence
read by time
scan
```

with explicit ordering and durability semantics.

## State Store

A State Store may require:

```text
save
load
version
delete
```

and checkpoint compatibility semantics.

## Measurement Store

A Measurement Store may require:

```text
append
range query
dimension filtering
time ordering
aggregation support
```

depending on workload.

## Analysis Store

An Analysis Store may require:

```text
store
retrieve
query by scope
query by provenance
query by time
```

depending on the analytical application.

## Storage Interface Ownership

Storage interfaces belong to the architectural layer that defines the required semantic contract.

A generic Core interface should not contain vendor-specific storage semantics.

Domain-specific storage requirements may be defined by the relevant domain or application layer.

## Dependency Direction

The desired dependency relationship is:

```text
Domain / Processing / Application
             ↓
      Storage Interface
             ↓
    Storage Implementation
```

not:

```text
Domain
   ↓
PostgreSQL API
```

or:

```text
Core
   ↓
SQLite API
```

unless an explicit architectural decision later changes this boundary.

## Storage Implementation Independence

Multiple implementations should be possible where practical:

```text
in-memory
file
SQLite
PostgreSQL
key-value
columnar
time-series
object storage
remote service
```

The architecture does not require every implementation to support every capability.

## In-Memory Storage

An in-memory implementation is useful for:

```text
tests
experiments
temporary applications
benchmarks
```

It may intentionally provide weaker durability guarantees.

It should still obey the semantic persistence interface it claims to implement.

## Testing Storage Implementations

Storage implementations should be tested against shared behavioral contracts.

Tests should verify:

```text
write/read
not found
ordering
identity preservation
versioning
partial failure
durability claims
concurrency claims
transaction semantics
corruption behavior
```

where applicable.

## Contract Testing

A storage interface should ideally have reusable contract tests.

Conceptually:

```text
Storage Contract Tests
        ↓
 ┌──────┴──────┐
 │             │
Memory       Database
Storage      Storage
```

Each implementation must satisfy the capabilities it advertises.

## Persistence and Testing

Tests should not depend exclusively on one physical storage technology.

Unit tests may use in-memory implementations.

Integration tests should exercise real storage implementations where their behavior cannot be meaningfully simulated.

## Consequences

### Positive

* Domain and processing logic remain independent of storage vendors.
* Storage capabilities become explicit.
* Durability and visibility semantics cannot be confused with successful API return.
* Checkpoint and recovery requirements have a clear persistence boundary.
* Multiple storage implementations can coexist.
* Storage contract testing becomes possible.

### Negative

* A single generic storage interface is insufficient for all workloads.
* Capability negotiation adds some complexity.
* Strong durability/atomicity guarantees may constrain backend choices.
* Storage migrations require explicit version management.

## Deferred Decisions

This ADR does not select:

```text
SQLite
PostgreSQL
MySQL
LMDB
RocksDB
LevelDB
Parquet
DuckDB
ClickHouse
TimescaleDB
filesystem layout
object storage
database ORM
query language
storage serialization format
transaction implementation
connection pool
```

These require concrete workload analysis and separate decisions.

## Decision Summary

```text
Persistence:
    Semantic requirement for durable retention

Storage:
    Physical retention mechanism

Interface:
    Explicit boundary between them

Logical identity:
    Independent of storage key

CRUD:
    Not universal

Capabilities:
    Explicit

Durability:
    Explicit guarantee

Visibility:
    Explicit guarantee

Ordering:
    Explicit semantic requirement

Partial writes:
    Explicitly represented

Historical events:
    Prefer append-oriented semantics where appropriate

Derived data:
    Provenance preserved

State:
    Semantic ownership remains with processor/domain

Checkpoint:
    State + recovery position + required metadata

Exactly-once:
    Not implied by persistence

Deduplication:
    Explicit identity/idempotency semantics

Retention:
    Explicit

Migration:
    Explicit

Physical backend:
    Replaceable
```

## Invariants

1. Semantic persistence requirements are separated from physical storage technology.
2. Logical identity is independent of storage keys and physical location.
3. Storage ordering is not automatically semantic ordering.
4. Read absence and read failure are distinct outcomes.
5. Successful write is not automatically equivalent to durable persistence.
6. Visibility guarantees must be explicit where they affect correctness.
7. Partial persistence must never be reported as complete success without qualification.
8. Historical data must not be silently overwritten when the source-of-truth contract is append-oriented.
9. Temporal queries must specify which temporal dimension they use.
10. Known, estimated, and unknown temporal information must remain distinguishable when persisted.
11. Serialization and persistence are separate architectural concerns.
12. Persistence of derived data must preserve provenance required for interpretation or reproducibility.
13. Processor state semantics remain owned by the processor/domain, not by the storage implementation.
14. A checkpoint is not merely a serialized state snapshot; it must satisfy the recovery contract.
15. Exactly-once processing is not implied by durable storage.
16. Deduplication and idempotency require explicit logical identity or idempotency semantics.
17. Storage errors must be represented through the established error model.
18. Storage errors do not automatically prescribe retry or recovery behavior.
19. Storage implementations must not expose vendor-specific mechanisms as implicit domain dependencies.
20. Storage capabilities must be explicit rather than assumed.
21. An implementation must not claim stronger guarantees than its physical backend provides.
22. Storage migrations must not silently change semantic meaning.
23. Retention and deletion must respect source-of-truth, provenance, recovery, and reproducibility requirements.
24. Storage observability remains distinct from analytical data.
25. Storage implementations remain replaceable behind explicit semantic contracts.

## Invariant

> **EVolution separates persistence semantics from physical storage: components request explicit retention, retrieval, ordering, durability, atomicity, and recovery guarantees through storage interfaces, while the underlying database, filesystem, or storage technology remains an interchangeable implementation concern.**
