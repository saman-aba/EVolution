# ADR 0009 — Error Immutability and Enrichment Semantics

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0006 defines `Result<T>` as the explicit result mechanism.

ADR 0007 defines `Error` as a value-owning type.

ADR 0008 defines copy and move semantics for `Error`.

The remaining question is how an `Error` behaves after construction, particularly when a higher layer needs to add diagnostic information.

The architecture must prevent shared mutable diagnostic state while still allowing errors to acquire useful context as they propagate through system layers.

## Decision

**`Error` is logically immutable after construction.**

An existing `Error` is never modified in place.

Additional context, cause information, or diagnostic metadata is added by producing a **new `Error` value**.

The original error remains unchanged.

The resulting model is:

```text
construct
    ↓
immutable Error
    ↓
inspect / propagate
    ↓
optional enrichment
    ↓
new immutable Error
```

This establishes a single rule for all post-construction behavior.

## Normative API Constraints

The following constraints are **mandatory** for the public `Error` API.

### 1. No public mutators

`Error` must not expose public operations that mutate an existing error's semantic contents.

The API must not provide operations equivalent to:

```cpp
error.set_code(...);
error.set_category(...);
error.set_message(...);
error.add_context(...);
error.set_cause(...);
error.replace_cause(...);
```

An `Error` instance is therefore not an in-place diagnostic builder.

### 2. Enrichment must return a new value

Operations that add context or diagnostic information must return a new `Error`.

Conceptually:

```cpp
Error enriched = error.with_context(...);
```

The input `error` remains unchanged.

### 3. Translation must return a new value

Changing an error to another abstraction-level error must produce a new `Error`.

Conceptually:

```cpp
Error translated = translate(error, ...);
```

Translation must not modify the source error.

### 4. Inspection must be const

Operations that inspect an `Error` must be callable on a `const Error`.

Conceptually:

```cpp
const Error& error = ...;

error.code();
error.category();
error.message();
```

Inspection must not require mutable access.

### 5. No mutable diagnostic references

The API must not expose mutable references, pointers, spans, or views through which callers can modify the internal diagnostic state.

For example, APIs equivalent to:

```cpp
std::string& message();
Context& context();
CauseChain& causes();
```

are forbidden.

Read-only views are permitted where their lifetime and ownership semantics are safe.

### 6. No borrowed mutable storage

An `Error` must not retain caller-owned mutable storage as part of its semantic state.

For example, the stored representation must not depend on:

```cpp
char*;
std::string_view;
std::span<const std::byte>;
T&;
```

when the referenced storage is owned externally and may cease to exist.

Construction APIs may accept non-owning inputs only if the resulting `Error` takes ownership or otherwise guarantees lifetime independently.

### 7. Enrichment must preserve the source error

An enrichment operation must preserve all observable information from its source error unless the operation is explicitly defined as translation or redaction.

In particular, enrichment must not silently change:

```text
code
category
cause
identity
```

### 8. No hidden mutation through shared state

Two `Error` values must never expose shared mutable diagnostic state.

Internal immutable sharing is permitted as an implementation optimization.

Shared mutable state is not.

### 9. Error identity is not object identity

The API must not use memory address or object identity as the semantic identity of an error.

Copying or enriching an `Error` must not implicitly create a new failure occurrence.

### 10. No automatic side effects

Copying, moving, inspecting, enriching, or translating an `Error` must not implicitly:

* log
* emit telemetry
* retry
* modify application state
* create analytical provenance
* perform I/O
* mutate external resources

Error construction and transformation are value operations.

### 11. Move operations preserve semantics

Move construction and move assignment must transfer the represented error without changing its observable meaning.

The moved-to object must contain the error semantics previously represented by the source.

### 12. Moved-from objects are not semantically inspectable

After an `Error` has been moved from, callers must not depend on its previous contents.

The moved-from object remains valid according to normal C++ moved-from semantics and may be destroyed or assigned a new value.

