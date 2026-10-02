# Storage and Persistence Model

## 1. Purpose

The Storage Model defines what information EVolution may retain, what can be reconstructed, and the conceptual distinction between persistent data and temporary execution state.

The model must remain independent of a specific storage technology.

At this stage, EVolution does not require a decision between:

```text
database
filesystem
embedded database
binary files
object storage
memory
```

Those are implementation decisions.

The architectural question is:

> **What information must exist beyond the lifetime of a processing operation?**

---

## 2. Persistence Is Not a Single Category

Different information has different persistence requirements.

A conceptual classification is:

```text
Source Data
    ↓
Historical Record
    ↓
Derived Data
    ↓
Cache
    ↓
Execution State
```

These categories should not be treated as interchangeable.

---

## 3. Source Data

Source data is information entering EVolution from outside the analytical system.

Examples:

```text
hand history
network capture
market feed
sensor data
simulation input
imported dataset
user input
```

Source data is important because it establishes the origin of the analytical process.

If source data is retained, it can potentially support:

```text
replay
reconstruction
reprocessing
algorithm comparison
validation
audit
```

Whether all source data must be retained depends on the application.

---

## 4. Events as Historical Records

Events are the fundamental historical representation of what happened.

If events are retained, many higher-level representations can potentially be reconstructed:

```text
Events
   ↓
State
   ↓
Measurements
   ↓
Patterns
   ↓
Analysis
```

This creates an important architectural property:

> **Derived information should not automatically become the only surviving representation of historical information.**

For example, storing only:

```text
win_rate = 7.2 BB/100
```

loses information that may be required later to calculate a different metric.

If the underlying events remain available, the metric can potentially be recalculated.

---

## 5. Event Retention

Event retention is therefore an architectural choice.

Possible models include:

```text
retain all events
retain events for a limited period
retain selected events
retain compressed events
retain only external source data
```

The conceptual architecture should support event retention without requiring that every deployment retain everything forever.

---

## 6. State Persistence

State can be persisted as an optimization.

For example:

```text
Events 1..1,000,000
        ↓
    State Snapshot
        ↓
Events 1,000,001..
```

Instead of replaying one million events, the system can restore the snapshot and replay only the remaining events.

A snapshot is therefore:

```text
checkpoint
```

rather than necessarily:

```text
authoritative historical source
```

The distinction is important.

---

## 7. Snapshot

A snapshot represents the state of a projection at a particular point in the input history.

Conceptually:

```text
Snapshot
{
    projection
    state
    source_position
    version
    provenance
}
```

`source_position` identifies the point in the event history from which the state was derived.

A snapshot without a source position is insufficient for reliable replay.

---

## 8. Derived Data

Derived data includes information calculated from other information.

Examples:

```text
measurements
time series
aggregations
patterns
analyses
decisions
```

Derived data may be:

```text
recomputed
cached
persisted
discarded
materialized
```

The correct choice depends on computational cost, retention requirements, and reproducibility requirements.

---

## 9. Recomputable Data

If a result can be deterministically reconstructed from retained inputs and processing definitions, it may not need to be treated as authoritative data.

For example:

```text
Events
   +
Metric Definition
   ↓
Win Rate
```

If events and the metric definition remain available, the win rate can potentially be recalculated.

This creates a useful distinction:

```text
Authoritative input
```

versus:

```text
Materialized result
```

A materialized result may still be persisted for performance.

Its existence does not necessarily make it the source of truth.

---

## 10. Materialized Data

A materialized result is a persisted representation of derived information.

For example:

```text
Events
   ↓
Measurements
   ↓
Aggregated Time Series
   ↓
Persisted Result
```

Materialization can reduce computation during repeated queries.

However, materialized data must retain sufficient provenance to determine:

```text
what produced it
which inputs were used
which configuration was used
which version produced it
```

Otherwise it becomes difficult to determine whether it is still valid.

---

## 11. Cache

A cache is temporary or replaceable data retained primarily to improve performance.

Examples:

```text
recent measurements
decoded objects
query results
computed patterns
loaded state
```

A cache should not normally be treated as the authoritative source.

Conceptually:

```text
Authoritative Data
       ↓
     Cache
```

If the cache is deleted:

```text
Cache
   X
```

the system should remain conceptually correct, even if reconstruction requires additional computation.

---

## 12. Execution State

Execution state belongs to the running processing system rather than the analytical data model.

Examples:

```text
processor buffers
worker state
temporary queues
open connections
in-memory indexes
partial calculations
```

Execution state may disappear when the process stops.

This does not necessarily imply data loss.

For example:

```text
Event History
    ↓
Processor
    ↓
Temporary State
```

The temporary state can potentially be reconstructed from the event history.

---

## 13. Persistence Levels

Different components may require different persistence guarantees.

A conceptual classification is:

```text
EPHEMERAL
    exists only during execution

CACHE
    replaceable optimization

RECOVERABLE
    persisted so execution can resume

DERIVED
    persisted result that can be recomputed

AUTHORITATIVE
    source that defines historical truth
```

A particular object may have different persistence behavior in different applications.

---

## 14. Source of Truth

Every persistent object should have a clear answer to:

> Is this authoritative, or can it be reconstructed?

For example:

```text
Event
    authoritative: yes

State Snapshot
    authoritative: no

Cached Measurement
    authoritative: no

Original Dataset
    authoritative: potentially yes
```

This prevents multiple representations from silently becoming competing sources of truth.

---

## 15. Persistence and Provenance

Persistence and provenance are closely related.

A persisted derived result should be able to identify its derivation.

