# EVolution State Model

## 1. Purpose

A **State** is a representation of what is currently true as a consequence of a sequence of events.

Events describe what happened.

State describes the resulting condition.

The conceptual relationship is:

```text
Events → State
```

More precisely:

```text
Initial State + Event → New State
```

A sequence of events therefore produces a sequence of states:

```text
S0
 │
 ├── Event 1 ──→ S1
 │
 ├── Event 2 ──→ S2
 │
 ├── Event 3 ──→ S3
 │
 └── ...
```

State is derived information. It is not the original historical observation.

---

## 2. State Is Domain-Specific

The EVolution core must not define what a particular state means.

The core provides the mechanisms required to:

* construct state from events
* apply events to state
* reconstruct state
* checkpoint state
* compare state versions
* maintain deterministic projections

Domains define the actual state.

For example, Poker may define:

```text
TableState
SessionState
HandState
PlayerState
BankrollState
```

Another domain may define completely different states.

The core must therefore understand the concept of state without understanding its domain semantics.

---

## 3. State Transition

A state transition is the transformation of one state into another as a consequence of an event.

Conceptually:

```text
StateTransition(S, E) → S'
```

Where:

* `S` is the previous state
* `E` is an event
* `S'` is the resulting state

For a sequence:

```text
S0 + E1 + E2 + ... + En → Sn
```

The same event stream and the same transition rules must produce the same resulting state.

---

## 4. Projection

A **Projection** is the mechanism that derives a particular state representation from an event stream.

Conceptually:

```text
Event Stream
      │
      ▼
   Projection
      │
      ▼
    State
```

Different projections may consume the same events while producing different states.

For example:

```text
                 ┌──→ PokerTableState
Event Stream ────┼──→ PlayerState
                 ├──→ BankrollState
                 └──→ StatisticsState
```

This allows EVolution to derive multiple analytical representations from the same underlying observations.

A projection defines:

1. which events it consumes
2. what state it maintains
3. how each relevant event changes that state
4. what invariants the resulting state must satisfy

---

## 5. State Reconstruction

A state must be reconstructible from the events that produced it.

Conceptually:

```text
Events
  │
  ▼
Replay
  │
  ▼
State
```

Given:

```text
S0 + E1 + E2 + E3
```

the projection should produce:

```text
S3
```

This is important because a state may be:

* persisted
* cached
* discarded
* reconstructed
* periodically checkpointed

The existence of a stored state must therefore not fundamentally change its meaning.

---

## 6. Snapshots

For long event streams, replaying every event from the beginning may be unnecessarily expensive.

A projection may therefore create a **Snapshot**:

```text
E1 E2 E3 E4 E5 E6 E7 E8 E9 E10
                │
                ▼
             Snapshot
                │
                ▼
              S10
```

Future reconstruction can then begin from the snapshot:

```text
Snapshot(S10)
     +
E11 + E12 + E13
     │
     ▼
S13
```

Snapshots are optimization mechanisms.

They must not become the authoritative historical record merely because they are faster to access.

---

## 7. State Scope

Every state has a scope.

The scope determines what the state represents.

Examples:

```text
HandState       → one poker hand
SessionState    → one playing session
PlayerState     → one player
TableState      → one table
BankrollState   → one bankroll
```

The same event may potentially affect multiple state projections.

For example:

```text
HandFinished
    │
    ├──→ HandState
    ├──→ SessionState
    ├──→ PlayerState
    └──→ BankrollState
```

This means that an event should not necessarily belong to exactly one state representation.

---

## 8. State Identity

A state representation must have an identifiable scope.

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

For example:

```text
scope    = "poker.session"
scope_id = "session-123"
version  = 842
```

The exact representation of state identity is intentionally left undecided.

---

## 9. State Version

A state version identifies the point in the event sequence from which the state was derived.

For an ordered stream:

```text
E1
E2
E3
E4
```

the resulting state may be represented conceptually as:

```text
State(version = 4)
```

This allows the system to determine whether a state is:

* current
* stale
* incomplete
* ahead of another representation