The API must not require callers to inspect a moved-from `Error`.

### 13. Result propagation must preserve the contract

`Result<T>` must not provide an alternate mechanism that permits mutation of its contained `Error`.

Access to an error through `Result<T>` must preserve the same immutability rules.

### 14. Builder types are separate from `Error`

If mutable construction is useful, it must occur through a distinct builder or construction mechanism.

A builder may mutate its own construction state:

```text
ErrorBuilder
    ↓
finalize()
    ↓
Error
```

The resulting `Error` is immutable.

The public `Error` type must not double as a mutable builder.

## Immutability Contract

Once an `Error` becomes an externally visible value, its semantic contents are fixed.

The following are immutable:

```text
error code
error category
message
context
cause information
identity references
source location
other contractually visible diagnostic metadata
```

The public API is therefore inspection-oriented rather than mutation-oriented.

## Enrichment

Enrichment adds diagnostic information to an existing error without modifying it.

Conceptually:

```text
Error A
   │
   │ enrich
   ▼
Error B
```

After enrichment:

```text
A = original error
B = enriched error
```

`A` remains unchanged.

`B` contains the information represented by `A` plus the additional diagnostic information.

For example:

```text
StorageUnavailable
```

may become:

```text
StorageUnavailable
    context:
        operation = "persist analysis"
        analysis_id = 42
```

The exact API is implementation-specific, but the semantic operation is always:

```text
existing Error
    ↓
new Error
```

## Enrichment Does Not Change the Underlying Error

Normal enrichment preserves the existing error's machine-relevant identity.

For example, adding:

```text
operation = "persist analysis"
```

must not silently change:

```text
StorageUnavailable
```

into:

```text
ProcessingFailure
```

Therefore:

```text
Enrichment
    = same failure + additional context
```

rather than:

```text
Enrichment
    = replacement of failure
```

## Enrichment vs Translation

Enrichment and translation are separate operations.

### Enrichment

Adds information while preserving the underlying error.

```text
StorageUnavailable
        ↓
StorageUnavailable
+ operation context
+ object identity
```

### Translation

Creates an abstraction-level error appropriate to the receiving layer.

```text
OS ENOSPC
    ↓
StorageResourceExhausted
```

When useful, translation preserves the original error as cause information.

Therefore:

```text
Enrichment:
    same error meaning + more context

Translation:
    new abstraction-level error + underlying cause
```

Both produce new immutable `Error` values.

## Cause Chain

A translated or higher-level error may preserve the lower-level failure through a cause chain.

Example:

```text
AnalysisPersistenceFailed
    caused by
StorageWriteFailed
    caused by
FilesystemResourceExhausted
```

The resulting `Error` owns the complete diagnostic representation according to ADR 0007.

The original errors remain unchanged.

## Context vs Cause

Context describes circumstances surrounding a failure.

Cause describes another failure that contributed to the current failure.

For example:

```text
Error:
    StorageWriteFailed

Context:
    path = "results/analysis-42"
    operation = "persist"

Cause:
    FilesystemResourceExhausted
```

Context should not be used merely to encode another error.

Cause should not be used merely to store arbitrary operation metadata.

## Efficient Enrichment

Immutability is a semantic property, not a requirement that every enrichment physically copy every byte.

The implementation may use move-aware construction or immutable internal sharing if this can be done without changing observable semantics.

For example:

```text
lvalue:
    Error A → derive/copy → Error B

rvalue:
    Error A → transfer/rebuild → Error B
```

Both must produce the same observable `Error B`.

The implementation must not expose ownership or sharing details through the API.

## Construction vs Enrichment

Construction and enrichment are distinct.

During construction, implementation code may assemble the internal representation before the `Error` becomes externally visible.

Conceptually:

```text
ErrorBuilder
    ↓
finalize
    ↓
immutable Error
```

After finalization, the resulting `Error` follows this ADR's immutability contract.

A mutable builder is therefore an implementation mechanism, not a mutable `Error`.

