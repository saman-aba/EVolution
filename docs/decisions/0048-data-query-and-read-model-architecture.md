# ADR 0048: Data Query and Read Model Architecture

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution treats **querying** and **read models** as explicit architectural concerns rather than exposing storage implementations directly to applications, processors, or interfaces.

The architecture distinguishes:

```text
Stored Data
    ↓
Query
    ↓
Read Model / Result
    ↓
Application / Analysis / Interface
```

A query requests information according to explicit semantic requirements.

A read model is a representation optimized for one or more query patterns and may be:

* authoritative
* derived
* materialized
* cached
* temporary

A read model is not automatically the source of truth.

---

## 1. Query vs Storage

A query expresses what information is required.

Storage determines how that information is physically retained and retrieved.

Therefore:

```text
Query
    ≠
SQL statement
    ≠
Database API call
    ≠
File scan
```

Applications should depend on query contracts rather than physical storage mechanisms where practical.

---

## 2. Query Contract

A query contract should define, where relevant:

```text
Query
{
    input
    scope
    temporal semantics
    consistency
    ordering
    identity
    filtering
    pagination
    result shape
}
```

The exact representation remains implementation-dependent.

A query must have sufficient semantics for a caller to understand what information it is requesting.

---

## 3. Query Ownership

Queries belong to the layer that understands their semantic meaning.

For example:

```text
Generic storage query
    → Storage

Poker hand history query
    → Poker domain / application

Profitability analysis query
    → Analysis / application
```

Core may provide generic query mechanisms, but must not encode domain-specific query meaning.

---

## 4. Query Types

Queries may return:

```text
Single object
Collection
Ordered sequence
Time range
Aggregation
Snapshot
Materialized read model
Streaming result
Reference / handle
```

The result shape must be explicit.

A query returning zero results is not automatically an error.

For example:

```text
Query:
    Find hand by HandId

Result:
    FOUND
    NOT_FOUND
    ERROR
```

`NOT_FOUND` is distinct from operational failure.

---

## 5. Query Consistency

Queries may require different consistency semantics.

Conceptually:

```text
LATEST
SNAPSHOT
VERSIONED
RUN_SPECIFIC
EVENT_TIME
```

### LATEST

Return the latest available representation according to the query's data source.

### SNAPSHOT

Return information from a defined snapshot.

### VERSIONED

Return a specific version of the requested information.

### RUN_SPECIFIC

Return information produced by a particular execution/run.

### EVENT_TIME

Select information according to domain/event temporal semantics.

The exact consistency mechanisms remain implementation-dependent.

---

## 6. Temporal Query Semantics

Temporal queries must explicitly identify which time dimension they use.

Possible dimensions include:

```text
Event Time
Ingestion Time
Processing Time
Measurement Time
Storage Time
```

The query must not silently substitute one for another.

For example:

```text
events between T1 and T2
```

must specify whether this means:

```text
event_time ∈ [T1, T2)
```

or:

```text
ingestion_time ∈ [T1, T2)
```

The default interval convention is:

```text
[start, end)
```

unless a domain contract explicitly requires another boundary convention.

---

## 7. Unknown and Estimated Time

Queries must preserve the temporal knowledge states defined by the Time Model:

```text
KNOWN
ESTIMATED
UNKNOWN
```

A query over event time must not silently convert unknown event time into ingestion time.

Estimated timestamps remain estimated.

For example:

```text
Event:
    event_time = estimated T
    ingestion_time = T2
```

must not become:

```text
event_time = T2
```

merely because the query requires an event-time index.

The query contract must define how unknown or estimated temporal information is handled.

---

## 8. Ordering

Query results require explicit ordering semantics when ordering matters.

Possible ordering dimensions include:

```text
Domain Sequence
Event Time
Measurement Time
Processing Sequence
Logical Identity
Storage Order
```

Storage order must not automatically become semantic ordering.

For example:

```text
database insertion order
```

does not imply:

```text
event occurrence order
```

unless the persistence contract explicitly establishes that relationship.

---

## 9. Deterministic Ordering

If a query promises deterministic ordering, ties must have explicit semantics.

For example:

```text
ORDER BY event_time
```

may be insufficient when multiple events have equal timestamps.

The query may additionally require:

```text
event_time
+
sequence
+
identity
```

or another domain-defined ordering relation.

The ordering contract must be stable across supported implementations when deterministic ordering is promised.

---

## 10. Filtering

Filtering criteria should be semantic where possible.

Examples:

```text
Event type
Scope
Entity identity
Time range
Metric
Dimension
Version
Provenance
```

A storage implementation may translate these criteria into indexes, predicates, scans, or other mechanisms.

The physical query representation must not become the application-level contract.

---

## 11. Query Scope

Queries should define their scope explicitly.

Possible scopes include:

```text
Global
Application
Session
Entity
Event Stream
Time Range
Processing Run
Graph Execution
Dataset
```

Scope affects both correctness and performance.

A query must not silently broaden its scope because a storage backend cannot efficiently support the requested restriction.

