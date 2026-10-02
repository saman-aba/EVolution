# ADR 0007 — Error Ownership Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0006 established `Result<T>` as the explicit error-return mechanism and defined a project-level `Error` abstraction.

The remaining question is ownership.

Errors can contain:

* error codes
* categories
* messages
* contextual information
* underlying causes
* object identities
* diagnostic information

These values frequently cross module boundaries and may outlive the stack frame in which they were created.

The ownership model must therefore make error lifetime obvious and must not allow errors to accidentally retain references to temporary data.

At the same time, error creation should remain inexpensive enough for a systems-oriented application.

## Decision

**`Error` is a value-owning type.**

An `Error` owns all dynamic information required to describe itself.

The default ownership rule is:

> **An error returned from an EVolution API must not borrow dynamically allocated data from the operation that created it.**

The error should therefore be safely movable, copyable where practical, and valid independently of the lifetime of the originating operation.

The initial model is:

```text
Error
├── code
├── category
├── primary diagnostic information
├── context frames
└── cause chain
```

All dynamic diagnostic data is owned by the `Error`.

No `Error` may contain an owning raw pointer.

No `Error` may normally contain a non-owning reference or view to temporary application data.

## Ownership Principle

The lifetime relationship is:

```text
Operation
    │
    │ creates
    ▼
  Error
    │
    ├── owns message
    ├── owns context
    └── owns diagnostic cause information
```

After the operation returns:

```text
Operation destroyed
        │
        ▼
     Error remains valid
```

This must hold regardless of whether the error is:

* returned directly
* stored in a `Result`
* propagated through several layers
* queued
* collected in a batch result
* returned from asynchronous processing

## Value Semantics

`Error` should behave conceptually like a value:

```cpp
Error a = make_error(...);
Error b = a;
```

After copying:

```text
a
└── independent owned state

b
└── independent owned state
```

Modifying one error must not unexpectedly modify another.

This avoids hidden shared mutable state.

Move operations should transfer ownership efficiently:

```cpp
Error a = ...;
Error b = std::move(a);
```

After the move, `b` owns the error information.

The moved-from state follows normal C++ moved-from object semantics.

## No Borrowed Error Messages

An error must not store a `std::string_view` pointing to a caller-owned dynamic string.

Avoid:

```cpp
Error(std::string_view message);
```

as a storage representation where the view may outlive its source.

Instead, construction may accept a view for convenience while immediately copying it into owned storage:

```text
temporary input
      ↓
copy
      ↓
Error-owned message
```

For example, this is conceptually acceptable:

```cpp
return make_error("unable to load event");
```

because the resulting error owns the required message data.

The exact construction API is implementation-specific.

## String Literals

Static string literals do not require ownership.

An implementation may internally represent immutable compile-time diagnostic text without allocating.

For example:

```text
"invalid event"
```

has static lifetime.

However, this is an implementation optimization.

The observable ownership contract remains:

> The caller must never need to keep the original message source alive after the `Error` is returned.

## Context Ownership

Structured error context is owned by the error.

For example:

```text
Error
├── code = StorageUnavailable
├── operation = "persist measurement"
├── object_id = "series-42"
└── source = "session-17"
```

The values represented by:

```text
operation
object_id
source
```

must remain valid after the originating operation returns.

Therefore dynamically supplied context is copied or otherwise owned by the `Error`.

## Context Representation

The initial conceptual representation is a sequence of owned context fields.

For example:

```text
ErrorContext
{
    key
    value
}
```

with:

```text
Error
{
    code
    category
    message
    context[]
}
```

Both keys and values are owned by the error.

An arbitrary pointer-based context dictionary is not part of the initial design.

This keeps lifetime and serialization semantics predictable.

## Cause Ownership

An error may contain information about an underlying cause.

The cause must also remain valid after the original operation returns.

The initial ownership model therefore avoids a pointer-based recursive cause chain.

Instead, causes are represented as **owned diagnostic frames**.

Conceptually:

```text
Error
├── primary frame
├── cause frame
├── cause frame
└── cause frame
```

Each frame contains the information necessary to describe that level of failure.

For example:

```text
Application
    ↓
Analysis persistence failed
    ↓
Storage write failed
    ↓
Filesystem unavailable
```

may become:

```text
Error
[
    AnalysisPersistenceFailed,
    StorageWriteFailed,
    FilesystemUnavailable
]
```

The exact container and API are implementation details.

The important property is:

> **The complete diagnostic chain is owned by the top-level `Error`.**

## Why Not `shared_ptr<Error>`?

The initial design does not use:

```cpp
std::shared_ptr<Error>
```

for ordinary cause ownership.

Shared ownership would introduce:

* additional allocation
* reference counting
* more complicated lifetime semantics
* potential cycles
* less obvious ownership

There is no current architectural requirement for multiple independent owners of a cause chain.

A top-level `Error` should own its complete diagnostic representation.

## Why Not `unique_ptr<Error>`?

A recursive:

```cpp
std::unique_ptr<Error>
```

cause chain would provide clear ownership but would make `Error` naturally move-oriented and potentially make copying `Error` more expensive or unavailable.

Since errors are useful as values and may be copied into batch results, logs, tests, or higher-level results, the initial architecture favors a flat owned cause representation instead.

If profiling later demonstrates a strong need for recursive heap-backed causes, that can be reconsidered separately.

## Identity References

An error may identify an affected EVolution object.

For example:

```text
event_id
measurement_id
series_id
processor_id
```

These references should normally use the project's value-based identity representation.

The error should not retain ownership of the referenced domain object merely because it reports an error concerning that object.

Therefore:

```text
Error
    └── references object identity
```

does **not** mean:

```text
Error
    └── owns object
```

This distinction is important.

## Error Does Not Own Domain Objects

An error must not implicitly retain:

```text
Event
State
Measurement
Processor
Storage
Analysis
Context entity
```

unless the error contract explicitly requires a serialized/value representation of part of that object.

For example:

```text
Error
{
    code: InvalidEvent,
    event_id: 1234
}
```

is preferable to:

```text
Error
{
    code: InvalidEvent,
    event: shared_ptr<Event>
}
```

The first reports the affected object.

The second creates an unintended lifetime dependency.

## Source Location

Diagnostic source location may be stored using C++20 `std::source_location`.

This is a value-type diagnostic descriptor and does not imply ownership of the source file or function strings.

Conceptually:

```text
Error
{
    code
    source_location
}
```

The source-location information is diagnostic metadata, not application data.

Source location should be captured at the point where the error is constructed when useful.

## Error and Provenance References

An error may contain identifiers referring to provenance information.

For example:

```text
configuration_id
processor_id
source_id
input_id
```

The error owns the identifier value but does not own the referenced provenance object.

This preserves the distinction:

```text
owned reference value
        ≠
owned referenced object
```

## Error and External Resources

An `Error` must not normally own external resources such as:

```text
file descriptors
sockets
database connections
mutexes
transactions
buffers owned by another subsystem
```

An error describes the failure of an operation involving those resources.

Resource cleanup remains the responsibility of the component that owns the resource.

For example:

```text
Storage
   │
   ├── owns database connection
   │
   └── returns Error
```

The returned error must not keep the database connection alive.

## Error and Allocators

The initial error model does not introduce a custom allocator requirement.

Normal error storage should use ordinary C++ value ownership.

If a high-frequency subsystem later requires:

* arena allocation
* monotonic allocation
* per-thread allocation
* embedded storage
* allocation-free error paths

that should be evaluated separately against measured requirements.

The architectural ownership rule does not mandate a particular allocator.

## Error and Thread Safety

An `Error` value should be safe to transfer between threads when copied or moved according to normal C++ value semantics.

The error itself should not contain mutable shared state.

This allows:

```text
worker thread
    ↓
Result<T>
    ↓
queue
    ↓
application thread
```

without requiring the error object itself to contain synchronization primitives.

