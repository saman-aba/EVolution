# ADR 0008 — Error Copy and Move Contract

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0006 defines `Result<T>` as an expected-like abstraction containing either a value or an `Error`.

ADR 0007 defines `Error` as a value-owning type whose dynamic diagnostic information remains valid independently of the operation that created it.

The next requirement is to define the exact copy and move semantics of `Error`.

Because `Error` may contain:

* diagnostic messages
* structured context
* cause information
* identifiers
* source-location metadata

its copy and move behavior must be predictable.

The contract must also avoid accidentally introducing shared mutable state or unnecessary ownership complexity.

## Decision

`Error` follows **normal C++ value semantics**.

The required semantic contract is:

```text
Copy:
    produces an independent equivalent Error value.

Move:
    transfers the Error's owned state to the destination.

Destruction:
    releases all state owned by that Error.

Assignment:
    follows the same ownership semantics as construction.
```

An `Error` must not require shared ownership merely to support copying.

## Copy Contract

Given:

```cpp
Error a = make_error(...);
Error b = a;
```

`b` is an independent copy of `a`.

Immediately after copying:

```text
a ≡ b
```

with respect to all observable error information.

Both remain valid independently.

Destroying or modifying `a` must not invalidate `b`.

Likewise, destroying or modifying `b` must not invalidate `a`.

Conceptually:

```text
        copy
   ┌─────────────┐
   │             ▼
Error A      Error B
  owns          owns
  data          data
```

The two errors do not share mutable diagnostic state.

## Copy Equivalence

A copied error must preserve its observable semantics.

The copy should preserve:

```text
error code
error category
diagnostic message
context
cause information
identity references
source-location metadata
other contractually visible diagnostic fields
```

Copying must not silently:

* change the error code
* discard the cause chain
* discard required context
* change the affected object identity
* alter the semantic category

Implementation-specific caches or derived representations do not need to be copied if they can be reconstructed without changing observable behavior.

## Copy Independence

After:

```cpp
Error b = a;
```

the following must be true:

```text
modify(a)
    ↓
b remains valid and semantically unchanged
```

and:

```text
modify(b)
    ↓
a remains valid and semantically unchanged
```

If `Error` is immutable after construction, this requirement is naturally satisfied.

If diagnostic enrichment is supported, enrichment must not unexpectedly mutate another copied error.

## Shared Immutable Data

The architecture does not prohibit an implementation from internally sharing immutable data as an optimization.

For example:

```text
Error A ──┐
          ├── immutable diagnostic storage
Error B ──┘
```

may be acceptable if all of the following remain true:

* sharing is invisible to the API
* data is immutable
* lifetime is correctly managed
* copying remains value-semantic
* no caller-visible synchronization requirement is introduced
* performance characteristics are acceptable

However, shared ownership is **not required by the architecture**.

The default conceptual model remains independent value ownership.

## Move Contract

Given:

```cpp
Error a = make_error(...);
Error b = std::move(a);
```

ownership of `a`'s internal state is transferred to `b` where practical.

The move should avoid unnecessary copying of dynamically allocated diagnostic data.

Conceptually:

```text
Before:

Error A ──owns──> diagnostic data
Error B

After:

Error A

Error B ──owns──> diagnostic data
```

The moved-to object `b` contains the semantic error represented by `a` before the move.

## Moved-From State

After moving from an `Error`:

```cpp
Error b = std::move(a);
```

`a` remains a valid C++ object.

Its exact state is unspecified except for the normal requirements of a moved-from C++ value.

The contract does **not** require:

```text
a == empty
```

or:

```text
a.code() == some special value
```

unless the eventual concrete implementation explicitly defines such behavior.

Callers must not depend on the contents of a moved-from `Error`.

The only required assumption is that `a` may safely be:

* destroyed
* assigned a new value
* otherwise used only according to the concrete type's documented moved-from guarantees

## Move Must Not Change Semantics

Moving an error must not transform its meaning.

If:

```text
a.code() == StorageUnavailable
```

before the move, the resulting `b` must represent the same error.

The move operation must not:

* translate the error
* drop diagnostic context
* truncate the cause chain
* change the error category
* change identity references

unless explicitly documented as an implementation limitation, which is not the intended design.

## Move Assignment

Move assignment follows the same ownership rule:

```cpp
b = std::move(a);
```

After the operation:

```text
b
```

owns the error state formerly represented by `a`.

Any previously owned state in `b` is released or replaced according to normal C++ assignment semantics.

`a` becomes valid but moved-from.

## Copy Assignment

Copy assignment produces an independent semantic copy:

```cpp
b = a;
```

After the operation:

```text
b ≡ a
```

and subsequent independent lifetime or modification of either object must not invalidate the other.

Any previous error state owned by `b` is replaced.

## Self-Assignment

Normal C++ self-assignment must be safe:

```cpp
a = a;
```

The operation must not corrupt or lose the error state.

The implementation should not introduce special externally visible semantics for self-assignment.

## Self-Move Assignment

Self-move assignment:

```cpp
a = std::move(a);
```

is not a normal usage pattern and callers must not depend on a particular semantic result.

The implementation should nevertheless avoid undefined behavior where practical.

No architectural behavior is assigned to self-move.

## Exception Guarantees

Copy and move operations should provide strong practical guarantees where the underlying representation permits them.

In particular:

### Move

Move construction and move assignment should ideally be `noexcept`.

This is desirable because containers such as:

```cpp
std::vector<Error>
```

can then move errors efficiently during reallocation.

The concrete implementation should make `noexcept` decisions based on actual member types.

### Copy

Copying may require allocation if diagnostic data is dynamically stored.

Therefore copy construction and copy assignment may fail through the normal C++ allocation mechanism.

The architecture does not require allocation-free error copying.

If the concrete representation can provide stronger guarantees, it may do so.

## Result Interaction

`Result<T>` must preserve the copy/move semantics of its contained `Error`.

For:

```cpp
Result<T> result;
```

copying a failed result should produce an independent error:

```text
Result A
└── Error A

copy

Result B
└── Error B
```

Moving a failed result should transfer the contained error without requiring unnecessary diagnostic-data copies.

The `Result` implementation must not introduce different error ownership semantics from `Error` itself.

## Returning Errors

The API should support natural return-value usage:

```cpp
Result<T> operation()
{
    return make_error(...);
}
```

or the equivalent project-specific construction mechanism.

The implementation should rely on C++ move elision and move semantics rather than requiring callers to manually manage error ownership.

Callers should not need:

```cpp
new Error(...)
```

or:

```cpp
shared_ptr<Error>
```

to return an error.

## Propagation

Error propagation should preserve value semantics.

Conceptually:

```text
Layer A
    │
    │ Error
    ▼
Layer B
    │
    │ propagated Error
    ▼
Layer C
```

If no additional context is required, propagation should not create unnecessary independent copies merely for architectural correctness.

The implementation should allow efficient movement of the error through the call chain.

When a layer needs to retain the original error while creating a new enriched error, copying is permitted.

## Enrichment

Suppose a lower layer returns:

```text
StorageUnavailable
```

and a higher layer adds:

```text
operation = "persist analysis"
```

The resulting error may be a new value:

```text
Original Error
       ↓
copy / move + enrichment
       ↓
Enriched Error
```

The original and enriched errors must remain independently valid.

Enrichment must not unexpectedly mutate an error already owned by another component.

## Error Immutability

The preferred model is that `Error` behaves as an immutable diagnostic value after construction.

This means callers generally:

```text
construct
→ inspect
→ propagate
```

rather than:

```text
construct
→ share
→ mutate from multiple locations
```

If enrichment is required, APIs should preferably create or return an enriched value rather than relying on shared mutable diagnostic state.

The exact mutability API remains an implementation detail.

## No Pointer Identity Semantics

The identity of an `Error` is its semantic value and associated occurrence information, not its memory address.

Code must not rely on:

```cpp
&error_a != &error_b
```

to determine whether two errors are different.

Likewise, copying an error must not be considered to create a new domain-level failure occurrence merely because a second C++ object exists.

## Error Identity vs Copy

