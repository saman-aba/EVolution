# ADR 0039 — Command and Query Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0038 established Application Operations as the semantic boundary between external invocation and application behavior.

Application operations fall broadly into:

```text
Command
Query
```

The distinction is useful because commands and queries have different semantic expectations around:

```text
state change
side effects
idempotency
consistency
caching
retries
authorization
completion
```

However, adopting the command/query distinction does not require adopting a complete CQRS architecture, separate databases, message buses, or event sourcing.

EVolution needs the semantic distinction while keeping the physical implementation replaceable.

## Decision

EVolution distinguishes **Commands** and **Queries** as semantic categories of Application Operations.

```text
Application Operation
├── Command
└── Query
```

The distinction is based on intended semantic behavior rather than implementation technology.

```text
Command:
    requests an action or state transition

Query:
    requests information
```

EVolution does **not** require a strict CQRS architecture.

## Command

A Command represents a request to perform an application action.

Conceptually:

```text
Command
{
    type
    input
    context
    identity?
}
```

Examples:

```text
ImportDataset
StartProcessing
StopProcessing
RunAnalysis
CreateGraph
DeleteDerivedData
```

These are illustrative application operations.

## Query

A Query represents a request to retrieve information.

Conceptually:

```text
Query
{
    type
    input
    context
    consistency?
}
```

Examples:

```text
GetAnalysis
GetMeasurement
GetEvent
GetProcessingStatus
ListMetrics
QueryTimeSeries
```

## Semantic Difference

The fundamental distinction is:

```text
Command:
    "perform this"

Query:
    "tell me this"
```

A command may change application/domain state or cause external side effects.

A query should not intentionally cause semantic state changes.

## Query Implementation Effects

Queries may still cause implementation-level effects.

For example:

```text
Query
    ↓
cache miss
    ↓
cache population
```

This does not make the operation a command because cache population is not part of the requested semantic behavior.

Likewise:

```text
Query
    ↓
metrics
    ↓
trace
```

does not make the query a command.

## Command Side Effects

Commands may have side effects.

Examples:

```text
write storage
start processing
modify state
send external message
create analysis
```

Side effects must be documented where they matter.

## Query Purity

A query is not required to be mathematically pure.

For example, querying a live system may depend on current state.

The important property is:

> A query does not intentionally request a semantic state transition.

## Command Result

A command may return:

```text
void
identifier
created object
operation handle
result
```

depending on its contract.

Examples:

```text
CreateGraph
    → GraphId

StartAnalysis
    → OperationId

DeleteAnalysis
    → void
```

## Query Result

A query returns information.

Examples:

```text
GetAnalysis
    → Analysis

GetMeasurement
    → Measurement

QueryTimeSeries
    → TimeSeries
```

The result may be:

```text
single value
collection
stream
snapshot
reference
```

according to the query contract.

## Result vs No Result

A successful command may legitimately return no semantic value.

Conceptually:

```text
Result<void>
```

This does not mean the command failed.

Similarly, a query may legitimately return no matching object.

That must be distinguished from query failure.

For example:

```text
Query:
    object not found

Result:
    successful query
    no value
```

versus:

```text
Query:
    storage unavailable

Result:
    Error(StorageUnavailable)
```

## Optional Result vs Error

The distinction follows the `Result<T>` model.

Conceptually:

```text
Result<Optional<T>>
```

may be appropriate when:

```text
operation succeeded
and
there may or may not be a matching value
```

It should not be used merely to hide operational failure.

## Command Validation

Commands should be validated before execution where possible.

Validation may include:

```text
input validity
application state
resource requirements
authorization
domain preconditions
component availability
```

Validation does not guarantee successful execution.

A dependency may still fail during execution.

## Query Validation

Queries may validate:

```text
input format
requested scope
time range
pagination
requested fields
authorization
resource limits
```

Query validation must not silently change the requested semantics.

## Preconditions

Commands may define explicit preconditions.

For example:

```text
ActivateGraph
    requires:
        graph exists
        graph validated
        required processors initialized
```

If a precondition is not satisfied:

```text
Error(InvalidState)
```

or another appropriate error should be returned.

## Postconditions

Commands may define postconditions.

For example:

```text
CreateGraph
    postcondition:
        GraphId refers to a created graph
```

Postconditions should describe semantic results, not implementation details.

## Query Consistency

Queries may require explicit consistency semantics.

Possible conceptual levels include:

```text
LATEST
SNAPSHOT
VERSIONED
RUN_SPECIFIC
EVENT_TIME
```

No universal consistency level is selected.

## Query Snapshot

A query may explicitly request a consistent snapshot.

For example:

```text
QueryTimeSeries
    snapshot = S42
```

