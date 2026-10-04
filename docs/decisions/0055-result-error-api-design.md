# ADR 0055: Result and Error API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will implement its project-level `Result<T>` and `Error` APIs as explicit value-oriented types based on the expected/error model established by ADRs 0005–0009.

The public semantic API will be:

```cpp
evolution::Result<T>
evolution::Error
```

The exact internal representation may change without changing the semantic contract.

The initial implementation must support C++20 and therefore must not require `std::expected`.

---

# 1. Result Semantics

`Result<T>` represents exactly two states:

```text
VALUE
ERROR
```

Conceptually:

```text
Result<T>
├── value
└── error
```

A `Result<T>` cannot simultaneously represent both a successful value and an operational error.

---

# 2. Result<void>

Operations that do not produce a value use:

```cpp
Result<void>
```

Conceptually:

```text
Result<void>
├── success
└── error
```

Success must be distinguishable from failure without requiring a sentinel value.

---

# 3. Success Is Not Optionality

`Result<T>` and `std::optional<T>` have different semantics.

```text
optional<T>
    → value or valid absence

Result<T>
    → successful value or operation failure
```

The API must use the appropriate abstraction rather than treating them as interchangeable.

---

# 4. Not Found

A query may legitimately produce no matching object.

The semantic distinction is:

```text
Query
├── Found
├── Not Found
└── Error
```

Whether this is represented as:

```cpp
Result<std::optional<T>>
```

or another query-specific result type depends on the query contract.

The distinction must remain explicit.

---

# 5. Result Construction

A successful result should be constructible from a valid value.

An error result should be constructible from an `Error`.

Conceptually:

```cpp
Result<T> result = value;
Result<T> result = error;
```

The exact constructors/factory functions are implementation details.

---

# 6. Result Inspection

The API should provide explicit inspection.

Conceptually:

```cpp
result.has_value()
result.has_error()
```

The implementation may provide one canonical representation and derive the other inspection method.

The caller must not need to inspect internal storage.

---

# 7. Value Access

The API should provide explicit value access.

Possible conceptual forms include:

```cpp
value()
value_or(...)
```

The exact API should ensure that invalid value access cannot silently produce an unrelated value.

Accessing a value from an error result is a programmer/API misuse condition rather than an ordinary operational failure.

---

# 8. Error Access

The API should provide read-only access to the contained error when the result represents failure.

Conceptually:

```cpp
const Error& error() const;
```

The caller must not be able to mutate the contained error through this access.

---

# 9. Error Immutability

`Error` remains logically immutable after construction.

Therefore:

```cpp
result.error()
```

must not expose mutable diagnostic state.

If additional context is required:

```cpp
Error enriched = result.error().with_context(...);
```

produces a new error.

---

# 10. Result Value Ownership

A successful `Result<T>` owns its contained value according to the normal value semantics of `T`.

It must not silently borrow a temporary value.

Move-only types must be supported where practical.

For example, a result may conceptually contain:

```cpp
Result<std::unique_ptr<T>>
```

when ownership semantics require it.

---

# 11. Error Ownership

A `Result<T>` owns its contained `Error`.

The error must remain valid independently of the operation that produced it.

No operation-local reference may escape through the result.

---

# 12. Copy Semantics

If `T` and `Error` are copyable, `Result<T>` should support copying.

Copying a result produces an independent value with equivalent observable semantics.

Copying must not:

* log
* mutate global state
* create provenance
* trigger retries
* duplicate external effects

---

# 13. Move Semantics

`Result<T>` should support efficient move construction and assignment where `T` permits it.

Moving a successful result transfers its value.

Moving an error result transfers its error.

The moved-from result remains a valid C++ object but its contained semantic state must not be relied upon unless explicitly documented.

---

# 14. `noexcept`

Move operations should preferably be `noexcept` when the contained type permits it.

The implementation must not mark operations `noexcept` when their actual behavior can violate that guarantee.

---

# 15. Result Propagation

Propagation should preserve the original error semantics.

Conceptually:

```text
Operation A
    ↓
Error E
    ↓
Operation B
    ↓
Error E
```

Operation B may add useful context:

```text
E
 ↓
E + context(B)
```

but must not silently change the original error meaning.

---

# 16. Error Translation

Translation is different from enrichment.

Enrichment:

```text
Same Error
+
Additional Context
```

Translation:

```text
Original Error
      ↓
New abstraction-level Error
      +
Original Error as Cause
```

Translation is appropriate when crossing an abstraction boundary.

For example:

```text
Storage-specific failure
        ↓
Storage abstraction error
```

The underlying cause should remain available when useful.

---

# 17. Error Codes

Every machine-actionable error must have a stable code.

Conceptually:

```cpp
enum class ErrorCode;
```

Codes should represent semantic conditions rather than implementation-specific text.

For example:

```text
InvalidInput
InvalidConfiguration
NotFound
InvalidState
ProcessingFailure
StorageUnavailable
ResourceExhausted
ExternalFailure
Unsupported
Cancelled
InternalFailure
```

The exact enumeration organization may later be refined.

