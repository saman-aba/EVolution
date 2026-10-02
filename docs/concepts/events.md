# Event Model

## 1. Purpose

An `Event` is the fundamental unit of information entering EVolution.

An event represents something that **happened at a particular point in time**.

EVolution processes events to derive higher-level representations:

```text
Events
  ↓
State
  ↓
Measurements
  ↓
Aggregations
  ↓
Patterns
  ↓
Models / Insights
```

The Event model belongs to the EVolution core and must not contain domain-specific concepts such as poker hands, players, cards, prices, or trades.

---

## 2. Event Properties

Every event has the following conceptual properties:

```text
Event
├── identity
├── timestamp
├── type
├── source
├── sequence
└── payload
```

### Identity

Each event has a unique identifier.

The identifier exists to distinguish individual events and to allow other components to reference a specific event.

The core must not require a particular identifier format.

---

### Timestamp

An event has a timestamp representing when the event occurred.

The timestamp must have sufficient precision for the domain producing the event.

The core should use an explicit time representation rather than relying on implicit ordering through timestamps alone.

Timestamp equality must therefore not imply event equality or ordering.

---

### Type

`type` identifies the semantic kind of event.

Examples from the poker domain:

```text
SessionStarted
HandStarted
CardsDealt
PlayerAction
CardRevealed
HandFinished
SessionFinished
```

The core treats the type as an identifier.

Its meaning belongs to the domain that defines it.

---

### Source

`source` identifies where the event originated.

Examples:

```text
poker_client
hand_history
manual_input
imported_file
simulation
```

The source is metadata and must not determine the semantic meaning of the event.

---

### Sequence

Events may contain a sequence number representing their order within a stream.

Sequence numbers are useful when multiple events have identical timestamps or when the source provides an authoritative ordering.

A sequence number is local to its event stream and does not provide global ordering across independent streams.

---

### Payload

The payload contains the event-specific data.

The core must treat the payload as opaque domain data.

For example, a poker domain may define:

```text
PlayerAction
{
    player
    action
    amount
}
```

The core does not need to understand any of these fields.

---

## 3. Event Immutability

Events are historical facts.

Once an event has entered the EVolution system, it must be treated as immutable.

If an event was incorrect, the system should represent the correction through another mechanism rather than silently modifying the original event.

This preserves the ability to reconstruct the state of the system from its historical event stream.

---

## 4. Event Ordering

EVolution must distinguish between:

```text
event timestamp
```

and

```text
event ordering
```

Timestamp represents when an event occurred.

Sequence represents ordering within a stream.

These are related but not interchangeable.

A consumer must not assume that sorting events by timestamp alone reconstructs the original event order.

---

## 5. Event Streams

Events are normally processed as part of an event stream.

An event stream represents an ordered sequence of related events.

Examples:

```text
Poker session
    ↓
Event stream

Poker hand
    ↓
Event stream

Imported historical dataset
    ↓
Event stream

Simulation
    ↓
Event stream
```

A stream may contain events from a single source or from multiple sources after explicit normalization.

---

## 6. Domain Events

The core defines the event infrastructure.

Domains define event semantics.

For example:

```text
core/
    Event
    EventId
    Timestamp
    EventStream

domains/
    poker/
        events/
            SessionStarted
            HandStarted
            CardsDealt
            PlayerAction
            CardRevealed
            HandFinished
```

The poker domain therefore depends on the core event model, while the core remains unaware of poker.

---

## 7. Event Lifecycle

The conceptual lifecycle of information is:

```text
External Data
     ↓
Ingestion
     ↓
Normalization
     ↓
Event
     ↓
Event Stream
     ↓
State Reconstruction
     ↓
Measurements
     ↓
Aggregation
     ↓
Analysis
```

Not every external data source must map directly to a domain event.

An ingestion layer may first normalize raw input before producing events.

For example:

```text
Poker client / hand history
          ↓
      Raw input
          ↓
       Parser
          ↓
    Normalization
          ↓
    Poker Event
```

---

## 8. Event vs State

An event describes something that happened.

State describes what is true as a consequence of events.

For example:

```text
Event:

PlayerAction
    player = Hero
    action = Raise
    amount = 2.5 BB
```

may contribute to:

```text
State:

Hero
    stack = 47.5 BB
    position = BTN
    current_action = Raise
```

State must therefore be reconstructible, where practical, from the event history.

---

## 9. Event vs Measurement

An event is an observation of something that happened.

A measurement is a value derived from one or more events.

For example:

```text
Events:

HandFinished
HandFinished
HandFinished
...
```

can produce:

```text
Measurement:

BB / 100
```

The measurement is not itself a historical event.

This distinction must remain explicit throughout the architecture.

---

## 10. Event vs Aggregation

An aggregation combines information from multiple events or measurements over a defined window.

Example:

```text
Events
    ↓
1,000 hands
    ↓
Poker performance candle
```

The resulting candle is an analytical representation, not an event.

This distinction allows EVolution to maintain a clean separation between:

```text
what happened
```

and:

```text
what we calculate about what happened
```

---

## 11. Determinism

Given the same valid event stream and the same analysis configuration, state reconstruction and deterministic measurements should produce the same result.

This property is important for:

* reproducibility
* testing
* debugging
* historical analysis
* simulation
* regression testing

Non-deterministic analysis must explicitly identify its source of non-determinism.

---

## 12. Initial Core Contract

The initial conceptual contract for the core Event is:

```text
Event
{
    id
    timestamp
    type
    source
    sequence
    payload
}
```

The exact representation, serialization format, memory layout, ownership model, and programming-language implementation are intentionally left undecided at this stage.

Those are implementation decisions and must not constrain the conceptual model prematurely.

---

## 13. Design Constraint

The Event model must remain sufficiently generic that the following can eventually be represented without modifying the core:

```text
Poker
Crypto markets
Simulations
Imported datasets
Other future analytical domains
```

The first implementation domain is poker, but the architecture must not assume that poker is the permanent or exclusive domain of EVolution.