## Error and Asynchronous Processing

For asynchronous operations, the returned or delivered `Error` must own all information necessary to remain valid after the initiating call has returned.

For example:

```text
submit()
   ↓
operation continues
   ↓
worker fails
   ↓
Error created
   ↓
callback / queue / future
```

The error must not depend on the stack or lifetime of `submit()`.

## Error and Batch Results

A batch result may contain multiple errors:

```text
BatchResult
├── successful items
├── Error
├── Error
└── Error
```

Each error independently owns its diagnostic information.

The batch result owns the collection of errors.

There is no shared mutable diagnostic state between errors unless a future optimization explicitly introduces immutable sharing.

## Error Copying

Copying an error must preserve its observable meaning.

For:

```text
Error A
```

and:

```text
Error B = A
```

the following should remain equivalent:

```text
code
category
message
context
cause information
diagnostic source
```

The two objects should nevertheless own independent mutable storage.

## Error Equality

Error ownership does not imply that errors are directly comparable as complete values.

Two errors may have the same:

```text
code
```

without being identical errors.

For example:

```text
Error A:
    STORAGE_UNAVAILABLE
    series = 1

Error B:
    STORAGE_UNAVAILABLE
    series = 2
```

They represent different occurrences.

Programmatic handling should normally compare stable error codes or explicitly defined fields rather than entire error objects.

## Lifetime Rule

The central lifetime rule is:

```text
Error lifetime
    ≥
all dynamic diagnostic data required by Error
```

There must be no requirement for:

```text
caller-owned string
caller-owned buffer
temporary object
stack variable
domain object
resource handle
```

to remain alive merely because an `Error` references information associated with it.

## Ownership Summary

```text
┌──────────────────────────────────────────────┐
│                    Error                    │
├──────────────────────────────────────────────┤
│ Owns                                        │
│  • diagnostic message                       │
│  • context values                           │
│  • cause/diagnostic frames                  │
│  • copied identity values                   │
│  • error metadata                            │
│                                              │
│ References but does not own                  │
│  • affected domain objects                  │
│  • provenance objects                       │
│  • external resources                       │
│                                              │
│ Never owns implicitly                        │
│  • sockets                                   │
│  • files                                     │
│  • database connections                     │
│  • processor instances                      │
│  • domain state                              │
└──────────────────────────────────────────────┘
```

## Consequences

### Positive

* Error lifetime is predictable.
* Errors can safely cross asynchronous and module boundaries.
* No dangling diagnostic references.
* Cause chains have clear ownership.
* Errors remain ordinary value-like data.
* Domain objects are not accidentally kept alive.
* External resources remain owned by their responsible components.
* The model is compatible with C++20.

### Negative

* Diagnostic strings and context may require copying.
* Large diagnostic payloads increase error size or allocation cost.
* Cause chains require explicit representation.
* Highly optimized error paths may eventually need specialized allocation strategies.

These are acceptable defaults until actual measurements demonstrate a need for more complex ownership mechanisms.

## Deferred Decisions

The following remain open:

* exact `Error` class layout
* small-string optimization
* exact context representation
* exact cause-frame representation
* source-location capture policy
* error serialization
* custom allocator support
* compact error representation for hot paths

These should be resolved when the concrete Core API is designed.

## Decision Summary

```text
Error ownership:             Value ownership
Dynamic diagnostic data:     Owned
Messages:                    Owned
Context:                     Owned
Cause chain:                 Owned diagnostic frames
Domain objects:              Not owned
Provenance objects:          Not owned
External resources:          Not owned
Raw owning pointers:        Forbidden
Borrowed dynamic strings:   Forbidden in stored Error state
Shared mutable state:       Forbidden
Thread synchronization:     Not part of Error
Custom allocation:          Deferred
```

## Invariant

> **An EVolution `Error` is self-contained: after an error leaves the operation that created it, all information required to interpret that error remains valid without extending the lifetime of the originating operation, domain objects, or external resources.**