The version does not necessarily have to be a global number. Its semantics depend on the stream and projection.

---

## 10. State Invariants

A projection may define invariants that must always hold.

For example, a Poker bankroll state might require:

```text
bankroll >= 0
```

A hand state might require:

```text
hand cannot be finished before it starts
```

An invalid event sequence may therefore result in:

```text
invalid transition
```

rather than silently producing an invalid state.

The core provides the mechanism for representing and propagating such failures; domain projections define the actual invariants.

---

## 11. State Is Not a Measurement

State and measurement must remain conceptually separate.

For example:

```text
State:
    bankroll = $150

Measurement:
    bankroll_change = +$20
```

Another example:

```text
State:
    hands_played = 1000

Measurement:
    win_rate = 4.2 BB/100
```

A measurement is an observation derived from state, events, or both.

State describes a condition.

Measurement quantifies some property of data.

---

## 12. State Is Not an Analysis Result

A state should represent domain condition rather than interpretation.

For example:

```text
State:
    session_loss = -35 BB
```

is different from:

```text
Analysis:
    possible_tilt = true
```

The second is an interpretation derived from evidence.

Keeping these concepts separate prevents analytical conclusions from becoming indistinguishable from historical or domain state.

---

## 13. Derived State

Not all state has to come directly from external events.

A projection may derive additional state from previously derived state or events.

For example:

```text
Raw Events
    │
    ▼
Poker State
    │
    ▼
Performance State
    │
    ▼
Behavioral State
```

However, derived state must remain distinguishable from directly observed state.

The system must be able to identify the origin and derivation rules of state.

---

## 14. Determinism

A projection should be deterministic.

Given:

```text
Initial State
+
same Event Stream
+
same Projection Configuration
```

it should produce:

```text
same resulting State
```

This property is important for:

* replay
* debugging
* testing
* historical analysis
* reproducibility
* distributed processing
* validation of analytical results

Non-deterministic behavior must therefore be explicit rather than accidental.

---

## 15. Invalid Event Sequences

Not every event is necessarily valid for every state.

For example:

```text
HandFinished
```

cannot necessarily be applied to a state in which:

```text
hand does not exist
```

A projection may therefore reject an event.

Conceptually:

```text
State + Event
      │
      ├── valid ──→ New State
      │
      └── invalid → Transition Error
```

The system must not silently invent state to accommodate invalid historical data.

How invalid external data is handled during ingestion is a separate concern from how valid state transitions are defined.

---

## 16. State and Event Streams

A projection operates over an event stream.

Conceptually:

```text
Event Stream
     │
     ▼
  Projection
     │
     ▼
    State
```

The same event stream may feed multiple projections.

Likewise, different projections may operate over different scopes of the same domain.

This establishes an important architectural separation:

```text
Event Storage / Stream
        ≠
Projection
        ≠
State Representation
```

The core should not require these to be implemented by the same component.

---

## 17. Event Sourcing Is Not Yet an Architectural Commitment

The State model requires event replay conceptually, but this does not yet mean that EVolution must use full event sourcing as its persistence architecture.

The system may eventually support:

* event logs
* imported event streams
* snapshots
* materialized state
* external datasets
* temporary in-memory streams

Whether events are always the authoritative persistent source of truth is a separate architectural decision.

That decision must not be made implicitly by the State model.

---

## 18. Conceptual Contract

The initial conceptual contract is:

```text
Projection
{
    initial_state
    apply(state, event)
    resulting_state
}
```

And:

```text
State
{
    scope
    scope_id
    version
    data
}
```

These are conceptual models only.

The following remain intentionally undecided:

* programming-language representation
* memory ownership
* serialization
* persistence
* snapshot format
* concurrency model
* projection API
* generic vs typed state representation
* state storage engine

---

## 19. Core Invariant

The EVolution core understands **how state is derived**, but not **what a particular state means**.

Therefore:

```text
Core:
    Event → Projection → State

Domain:
    defines Event semantics
    defines State semantics
    defines valid transitions
    defines State invariants
```

This keeps the analytical engine domain-independent while allowing domains to build rich state models on top of it.
