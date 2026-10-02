# ADR 0006 — Result and Error Type

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0005 established that EVolution uses explicit error results for expected operational failures.

The project now needs a concrete C++ representation.

The representation must work across:

* Core
* Processing
* Domain
* Analysis
* Storage
* Ingestion
* Applications
* Interfaces

It must also remain lightweight enough for a systems-oriented project.

The error mechanism should not force exceptions into normal control flow, introduce unnecessary heap allocations, or make simple functions unnecessarily complicated.

## Decision

EVolution will use **`std::expected<T, E>` as the standard result mechanism**, with a project-defined `Error` type as the error value.

The conceptual API is therefore:

```cpp
std::expected<T, Error>
```

and for operations that do not return a value:

```cpp
std::expected<void, Error>
```

C++20 is the project language baseline, while `std::expected` is standardized in C++23. Therefore, EVolution will provide a small compatibility abstraction rather than requiring C++23 throughout the project.

The project-level abstraction will conceptually be:

```cpp
evolution::Result<T>
```

with semantics equivalent to:

```cpp
std::expected<T, Error>
```

where the implementation may use:

* `std::expected` when the project eventually adopts C++23, or
* a small C++20-compatible implementation while C++20 remains the baseline.

The rest of the architecture must depend on `Result<T>`, not directly on a particular implementation.

## Rationale

### Explicit control flow

Expected failures remain visible in the function signature:

```cpp
Result<Measurement> calculate(...);
```

rather than being hidden behind exceptions.

### Standard semantic model

`std::expected` provides an established semantic model for:

```text
success → value
failure → error
```

The project should not invent a fundamentally different abstraction when the standard library already defines the desired semantics.

### C++20 compatibility

The project has deliberately selected C++20 as its language baseline.

Therefore introducing C++23 solely for `std::expected` would unnecessarily raise the language requirement.

A project-level `Result<T>` allows the architectural API to remain stable.

## Result Type

Conceptually:

```cpp
template<typename T>
using Result = /* expected-like<T, Error> */;
```

For a successful operation:

```cpp
Result<int> result = 42;
```

For failure:

```cpp
Result<int> result = unexpected(error);
```

For operations without a meaningful value:

```cpp
Result<void>
```

The exact syntax of the compatibility implementation is an implementation detail.

## Result Semantics

A `Result<T>` has exactly one of two states:

```text
VALUE
ERROR
```

It must never represent both simultaneously.

Conceptually:

```text
Result<T>
├── value: T
└── error: Error
```

Only one is active.

## No Sentinel Errors

Functions should not use arbitrary values to represent failure when the return value has a legitimate domain meaning.

Avoid:

```cpp
int find_value(); // -1 means error
```

when `-1` could otherwise be meaningful.

Prefer:

```cpp
Result<int> find_value();
```

Similarly, avoid:

```text
nullptr = failure
empty string = failure
zero = failure
false = failure
```

unless the API explicitly defines that representation as part of its semantics.

## Optional vs Error

`Result<T>` and optional values represent different concepts.

Use an optional-like representation when:

```text
No value is a valid outcome.
```

Use `Result<T>` when:

```text
The operation failed to produce its result.
```

For example:

```text
find_measurement(id)
    → optional<Measurement>
```

may mean:

```text
measurement does not exist
```

while:

```text
load_measurement(id)
    → Result<Measurement>
```

may distinguish:

```text
found
not found
storage unavailable
corrupted data
permission failure
```

The API contract determines which semantic model is correct.

## Error Type

`Error` is a generic infrastructure type.

Conceptually:

```cpp
struct Error {
    ErrorCode code;
    ErrorCategory category;
    // optional contextual information
};
```

The initial design should keep the error object small.

An error should not automatically contain large strings, stack traces, serialized objects, or arbitrary metadata.

Large diagnostic information should be attached through appropriate mechanisms when required.

## Error Code

The error code provides stable machine-readable identity.

Conceptually:

```cpp
enum class ErrorCode {
    InvalidInput,
    InvalidConfiguration,
    NotFound,
    InvalidState,
    ProcessingFailure,
    StorageUnavailable,
    ResourceExhausted,
    ExternalFailure,
    Unsupported,
    Cancelled,
    InternalFailure
};
```