Copying:

```cpp
Error b = a;
```

does not create a new underlying failure event.

Both values describe the same error occurrence unless the surrounding application explicitly assigns a new occurrence identity.

This distinction is important for:

* logging
* telemetry
* provenance
* retries
* diagnostics

The C++ copy operation itself must not generate a new logical error identity.

## Error and Logging

Copying or moving an error must not automatically log it.

For example:

```text
copy Error
    ≠
log Error
```

Logging remains an explicit operational concern.

This prevents accidental duplicate logging when errors move through multiple layers.

## Error and Provenance

Similarly:

```text
copy Error
    ≠
new provenance event
```

The error's provenance information, if present, is copied or moved as diagnostic data.

The act of copying the C++ value does not represent a new analytical or operational event.

## Error and Thread Transfer

Because `Error` follows value semantics, transferring an error between threads should normally be possible through ordinary move/copy operations, subject to the requirements of the containing `Result` and surrounding synchronization.

The error itself must not require a mutex or reference-counted mutable state merely to be transferred.

Example:

```text
worker
  │
  │ Result<T>
  ▼
queue
  │
  │ move
  ▼
consumer
```

The queue owns the transferred value according to its own ownership contract.

## Error Size

Copy/move semantics do not require a particular object size.

However, the implementation should avoid making `Error` unnecessarily large merely to satisfy diagnostic flexibility.

A useful initial target is:

```text
small fixed metadata
+
optional dynamically owned diagnostics
```

rather than embedding large arbitrary buffers directly into every error object.

Actual size and allocation behavior should be measured once the concrete representation is implemented.

## Testing Contract

The concrete `Error` implementation must test:

### Copy construction

```text
copy preserves observable error information
copy remains independently valid
```

### Copy assignment

```text
destination previous state is replaced
source remains unchanged
```

### Move construction

```text
destination receives source semantics
no required diagnostic information is lost
source remains valid as moved-from object
```

### Move assignment

```text
destination previous state is replaced
destination receives source semantics
source remains valid as moved-from object
```

### Independence

```text
copying does not create shared mutable diagnostic state
```

### Cause chain

```text
copy preserves causes
move preserves causes
```

### Context

```text
copy preserves context
move preserves context
```

### Result integration

```text
Result<T> copies and moves Error correctly
```

### Containers

Errors should behave correctly when stored in standard containers such as:

```cpp
std::vector<Error>
```

This is particularly relevant for batch processing.

## Consequences

### Positive

* `Error` behaves predictably as a C++ value.
* Ownership remains easy to reason about.
* Move operations can be efficient.
* Errors can safely be collected in containers.
* Async and batch APIs can transfer errors without special ownership mechanisms.
* No requirement for `shared_ptr` or reference-counted mutable state.
* Copying does not accidentally create a new logical failure occurrence.

### Negative

* Copying diagnostic data may require allocations or string copies.
* Immutable/value-oriented enrichment may create additional error values.
* A sophisticated internal representation may eventually use immutable sharing for performance.

These are acceptable trade-offs for a clear ownership model.

## Deferred Decisions

The following remain implementation details:

* whether `Error` is explicitly immutable
* exact copy/move constructors
* whether move operations are `noexcept`
* small-string optimization
* internal immutable sharing
* exact cause representation
* exact context representation
* allocator strategy
* diagnostic string storage

These must preserve the semantic contract defined here.

## Decision Summary

```text
Error semantics:              Value type
Copy:                         Independent semantic copy
Copy ownership:               Independent / safely immutable
Move:                         Transfer owned state
Moved-from object:            Valid, contents unspecified
Copy may allocate:            Yes
Move should be noexcept:      Preferably yes
Shared mutable state:         Forbidden
shared_ptr required:          No
Pointer identity:             Not semantic
Copy creates new failure:     No
Move changes error meaning:   No
Result propagation:           Preserve value semantics
Thread transfer:              Supported through normal value semantics
```

## Invariant

> **Copying an EVolution `Error` produces an independent value with the same observable error semantics; moving an `Error` transfers those semantics and owned state without changing the meaning of the error.**
