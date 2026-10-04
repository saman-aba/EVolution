# ADR 0064: Core Event Stream and History API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will define an **Event Stream** as an ordered logical sequence of Events associated with a defined stream scope.

```text
Event
    ↓
Event Stream
    ↓
Projection / Processing / Query
```

The Event Stream is a semantic history abstraction.

It is not inherently:

* a queue,
* a database table,
* a file,
* a network stream,
* or an execution mechanism.

The physical storage and delivery mechanism remains an implementation concern.

---

# 1. Event Stream

Conceptually:

```text
EventStream
{
    identity
    scope
    events
}
```

The exact representation remains implementation-defined.

An Event Stream provides access to Events according to an explicit ordering and history contract.

---

# 2. Event Stream Identity

An Event Stream may have a logical identity:

```cpp
using EventStreamId = evolution::Id<EventStreamTag>;
```

The stream identity identifies the logical stream.

It does not identify:

```text
Event
Event source
Storage location
Processing connection
Queue
Run
```

---

# 3. Stream Scope

Every Event Stream should have a defined scope.

Examples:

```text
session
table
player
market
instrument
application
system
```

The scope determines which Events logically belong to the stream.

---

# 4. Stream Scope vs Event Source

An Event Source describes where an Event originated.

A Stream Scope describes the logical history to which the Event belongs.

For example:

```text
Event source:
    hand_history_file

Event stream:
    poker_session_42
```

They are not interchangeable.

---

# 5. Stream Membership

An Event belongs to a stream only according to an explicit membership rule.

Membership may be based on:

```text
Event type
domain identity
scope
source
external stream identity
ingestion mapping
```

The Core does not impose a universal membership rule.

---

# 6. Event Stream Ordering

An Event Stream must define how its Events are ordered.

Possible ordering mechanisms include:

```text
event sequence
event time
ingestion order
source order
domain-defined order
```

The ordering mechanism must be explicit.

---

# 7. Event Time Is Not Ordering

An Event Stream must not assume that timestamps create a total ordering.

Two Events may have:

```text
same timestamp
```

or:

```text
different timestamps
but unknown relative ordering
```

A sequence or other explicit ordering mechanism may therefore be required.

---

# 8. Stream Sequence

A stream may assign a sequence position:

```text
E1 → sequence 1
E2 → sequence 2
E3 → sequence 3
```

Sequence semantics must identify the scope in which the sequence is valid.

---

# 9. Event Sequence vs Stream Sequence

An Event may already contain a domain/source sequence.

The Event Stream may have its own sequence.

For example:

```text
source sequence:
    1001

stream position:
    42
```

These are distinct concepts.

The stream must not overwrite the Event's original sequence.

---

# 10. Storage Order

Physical storage order is not automatically Event Stream order.

For example:

```text
database row order
file offset
memory address
```

must not become semantic ordering merely because it is convenient.

---

# 11. Append

An Event Stream may support append semantics.

Conceptually:

```text
append(Event)
    → Result<StreamPosition>
```

The exact API remains deferred.

Appending an Event does not automatically imply durable persistence.

---

# 12. Append Result

A successful append may provide:

```text
StreamPosition
```

identifying the Event's position in the stream.

This position is distinct from EventId.

---

# 13. EventId vs Stream Position

These answer different questions:

```text
EventId:
    Which Event is this?

Stream position:
    Where does this Event occur in this stream?
```

The same Event may theoretically appear in multiple streams and therefore have different stream positions.

---

# 14. Duplicate Event Identity

Appending an Event with an identity already present in a stream may produce:

```text
accepted duplicate
rejected duplicate
idempotent success
new stream entry
```

The behavior must be explicitly defined by the stream contract.

There is no universal duplicate policy.

---

# 15. Idempotent Append

A stream may support idempotent append using EventId or another explicit key.

For example:

```text
append(E1)
append(E1)
```

may result in only one logical stream occurrence.

This is a delivery/storage contract, not an inherent property of Event identity.

---

# 16. Historical Immutability

An Event Stream representing historical Events should normally be append-oriented.

Existing historical Events must not be silently modified.

Corrections should be represented explicitly.

---

# 17. Event Correction

Possible correction mechanisms include:

```text
correction event
replacement version
superseding event
new corrected stream
```

The chosen mechanism is domain/storage-specific.

The original historical fact should remain traceable when historical integrity requires it.

---

# 18. Deletion

Deletion from a historical Event Stream must not be treated as an ordinary mutation.

If retention requires deletion, the storage contract must define:

```text
retention policy
deletion semantics
historical consequences
provenance implications
```

---

# 19. Reading

An Event Stream may provide sequential reading:

