# ADR 0038 — Application Operation Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

The Application layer composes EVolution capabilities into concrete workflows.

Interfaces such as:

```text
CLI
HTTP
gRPC
IPC
Library
Messaging
```

need a well-defined way to invoke those workflows.

Without an explicit application-operation model, interface code can gradually become responsible for:

```text
processing orchestration
storage access
domain logic
lifecycle management
analysis execution
```

This would make interfaces tightly coupled to internal implementation.

EVolution therefore needs a semantic boundary between:

```text
External Request
        ↓
Application Operation
        ↓
Application Workflow
        ↓
Domain / Processing / Analysis / Storage
```

## Decision

EVolution represents externally invocable application behavior as **Application Operations**.

An Application Operation is a bounded semantic request to perform or observe an application capability.

Conceptually:

```text
Operation Request
        ↓
Application Operation
        ↓
Operation Result
```

An operation may be:

```text
Command
Query
Long-running Operation
```

The exact transport used to invoke the operation is outside this model.

## Application Operation

Conceptually:

```text
ApplicationOperation
{
    operation_type
    input
    configuration?
    context
    identity?
}
```

The resulting operation produces:

```text
OperationResult
{
    outcome
    output?
    error?
    metadata?
}
```

The exact C++ representation is deferred.

## Command

A **Command** requests that the application perform an operation that may change state or cause side effects.

Examples:

```text
ImportDataset
StartProcessing
StopProcessing
RunAnalysis
CreateProcessingGraph
PersistResult
```

A command does not automatically imply a domain event.

For example:

```text
StartProcessing
```

is an application command.

If starting processing causes a domain-relevant event, that event must be explicitly produced by the appropriate domain/application component.

## Query

A **Query** requests information without intentionally changing application state.

Examples:

```text
GetAnalysis
GetMeasurement
GetProcessingStatus
GetStoredEvent
ListAvailableMetrics
```

A query may cause internal cache activity or other implementation effects, but those effects must not change the semantic meaning of the query.

## Command vs Query

The distinction is semantic:

```text
Command:
    request behavior or state change

Query:
    request information
```

It is not based solely on whether an implementation performs a write internally.

For example:

```text
GetCachedMeasurement
```

remains a query even if the cache is populated during the operation.

## Application Operation vs Domain Event

These are distinct concepts.

```text
Application Operation:
    request to perform or observe application behavior

Domain Event:
    historical fact about something that happened
```

A command does not become a domain event merely because it was accepted.

## Application Operation vs Processor Operation

These are also distinct.

```text
Application Operation:
    user/application-level workflow

Processor Operation:
    execution of a unit of processing
```

For example:

```text
RunAnalysis
    ↓
construct analysis graph
    ↓
submit processing work
    ↓
processor operations
    ↓
analysis result
```

One application operation may cause many processor operations.

One processor operation may also be initiated without a corresponding external application operation.

## Operation Identity

An operation may have an explicit logical identity when tracking it across its lifetime is necessary.

Conceptually:

```text
OperationId
```

Operation identity is distinct from:

```text
EventId
ProcessorId
GraphId
RunId
RequestId
TraceId
```

These identities may be correlated but must not be treated as interchangeable.

## Request Identity

An external interface may provide a request identity.

For example:

```text
HTTP request ID
RPC request ID
CLI invocation ID
```

Request identity belongs to the interface boundary.

If the request initiates an application operation, the relationship may be represented through correlation metadata.

## Correlation

Correlation connects related operations.

For example:

```text
RequestId
    ↓
OperationId
    ↓
RunId
    ↓
Processor Operations
```

Correlation does not imply identity.

Two operations may be correlated without being the same logical object.

## Operation Context

An application operation may receive contextual information such as:

```text
principal
request identity
correlation identity
deadline
cancellation
execution mode
run identity
interface metadata
```

Only information relevant to application behavior should cross the application boundary.

Transport-specific information should normally remain at the interface layer.

## Configuration

An operation may receive configuration.

However:

```text
Operation Configuration
```

must remain distinct from:

```text
Application Configuration
Component Configuration
Execution Context
```

An operation may select or override explicitly permitted behavior, but it must not mutate hidden global configuration.

## Operation Input

Operation input should represent the semantic request.

For example:

```text
RunAnalysis
{
    dataset
    analysis_configuration
}
```

The input should not contain unnecessary transport-specific structures.

For example, an application operation should not require:

```text
HTTP headers
HTTP status codes
TCP connection objects
```

unless those are genuinely part of application semantics.

## Operation Output

Operation output should represent the semantic result of the application operation.

For example:

```text
RunAnalysis
    →
AnalysisResult
```

