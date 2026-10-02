# Error Handling Scope

## Purpose

Error handling in EVolution defines how the system represents, propagates, classifies, and responds to conditions in which an operation cannot produce its normal result.

The purpose is not to create one universal error mechanism for every situation.

Instead, EVolution distinguishes:

* expected operational errors
* invalid input
* domain validation failures
* processing failures
* infrastructure failures
* cancellation
* internal invariant violations
* fatal process failures

The error-handling model must preserve the architectural distinction between **reporting a failure** and **deciding what to do about it**.

---

# 1. Scope

Error handling applies to operations that cross EVolution component boundaries.

This includes:

```text
Ingestion
    ↓
Events
    ↓
State / Projection
    ↓
Measurements
    ↓
Time Series
    ↓
Aggregation
    ↓
Patterns
    ↓
Analysis
    ↓
Storage
    ↓
Application / Interface
```

It applies to both synchronous and asynchronous processing.

It also applies to:

* configuration
* initialization
* loading
* persistence
* replay
* batch processing
* streaming processing
* external integrations

The model does **not** require every internal function to expose an error result.

Small internal operations may establish stronger preconditions and rely on assertions or other mechanisms when failure represents a programming error rather than an expected runtime condition.

---

# 2. Error Handling Has Four Separate Responsibilities

EVolution treats these as distinct responsibilities:

```text
Detection
    ↓
Representation
    ↓
Propagation
    ↓
Response
```

### Detection

A component determines that the expected operation cannot proceed normally.

### Representation

The failure is converted into a structured representation that another component can understand.

### Propagation

The error moves across component boundaries when the current component cannot or should not handle it.

### Response

A higher-level component decides what to do:

```text
retry
skip
reject
fallback
pause
abort
terminate
notify
```

These responsibilities must not be collapsed into one mechanism.

For example:

> A storage component reports `STORAGE_UNAVAILABLE`; it does not automatically decide whether the entire application should terminate.

---

# 3. Error Handling Does Not Define Policy

Error handling reports conditions.

Policy determines responses.

For example:

```text
Storage
    │
    │ STORAGE_UNAVAILABLE
    ▼
Processor
    │
    ├── retry
    ├── buffer
    └── propagate
```

The storage layer reports the condition.

The processor or application decides the appropriate response based on its operational requirements.

This distinction is important because the same error may require different responses in different contexts.

---

# 4. Expected Errors

An expected error is a condition that can occur during legitimate operation and that the caller may reasonably need to handle.

Examples:

```text
invalid input
missing object
unsupported operation
invalid configuration
storage unavailable
resource unavailable
external dependency failure
invalid state transition
cancelled operation
```

These errors belong to the explicit error model.

They must not require the caller to detect failure through:

```text
log parsing
magic return values
global state
undefined behavior
process termination
```

---

# 5. Programming Errors

A programming error occurs when an internal invariant is violated.

Examples:

```text
impossible state
invalid internal ownership
broken component invariant
unexpected null where contract forbids it
corrupted internal structure
```

These are fundamentally different from expected operational failures.

They should generally be detected close to the violated invariant.

Possible mechanisms include:

```text
assertion
debug assertion
diagnostic failure
fail-fast behavior
exception at an appropriate boundary
```

The system must not routinely convert programming errors into ordinary recoverable errors merely to keep processing alive.

Doing so can hide defects and produce invalid analytical results.

---

# 6. Input Validation

Input validation belongs to the boundary that first understands the input contract.

For example:

```text
External Data
      ↓
Ingestion
      ↓
Validation
      ↓
Event
```

If external data is malformed, ingestion should normally report the problem.

Core should not need to understand the external file format in order to reject malformed input.

Likewise:

```text
Poker Event
      ↓
Poker Domain Validation
```

belongs to the Poker domain rather than generic Core.

Validation therefore follows semantic ownership.

---

# 7. Domain Errors

Domain errors represent violations of domain rules.

For example:

```text
Event sequence is invalid
Action is not legal in current state
Required domain entity is missing
Domain-specific invariant is violated
```

Generic infrastructure should provide the mechanism for representing such failures without defining their meaning.

Conceptually:

```text
Core Error
    ↑
Domain-specific error code
```

The domain owns the meaning.

Core owns the generic representation.

---

# 8. Processing Errors

Processing errors occur while executing a processor or processing graph.

They may originate from:

```text
input
processor logic
state
resource
dependency
external service
```

Processing must distinguish between:

```text
local input failure
processor failure
graph failure
application failure
```

For example:

```text
One malformed event
        ↓
Reject event
        ↓
Continue stream
```

is fundamentally different from:

```text
Processor invariant violated
        ↓
Stop processor
        ↓
Stop graph
```

The processing contract determines the propagation boundary.

---

# 9. Infrastructure Errors

