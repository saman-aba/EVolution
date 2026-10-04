# ADR 0061: State and Projection API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will distinguish between **State** and the **Projection** that derives State from historical information.

```text
Event Stream
    ↓
Projection
    ↓
State
```

A State represents what is currently known to be true within a defined scope as a consequence of previously processed information.

A Projection defines how Events are transformed into State.

The Core provides generic State and Projection mechanisms.

Domains define the semantic meaning of concrete State and the rules by which Events affect it.

---

# 1. State

State represents the current or reconstructed condition of a domain object or scope.

Conceptually:

```text
State
{
    scope
    scope_id
    version
    data
}
```

The exact representation remains implementation-defined.

---

# 2. State vs Event

An Event describes something that happened.

State describes what is true as a consequence.

```text
Event:
    PlayerAction(fold)

State:
    player.status = folded
```

An Event is historical information.

State is a derived representation.

---

# 3. State vs Measurement

State is not inherently quantitative.

For example:

```text
State:
    hand.phase = FLOP
    player.position = BUTTON
    player.status = ACTIVE
```

A Measurement may subsequently derive:

```text
hands_played = 100
win_rate = 0.42
```

State describes condition.

Measurement describes quantity.

---

# 4. State Scope

Every State must have an explicit semantic scope.

Examples:

```text
table
session
hand
player
market
instrument
system
```

The scope determines which Events are relevant.

---

# 5. State Identity

A State may identify the logical object whose state is represented.

For example:

```cpp
using StateId = evolution::Id<StateTag>;
```

However, not every temporary state representation requires a persistent StateId.

State identity semantics depend on whether the State represents:

```text
a logical domain entity
a projection instance
a snapshot
a temporary execution state
```

These identities must not be conflated.

---

# 6. State Version

A State must be associated with the point in history from which it was derived.

Conceptually:

```text
State
{
    ...
    version
}
```

The version may identify:

```text
event sequence
event identity
projection position
domain revision
```

depending on the projection contract.

---

# 7. State Version vs State Identity

These are different.

```text
StateId
    → identifies the logical state-bearing object

Version
    → identifies the historical point represented by the state
```

Processing the next Event normally changes the represented version without changing the identity of the logical domain object.

---

# 8. State Immutability

Historical State representations should normally be treated as immutable values.

An executing Projection may maintain mutable internal state for efficiency, but externally visible State snapshots should have value semantics.

This distinguishes:

```text
Projection execution state
```

from:

```text
State representation
```

---

# 9. Mutable Projection State

A Projection may internally maintain mutable state:

```text
Projection
    current_state
    apply(event)
```

This is an implementation optimization.

It must not change the semantic model that:

```text
Events + initial state → resulting state
```

---

# 10. Projection

A Projection defines the transformation from historical information to State.

Conceptually:

```text
Projection
{
    initial_state
    apply(state, event)
    resulting_state
}
```

The exact API remains implementation-defined.

---

# 11. Projection as a Contract

A Projection must define:

```text
input event types
state type
initial state
transition rules
validation rules
output state
failure behavior
configuration
provenance
```

---

# 12. State Transition

The fundamental operation is:

```text
Initial State + Event
        ↓
    Projection
        ↓
   New State
```

For an event sequence:

```text
S0 + E1 → S1
S1 + E2 → S2
S2 + E3 → S3
```

Therefore:

```text
S0 + E1 + E2 + E3 → S3
```

---

# 13. Replay

A State should be reconstructable by replaying the relevant Event stream when the required historical data is available.

```text
Events
  ↓
Replay
  ↓
Projection
  ↓
State
```

Replay is therefore a primary mechanism for State reconstruction.

---

# 14. Deterministic Projection

For a deterministic Projection:

```text
Initial State
+
same Event sequence
+
same configuration
+
same relevant execution context
```

must produce an equivalent State.

Uncontrolled external state must not silently influence deterministic projection.

---

# 15. Projection Configuration

Projection behavior may depend on explicit configuration.

Examples:

```text
validation mode
event compatibility
domain rules version
historical interpretation
```

Configuration follows the Configuration Model.

It must not be hidden in mutable global state.

---

# 16. Projection Version

