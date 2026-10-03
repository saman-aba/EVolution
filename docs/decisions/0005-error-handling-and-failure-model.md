# ADR 0005 — Error Handling and Failure Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution is an analytical platform composed of multiple modules and processing stages.

Failures can occur at many levels:

* invalid input
* malformed events
* invalid state transitions
* unavailable storage
* failed ingestion
* processor failures
* configuration errors
* resource exhaustion
* external system failures
* analytical algorithm failures
* programming errors

These failures do not all have the same meaning.

A malformed external event may be expected and recoverable, while a violated internal invariant may indicate a programming defect.

The architecture therefore needs to distinguish different classes of failure without forcing every component into the same recovery strategy.

The error model must also work for:

* synchronous functions
* streaming processors
* batch processing
* replay
* applications
* storage
* ingestion
* domain logic

## Decision

EVolution will use a **typed, explicit error model based primarily on return values**, rather than using exceptions as the normal mechanism for expected operational failures.

Functions that can fail in an expected way should communicate failure explicitly through their return type.

C++ exceptions are not prohibited, but they are **not the default application-level error propagation mechanism**.

The initial policy is:

```text
Expected operational failure
        ↓
Explicit return value

Programming invariant violation
        ↓
Assertion / fail-fast / exception where appropriate

Fatal process-level failure
        ↓
Application-level termination
```

The exact representation of the explicit error result will be defined at the implementation/API level.

## Failure Categories

Failures are divided into four broad categories.

### 1. Expected Operational Errors

These are failures that can occur during normal operation.

Examples:

```text
invalid external input
missing data
storage unavailable
timeout
resource unavailable
unsupported input
configuration value rejected
```

These should be represented explicitly.

The caller should be able to determine what happened without relying on parsing an exception message.

### 2. Domain Validation Errors

These occur when data violates domain rules.

Examples:

```text
invalid event sequence
invalid hand state transition
measurement outside permitted domain
invalid domain configuration
```

These are also expected failures from the perspective of an API boundary and should normally be returned explicitly.

### 3. Internal Invariant Violations

An invariant violation means the implementation reached a state that should have been impossible if its contract were respected.

Examples:

```text
invalid internal state
corrupted ownership relationship
impossible state transition
broken processor invariant
```

These should not normally be converted into ordinary recoverable errors.

They indicate a programming or architectural defect.

Assertions, diagnostics, or fail-fast behavior may therefore be appropriate.

### 4. Fatal Environment or Process Failures

Some failures may make continued execution impossible.

Examples:

```text
unrecoverable initialization failure
critical resource exhaustion
corrupted process state
fatal dependency failure
```

The application decides whether to terminate, restart, isolate the failed component, or enter a degraded mode.

The Core library itself should not arbitrarily terminate the entire process.

## Explicit Error Results

The preferred conceptual API is:

```text
Result<T, Error>
```

where:

```text
Success → T
Failure → Error
```

For operations that do not return a value:

```text
Result<void, Error>
```

The exact C++ representation remains an implementation decision.

The error object should contain structured information rather than only a human-readable string.

Conceptually:

```text
Error
{
    category
    code
    message?
    context?
    cause?
}
```

The following information may be useful:

* error category
* stable error code
* human-readable message
* operation/context
* underlying cause
* relevant object identity
* provenance
* recoverability

Not every error requires every field.

## Error Categories

The initial conceptual categories are:

```text
Validation
Configuration
Input
State
Processing
Storage
Resource
External
Internal
Unsupported
Cancellation
```

These categories are intentionally generic.

Domains may define more specific error codes beneath them.

For example:

```text
Domain
 └── Poker
      └── InvalidAction
```

The Core error representation must not contain Poker-specific error semantics.

## Error Identity

Errors should have stable machine-readable codes.

For example:

```text
INPUT_MALFORMED
CONFIG_INVALID
STORAGE_UNAVAILABLE
STATE_INVALID_TRANSITION
RESOURCE_EXHAUSTED
OPERATION_CANCELLED
```

Human-readable messages are not stable API identifiers.

Therefore:

> **Code determines programmatic meaning; message provides human-readable explanation.**

## Error Context