The application should not return transport-specific representations.

An HTTP adapter can translate:

```text
AnalysisResult
```

into an HTTP response.

## Operation Errors

Application operations use the established:

```text
Result<T>
```

model.

Conceptually:

```text
Result<Output>
```

contains either:

```text
successful output
```

or:

```text
Error
```

Expected application failures should not normally be represented as exceptions.

## Error Translation

Lower-level errors may be translated when crossing an abstraction boundary.

For example:

```text
StorageUnavailable
        ↓
ApplicationDependencyFailure
```

The original cause should remain available when useful.

The application must not expose internal implementation details unnecessarily.

## Validation

Validation occurs at multiple boundaries.

Conceptually:

```text
Interface Validation
        ↓
Application Validation
        ↓
Domain Validation
        ↓
Processing Validation
```

These layers have different responsibilities.

### Interface Validation

Validates transport-level correctness.

Examples:

```text
malformed request
invalid encoding
missing protocol field
invalid authentication metadata
```

### Application Validation

Validates whether the requested operation is meaningful for the application.

Examples:

```text
unknown workflow
missing required application dependency
invalid operation combination
invalid application-level configuration
```

### Domain Validation

Validates domain semantics.

Examples:

```text
invalid poker action
invalid domain state transition
```

Application validation must not duplicate domain semantics unnecessarily.

## Operation Lifecycle

A short synchronous operation may conceptually be:

```text
RECEIVED
    ↓
VALIDATING
    ↓
EXECUTING
    ↓
COMPLETED
```

A longer operation may require explicit lifecycle state.

Conceptually:

```text
CREATED
    ↓
ACCEPTED
    ↓
RUNNING
    ↓
COMPLETED
```

Failure:

```text
FAILED
```

Cancellation:

```text
CANCELLED
```

The exact public operation lifecycle is not yet selected.

## Operation Acceptance

Receiving a request does not necessarily mean that the operation has been accepted.

Conceptually:

```text
Request
    ↓
Validation
    ↓
Admission
    ↓
Accepted
```

The application may reject an operation because of:

```text
invalid input
invalid state
resource exhaustion
application policy
unavailable dependency
```

An operation that was never accepted is distinct from an accepted operation that later failed.

## Synchronous Operations

For short operations, the caller may wait for completion:

```text
Request
    ↓
Execute
    ↓
Result
```

This is a semantic execution model, not a requirement that the implementation use a particular thread or blocking mechanism.

## Asynchronous Operations

Long-running work may be accepted separately from completion:

```text
Request
    ↓
Operation Accepted
    ↓
OperationId
```

Later:

```text
OperationId
    ↓
Status / Result
```

This allows interfaces to remain responsive while processing continues.

## Long-Running Operations

Examples include:

```text
large dataset import
historical replay
large analysis
model training
full event reconstruction
```

A long-running operation may create or coordinate a processing run.

The application operation remains the external workflow boundary.

## Operation vs Run

An operation requests application behavior.

A run represents an execution instance.

For example:

```text
RunAnalysis
    ↓
OperationId: O1
    ↓
RunId: R1
```

If the operation is retried and produces another execution:

```text
OperationId: O1
    ↓
RunId: R2
```

The exact relationship depends on the operation contract.

Operation identity and execution/run identity must remain distinct.

## Operation Status

Long-running operations may expose status such as:

```text
PENDING
RUNNING
COMPLETED
FAILED
CANCELLED
```

Status is operational state.

It must not be confused with:

```text
domain state
processor lifecycle state
application lifecycle state
```

## Operation Completion

Completion must have explicit semantics.

Possible outcomes include:

```text
SUCCESS
FAILURE
CANCELLED
```

A completed operation must not be reported as successful if required work was silently lost.

## Partial Results

Some operations may produce partial results.

For example:

```text
Import 100 datasets
    ↓
97 succeeded
3 failed
```

The operation contract must explicitly define whether such an outcome is:

```text
success
partial success
failure
```

and how individual failures are represented.

Partial results must not be silently interpreted as complete results.

## Operation Cancellation

An accepted operation may support cancellation.

Cancellation is an explicit control request.

Conceptually:

```text
Running Operation
        ↓
Cancel Request
        ↓
Cancellation
```

Cancellation follows the processing cancellation semantics where processing work is involved.

Cancellation is distinct from operation failure.

## Cancellation Result

A cancelled operation may return:

```text
Error(Cancelled)
```

or an equivalent explicit cancellation outcome.

This does not imply that the application or processor failed.

## Cancellation Propagation

If an application operation coordinates multiple components, cancellation propagation must be explicit.

For example:

```text
Application Operation
        ↓
Processing Graph
        ↓
Processor A
Processor B
Processor C
```

Cancellation may need to propagate to all admitted work.

The exact propagation policy belongs to the operation/workflow contract.

## Deadlines and Timeouts

A caller may provide a deadline.

A deadline is an execution constraint, not domain time.

For example:

```text
request deadline = 500 ms
```

must not become:

```text
event_time = current_time + 500 ms
```

Deadline expiration may produce cancellation or failure according to the operation contract.

## Operation Idempotency

Commands may need explicit idempotency semantics.

For example:

```text
ImportDataset(dataset_id)
```

may define that repeated requests with the same idempotency identity do not create multiple logical imports.

Idempotency is not universal.

Each command that requires it must define:

```text
idempotency key
scope
retention
duplicate behavior
result behavior
```

## Idempotency Key

An idempotency key must be stable within its declared scope.

It must not depend on:

```text
memory address
thread identity
execution order
temporary object address
```

A logical identity may be used where appropriate.

## Duplicate Requests

Duplicate request and duplicate execution are distinct.

```text
Duplicate Request:
    caller sends equivalent command again

Duplicate Execution:
    same admitted work executes more than once
```

Both must be handled according to their respective contracts.

## Query Repeatability

Queries may be repeated.

However, two identical queries may produce different results if the underlying data changed.

Therefore:

```text
same query
```

does not automatically imply:

```text
same result
```

Reproducibility depends on the relevant input state and execution context.

## Query Consistency

A query contract may specify the consistency level required.

Conceptually:

```text
LATEST
SNAPSHOT
VERSIONED
EVENT_TIME
RUN_SPECIFIC
```

No universal consistency mode is selected.

## Query Temporal Semantics

Queries involving time must specify which temporal dimension they use.

For example:

```text
event time
ingestion time
measurement time
processing time
```

A query must not silently substitute one for another.

## Query Windows

Temporal queries should use explicit boundaries.

The preferred conceptual interval is:

```text
[start, end)
```

where appropriate.

This avoids ambiguity at adjacent boundaries.

## Pagination

Queries returning collections may require pagination.

Pagination semantics should define:

```text
ordering
cursor
page size
continuation
consistency
```

The exact mechanism is deferred.

## Streaming Operations

An application operation may produce a stream.

For example:

```text
Query
    ↓
Measurement Stream
```

Streaming contracts must define:

```text
ordering
completion
cancellation
failure
backpressure
delivery
```

A stream is not automatically a collection.

## Operation Result Streaming

For long-running processing, results may become available incrementally.

The operation contract must distinguish:

```text
intermediate result
final result
```

Intermediate output must not be presented as final unless the contract explicitly allows it.

## Operation Side Effects

Commands may cause side effects.

For example:

```text
write storage
start processing
send external message
modify external system
```

Side effects require explicit:

```text
ownership
delivery semantics
idempotency
retry behavior
failure behavior
recovery behavior
```

where applicable.

## Query Side Effects

Queries should not intentionally cause domain-level side effects.

Implementation-level effects such as:

```text
cache population
metrics
trace creation
```

do not necessarily violate this rule because they are operational rather than semantic.

## Operation and Transactions

An application operation may coordinate multiple changes.

The operation contract must define whether the outcome requires:

```text
atomicity
partial success
compensation
best effort
```

The existence of an application operation does not imply a distributed transaction.

## Operation and Storage

An application operation may perform multiple storage operations.

Storage-level transaction semantics remain owned by Storage.

The application may require stronger semantics and coordinate them explicitly where supported.

## Operation and Processing Graph

A command may:

```text
create graph
validate graph
activate graph
submit work
stop graph
```

The application operation invokes graph capabilities through their contracts.

It must not directly manipulate processor internals.

## Operation and Analysis

An application operation may invoke analysis:

```text
RunAnalysis
    ↓
Analyzer
    ↓
Analysis
```

The operation should return the resulting analysis or an explicit asynchronous operation handle, depending on workflow requirements.

## Operation and Domain

Application operations may invoke domain operations.

The application should orchestrate domain capabilities without duplicating domain invariants.

For example:

```text
Application:
    request state transition

Domain:
    determine whether transition is valid
```

## Operation and Provenance

When an operation materially affects a result, provenance may include:

```text
operation identity
application identity/version
configuration
graph
component versions
input identities
run identity
relevant execution context
```

Not every short-lived operation requires persistent provenance.

The requirement depends on reproducibility and auditability needs.

## Operation and Observability

Operations may produce operational telemetry:

```text
operation duration
status
failure count
queue wait
processing duration
```

These are observability signals unless intentionally promoted into analytical data.