Infrastructure errors originate from mechanisms rather than domain semantics.

Examples:

```text
Storage unavailable
File I/O failure
Network failure
Resource exhaustion
Serialization failure
Operating-system failure
```

Infrastructure components report these failures.

They do not reinterpret them as domain conclusions.

For example:

```text
Database connection failure
```

should remain a storage/infrastructure failure rather than becoming a domain-level analytical result.

---

# 10. Cancellation

Cancellation is an intentional termination of an operation.

It is not necessarily an error in the conventional sense.

The error model nevertheless needs a representation for cancellation because an operation may need to return through the same control boundary.

Conceptually:

```text
Running
   ↓
Cancellation requested
   ↓
Cancelled
```

Cancellation must remain distinguishable from:

```text
Failed
```

because callers may treat the two differently.

---

# 11. Partial Failure

EVolution processes collections and streams, so failure does not always apply to the entire operation.

For example:

```text
Batch
 ├── A → success
 ├── B → success
 ├── C → rejected
 └── D → success
```

The error model must allow a component to represent this without falsely reporting:

```text
entire batch failed
```

or:

```text
entire batch succeeded
```

when neither is true.

Partial-failure semantics belong to the contract of the operation performing the batch or stream processing.

---

# 12. Error Propagation

An error may cross multiple architectural boundaries.

Example:

```text
Storage
   ↓
Measurement Processor
   ↓
Analysis
   ↓
Application
```

Each boundary may:

1. handle the error
2. propagate it unchanged
3. add context
4. translate it into a higher-level error

It must not silently discard the error unless that behavior is explicitly part of the contract.

---

# 13. Error Context

Error context answers:

> What operation was being performed when the failure occurred?

For example:

```text
code:
    STORAGE_UNAVAILABLE

context:
    persist measurement batch

object:
    measurement-series-42

cause:
    connection unavailable
```

Context may be accumulated as an error moves upward.

However, error context must not become an uncontrolled dump of arbitrary state.

The context should remain:

* structured
* bounded
* relevant
* safe for the intended boundary

---

# 14. Error Cause

An error may have an underlying cause.

For example:

```text
Application failure
    ↓
Analysis persistence failed
    ↓
Storage write failed
    ↓
Filesystem error
```

The original cause should remain available when useful.

This allows higher-level errors to communicate meaning without destroying the underlying diagnostic information.

---

# 15. Error Classification

The initial generic classification is:

```text
Validation
Configuration
Input
State
Processing
Storage
Resource
External
Unsupported
Cancellation
Internal
```

These categories describe **where or why a failure occurred**.

They do not automatically determine what action should be taken.

For example:

```text
Storage
```

does not automatically mean:

```text
Retry
```

and:

```text
Validation
```

does not automatically mean:

```text
Abort application
```

---

# 16. Recoverability

Recoverability is separate from classification.

An error may be:

```text
Recoverable
Retryable
Non-retryable
Unknown
```

These properties should be provided only when they have meaningful semantics.

The caller remains responsible for deciding whether recovery is appropriate.

This avoids embedding operational policy inside low-level error types.

---

# 17. Retry Semantics

Retry is outside the generic error representation.

A component may provide information useful for retrying, such as:

```text
temporary
retryable
idempotent
```

but a generic error object must not automatically retry an operation.

Retry policy belongs to the component or application that owns the operation.

This is especially important for operations with side effects.

---

# 18. Logging

Logging is outside error representation.

An error:

```text
Error
```

and an operational log:

```text
Log Entry
```

are different objects with different purposes.

A low-level function returning an error should not automatically log it.

Otherwise a single failure can generate repeated messages at every architectural layer.

The component with sufficient operational context should normally decide whether the error should be logged.

---

# 19. Metrics and Telemetry

Error handling must not be confused with observability.

These are separate:

```text
Error       → communicates failure
Log         → communicates diagnostic information
Metric      → measures behavior
Trace       → records execution path
```

A processing system may record:

```text
processor_errors_total = 17
```

without changing the error returned by the processor.

Observability may consume error information, but the error model should not depend on a particular telemetry system.

---

# 20. Provenance

Errors may contain references to provenance or processing context when useful for diagnosis.

However:

```text
Error
```

is not itself:

```text
Provenance
```

Analytical provenance answers:

> Where did this result come from?

Error context answers:

> Why did this operation fail?

The two may reference some of the same objects but remain conceptually separate.

---

# 21. Error Boundaries

An architectural boundary should define what happens to errors crossing it.

Important boundaries include:

```text
External → Ingestion
Ingestion → Domain
Domain → Processing
Processing → Storage
Processing → Analysis
Library → Application
Application → Interface
```

Each boundary should eventually specify:

```text
accepted errors
returned errors
translated errors
handled errors
exception behavior
```

This makes failure behavior part of the component contract.

---

# 22. Process-Level Failure