All returned observations then correspond to that declared snapshot.

The exact snapshot mechanism is deferred.

## Query Temporal Semantics

A query involving time must explicitly identify the relevant temporal dimension.

For example:

```text
event time
measurement time
ingestion time
processing time
```

A query must not silently substitute one temporal dimension for another.

## Time Range Queries

Time ranges should use explicit boundary semantics.

Preferred conceptual representation:

```text
[start, end)
```

This avoids ambiguity when adjacent ranges are queried.

## Query Ordering

Queries returning multiple results must define ordering when ordering matters.

Possible semantics include:

```text
identity order
event sequence
event time
measurement time
storage order
```

Storage order must not automatically become semantic order.

## Query Pagination

Pagination is a query concern.

A paginated query should define:

```text
ordering
cursor semantics
page size
continuation
consistency
```

The physical pagination mechanism remains deferred.

## Query Streaming

A query may return a stream when the result set is large or naturally unbounded.

A streaming query must define:

```text
ordering
completion
cancellation
backpressure
delivery
failure
```

A stream is not automatically equivalent to a fully materialized collection.

## Command Streaming

A command may itself consume a stream.

For example:

```text
ImportEvents
    ← Event Stream
```

The command then needs explicit semantics for:

```text
admission
partial processing
completion
cancellation
failure
delivery
```

The command/query distinction does not depend on whether the input or output is streamed.

## Long-Running Commands

Commands may initiate long-running operations.

For example:

```text
RunHistoricalAnalysis
    ↓
OperationId
```

The command may complete when the operation is accepted rather than when the analysis finishes.

The command contract must explicitly define what "successful completion" means.

## Command Acceptance

A command request can have several stages:

```text
Received
    ↓
Validated
    ↓
Admitted
    ↓
Accepted
    ↓
Executed
```

These stages must not be conflated.

For example:

```text
Accepted
```

does not necessarily mean:

```text
Completed
```

## Command Rejection

A command may be rejected before execution.

Examples:

```text
invalid input
invalid state
resource exhausted
unsupported operation
unauthorized
dependency unavailable
```

The rejection is distinct from execution failure.

## Command Failure

A command can be accepted and then fail.

For example:

```text
Command accepted
    ↓
Storage failure
    ↓
Command failed
```

The error should identify the relevant failure without pretending that the command was never accepted.

## Command Cancellation

A command may support cancellation.

Cancellation must be explicit in its contract.

For a long-running command:

```text
Command
    ↓
Operation
    ↓
Cancel
```

The resulting operation state must distinguish:

```text
CANCELLED
```

from:

```text
FAILED
```

## Command Idempotency

Commands may define idempotency.

For example:

```text
CreateAnalysis(idempotency_key)
```

may guarantee that repeated equivalent requests do not create multiple logical analyses.

The exact guarantee must be documented.

## Idempotency Scope

An idempotency key has a scope.

Conceptually:

```text
idempotency key
+
operation type
+
application scope
```

may define uniqueness.

The scope must not be assumed universal.

## Idempotency and Result Reuse

When a duplicate command is received, the application may:

```text
return previous result
return existing object identity
return current operation status
reject duplicate
```

The chosen behavior belongs to the command contract.

## Idempotency and Failure

A failed command does not automatically imply that a retry is safe.

For example:

```text
external side effect
    ↓
unknown completion
```

may require explicit recovery semantics before retrying.

## Query Caching

Queries may be cached where appropriate.

Caching must preserve declared query semantics.

A cache must not silently change:

```text
consistency
temporal scope
authorization
version
identity
```

## Cache Invalidation

Cache invalidation is an implementation concern unless cache freshness is itself part of the query contract.

If freshness affects semantic correctness, it must be explicit.

For example:

```text
Query:
    data no older than 5 seconds
```

is a semantic requirement.

## Query Authorization

Queries may require authorization.

Authorization should be evaluated according to the application's security model.

A query should not expose information merely because it is technically readable from storage.

## Command Authorization

Commands may require stronger authorization because they may change state or cause external effects.

Authorization remains separate from:

```text
input validation
domain validation
resource availability
```

## Command and Domain Events

A command may cause domain events.

For example:

```text
PlaceAction
    ↓
Domain validation
    ↓
PlayerAction event
```

The command is the request.

The event is the resulting historical fact.

The application must not claim that a command succeeded merely because it received the command.

## Command and Event Identity

These identities remain distinct:

```text
CommandId
EventId
OperationId
RunId
```

A relationship may exist between them through provenance or correlation.

One must not be substituted for another.

## Query and Event History

A query may retrieve historical events.

For example:

```text
QueryEvents
```

The query itself is not a domain event.