```text
read(position)
read_range(start, end)
scan(...)
```

The exact API remains deferred.

Reads must preserve the stream's ordering contract.

---

# 20. Range Semantics

Range queries should use explicit boundaries.

The preferred temporal convention is:

```text
[start, end)
```

For sequence ranges, the API should similarly define whether boundaries are inclusive or exclusive.

Ambiguous range semantics are prohibited.

---

# 21. Stream Cursor

A sequential reader may maintain a logical cursor:

```text
Cursor
{
    stream
    position
}
```

A cursor identifies a reading position.

It does not change Event identity.

---

# 22. Cursor vs Processing Offset

A read cursor may later be used by processing infrastructure as an input position.

However:

```text
Cursor
    → reading state

Processing offset
    → execution/recovery state
```

They should remain conceptually distinct even when represented by the same underlying position.

---

# 23. Snapshot Position

A State snapshot may identify the stream position through which it was derived.

For example:

```text
Snapshot:
    stream = S1
    position = 1000
```

This establishes a recovery/reconstruction boundary.

---

# 24. Projection Replay

A Projection can consume an Event Stream:

```text
Event Stream
     ↓
   Replay
     ↓
 Projection
     ↓
   State
```

Replay must preserve the Event Stream's declared ordering semantics.

---

# 25. Stream Replay

Replay means reading historical Events again.

Replay does not create new Events.

It creates a new processing execution over existing historical information.

---

# 26. Replay Run

A replay may have its own:

```text
RunId
```

The RunId identifies the execution.

It does not replace:

```text
EventId
EventStreamId
StreamPosition
```

---

# 27. Live Consumption

An Event Stream may also support live consumption.

Conceptually:

```text
Historical Events
       ↓
       ├── Replay
       │
       └── Live Tail
```

The distinction between historical reading and live subscription belongs to the stream/processing contract.

---

# 28. Historical and Live Semantics

A consumer must know whether it is receiving:

```text
historical data
live data
historical + live continuation
```

This must not be inferred from timing behavior.

---

# 29. Stream End

Historical streams may have a known end.

Live streams may have no currently known end.

The API must distinguish:

```text
end of available history
temporary absence of new events
stream termination
error
```

These are not equivalent.

---

# 30. Empty Stream

An empty stream is valid.

It means:

```text
no Events currently belong to the requested history
```

It does not mean:

```text
error
stream unavailable
state = zero
```

---

# 31. Missing Event

A gap in stream positions may indicate:

```text
missing data
filtered positions
retention
unknown history
```

The semantics must be explicit.

A missing Event must not silently become an empty Event.

---

# 32. Event Filtering

A reader may filter Events.

Filtering must preserve the semantics of the underlying stream.

For example:

```text
all events
    ↓ filter(PlayerAction)
selected events
```

The filtered sequence is a derived view.

Its positions must not automatically be interpreted as positions in the original stream unless explicitly represented.

---

# 33. Filtered Stream

A filtered Event sequence may be represented as a view rather than a new logical Event Stream.

For example:

```text
Stream S1
    ↓
Filter F
    ↓
View V1
```

A view does not automatically create a new Event identity.

---

# 34. Stream Views

Future stream views may support:

```text
filtering
projection
mapping
partitioning
windowing
time ranges
sequence ranges
```

These are analytical/processing operations rather than changes to the underlying history.

---

# 35. Partitioning

A stream may be partitioned by an explicit key.

Examples:

```text
player_id
table_id
session_id
instrument_id
```

Partitioning must preserve the semantics required by consumers.

---

# 36. Partition Ordering

Ordering guarantees may apply:

```text
per stream
per partition
globally
```

These scopes must be explicit.

A per-partition sequence must not be interpreted as a global sequence.

---

# 37. Fan-Out

Multiple consumers may read the same Event Stream.

For example:

```text
             ┌→ Projection A
Event Stream ┼→ Projection B
             └→ Analysis C
```

Consumers must not mutate the historical stream through ordinary reads.

---

# 38. Fan-In

Multiple streams may be combined.

For example:

```text
Stream A ─┐
Stream B ─┼→ Merge
Stream C ─┘
```

A merged stream/view must define:

```text
ordering
identity
duplicate semantics
conflict handling
source attribution
```

It must not imply a globally correct ordering when none exists.

---

# 39. Stream Merge and Event Time

Merging streams by Event time does not guarantee causal ordering.

Events with equal or uncertain timestamps may remain partially ordered.

The resulting ordering contract must expose this limitation.

---

# 40. Stream Correlation

Streams may be correlated using domain identifiers.

For example:

```text
session_id
player_id
instrument_id
```

Correlation is not identity.