An error may accumulate context as it propagates upward.

For example:

```text
Storage error
    ↓
Measurement persistence failed
    ↓
Analysis checkpoint failed
    ↓
Replay operation failed
```

The final error should make it possible to understand the operation path without destroying the original cause.

Conceptually:

```text
Error
{
    code
    context
    cause
}
```

This creates an error chain.

The system should avoid producing meaningless chains containing the same information repeatedly.

## Recoverability

An error should distinguish, where meaningful, between:

```text
Recoverable
Retryable
Non-retryable
Unknown
```

These properties must not be inferred solely from the error category.

For example:

```text
STORAGE_UNAVAILABLE
```

may be retryable in one situation and fatal in another.

Retry policy belongs to the component responsible for the operation.

An error describes the failure; the caller decides what recovery policy applies.

## Retry

Retry behavior must not be hidden inside generic error handling.

A retry may be appropriate for:

* temporary storage failure
* transient network failure
* temporary resource exhaustion

But retrying can also amplify failures.

Therefore a component should explicitly define whether an operation is:

```text
retry-safe
idempotent
non-idempotent
unknown
```

when retries are possible.

The generic error model does not automatically retry operations.

## Cancellation

Cancellation is distinct from failure.

An operation may stop because the caller requested cancellation:

```text
Operation
    ↓
Cancellation requested
    ↓
Operation stops
```

This should be distinguishable from:

```text
Operation
    ↓
Unexpected failure
```

Cancellation should therefore have an explicit representation in the error/result model where appropriate.

## Partial Failure

Processing systems frequently operate on collections rather than individual objects.

For example:

```text
Input batch
    ├── item A → success
    ├── item B → success
    ├── item C → failure
    └── item D → success
```

A batch operation must not automatically collapse this into a single boolean result.

The architecture should support explicit partial-result semantics.

Conceptually:

```text
BatchResult
{
    successful
    failed
    errors
}
```

The exact representation is deferred.

Whether processing continues after a partial failure is determined by the processor's contract.

## Streaming Failure

A failure inside a stream requires special handling.

A processor may:

1. reject one input and continue
2. skip invalid input
3. pause processing
4. enter degraded state
5. stop the processing graph
6. terminate the application

The generic processing model must therefore distinguish:

```text
Input-level failure
Processor-level failure
Graph-level failure
Application-level failure
```

The processor contract should define which failures are local and which propagate.

## Error Propagation

Errors should normally propagate through explicit interfaces.

Conceptually:

```text
Low-level operation
        ↓
Error
        ↓
Processor
        ↓
Processing graph
        ↓
Application
```

Each layer may:

* propagate the error
* add useful context
* translate it into a higher-level error
* handle it locally

A layer must not silently discard an error unless that behavior is explicitly part of its contract.

## Exceptions

Exceptions are permitted for situations where they provide a clear benefit, but they are not the normal mechanism for expected operational failures.

In particular, APIs should not require callers to use exceptions merely to detect ordinary conditions such as:

```text
invalid input
missing record
storage unavailable
unsupported operation
validation failure
```

Exceptions may be appropriate for:

* integration with third-party libraries that use exceptions
* exceptional internal control flow where justified
* construction failures where a valid object cannot be produced
* application boundaries that explicitly choose exception-based handling

When exceptions cross a module boundary, the boundary must document that behavior.

## `noexcept`

`noexcept` should be used where the component contract genuinely guarantees non-throwing behavior.

It should not be added mechanically to every function.

Incorrect `noexcept` declarations can convert ordinary failures into process termination.

The project's eventual API guidelines should therefore define where `noexcept` is expected.

## Assertions

Assertions are appropriate for programmer invariants.

For example:

```text
assert(state.version >= previous_version)
```

is conceptually different from:

```text
if (!input.valid())
    return Error(...)
```

The first represents an internal assumption.

The second represents an externally observable validation condition.

Assertions must not be used as a substitute for normal input validation.

## Logging

Errors and logging are related but distinct.

An error object communicates failure to the caller.

Logging communicates information to operators/developers.

A low-level component should not automatically log every error it returns.

Otherwise one failure may produce:

```text
Storage → log
Processor → log
Application → log
```