Reading an event does not create another copy of the historical fact unless the application explicitly defines such behavior.

## Command and Processing Graphs

A command may control a processing graph.

Examples:

```text
CreateGraph
ValidateGraph
ActivateGraph
StopGraph
```

The command invokes graph-level capabilities.

It must not bypass graph lifecycle contracts.

## Query and Processing Graphs

A query may inspect:

```text
graph definition
graph status
processor status
queue status
```

Operational status is not automatically domain state.

## Command and Storage

Commands may request persistence.

For example:

```text
PersistEvents
```

Storage determines persistence semantics.

The command determines application intent.

## Query and Storage

Queries may retrieve persisted information.

Storage ordering, physical schema, and backend-specific behavior must not silently redefine semantic query results.

## Query and Derived Data

A query may request derived data such as:

```text
measurement
time series
pattern
analysis
```

The result should carry sufficient identity/provenance according to its object contract.

## Querying Recomputable Data

If requested derived data is not persisted, an application may reconstruct it.

The query contract should make reconstruction semantics explicit where it affects:

```text
latency
consistency
algorithm version
result reproducibility
```

## Query and Reproducibility

Two identical queries may produce different results if their underlying inputs or versions differ.

Therefore reproducibility may require identifying:

```text
input state/version
configuration
algorithm version
execution context
```

## Query and Current State

A query for current state is inherently time-dependent.

For example:

```text
GetCurrentTableState
```

should not be interpreted as a historical snapshot unless explicitly requested.

## Historical Queries

A historical query may specify:

```text
state at event sequence N
state at timestamp T
state for run R
state for version V
```

The chosen temporal semantics must be explicit.

## Query Unknown Time

Queries involving event time must preserve the distinction:

```text
KNOWN
ESTIMATED
UNKNOWN
```

An unknown event timestamp must not be silently replaced with ingestion time.

## Query Results and Provenance

Results may need to identify:

```text
source
input version
algorithm
configuration
calculation time
```

when the result is derived.

The query itself does not automatically become part of analytical provenance.

## Query and Observability

Operational queries such as:

```text
GetProcessorHealth
GetQueueDepth
GetApplicationStatus
```

return observability information.

They should not automatically create analytical Measurements.

## Operational Query vs Analytical Query

The same underlying infrastructure may support both.

The distinction is semantic:

```text
Operational Query:
    "Is the processor healthy?"

Analytical Query:
    "What was the player's average aggression?"
```

Operational data should not silently become domain data.

## Commands and Transactions

A command may require atomicity.

For example:

```text
CreateAnalysis
```

may require:

```text
analysis record
+
provenance record
```

to be committed consistently.

The application may require a storage transaction or another mechanism if the storage contract supports it.

No universal transaction model is implied.

## Commands and Compensation

When a command spans components without a common transaction, failure may require compensation.

For example:

```text
external effect A
    ↓
external effect B
    ↓
B fails
```

The application may need explicit compensation.

Compensation is application policy, not automatic transaction rollback.

## Commands and External Effects

A command that sends an external message must define relevant delivery semantics.

For example:

```text
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

where applicable.

Exactly-once must never be assumed merely because the command returned successfully.

## Queries and External Systems

A query may retrieve information from an external system.

External availability and consistency then become part of the query's error/consistency contract.

## Command and Resource Limits

Commands may be subject to application-level limits:

```text
maximum concurrent jobs
maximum input size
maximum execution duration
maximum memory
```

Resource exhaustion should produce an explicit outcome.

## Query Resource Limits

Queries may also be limited:

```text
maximum result size
maximum scan range
maximum execution time
maximum concurrent queries
```

The application may reject or constrain queries according to explicit policy.

## Command Priority

Some applications may require command priority.

Priority is not automatically semantic.

If priority affects observable correctness, it must be part of the operation contract.

Otherwise it remains an execution concern.

## Query Priority

Likewise, query priority may be an execution concern unless it affects semantic behavior.

## Ordering Between Commands

Applications may require explicit ordering.

For example:

```text
CreateGraph
    before
ActivateGraph
```

The system must not infer this solely from network arrival order.

## Ordering Between Queries

Queries are generally independent unless the application defines a consistency relationship.

For example:

```text
Query A
Query B
```

does not imply that B observes A unless A changes relevant state and the consistency contract guarantees it.

## Command/Query Separation and CQRS

This ADR intentionally does not require full CQRS.

EVolution may use:

```text
same application service
same process
same storage
same model
```

for commands and queries.

The semantic distinction remains useful even without physical separation.

## Separate Read Models

A future application may introduce specialized read models:

```text
Domain State
    ↓
Projection
    ↓
Read Model
    ↓