---

# 18. Error Categories

Error category provides broader classification.

Conceptually:

```text
Error
{
    category
    code
    ...
}
```

A category should not replace the specific error code.

---

# 19. Human Messages

Error messages are diagnostic.

They must not be required for normal program logic.

Code must never perform behavior such as:

```text
if message == "something failed"
```

Messages may change without constituting a compatibility change.

---

# 20. Context

Error context should provide structured information useful for diagnosing where the failure occurred.

Examples may include:

```text
component
operation
input identity
processor identity
storage identity
configuration identity
```

Only appropriate information should be included.

---

# 21. Context Ownership

Error context is owned by the `Error`.

It must not retain:

```text
string_view
span
reference
raw pointer
```

to temporary operation data unless the lifetime is explicitly guaranteed by the contract.

The default is ownership.

---

# 22. Cause

An error may contain an underlying cause.

Conceptually:

```text
High-level Error
      ↓
Cause
      ↓
Underlying Error
```

Cause information is diagnostic/semantic context, not a mechanism for control flow.

The exact cause-chain representation remains implementation-defined.

---

# 23. Cause vs Context

The distinction is:

```text
Context
    → information about where/how the error occurred

Cause
    → underlying failure that contributed to the error
```

Example:

```text
Cannot initialize processor
    Context: processor = X

Cause:
    Storage unavailable
```

The two should not be collapsed into one unrestricted metadata structure.

---

# 24. Source Location

The implementation may capture source location information using C++20 facilities such as:

```cpp
std::source_location
```

when useful for diagnostics.

Source location is diagnostic metadata.

It does not define error identity or analytical provenance.

---

# 25. Error Identity

An `Error` object does not automatically have logical identity.

Two copies of the same error value do not represent two separate failures merely because they are two C++ objects.

If a domain or execution system requires a failure occurrence identity, that identity must be modeled separately.

---

# 26. Error and Provenance

Errors may reference provenance identifiers when diagnostically useful.

However:

```text
Error
    ≠
Provenance
```

An error must not automatically become an analytical provenance object.

---

# 27. Error and Logging

Returning an error does not automatically log it.

The component closest to the appropriate operational context may log it according to the observability contract.

This avoids duplicate logging such as:

```text
low-level layer logs
+
middle layer logs
+
application logs
```

for the same failure.

---

# 28. Error and Metrics

Errors do not automatically create metrics.

Operational metrics such as:

```text
processing_failures_total
```

belong to the observability layer.

If a domain intentionally models failures as analytical data, that must be an explicit domain decision.

---

# 29. Error and Retry

An error does not prescribe retry behavior.

For example:

```text
Error(ResourceExhausted)
```

does not automatically mean:

```text
retry
```

The recovery/supervision layer decides what to do according to policy.

---

# 30. Cancellation

Cancellation is represented distinctly from ordinary failure.

It may be represented as:

```text
ErrorCode::Cancelled
```

when an operation returns a `Result`.

This does not mean that cancellation is equivalent to processor malfunction.

---

# 31. Cancellation and Lifecycle

An operation may return:

```text
Result<T> → Error(Cancelled)
```

while the processor remains:

```text
ACTIVE
```

or transitions through:

```text
STOPPING → STOPPED
```

The result describes the operation.

The lifecycle state describes the processor.

---

# 32. Error Categories Are Not Recovery Policies

The following are separate concepts:

```text
Error Category
Error Code
Recoverability
Retryability
Recovery Action
```

For example:

```text
ResourceExhausted
    +
Retryable
```

does not itself prescribe how many retries should occur or with what delay.

---

# 33. Result Transformations

The API should eventually support safe transformations such as:

```text
map
and_then
transform_error
```

where these improve composition without obscuring error semantics.

For example:

```text
Result<A>
    ↓ map
Result<B>
```

and:

```text
Result<A>
    ↓ and_then
Result<B>
```

The exact functional API is an implementation decision.

---

# 34. Error Propagation

A common implementation pattern should be supported:

```text
operation()
    → Result<T>
```

with direct propagation of failure.

The API should make propagation concise without encouraging exceptions or hidden global error state.

---

# 35. No Global Error State

EVolution must not use a global mutable mechanism analogous to:

```text
errno
```

as the primary architectural error channel.

An operation's failure belongs to its returned result.

Thread-local error state may be used internally by external libraries but must be translated into the EVolution error model at the boundary.

---

# 36. Third-Party Exceptions

Third-party libraries may use exceptions.

At an EVolution boundary, those failures should normally be translated into:

```text
Result<T>
```

with an appropriate `Error`.

The translation should preserve useful cause information where possible.

---

# 37. Assertions

Assertions are for programmer invariants.

They must not be used as the normal mechanism for rejecting external input.

For example:

```text
Invalid user configuration
    → Result<Error>

Impossible internal invariant
    → assertion / fail-fast as appropriate
```

---

# 38. Fatal Failures

Some failures cannot meaningfully be represented as ordinary component errors.

Examples may include:

```text
Unrecoverable process corruption
Broken runtime invariant
Explicit fatal environment condition
```