This is only the generic Core-level taxonomy.

Domain-specific errors may extend the model without modifying the meaning of unrelated Core errors.

The exact representation of domain error namespaces remains to be designed.

## Error Category

Category provides broader classification than the specific error code.

Conceptually:

```text
Category
    ↓
Code
```

For example:

```text
Storage
    ├── Unavailable
    ├── PermissionDenied
    ├── Corrupted
    └── Conflict
```

The distinction allows callers to handle broad classes of failures without requiring knowledge of every individual code.

## Error Message

A human-readable message is optional.

Messages are:

* diagnostic
* non-contractual
* not suitable for programmatic comparison

Code should be used for machine decisions.

Therefore:

```cpp
if (error.code() == ErrorCode::NotFound)
```

is valid API behavior.

While:

```cpp
if (error.message() == "object not found")
```

is not.

## Error Context

Context should be structured where practical.

Examples:

```text
object identity
operation name
component
source
configuration identifier
```

The initial `Error` type should not become a generic key-value bag.

If arbitrary metadata becomes necessary, that requirement should be justified by an explicit architectural decision.

## Error Cause

Errors may reference an underlying cause.

Conceptually:

```text
Application error
    ↓
Storage error
    ↓
OS error
```

The cause chain should preserve useful diagnostic information.

However, cause chains should not become mandatory for every error.

Simple errors should remain cheap.

## Ownership and Lifetime

An error returned from a function must own or safely reference all information required to remain valid after the function returns.

An error must not contain dangling references to:

```text
local variables
temporary strings
stack objects
temporary buffers
```

The initial implementation should prefer value ownership and simple lifetime rules.

Non-owning views may be used only when their lifetime is explicit and guaranteed.

## Allocation Policy

Error construction should not require heap allocation for ordinary errors whenever practical.

This is particularly important for:

* high-frequency processing
* packet/event processing
* tight analytical loops
* low-latency components

However, diagnostics for unusual failures may reasonably allocate.

The architecture therefore distinguishes:

```text
Normal success path
        ↓
minimal overhead

Exceptional failure path
        ↓
diagnostic information may be more expensive
```

The exact small-object/heap strategy is an implementation detail.

## Error Construction

Errors should be easy to construct at the point where the failure is detected.

Conceptually:

```cpp
return unexpected(make_error(
    ErrorCode::InvalidInput
));
```

or an equivalent project API.

The construction mechanism should avoid requiring callers to manually populate unrelated fields.

## Error Propagation

Propagation should preserve the original error unless additional context provides genuine value.

Conceptually:

```text
low-level error
      ↓
add context
      ↓
return
```

A layer should not unnecessarily translate:

```text
STORAGE_UNAVAILABLE
```

into:

```text
PROCESSING_FAILURE
```

and discard the original code.

Higher-level translation is appropriate when the abstraction boundary genuinely changes the meaning.

## Error Translation

Example:

```text
Filesystem
    ↓
Storage
    ↓
Application
```

The filesystem implementation may report an OS-specific error.

The Storage module may translate it into:

```text
StorageUnavailable
```

while preserving the underlying cause.

The Application may then report:

```text
Unable to persist analysis result
```

without exposing filesystem implementation details.

## Exceptions and Result

The normal internal API is:

```text
Result<T>
```

not:

```text
throw
```

Third-party APIs may nevertheless throw.

Such APIs should be adapted at their boundary:

```text
Third-party exception
        ↓
Adapter
        ↓
Result<T, Error>
        ↓
EVolution
```

This prevents exception semantics from leaking unnecessarily through the architecture.

## Exceptions and Programming Errors

Exceptions may still be used when an invariant or construction failure is more naturally represented through an exception-based boundary.

However, this must remain explicit.

A component must not simultaneously provide ambiguous semantics such as:

```text
sometimes returns Error
sometimes throws for the same failure
```

unless the contract explicitly defines the distinction.

## `Result<void>`

Operations with no meaningful return value should use:

```cpp
Result<void>
```

rather than inventing dummy values:

```cpp
Result<bool>
Result<int>
Result<Status>
```

when those values do not represent actual operation results.

For example:

```cpp
Result<void> save(const Measurement&);
```

means:

```text
success
or
error
```

## Nested Results

APIs should avoid unnecessary nesting such as:

```cpp
Result<optional<T>>
```

unless the distinction between:

```text
operation failure
no value
value
```

is genuinely meaningful.

When this distinction is necessary, nested semantics must be documented clearly.

Otherwise the API should use a simpler representation.

## Error Handling in Hot Paths

The presence of `Result<T>` does not imply that every successful event-processing operation must perform expensive error-related work.

The implementation should allow the success path to remain lightweight.

For example:

```text
Fast path:
    process event
    produce measurement

Failure path:
    construct Error
    propagate failure
```

Performance-sensitive components should be benchmarked rather than assuming a particular implementation cost.

## Result and Partial Failure

`Result<T>` represents the outcome of one logical operation.

Batch and stream operations may require richer result types.

For example:

```text
BatchResult
{
    successful_count
    failed_count
    errors
}
```

A batch API should not force every item failure into one `Result<void>` if partial success is part of the contract.

## Result and Cancellation

Cancellation may be represented as an `Error` when cancellation exits through the same result channel:

```text
Result<T>
    ├── Value
    └── Error(Cancelled)
```

The caller must still be able to distinguish cancellation from ordinary operational failure.

## Result and Concurrency

`Result<T>` does not define concurrency semantics.

It may be passed between threads when `T` and `Error` satisfy the required C++ object/thread-safety properties.

Thread ownership and synchronization remain responsibilities of the surrounding component.

## Result and Serialization

`Result<T>` is an internal programming abstraction.

It is not automatically a wire or storage format.

External interfaces should translate it into their own representation:

```text
Result<T>
    ↓
Application
    ↓
Interface-specific response
```

Similarly, errors persisted for auditing or diagnostics require an explicit persistence schema.

## Result and Provenance

A failed operation does not automatically produce analytical provenance.

If a failure needs diagnostic provenance, the error may reference:

```text
source
object identity
processor
configuration
operation
```

but this remains operational error context.

## Testing

Every component returning `Result<T>` should test at least:

```text
success path
expected failure
error code
error propagation
boundary translation
```

When applicable:

```text
cancellation
partial failure
retry behavior
resource failure
```

Tests should verify error semantics rather than human-readable messages.

## C API Boundary

If a C-compatible API is required, `Result<T>` should not cross the C ABI directly.

Instead, the interface should use a C-compatible representation such as:

```text
return code
output parameter
error object
```

The C++ implementation can convert between:

```text
C ABI
    ↕
Result<T>
```

This keeps the C++ error abstraction independent of ABI constraints.

## Consequences

### Positive

* Explicit failures become part of the API contract.
* The architecture remains compatible with C++20.
* The project can migrate to `std::expected` without redesigning every interface.
* Error handling remains lightweight on normal paths.
* Exceptions remain isolated from ordinary operational control flow.
* Domain and infrastructure errors can share a common mechanism.

### Negative

* A compatibility implementation is required while using C++20.
* APIs become more explicit and sometimes more verbose.
* Error lifetime and allocation behavior require deliberate design.
* Batch and partial-failure APIs need additional result types where necessary.

## Deferred Decisions

The following remain open:

* exact C++20 `Result<T>` implementation
* whether the compatibility type is vendored or implemented internally
* exact `Error` storage representation
* error-code namespace mechanism
* diagnostic string storage
* source-location support
* stack-trace support
* serialization of errors
* logging integration
* ABI requirements

These should be addressed only when the corresponding implementation need arises.

## Decision Summary

```text
Primary result mechanism:     Result<T>
Semantic model:               expected-like
C++20 baseline:               Required
C++23 std::expected:          Future implementation option
Expected failures:            Return Result<T>
Normal exception flow:        No
Error identity:               Stable error code
Messages:                     Diagnostic only
Cause/context:                Optional
Hot-path allocation:          Avoid where practical
C ABI:                        Separate translation layer
Batch partial failure:        Dedicated result semantics
```

## Invariant

> **EVolution component APIs represent expected operational failure explicitly through `Result<T>` and must not make callers depend on exceptions or human-readable messages to determine normal failure conditions.**