resulting in duplicate messages.

The component that has sufficient operational context should generally decide whether an error should be logged.

## Error Messages

Error messages should be:

* concise
* descriptive
* contextual
* safe to expose at the intended boundary

Messages should not be relied upon for machine processing.

For example:

```text
code: STORAGE_UNAVAILABLE
message: "Unable to write measurement batch"
```

is preferable to requiring callers to interpret:

```text
"database connection failed at line 183..."
```

as an API contract.

## Provenance and Errors

Errors may contain provenance/context when that information is necessary to diagnose the failure.

For example:

```text
input source
event identity
processor identity
configuration version
storage operation
```

This does not make the error itself part of the analytical provenance graph.

Operational failure metadata and analytical provenance remain separate concepts.

## Error Handling and Determinism

Error behavior should be deterministic where practical.

Given the same:

```text
input
configuration
component version
```

a deterministic processor should produce the same success/failure classification.

External dependencies can naturally introduce nondeterminism, such as:

```text
network availability
storage availability
resource exhaustion
```

Such conditions should be represented explicitly rather than hidden.

## Error Handling Across Module Boundaries

Module contracts should document:

```text
Possible failures
Error categories
Recoverability
Retry semantics
Cancellation behavior
Partial-result behavior
Exception behavior
```

For example:

```text
Processor Contract

Input:
    Event

Success:
    Measurement

Possible failures:
    Validation
    State
    Resource

Retry:
    Defined by processor

Cancellation:
    Supported

Exceptions:
    Not part of normal API
```

This allows an AI agent implementing a component to understand its failure behavior without inferring it from unrelated code.

## Error Handling and Storage

Storage operations should distinguish between at least:

```text
Not found
Invalid request
Unavailable
Conflict
Corruption
Permission failure
Resource exhaustion
```

A storage implementation must not collapse every failure into:

```text
STORAGE_ERROR
```

when the caller needs to make different decisions.

## Error Handling and Configuration

Configuration errors should normally be detected as early as possible.

Conceptually:

```text
Configuration
      ↓
Validation
      ↓
Validated Configuration
      ↓
Component Initialization
```

A component should not wait until normal processing to discover a statically invalid configuration.

Runtime-dependent failures may still occur during initialization or operation.

## Error Handling and APIs

External interfaces should translate internal errors into interface-specific representations.

For example:

```text
Core Error
    ↓
Application Error
    ↓
HTTP / CLI / RPC representation
```

The HTTP layer should not force Core to understand HTTP status codes.

Likewise, Core should not expose CLI-specific error formatting.

## Consequences

### Positive

* Expected failures are explicit.
* Error handling is testable without exception interception.
* Module contracts can describe failure behavior.
* Domain errors remain separate from generic infrastructure.
* Processing graphs can make deliberate recovery decisions.
* Error codes can remain stable while messages change.
* AI-generated components have a clear failure-handling contract.

### Negative

* APIs may become more verbose.
* Callers must explicitly handle failures.
* Partial-failure semantics require additional design.
* Some third-party exception-based libraries require adapters.
* A good error taxonomy requires discipline to prevent generic catch-all errors.

## Deferred Decisions

The following remain intentionally undecided:

* exact `Result<T, E>` implementation
* whether `std::expected` or a custom result type is used
* exact error-code representation
* exception policy for specific modules
* logging framework
* error serialization format
* telemetry integration
* retry framework
* cancellation implementation
* process supervision/restart policy

These should be decided when the corresponding architectural or implementation boundary is designed.

## Decision Summary

```text
Expected operational errors:  Explicit results
Domain validation errors:     Explicit results
Internal invariant failures:  Assertions/fail-fast as appropriate
Fatal failures:               Application-level policy
Exceptions:                   Allowed, not default
Error identity:               Stable machine-readable codes
Messages:                     Human-readable, non-contractual
Retry:                        Caller/component policy
Cancellation:                 Distinct from ordinary failure
Partial failure:              Explicitly represented where required
Logging:                      Separate from error propagation
```

## Invariant

> **A failure that a caller is expected to handle must be represented explicitly and must not be hidden behind an implicit control-flow mechanism.**