---

## 12. Pagination

Large query results may require pagination.

Pagination semantics must define:

* page size
* continuation mechanism
* ordering
* consistency
* snapshot/version relationship
* expiration
* duplicate/omission behavior

Offset-based pagination is not universally reliable when the underlying dataset changes.

Cursor-based pagination may provide stronger semantics, but no particular mechanism is selected by this ADR.

---

## 13. Streaming Queries

Queries may produce streaming results.

A streaming query must define:

```text
Ordering
Completion
Cancellation
Failure
Backpressure
Delivery semantics
```

The fact that results are delivered incrementally does not make the query a processor.

Streaming query infrastructure must follow the relevant processing and interface contracts where applicable.

---

## 14. Query Result Ownership

Returned query data must follow explicit ownership semantics.

The caller must know whether the result is:

```text
Owned
Borrowed
Shared immutable
Reference / handle
Stream
```

A query result must not expose storage-owned memory beyond its promised lifetime.

Zero-copy query results are permitted only where their lifetime and ownership semantics are explicit.

---

## 15. Query and Read Models

A read model is a representation optimized for querying.

For example:

```text
Authoritative Events
        ↓
Projection
        ↓
Read Model
        ↓
Query
```

The read model may contain:

* indexes
* denormalized fields
* precomputed measurements
* aggregated values
* materialized relationships

The read model's existence does not make it authoritative.

---

## 16. Authoritative vs Derived Read Models

Every important read model should have a declared role:

```text
AUTHORITATIVE
DERIVED
CACHE
RECOVERABLE
EPHEMERAL
```

A derived read model must identify its source and derivation when required.

For example:

```text
Event Stream
    ↓
State Projection
    ↓
Materialized TableState
```

The materialized state may be reconstructed from its authoritative inputs.

---

## 17. Read Model Provenance

A derived read model should preserve enough provenance to determine:

```text
What produced it?
Which input data was used?
Which version of the projection was used?
Which configuration was used?
Which processing run produced it?
```

This is especially important for analytical results.

A read model that cannot be reconciled with its source may need to be rebuilt rather than trusted automatically.

---

## 18. Read Model Freshness

Derived read models may lag behind authoritative data.

Freshness must therefore be explicit when relevant.

Conceptually:

```text
Authoritative Position
        ↓
Processed Position
        ↓
Read Model Position
```

The system should not present stale derived information as current when freshness is semantically important.

Possible freshness representations include:

```text
Source position
Last processed event
Last update time
Version
Lag
```

These are operational or semantic metadata depending on the contract.

---

## 19. Read Model Consistency

A read model may temporarily be inconsistent with its source while being updated.

The architecture must define whether a query may observe:

```text
Fully consistent state
Eventually consistent state
Snapshot-consistent state
Version-consistent state
```

The query contract determines what is acceptable.

Consistency must not be inferred merely from the fact that the implementation uses a database.

---

## 20. Querying Derived Measurements

Measurements and time series may be queried directly.

For example:

```text
Metric
    ↓
Measurement Series
    ↓
Time Range Query
```

The query must preserve:

* metric identity
* scope
* dimensions
* temporal semantics
* units
* provenance
* version where relevant

A numeric value without its metric identity and units is not a sufficient analytical result when those semantics matter.

---

## 21. Querying Aggregated Data

Aggregated data must identify the aggregation semantics when required.

For example:

```text
Average over 1 hour
```

is not equivalent to:

```text
Average of 1-minute averages
```

unless the aggregation semantics make them equivalent.

Queries over pre-aggregated read models must therefore preserve:

* aggregation function
* input scope
* window
* grouping
* missing-data policy
* version/configuration where material

---

## 22. Query vs Analysis

A query retrieves or derives information according to an established query contract.

Analysis interprets information.

For example:

```text
Query:
    Retrieve all hands from session S.

Analysis:
    Determine whether aggression increased during the session.
```

A query should not silently become an analytical inference.

Likewise, an analysis should not bypass query/storage contracts merely to obtain data.

---

## 23. Query vs Aggregation

A query may request an aggregation:

```text
average(metric, window)
```

but the aggregation semantics remain explicit.

The query is the request.

The aggregation is the transformation.

They are therefore separate concepts.

---

## 24. Query vs Application Operation

A query is an Application Operation when exposed through an application boundary.

For example:

```text
External Client
    ↓
Query Operation
    ↓
Application
    ↓
Query
    ↓
Read Model / Storage
```

The transport protocol must not define the query's semantic meaning.

---

## 25. Query Authorization

Queries may require authorization.

Authorization must consider:

* principal
* requested scope
* resource identity
* sensitivity
* tenant/domain boundaries
* requested operation

Authorization failure is distinct from:

```text
NOT_FOUND
```

when the security contract requires that distinction.

The interface/application layer may intentionally hide resource existence where security policy requires it.

---

## 26. Query Caching

Query results may be cached.

A cache must preserve the semantics of the query.

A cache key may need to include:

```text
Query identity
Query parameters
Scope
Version
Consistency requirement
Temporal range
Authorization context
```

Caching must not silently return information from:

* the wrong user
* the wrong version
* the wrong time range
* the wrong consistency level
* an unauthorized scope

A cache remains a cache unless explicitly promoted to an authoritative source.

---

## 27. Query and Reproducibility

Queries that materially affect analytical results participate in reproducibility.

For example:

```text
Analysis
    ↓
Query Definition
    ↓
Dataset Version
    ↓
Result
```

The system should be able to identify the relevant query semantics when reproduction requires them.

A query's physical SQL statement or storage-specific representation is not necessarily part of semantic reproducibility.

---

## 28. Query Errors

Queries use the existing Result/Error model.

Possible outcomes include:

```text
SUCCESS
NOT_FOUND
INVALID_INPUT
INVALID_SCOPE
UNAVAILABLE
TIMEOUT
CANCELLED
STORAGE_FAILURE
RESOURCE_EXHAUSTED
UNAUTHORIZED
```

The exact error code belongs to the appropriate abstraction boundary.

Human-readable messages are not the query contract.

---

## 29. Query Cancellation and Deadlines

Long-running queries must support cancellation where their contract requires it.

Cancellation is distinct from failure.

A deadline is an execution constraint, not domain time.

For example:

```text
Query deadline exceeded
```

must not modify:

```text
event_time
measurement_time
```

or any other domain temporal value.

---

## 30. Query Resource Limits

Queries may consume significant:

* CPU
* memory
* storage I/O
* network bandwidth
* database connections
* execution slots

Query resource limits must be explicit.

Resource exhaustion must not silently truncate a result and report success unless truncation is explicitly part of the query contract.

If a query intentionally returns partial results, that fact must be represented explicitly.

---

## 31. Query and Storage Capabilities

Not every storage implementation must support every query capability.

A storage implementation may declare capabilities such as:

```text
Point Lookup
Range Scan
Ordered Scan
Aggregation
Streaming
Snapshot Reads
Versioned Reads
Transactions
```

The application or storage abstraction must determine whether the requested query can be fulfilled.

Unsupported queries should produce explicit failure rather than silently changing semantics.

---

## 32. Query Planning

Query planning may optimize how a query is executed.

Conceptually:

```text
Semantic Query
      ↓
Query Planning
      ↓
Physical Retrieval Plan
      ↓
Execution
      ↓
Semantic Result
```

The physical plan is not part of the semantic query.

Optimization must preserve query semantics.

Different storage implementations may therefore use completely different plans for the same query.

---

## 33. Materialization

A query or analytical result may be materialized for repeated use.

Materialization creates a derived object with its own:

* identity
* provenance
* version
* freshness
* lifecycle
* retention semantics

Materialization must not silently become authoritative.

---

## 34. Read Model Rebuilding

A derived read model should be rebuildable when its source and derivation information are retained.

Conceptually:

```text
Authoritative Data
      +
Projection Version
      +
Configuration
      +
Required Context
      ↓
Rebuild Read Model
```

A rebuild must not silently use a different semantic version if historical equivalence is required.

---

## 35. CQRS

This ADR does not require strict CQRS.

Commands and queries remain semantically distinct according to ADR 0039, but they may share:

* processes
* storage
* data models
* code
* infrastructure

Physical separation is optional.

---

## 36. Event Sourcing

This ADR does not require event sourcing.

An event stream may be authoritative for some domain/application, while another domain may use:

* snapshots
* source datasets
* relational state
* append-only measurements
* external records

The query architecture must work with the declared source-of-truth model.

---

## 37. Deferred Decisions

This ADR does not select:

* SQL
* NoSQL
* SQLite
* PostgreSQL
* time-series database
* columnar database
* query language
* ORM
* CQRS framework
* event-sourcing framework
* cache implementation
* pagination mechanism
* query planner
* indexing technology
* materialized-view mechanism
* distributed query engine

These remain implementation decisions.

---

## Decision Summary

```text
Query:                         Explicit semantic request
Storage:                       Physical persistence mechanism
Read model:                    Query-optimized representation
Read model authority:          Explicit
Consistency:                   Explicit
Temporal semantics:             Explicit
Ordering:                      Explicit
Pagination:                    Contract-defined
Streaming:                     Contract-defined
Ownership:                     Explicit
Freshness:                     Explicit when relevant
Provenance:                    Required for derived results when material
Caching:                       Allowed, semantics-preserving
Query vs analysis:             Distinct
Query vs aggregation:          Distinct
Query vs application operation: Distinct but composable
Authorization:                 Explicit
Cancellation:                  Distinct from failure
Resource limits:               Explicit
CQRS:                          Not required
Event sourcing:                Not required
Physical query mechanism:      Deferred
```

## Invariant

**EVolution treats queries as explicit semantic requests for information and read models as replaceable representations optimized for those requests; query semantics, temporal meaning, ordering, consistency, ownership, authorization, freshness, and provenance must remain explicit and must not be defined accidentally by the underlying storage technology.**