Two Events may correlate to the same entity while remaining independent Events.

---

# 41. Stream Provenance

An Event Stream should expose enough provenance to identify:

```text
source
ingestion process
normalization
stream construction
version
```

when the stream is derived rather than authoritative.

---

# 42. Derived Event Streams

A derived Event Stream may be produced from another stream.

For example:

```text
Raw Stream
    ↓
Normalization
    ↓
Normalized Event Stream
```

The derived stream must retain provenance to its source.

---

# 43. Event Stream as Source of Truth

A stream may be:

```text
authoritative
derived
cached
recoverable
ephemeral
```

according to the Storage Model.

An Event Stream being persistent does not automatically make it authoritative.

---

# 44. Stream Persistence

Persistence semantics are governed by the Persistence Interface Model.

A stream implementation may use:

```text
file
database
object storage
log
memory
remote service
```

without changing the logical Event Stream abstraction.

---

# 45. Stream Storage Key

Physical storage identifiers must remain separate from:

```text
EventStreamId
EventId
StreamPosition
```

A database primary key is not automatically a logical stream identity.

---

# 46. Stream Version

A stream definition may evolve.

Versioning may apply to:

```text
stream schema
membership rules
ordering contract
serialization
derived transformation
```

A semantic change must be versioned when required for compatibility.

---

# 47. Event Schema Version

Event schema version remains a property of Event representation/contract.

It must not be confused with Event Stream version.

For example:

```text
Stream version = 2
Event schema = 5
```

may be valid.

---

# 48. Stream Configuration

A stream may have configuration such as:

```text
retention
partitioning
ordering
deduplication
membership
```

Configuration must be explicit.

It must not be hidden in the stream implementation.

---

# 49. Stream Configuration and Reproducibility

When a derived stream depends on configuration, its provenance should identify the effective configuration when required for reproduction.

---

# 50. Stream Consistency

A reader must know what consistency guarantee it receives.

Possible semantics include:

```text
snapshot
latest committed
append-consistent
run-specific
versioned
```

The exact guarantees are implementation-specific but must be explicit.

---

# 51. Concurrent Append

Concurrent writers may append Events to the same stream only according to an explicit concurrency contract.

Possible semantics include:

```text
single writer
serialized writers
partitioned writers
externally ordered writers
```

The Core does not mandate a particular implementation.

---

# 52. Ordering Conflicts

If two writers provide incompatible ordering information, the stream must not silently fabricate a globally correct order.

Possible outcomes include:

```text
reject
partition
assign stream sequence
preserve partial ordering
require explicit merge policy
```

---

# 53. Backpressure

Backpressure belongs to the processing/delivery mechanism.

An Event Stream's logical existence does not imply a particular backpressure policy.

A live subscription may apply:

```text
BLOCK
BUFFER
DROP
SAMPLE
SPILL
```

according to the consumer contract.

Historical reads should not silently lose Events because a consumer is slow.

---

# 54. Delivery Semantics

Live Event Stream consumption may have:

```text
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

delivery semantics.

The stream abstraction does not automatically guarantee exactly-once delivery.

---

# 55. Consumer Position

A consumer may persist its position for recovery.

Conceptually:

```text
Consumer
    stream = S1
    position = 1000
```

Consumer position is execution state.

It is not part of the Event's logical identity.

---

# 56. Recovery

After failure, a consumer may resume from:

```text
last checkpoint
last acknowledged position
replayed position
stream beginning
```

according to the Recovery Model.

---

# 57. Unknown Completion

If a consumer fails after processing an Event but before recording completion, the Event may be processed again.

The stream must not assume exactly-once execution merely because the Event itself is unique.

---

# 58. Idempotent Consumers

Consumers requiring at-least-once delivery should define idempotency semantics.

Possible keys include:

```text
EventId
StreamPosition
domain-specific key
```

The correct key depends on the consumer's semantics.

---

# 59. Stream Errors

Expected stream operation failures should use the Result/Error model.

Examples:

```text
stream unavailable
invalid position
storage failure
unsupported operation
permission denied
corrupted record
```

An empty result must not silently represent an operational failure.

---

# 60. End-of-Stream

End-of-stream should be represented separately from failure.

For example:

```text
read:
    Event
    End
    Error