A Projection implementation may have its own version.

This is distinct from:

```text
State version
Event schema version
Metric version
Configuration version
Application version
```

A State's provenance should identify the projection version when it materially affects reproducibility.

---

# 17. Projection vs Processor

A Projection is a semantic concept.

A Processor is an execution contract.

A Projection may be implemented by a Processor:

```text
Processor
    executes
        Projection
```

but the two concepts must not be merged.

The Projection defines what transformation means.

The Processor defines how a transformation participates in EVolution processing.

---

# 18. State Transition Validation

A Projection may reject an Event if applying it would violate domain invariants.

Examples:

```text
hand already finished
player does not exist
invalid phase transition
negative balance where forbidden
```

The exact meaning belongs to the domain.

---

# 19. Invalid Event vs Invalid State Transition

These are distinct.

```text
Invalid Event
    → event itself violates its structural/semantic contract

Invalid State Transition
    → event is valid but cannot be applied to the current state
```

The appropriate error category should preserve this distinction where useful.

---

# 20. Unknown State

A Projection may not have enough information to construct complete State.

This must not silently become a fabricated default.

For example:

```text
unknown player balance
```

must not automatically become:

```text
balance = 0
```

unless the domain explicitly defines that default.

---

# 21. Partial State

A State may be intentionally partial.

For example:

```text
known:
    player position
    hand number

unknown:
    private cards
```

The representation must distinguish:

```text
known value
unknown value
not applicable
```

when required by the domain.

---

# 22. Initial State

Every Projection must define how its initial State is established.

Possible forms:

```text
empty state
domain-defined initial state
snapshot
external baseline
```

The initial State is part of reproducibility.

---

# 23. Snapshot

A Snapshot is a persisted representation of State at a particular historical point.

```text
Event Stream
    ↓
Projection
    ↓
Snapshot
```

A snapshot is an optimization for recovery or faster reconstruction.

It does not automatically replace the historical Event stream as the authoritative source.

---

# 24. Snapshot Version

A Snapshot must identify the State point it represents.

Conceptually:

```text
Snapshot
{
    state
    state_version
    projection_version
    provenance
}
```

The exact representation remains deferred.

---

# 25. Snapshot Compatibility

A Projection must determine whether a Snapshot is compatible with the current:

```text
projection version
configuration
state schema
domain schema
```

An incompatible Snapshot must not be silently treated as valid State.

---

# 26. Incremental Projection

A Projection should be able to process Events incrementally when its semantics permit.

```text
E1 → S1
E2 → S2
E3 → S3
```

This avoids reconstructing State from the beginning for every Event.

---

# 27. Batch Reconstruction

A Projection may also reconstruct State from a batch:

```text
Events[0..N]
    ↓
Projection
    ↓
State
```

Batch and incremental processing must produce semantically equivalent results when the Projection declares that guarantee.

---

# 28. Event Ordering

Projection semantics must define the ordering expected by the State transition rules.

Possible ordering sources include:

```text
event sequence
event time
input order
domain-defined ordering
```

Event time must not automatically be treated as a total ordering.

---

# 29. Out-of-Order Events

A Projection may receive Events out of event-time order.

The Projection contract must define whether it:

```text
rejects them
buffers them
reorders them
supports correction
reconstructs affected State
```

This is not globally decided by the Core.

---

# 30. Duplicate Events

Duplicate delivery is not necessarily duplicate historical information.

The Projection must define deduplication semantics using Event identity or another explicit domain rule.

It must not assume that:

```text
same payload == same Event
```

---

# 31. Event Corrections

Historical corrections must be explicit.

Possible mechanisms include:

```text
correction event
replacement record
new event version
reprojection
```

A State must not silently mutate historical meaning without a traceable cause.

---

# 32. Projection Failure

Projection failure must use the Error/Result model.

Conceptually:

```text
Result<State>
```

or an equivalent processing result.

Expected transition failures must not require process termination.

---

# 33. Projection Failure and Processor Failure

A single invalid Event does not necessarily mean that the Processor itself has failed.

For example:

```text
Event rejected
    → operation failure

Projection cannot continue under its contract
    → processor failure
```