## Error Identity During Enrichment

Enrichment does not create a new underlying failure occurrence.

For example:

```text
Error A
```

and:

```text
Error B = enrich(A)
```

refer to the same underlying failure unless translation explicitly introduces a new abstraction-level error.

Therefore:

```text
one failure
    ↓
three propagation layers
    ↓
three Error values
```

does not imply:

```text
three failures
```

This distinction matters for logging, telemetry, retries, and diagnostics.

## Error and Copy Semantics

ADR 0008 defines copying as producing an independent semantic value.

Immutability strengthens that contract:

```cpp
Error b = a;
```

means both values remain semantically equivalent, and neither can subsequently be mutated to affect the other.

No copy-on-write behavior is required.

An implementation may internally share immutable data as an optimization, provided that sharing remains invisible and no mutable shared state is exposed.

## Error and Thread Transfer

Because an `Error` is immutable after construction, concurrent inspection does not require synchronization on the error itself.

An error can therefore move through a processing path such as:

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

The containing queue and surrounding object lifetimes still require their normal synchronization rules.

`Error` itself does not require a mutex or mutable reference-counted state.

## Error and Logging

Enrichment never logs.

Copying, moving, and enriching an error do not automatically produce log records.

For example:

```text
copy Error
    ≠
log Error
```

Logging remains an explicit operational concern.

This prevents duplicate logging as an error moves through multiple layers.

## Error and Provenance

Enrichment does not create analytical provenance.

An error may contain provenance identifiers as diagnostic context, but:

```text
enrich(error)
    ≠
new analytical object
```

and:

```text
enrich(error)
    ≠
new provenance event
```

Operational error history and analytical provenance remain separate concerns.

## Error Translation and Information Loss

Higher layers may intentionally hide implementation-specific details at an external boundary.

For example:

```text
FilesystemResourceExhausted
        ↓
Application-facing
PersistenceFailed
```

The external representation may expose only the higher-level error while retaining the detailed cause internally.

Translation should avoid unnecessary information loss within the internal system.

Preferred:

```text
PersistenceFailed
    caused by
StorageWriteFailed
    caused by
FilesystemResourceExhausted
```

rather than silently discarding useful lower-level information.

## Consequences

### Positive

* `Error` has one clear post-construction semantic rule.
* No component can unexpectedly mutate an error owned elsewhere.
* Enrichment has explicit and predictable semantics.
* Copy and move behavior remains straightforward.
* Error propagation is safe across asynchronous and module boundaries.
* Cause chains remain distinguishable from context.
* Logging and provenance remain separate concerns.
* The implementation can optimize storage without changing the public contract.

### Negative

* Enrichment produces a new value.
* Diagnostic data may need to be copied.
* Deep enrichment chains may increase diagnostic storage.
* More sophisticated implementations may eventually require immutable sharing or other storage optimizations.

These are implementation concerns and do not justify mutable `Error` semantics.

## Deferred Decisions

The following remain implementation details:

* exact `Error` class layout
* exact enrichment API names
* explicit builder API
* small-string optimization
* immutable internal sharing
* exact cause representation
* exact context representation
* allocator strategy
* diagnostic string storage

All such implementations must preserve the normative API constraints defined by this ADR.

## Decision Summary

```text
Error mutability:                Immutable after construction
Public mutation:                 Forbidden
Enrichment:                      Produces new Error
Original error:                  Never modified
Context addition:                New Error
Cause addition:                  New Error
Translation:                     New Error
Inspection:                      const-safe
Borrowed mutable state:          Forbidden
Mutable diagnostic references:   Forbidden
Shared mutable state:            Forbidden
Internal immutable sharing:      Permitted
Builder mutation:                Separate builder only
Logging during transformation:   No
Provenance during transformation: No
```

## Invariant

> **An EVolution `Error` is immutable after construction; all public transformations produce new error values, and the API must not expose mutable or borrowed diagnostic state.**