The generic error model does not own process supervision.

An application may decide to:

```text
continue
degrade
restart
terminate
```

after receiving an error.

That is application/runtime policy.

For example:

```text
Core
  ↓
Error

Application
  ↓
Restart component
```

Core should not terminate the process merely because an operation failed.

---

# 23. Error Handling and Data Integrity

An error must not silently produce invalid analytical data.

For example:

```text
Event
  ↓
Measurement calculation fails
  ↓
fake/default measurement
```

is generally unacceptable unless explicitly defined by the metric contract.

The system should distinguish:

```text
no result
zero result
missing result
failed result
estimated result
```

These are analytically different states.

This is particularly important for time series and aggregation.

---

# 24. Error Handling and Determinism

For deterministic components, error classification should also be deterministic under equivalent inputs and configuration.

For example:

```text
same input
same configuration
same component version
        ↓
same success/failure classification
```

External conditions may naturally introduce nondeterminism:

```text
network availability
storage availability
resource pressure
```

Such nondeterminism must be represented as an environmental dependency rather than hidden inside analytical results.

---

# 25. Error Handling Does Not Replace Validation

Validation determines whether an input satisfies a contract.

Error handling determines how failure of that contract is represented and propagated.

Therefore:

```text
Validation
    ↓
detect invalid input

Error handling
    ↓
represent and propagate validation failure
```

They are related but separate responsibilities.

---

# 26. Error Handling Does Not Define Domain Semantics

Core may define:

```text
Error
ErrorCode
Result
```

but Core must not define domain-specific meanings such as:

```text
InvalidPokerAction
BadTrade
InvalidPacketSequence
```

Those belong to their respective domains.

The dependency direction remains:

```text
Domain meaning
      ↓
Core mechanism
```

not:

```text
Core
  ↓
Poker semantics
```

---

# 27. Scope Boundary

The error-handling architecture therefore covers:

```text
┌─────────────────────────────────────────┐
│              Error Handling             │
├─────────────────────────────────────────┤
│ Detection                               │
│ Representation                          │
│ Classification                          │
│ Context                                 │
│ Cause                                   │
│ Propagation                             │
│ Cancellation                            │
│ Partial failure                         │
│ Recoverability metadata                 │
│ Module-boundary contracts               │
└─────────────────────────────────────────┘
```

It does **not** own:

```text
┌─────────────────────────────────────────┐
│          Separate Responsibilities      │
├─────────────────────────────────────────┤
│ Retry policy                            │
│ Logging                                 │
│ Metrics / telemetry                     │
│ Process supervision                     │
│ Domain policy                           │
│ User-interface presentation             │
│ Storage retry mechanisms                │
│ Distributed consensus                   │
│ Operational alerting                    │
└─────────────────────────────────────────┘
```

Those systems may consume error information but remain independently defined.

---

# 28. Conceptual Error Model

At the conceptual level:

```text
Operation
    │
    ├── Success → Result
    │
    └── Failure
          │
          ▼
        Error
          │
          ├── Category
          ├── Code
          ├── Context
          ├── Cause
          └── Recovery information?
                    │
                    ▼
                 Caller
                    │
                    ├── Handle
                    ├── Retry
                    ├── Skip
                    ├── Propagate
                    ├── Degrade
                    └── Abort
```

The key separation is:

```text
Error = what went wrong
Policy = what should be done about it
```

---

# 29. Relationship to Existing Concepts

Error handling interacts with the existing EVolution concepts as follows:

| Concept     | Relationship                                                     |
| ----------- | ---------------------------------------------------------------- |
| Event       | Event ingestion/validation may fail                              |
| State       | State transitions may reject invalid events                      |
| Measurement | Measurement computation or persistence may fail                  |
| Time Series | Missing/failed observations must remain distinguishable          |
| Aggregation | Partial input failure requires explicit semantics                |
| Pattern     | Pattern detection may fail without implying absence of a pattern |
| Analysis    | Analysis may fail without producing a finding                    |
| Context     | Error context describes circumstances of failure                 |
| Provenance  | Provenance may help diagnose failure origin                      |
| Identity    | Errors may reference affected object identities                  |
| Processing  | Processing determines failure propagation                        |
| Storage     | Storage reports infrastructure/persistence failures              |
| Application | Application determines operational response                      |

---

# 30. Invariants

The error-handling model establishes the following invariants:

> **Expected failures must be representable without terminating the process.**

> **Programming invariant violations must not be silently converted into ordinary operational failures.**

> **An error describes a failure; it does not determine the recovery policy.**

> **Logging, telemetry, retry, and process supervision remain separate concerns.**

> **Failure must not silently become valid analytical data.**

> **Domain-specific error meaning belongs to the domain that owns that meaning.**

> **Errors crossing module boundaries must have explicit propagation semantics.**