The distinction follows the Processing lifecycle and execution models.

---

# 34. State Provenance

Derived State should identify, where required:

```text
source Event stream
initial State
projection identity/version
configuration
relevant execution context
historical position
```

This allows State reconstruction and debugging.

---

# 35. State and Provenance

Provenance answers:

```text
Where did this State come from?
```

State itself answers:

```text
What is currently represented?
```

These must remain separate.

---

# 36. State and Context

Context describes circumstances surrounding State.

For example:

```text
State:
    player.stack = 100 BB

Context:
    table = T1
    game_type = NLHE
    stakes = 1/2
```

Context may be required to interpret State, but must not be silently embedded into generic State representation.

---

# 37. State and Measurements

Measurements may be calculated from State:

```text
State
   ↓
Measurement
```

For example:

```text
State:
    player.actions = 50
    player.folds = 20

Measurement:
    fold_rate = 0.40
```

The Measurement should retain appropriate provenance.

---

# 38. Multiple Projections

The same Event stream may feed multiple Projections.

```text
             ┌→ TableState
Events ──────┼→ PlayerState
             ├→ SessionState
             └→ StatisticsState
```

Each Projection has independent semantics.

A change to one Projection must not redefine another.

---

# 39. Projection Composition

Projections may depend on other derived State when explicitly declared.

For example:

```text
Events
   ↓
Base State
   ↓
Derived Projection
   ↓
Derived State
```

Such dependencies must be represented in the processing/provenance model.

---

# 40. Projection Cycles

Projection dependencies must not create implicit cycles.

For example:

```text
State A → Projection B → State B
State B → Projection A → State A
```

is invalid unless a specific iterative model explicitly supports it.

---

# 41. State Hierarchies

Domains may define hierarchical State:

```text
Session
 ├── Table
 │    └── Hand
 │         └── Player
```

The hierarchy is domain-specific.

Core only provides mechanisms for representing and processing state relationships.

---

# 42. State Ownership

A Projection owns its mutable execution state according to the Ownership Model.

Externally returned State values are owned by their recipient unless explicitly represented as borrowed views.

No State API should require callers to retain pointers to Projection internals.

---

# 43. State Lifetime

A State snapshot may outlive:

```text
Projection instance
Processor
Execution run
Application process
```

when persisted or copied.

Therefore State representation must not depend on the lifetime of its producing Projection.

---

# 44. State Equality

State equality is domain-dependent.

Structural equality may be useful for testing.

However, two State representations may be semantically equivalent while differing in:

```text
cache fields
derived implementation fields
representation
internal ordering
```

Therefore the Core must not impose universal semantic equality rules.

---

# 45. State Serialization

Serialization of State must preserve all information required by the State contract.

Where applicable:

```text
State identity
scope
scope identity
state version
domain schema version
projection version
data
provenance
```

The serialization format remains implementation-defined.

---

# 46. State Persistence

State may be:

```text
ephemeral
cached
recoverable
derived
authoritative
```

according to the Storage Model.

A State being persisted does not automatically make it authoritative.

---

# 47. State as Cache

A State snapshot may exist only to accelerate reconstruction.

In that case:

```text
Event stream = authoritative
State snapshot = cache
```

The distinction must be explicit.

---

# 48. State as Authoritative Data

Some domains may intentionally treat State as authoritative.

For example, an external system may provide a current account state without exposing its complete event history.

In this case the State is source data rather than a projection of EVolution Events.

The provenance model must distinguish this case.

---

# 49. External State Import

External State may enter EVolution through ingestion.

It must not be assumed to have been generated by an internal Projection.

For example:

```text
External State
    ↓
Ingestion
    ↓
Normalized representation
    ↓
EVolution State / Event
```

The exact mapping is domain-specific.

---

# 50. State Reconstruction from Measurements

Measurements generally should not be treated as a substitute for State.

A Measurement may be derived from State, but it normally does not contain enough semantic information to reconstruct the original State.

---

# 51. State and Time

State may represent:

```text
current state
state at event position N
state at event time T
state at processing time T
```

The temporal interpretation must be explicit.

---

# 52. Historical State Query

A historical State query should identify the requested historical boundary.