## Operation and Security

An operation may require authorization.

The interface may authenticate a caller, while the application/security boundary determines whether the caller is authorized to perform the operation.

Authorization is distinct from domain validation.

For example:

```text
Authenticated:
    caller identity established

Authorized:
    caller may run this operation
```

## Operation and Secrets

Operation inputs may contain secrets.

Secrets must not automatically appear in:

```text
logs
errors
traces
provenance
operation status
```

Redaction rules must be applied at appropriate boundaries.

## Operation Timeout vs Cancellation

Timeout and cancellation are related but distinct.

```text
Cancellation:
    explicit request to stop

Timeout:
    deadline condition became true
```

A timeout may result in cancellation if the operation contract defines it that way.

The resulting outcome must remain explicit.

## Operation Failure Scope

Failure may occur at different levels:

```text
request validation
operation admission
workflow orchestration
processor execution
storage
external dependency
```

A lower-level failure should not automatically be represented as total application failure.

The operation determines the appropriate semantic result.

## Retry

Retry is not automatically implied by a retryable error.

The application operation may define retry policy:

```text
no retry
bounded retry
caller retry
supervisor retry
```

Retries must respect:

```text
idempotency
delivery semantics
cancellation
deadlines
resource limits
```

## Operation Recovery

A failed long-running operation may be recoverable.

Recovery may include:

```text
retry
resume
restore
replay
restart
manual intervention
```

The relevant recovery policy must be explicit.

## Operation State Persistence

Long-running operation state may require persistence.

Persisted operation state is distinct from:

```text
domain state
processor state
checkpoint state
```

If an operation must survive application restart, its recovery contract must explicitly define what state is persisted and how it is reconstructed.

## Operation History

An application may retain operation history.

For example:

```text
OperationId
Status
CreatedAt
StartedAt
CompletedAt
Result reference
Error
RunId
```

This is application execution information, not automatically a domain event stream.

## Operation Ordering

Multiple application operations may have ordering requirements.

For example:

```text
CreateGraph
    before
ActivateGraph
```

Ordering must be explicit where semantically required.

The existence of request arrival order does not automatically establish application semantic ordering.

## Concurrent Operations

Applications may receive multiple operations concurrently.

Concurrency semantics must be explicitly defined where operations interact.

For example:

```text
StartProcessing
StopProcessing
```

may require serialization.

The application must not rely on incidental thread scheduling to determine semantic ordering.

## Operation Admission and Backpressure

Application operations may themselves have admission limits.

For example:

```text
maximum concurrent analyses
maximum queued imports
maximum request size
```

Admission should produce explicit outcomes such as:

```text
accepted
rejected
delayed
cancelled
```

rather than silently losing requests.

## Application Operation vs Queue

An operation may create queued processing work.

The operation itself is not the queue item unless the application contract explicitly defines that relationship.

For example:

```text
RunAnalysis
    ↓
Operation O1
    ↓
Processing work W1
Processing work W2
Processing work W3
```

These are distinct logical objects.

## Operation Identity and Persistence

If operation history is persistent, operation identity must survive:

```text
restart
serialization
storage migration
recovery
```

It must not depend on memory addresses or process-local object identity.

## Operation Versioning

An operation definition may evolve.

Operation version is distinct from:

```text
application version
API version
domain version
processor version
serialization schema version
```

Versioning becomes important when stored requests or long-running operations must survive application upgrades.

## Operation Compatibility

A persisted or queued operation must not be interpreted under incompatible semantics without explicit migration or rejection.

Compatibility may depend on:

```text
operation version
application version
input schema
configuration
component versions
```

## Operation API Boundary

The application operation boundary should be transport-independent.

Conceptually:

```text
HTTP Request
     ↓
HTTP Adapter
     ↓
Application Operation
     ↓
Result
     ↓
HTTP Response
```

Likewise:

```text
CLI Command
     ↓
CLI Adapter
     ↓
Application Operation
```

The application operation must not know whether it was invoked through HTTP, CLI, gRPC, or another interface.

## Interface Mapping

An interface adapter is responsible for translating:

```text
external request
    ↓
operation input
```

and:

```text
operation result
    ↓
external response
```

The application owns the semantic operation.

## Operation Composition

An application operation may invoke other application capabilities.

However, nested operations should have explicit semantics.

For example:

```text
ImportDataset
    ↓
Normalize
    ↓
BuildEvents
    ↓
PersistEvents
```

The internal steps do not necessarily need to become externally visible operations.

## Operation Boundaries

An operation boundary should be chosen around a meaningful application capability rather than every internal function.

Too fine:

```text
ReadFile
ParseLine
AllocateBuffer
```