Application/process-level policy determines whether the process terminates.

`Result<T>` does not guarantee that every conceivable failure can or should be recovered from.

---

# 39. Error Safety

Errors must not accidentally contain:

* passwords
* private keys
* authentication tokens
* secret configuration
* sensitive payloads

unless explicitly permitted by a security contract.

Error construction should favor safe diagnostics.

---

# 40. Error Size

The error type should remain reasonably lightweight on the common success path.

Dynamic diagnostic storage may be used when required.

The implementation should avoid mandatory heap allocation for every successful operation merely because the result type can contain an error.

---

# 41. Result Performance

The success path should be efficient.

The implementation should take advantage of:

* value elision
* move semantics
* trivial storage where possible
* compiler optimization

Performance assumptions must be validated with benchmarks where Result appears in hot paths.

---

# 42. Result in Hot Paths

Using `Result<T>` does not imply that failure handling is expensive in the success path.

The implementation should keep the success representation compact.

However, premature custom representations must be avoided until measurements show a need.

---

# 43. Result and ABI

`Result<T>` is a C++ API abstraction.

It must not automatically become the representation of a C ABI.

A C ABI requires explicit translation into stable C representations.

---

# 44. Result and Serialization

`Result<T>` is primarily an operation result and is not automatically a persistence format.

If an application needs to persist an operation result, it must define an explicit serialization model.

---

# 45. Result and Provenance

A successful result does not automatically acquire provenance merely because it was returned through `Result<T>`.

Provenance is attached when the produced object or operation contract requires it.

---

# 46. Batch Results

Batch operations may require more than:

```text
Result<vector<T>>
```

when individual items can independently succeed or fail.

A richer batch result may conceptually contain:

```text
BatchResult
{
    successful
    failed
    cancelled
    rejected
}
```

The exact representation is determined by the batch operation contract.

---

# 47. Partial Failure

Partial failure must never be hidden as ordinary success.

For example:

```text
100 inputs
80 succeeded
20 failed
```

must be represented according to an explicit batch contract.

Returning only the 80 successful results without indicating the 20 failures is not valid unless the operation explicitly defines that behavior as lossy.

---

# 48. Error Translation Boundaries

Expected translation boundaries include:

```text
External Library
      ↓
Core / Module Error

Storage Implementation
      ↓
Storage Interface Error

Domain
      ↓
Application Error

Application
      ↓
Interface-specific Error
```

The lower-level error should be preserved as a cause where useful.

---

# 49. Error Code Ownership

Generic error codes belong to Core where they represent genuinely generic conditions.

Domain-specific error codes belong to the domain.

For example:

```text
InvalidConfiguration
    → Core

InvalidPokerAction
    → Poker domain
```

Core must not accumulate domain-specific error enumerations.

---

# 50. Error Compatibility

Machine-readable error codes are part of an API contract when exposed publicly.

Removing or changing the meaning of a public error code requires an explicit compatibility decision.

Diagnostic messages do not have the same compatibility requirement.

---

# 51. Testing Requirements

The implementation must test at least:

```text id="1ynrdy"
Result success
Result failure
Result<void> success
Result<void> failure
Error code
Error category
Error copy
Error move
Error immutability
Error enrichment
Cause preservation
Result propagation
Cancellation
Nested/translated errors
Batch partial failure where applicable
```

Tests should verify semantic behavior rather than internal storage layout.

---

# 52. Initial API Shape

The first implementation should provide a small API surface approximately equivalent to:

```cpp
namespace evolution
{

template <typename T>
class Result;

class Error;

}
```

The initial implementation should avoid adding dozens of convenience methods before actual usage demonstrates their value.

---

# 53. Deferred Decisions

This ADR does not decide:

* exact `Result<T>` storage representation
* `std::variant` vs custom union/storage
* exact monadic API
* exact error context representation
* exact cause representation
* small-object optimization
* allocator strategy
* error string storage
* source-location storage
* stack-trace support
* error serialization
* C ABI representation
* module-specific error-code organization

---

# Decision Summary

```text
Primary result type:          Result<T>
Void operations:              Result<void>
Semantic model:               expected-like
Success states:               VALUE / SUCCESS
Failure state:                ERROR
Optional absence:             std::optional
Errors:                       Value-owned
Error mutation:               Prohibited
Error enrichment:             New Error value
Error translation:            New Error + cause
Error messages:               Diagnostic only
Error identity:               Not implicit
Error logging:                Separate
Error metrics:                Separate
Retry behavior:               Separate
Cancellation:                 Explicit
Global error state:           Rejected
Third-party exceptions:       Translate at boundary
Assertions:                   Internal invariants
C++ baseline:                 C++20
std::expected:                Future implementation option
C ABI:                        Translation required
Batch partial failure:        Explicit
```

## Invariant

**EVolution represents expected operational failure explicitly through `Result<T>` and immutable, value-owned `Error` objects: errors preserve machine-readable identity and useful context without becoming recovery policy, logging, provenance, or global state, while the API remains independent of the eventual C++20 implementation strategy.**