For example:

```text
State at event sequence 100
State at event E123
State at event time T
```

These are not necessarily equivalent.

---

# 53. Current State

"Current" must not implicitly mean:

```text
latest wall-clock time
```

It means the latest State according to the relevant data/processing consistency contract.

---

# 54. State Consistency

A State query must define its consistency semantics where required.

Possible semantics include:

```text
latest processed
event-time consistent
snapshot
versioned
run-specific
```

The Query Model governs the external query contract.

---

# 55. Projection Determinism and Concurrency

A Projection may execute concurrently only when its contract permits it.

Parallel processing must preserve semantic guarantees such as:

```text
ordering
partitioning
state isolation
determinism
```

Concurrency belongs to the Processing layer, not State semantics.

---

# 56. Partitioned State

State may be partitioned by an explicit key.

For example:

```text
player_id
table_id
session_id
instrument_id
```

The partition key must have semantic meaning.

Partitioning for execution efficiency must not accidentally change State semantics.

---

# 57. State Merge

Independent State partitions may be merged only if the domain explicitly defines a valid merge operation.

Not every State is mergeable.

For example:

```text
sum-like state
```

may be mergeable, while:

```text
ordered game state
```

may not be.

---

# 58. State Reset

A Projection may support resetting to an initial State.

Reset semantics must explicitly define:

```text
initial state
configuration
version
provenance
```

Reset must not silently erase persisted historical information.

---

# 59. Projection API Shape

The initial API should conceptually resemble:

```cpp
namespace evolution::state
{

class State;
class Projection;

}
```

A more domain-specific implementation may use strongly typed State and Projection types without requiring a universal runtime polymorphic State base class.

---

# 60. Typed Projection

The preferred conceptual model is typed:

```text
Projection<Event, State>
```

rather than:

```text
Projection<any, any>
```

This allows incompatible Event/State combinations to be detected earlier.

The exact C++ mechanism remains deferred.

---

# 61. Domain Projection

A domain may define:

```cpp
class HandProjection;
class HandState;
```

The domain owns the meaning.

Core provides the generic processing contract and supporting infrastructure.

---

# 62. Projection Registration

Projections may participate in the Registration/Discovery model.

A Projection definition may expose:

```text
identity
version
input event types
output state type
configuration requirements
capabilities
```

Registration does not imply activation.

---

# 63. Projection Provenance

A State generated by a Projection should record the Projection identity/version when that information is material to reproducibility.

For example:

```text
State
    projection = HandProjection
    projection_version = 3
```

---

# 64. Testing Requirements

Projection tests must cover:

```text
initial state
single event transition
multi-event replay
valid transitions
invalid transitions
duplicate events
ordering
out-of-order behavior
unknown data
partial state
determinism
snapshot reconstruction
serialization
projection version compatibility
failure propagation
copy/move/lifetime
```

Domain-specific invariant tests belong with the domain.

---

# 65. Deferred Decisions

This ADR does not select:

* exact State storage representation
* exact Projection C++ API
* runtime polymorphism vs templates
* snapshot format
* state database
* event-sourcing implementation
* event ordering implementation
* watermark implementation
* late-event buffering implementation
* distributed projection architecture
* state merge framework
* universal state serialization format
* state cache implementation

---

# Decision Summary

```text
State:
    Representation of what is true within a defined scope

Projection:
    Rules for deriving State from historical information

Event:
    Historical fact

State version:
    Historical point represented by State

Projection version:
    Version of the transformation logic

Snapshot:
    Persisted State representation for reconstruction/recovery

Replay:
    Reconstruction of State from historical information

State identity:
    Identity of the logical state-bearing object

Projection identity:
    Identity of the transformation definition

State semantics:
    Domain-owned

Projection semantics:
    Domain-owned

Projection execution:
    Processing-owned

State persistence:
    Storage-owned

State provenance:
    Required where reconstruction/interpretation demands it
```

## Invariant

**State represents the condition resulting from historical information, while a Projection defines how that condition is derived; Core provides the generic mechanisms and contracts, domains define state semantics and transitions, and processing/storage layers determine how projections execute and how state is retained without changing its meaning.**