For example:

```text
Persisted Pattern
    ↓
derived from:
    Time Series 42
    Detector v3
    Configuration 17
```

Without provenance, persisted analytical results become difficult to validate.

---

## 16. Storage Identity

Persisted objects require stable identity.

For example:

```text
Event ID
Measurement ID
Time Series ID
Pattern ID
Analysis ID
```

Identity should remain independent from physical storage location.

For example:

```text
measurement-123
```

should identify the same logical measurement regardless of whether it is stored in:

```text
memory
file
database
remote storage
```

This keeps the conceptual model independent of storage technology.

---

## 17. Storage Location vs Logical Identity

Logical identity and physical location are different concepts.

```text
Logical object:
    Measurement 123

Physical representation:
    database row
    file record
    memory object
```

The architecture should avoid exposing physical storage details to analytical components unless required.

---

## 18. Persistence and Versioning

Persisted derived data may become invalid when its inputs or producing algorithm change.

For example:

```text
Events v1
Metric v1
    ↓
Measurement A
```

Later:

```text
Metric v2
    ↓
Measurement B
```

Both may be valid historical results.

They should not silently overwrite each other while appearing to be the same analytical result.

Version information should therefore participate in the identity or provenance of derived data where required.

---

## 19. Incremental Persistence

Streaming systems may persist data incrementally.

For example:

```text
Event
   ↓
Process
   ↓
Persist
```

This reduces the amount of information lost if the process terminates unexpectedly.

However, persistence frequency introduces a tradeoff between:

```text
durability
```

and:

```text
processing overhead
```

The exact durability strategy is an implementation decision.

---

## 20. Checkpointing

Long-running processing may periodically create checkpoints.

Conceptually:

```text
Events
  ↓
Processor
  ↓
Checkpoint
  ↓
More Events
  ↓
Checkpoint
```

A checkpoint should identify the input position associated with the stored execution state.

For example:

```text
checkpoint:
    processor = player-state
    input_position = event-184203
    state_version = 12
```

This allows processing to resume without replaying the entire history.

---

## 21. Recovery

Recovery means restoring processing after interruption.

A conceptual recovery sequence is:

```text
Persisted State
      ↓
Restore
      ↓
Input Position
      ↓
Replay Remaining Input
      ↓
Continue Processing
```

Recovery should preserve the semantic result that would have been obtained from uninterrupted processing, subject to explicitly documented limitations.

---

## 22. Deletion

Deletion has different meanings depending on the object.

Deleting:

```text
Cache
```

normally removes only an optimization.

Deleting:

```text
Event History
```

may make historical reconstruction impossible.

Deleting:

```text
Materialized Analysis
```

may be harmless if it can be reproduced.

Therefore deletion semantics should depend on the object's persistence classification.

---

## 23. Retention

Retention defines how long information remains available.

Possible retention policies include:

```text
forever
fixed duration
fixed number of observations
until replaced
until manually deleted
application-defined
```

Retention should be treated independently from storage technology.

For example:

```text
Event History:
    retain indefinitely

Execution Logs:
    retain 30 days

Cache:
    retain until invalidated
```

---

## 24. Storage and Reproducibility

The ability to reproduce an analysis depends on what information has been retained.

A minimal conceptual chain is:

```text
Source Data
    +
Transformation
    +
Configuration
    +
Version
    ↓
Reproducible Result
```

If source data is deleted, the result may no longer be reproducible even if the result itself remains stored.

Therefore retention decisions can directly affect analytical reproducibility.

---

## 25. Storage and Querying

Storage exists partly to support retrieval.

Different access patterns may require different physical representations.

Examples:

```text
retrieve event by ID
retrieve events by time range
retrieve measurements for a player
retrieve time series
retrieve all patterns in a period
retrieve provenance graph
```

The conceptual model should describe these logical operations without prematurely selecting a storage technology.

---

## 26. Storage Is Not Processing

Storage answers:

> Where does information remain?

Processing answers:

> How is information transformed?

For example:

```text
Storage:
    Event History

Processing:
    Event → Measurement

Storage:
    Measurement Series
```

These responsibilities should remain separate.

A storage component should not silently define analytical semantics.

---

## 27. Storage Is Not the Event Model

An event may be stored in:

```text
file
database
memory
network stream
```

without changing what the event means.

Therefore:

```text
Event Model
```

and:

```text
Storage Model
```

must remain separate architectural concerns.

---

## 28. Storage Abstraction

The architecture should conceptually allow different storage implementations behind a common logical boundary.

For example:

```text
             ┌── File Storage
             │
Logical Data ├── Database Storage
             │
             ├── Memory Storage
             │
             └── Remote Storage
```

The analytical layer should depend on logical storage capabilities rather than a particular storage implementation where practical.

---

## 29. Storage Capabilities

Different storage implementations may support different capabilities.

For example:

```text
read
write
append
update
delete
query
range_scan
snapshot
transaction
stream
```

A component should depend only on the capabilities it actually requires.

For example, an event ingestion component may only require:

```text
append
```

while an analysis application may require:

```text
range_scan
query
```

This avoids forcing every storage implementation to support every possible operation.

---

## 30. Core Architectural Invariant

The storage layer defines how information is retained and retrieved.

It should not define:

* what an event means
* how a measurement is calculated
* what a pattern means
* how analysis is performed
* which decision should be made

The fundamental separation is:

```text
Semantic Model
       ↓
Processing Model
       ↓
Persistence
```

Therefore:

> **Persistent data should have a clear logical identity, retention semantics, and provenance, while physical storage remains an implementation concern.**