Too coarse:

```text
RunEverything
```

The operation should represent a meaningful unit of application behavior.

## Operation Contract

Each externally meaningful operation should document:

```text
operation identity
input
output
validation
preconditions
postconditions
errors
cancellation
deadline behavior
idempotency
side effects
delivery semantics
partial results
ordering
resource requirements
provenance
lifecycle
```

Not every field is required for every operation, but unsupported semantics must be explicit where relevant.

## Testing

Application operation tests should verify:

```text
valid input
invalid input
authorization boundary
admission
successful completion
failure
cancellation
deadline
partial result
idempotency
duplicate request
dependency failure
recovery
provenance
```

Transport-specific tests remain at interface boundaries.

## Consequences

### Positive

* Interfaces remain thin adapters.
* Application workflows have explicit semantic boundaries.
* Commands and queries become distinguishable.
* Long-running work can be represented without tying the architecture to a particular asynchronous mechanism.
* Idempotency and cancellation become explicit concerns.
* Application operations can be invoked through multiple interfaces.
* Operation identity and execution identity remain separate.

### Negative

* Adds another explicit conceptual layer.
* Long-running operations require additional state and lifecycle semantics.
* Command/query boundaries must be maintained carefully.
* Operation-level idempotency and recovery can become complex for side-effecting workflows.

## Deferred Decisions

This ADR does not select:

```text
CQRS framework
command bus
query bus
message broker
job queue
future/promise mechanism
async framework
HTTP API structure
gRPC API structure
operation database schema
distributed transaction mechanism
workflow engine
```

It also does not require a strict CQRS implementation.

The command/query distinction is semantic rather than a mandate for separate physical infrastructure.

## Decision Summary

```text
Application Operation:
    Semantic boundary for application behavior

Command:
    Requests behavior or state change

Query:
    Requests information

Operation Identity:
    Identifies a tracked application operation

Request Identity:
    Interface-level identity

Run Identity:
    Identifies an execution instance

Correlation:
    Connects related objects without replacing identity

Operation Result:
    Success / Failure / Cancellation semantics

Long-running Operation:
    May outlive the initiating interface request

Cancellation:
    Explicit termination request

Deadline:
    Execution constraint, not domain time

Idempotency:
    Explicit where required

Partial Results:
    Explicitly defined

Side Effects:
    Explicitly defined

Transport:
    Outside application operation semantics

Domain Events:
    Distinct from application commands

Processor Operations:
    Distinct from application operations
```

## Invariants

1. Application operations are the semantic boundary between external invocation and application workflows.
2. Interfaces translate external requests into application operations and must not implement application workflows themselves.
3. Commands and queries are semantically distinct.
4. Application commands are not automatically domain events.
5. Application operations are distinct from processor operations.
6. Operation identity is distinct from request, run, processor, graph, trace, and domain identities.
7. Correlation must not be treated as logical identity.
8. Operation input and output must remain transport-independent.
9. Expected operation failures are represented through `Result<T>` and stable error semantics.
10. Validation must remain separated into interface, application, and domain responsibilities.
11. Operation acceptance is distinct from request receipt.
12. Operation completion is distinct from operation acceptance.
13. Cancellation is distinct from failure.
14. Deadline expiration must not silently become domain time.
15. Long-running operations must have explicit lifecycle and completion semantics.
16. Partial results must never be silently represented as complete results.
17. Idempotency must be explicit for commands where duplicate requests or execution can cause semantic problems.
18. Duplicate requests and duplicate execution are distinct conditions.
19. Queries must not intentionally produce domain-level side effects.
20. Application operations must not bypass processor, graph, domain, storage, or interface contracts.
21. Operation retries must respect idempotency, delivery semantics, cancellation, deadlines, and resource constraints.
22. Operation recovery must be explicitly defined where recovery is supported.
23. Persisted operation state must remain distinct from domain state and processor state.
24. Operation history is not automatically a domain event stream.
25. Operation ordering must be explicit when semantic ordering is required.
26. Concurrent application operations must not rely on incidental execution scheduling for semantic correctness.
27. Application operation contracts must document materially relevant input, output, lifecycle, failure, cancellation, side-effect, and reproducibility semantics.
28. Transport-specific concerns must remain outside the semantic application operation model.
29. The application operation model does not require a particular CQRS, messaging, asynchronous, or workflow framework.

## Invariant

> **EVolution treats an Application Operation as the explicit semantic boundary between external invocation and application behavior: interfaces translate requests into operations, applications execute those operations according to explicit contracts, and commands, queries, processor work, domain events, execution runs, and transport mechanisms remain distinct concepts.**