```

The exact C++ representation is deferred.

---

# 61. Cancellation

Long-running stream reads/subscriptions may be cancelled.

Cancellation is distinct from:

```text
stream error
end of stream
consumer failure
```

---

# 62. Stream Lifecycle

A logical Event Stream may conceptually move through:

```text
DEFINED
AVAILABLE
ACTIVE
CLOSED
FAILED
```

The exact lifecycle depends on the implementation.

A persistent historical stream may not require an operational lifecycle at all.

---

# 63. Stream Closure

Closure semantics must distinguish:

```text
historical stream completed
producer stopped
administrative closure
failure
```

A closed stream must not necessarily imply data corruption.

---

# 64. Event Stream API Shape

The initial conceptual API may resemble:

```cpp
namespace evolution::event
{

class EventStream;
class StreamPosition;
class StreamCursor;

}
```

The exact method set remains deferred.

---

# 65. Typed Event Streams

Where practical, consumers should be able to express the expected Event type:

```text
EventStream<PlayerAction>
```

or an equivalent strongly typed contract.

A universal untyped Event stream may still be required at generic ingestion or runtime extension boundaries.

---

# 66. Stream Read Contract

Conceptually:

```text
read:
    position
    limits
    ordering
    filtering
    cancellation
    consistency
```

must be explicit.

The API must not hide potentially expensive or blocking behavior behind an apparently trivial operation.

---

# 67. Stream Write Contract

Conceptually:

```text
append:
    Event
    durability requirement
    idempotency
    ordering
```

must be explicit where relevant.

---

# 68. Stream Query vs Stream Consumption

A historical range query and a live subscription are different operations.

```text
historical query:
    inspect existing history

subscription:
    continue receiving future Events
```

They may share implementation but must remain semantically distinct.

---

# 69. Stream and Query Model

The Query Model may expose Event Stream history through queries.

However:

```text
Query
    → asks for information

Event Stream
    → represents historical sequence
```

The query layer must not redefine stream semantics.

---

# 70. Stream and Storage

The Storage layer implements persistence capabilities required by Event Streams.

The Event Stream contract remains independent of the selected storage technology.

---

# 71. Stream and Projection

Projection is one of the primary consumers of Event Streams.

```text
Event Stream
     ↓
Projection
     ↓
State
```

The Projection must consume Events according to the stream's ordering and consistency guarantees.

---

# 72. Stream and Measurement

Measurements may be derived from Event Streams.

```text
Event Stream
     ↓
Measurement processor
     ↓
Measurement
```

Measurement semantics remain independent from stream mechanics.

---

# 73. Stream and Analysis

Analysis may consume:

```text
Events
States
Measurements
Time Series
Patterns
```

An Event Stream is therefore one possible analytical input.

---

# 74. Stream History vs State

An Event Stream contains historical observations.

State contains a derived representation.

The stream should not be reconstructed from State unless the domain explicitly defines State as an authoritative source.

---

# 75. Stream History vs Snapshot

A Snapshot is a State optimization.

It does not become an Event Stream merely because it is persisted.

---

# 76. Stream History vs Log

A physical append-only log may implement an Event Stream.

However, a generic log does not automatically satisfy the Event Stream contract.

The implementation must provide the required semantic guarantees.

---

# 77. Testing Requirements

Tests must cover:

```text
stream identity
scope
membership
ordering
sequence
append
duplicate handling
idempotency
historical immutability
range reads
cursor behavior
empty streams
end-of-stream
missing positions
filtering
partitioning
fan-out
merge
replay
live consumption
delivery semantics
consumer position
recovery
cancellation
provenance
serialization
version compatibility
```

---

# 78. Deferred Decisions

This ADR does not select:

* exact EventStream C++ API
* storage backend
* append-only log implementation
* database schema
* cursor implementation
* live subscription mechanism
* message broker
* partitioning mechanism
* watermark implementation
* late-event handling implementation
* exactly-once implementation
* distributed consensus mechanism
* retention implementation
* stream registry implementation
* query engine

---

# Decision Summary

```text
Event Stream:
    Logical ordered history of Events

Stream identity:
    Identifies logical stream

Event identity:
    Identifies Event

Stream position:
    Identifies Event location within stream

Ordering:
    Explicit

Event time:
    Not automatically total ordering

Storage order:
    Not semantic

History:
    Normally append-oriented

Correction:
    Explicit

Replay:
    Reprocesses history, does not create Events

Live subscription:
    Separate semantic operation

Consumer position:
    Execution/recovery state

Delivery:
    Explicit

Backpressure:
    Processing/delivery concern

Persistence:
    Storage concern

Provenance:
    Required for derived streams when material

State:
    Derived representation, not stream history

Query:
    Access mechanism, not stream semantics
```

## Invariant

**An Event Stream is a logical, explicitly ordered history of Events; its identity, ordering, positions, membership, delivery, persistence, and recovery semantics are distinct from Event identity, physical storage, processing queues, and execution state, allowing the same historical stream to support replay, projection, analysis, and other consumers without changing its meaning.**