Query
```

This is allowed.

It is not required by this ADR.

## Query Projections

A projection used to answer queries remains a derived representation.

Its provenance and source-of-truth status must follow the Storage and Provenance models.

## Command Processing

Commands may invoke:

```text
domain operations
processing graphs
storage
analysis
external systems
```

The application operation remains the orchestration boundary.

## Query Processing

Queries may also invoke processing when required.

For example:

```text
Query
    ↓
compute missing derived result
    ↓
return result
```

This does not turn the query into a command if the computation is not itself a requested semantic state change.

## Query Materialization

A query may trigger materialization of a derived cache.

If materialization changes a source-of-truth or domain state, then the operation may no longer semantically qualify as a query and should be modeled accordingly.

## Command and Event Sourcing

This model does not require event sourcing.

A command may produce domain events in an event-sourced domain, but ordinary stateful persistence remains valid.

## Query and Event Sourcing

Likewise, queries may read projections derived from events without requiring a universal event-sourcing architecture.

## Testing

Command tests should cover:

```text
validation
authorization
admission
preconditions
success
failure
cancellation
idempotency
side effects
delivery
recovery
```

Query tests should cover:

```text
validation
authorization
consistency
temporal semantics
ordering
pagination
missing result
failure
caching
provenance
```

## Contract Tests

Command/query contracts should be testable independently of transport.

For example:

```text
HTTP
gRPC
CLI
Library
```

should be able to invoke the same semantic operation contract.

## Consequences

### Positive

* Command and query semantics are explicit.
* The architecture gains many CQRS benefits without requiring CQRS infrastructure.
* Interfaces can remain transport-independent.
* Idempotency, consistency, and side effects become explicit.
* Query caching and read models can be introduced later without changing the conceptual model.
* Event sourcing remains optional.

### Negative

* Applications must document whether an operation is a command or query.
* Some operations have ambiguous boundaries and require careful semantic analysis.
* Long-running commands require additional operation-state management.
* Query consistency can become complex for live and distributed data.

## Deferred Decisions

This ADR does not select:

```text id="s9xk1f"
CQRS framework
command bus
query bus
event bus
message broker
event sourcing
separate read database
read-model technology
cache technology
transaction framework
distributed transaction mechanism
```

## Decision Summary

```text id="f4q6q8"
Application Operation:
    Command or Query

Command:
    Requests action/state transition

Query:
    Requests information

CQRS:
    Not required

Command side effects:
    Explicit

Query semantic side effects:
    Not allowed

Query implementation effects:
    Permitted

Command idempotency:
    Explicit where required

Query consistency:
    Explicit where relevant

Command completion:
    Explicit

Query result:
    Explicit

Missing query result:
    Distinct from failure

Domain events:
    Distinct from commands

Processor operations:
    Distinct from commands/queries

Transport:
    Independent

Event sourcing:
    Optional

Read models:
    Optional
```

## Invariants

1. Commands and queries are distinct semantic categories of Application Operations.
2. A command requests behavior; a query requests information.
3. A query must not intentionally request a semantic state transition.
4. Implementation-level effects such as caching and telemetry do not automatically make a query a command.
5. Commands may cause domain events, but commands and events remain distinct objects.
6. Commands may have side effects, and relevant side effects must be explicit.
7. Query results must distinguish no matching value from operational failure.
8. Query temporal semantics must explicitly identify the relevant temporal dimension.
9. Unknown event time must not be silently replaced with ingestion or current time.
10. Query ordering must be explicit when ordering affects meaning.
11. Query consistency must be explicit where multiple consistency interpretations are possible.
12. Command acceptance is distinct from command completion.
13. Accepted commands may subsequently fail or be cancelled.
14. Command retries must respect idempotency and delivery semantics.
15. Duplicate requests and duplicate execution remain distinct conditions.
16. Commands and queries must remain transport-independent.
17. Application operations must not bypass domain, processing, storage, or lifecycle contracts.
18. Operational queries must not silently become analytical measurements.
19. Derived query results must preserve relevant identity and provenance.
20. A read model is a derived representation unless explicitly defined otherwise.
21. CQRS is not required by the command/query distinction.
22. Event sourcing is not required.
23. Separate read infrastructure is not required.
24. Physical separation of commands and queries must not be introduced merely because the semantic distinction exists.
25. Every command/query contract must define materially relevant validation, result, failure, consistency, lifecycle, and side-effect semantics.

## Invariant

> **EVolution distinguishes commands from queries semantically: commands request application behavior or state transitions, while queries request information; the distinction governs contracts for side effects, consistency, idempotency, and completion without requiring CQRS, event sourcing, separate databases, or any particular transport or execution technology.**
