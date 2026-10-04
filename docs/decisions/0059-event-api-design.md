# ADR 0059: Event API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent an Event as an immutable historical observation describing something that occurred.

The Core Event model will provide generic structure without defining domain meaning.

Conceptually:

```text
Event
{
    id
    event_time
    type
    source
    sequence?
    payload
    provenance?
}
```

The Core understands the structure and lifecycle of an Event.

The Domain defines what a particular Event type means.

---

# 1. Event Semantics

An Event represents something that happened.

Examples of domain-level events may include:

```text
PlayerAction
TradeExecuted
CardDealt
PacketReceived
SessionStarted
```

Core does not interpret these meanings.

---

# 2. Event vs State

An Event describes:

```text
what happened
```

State describes:

```text
what is currently true as a consequence
```

For example:

```text
Event:
    PlayerAction(FOLD)

State:
    PlayerStatus = Folded
```

The event is historical input.

The state is a derived representation.

---

# 3. Event vs Measurement

An Event is not automatically a measurement.

For example:

```text
PlayerAction(FOLD)
```

is an event.

A derived value such as:

```text
fold_rate = 42%
```

is a measurement.

The latter is derived from events/state.

---

# 4. Event Immutability

Once an Event is constructed and accepted as historical information, its semantic content is immutable.

The following must not be modified in place:

```text
identity
event type
event time
source
sequence
payload
historical provenance
```

If an observation must be corrected, the correction must follow an explicit correction model.

---

# 5. Event Identity

Every persistent Event should have an `EventId`.

Conceptually:

```cpp id="m6byj9"
using EventId = evolution::Id<EventTag>;
```

The identity follows the Identity Model.

The identity must not depend on:

```text
memory address
container position
processing thread
storage row
```

---

# 6. Event Type

An Event has a type describing what kind of event it is.

Conceptually:

```text id="7f0n8n"
EventType
```

The type is semantic metadata.

Core may represent an event type generically, but the domain owns its meaning.

---

# 7. Event Type Identity

Event type identity must be stable enough to distinguish event semantics within its declared scope.

Examples:

```text id="j2fgr2"
Poker.PlayerAction
Poker.HandStarted

Market.Trade
Market.OrderBookUpdate
```

The exact representation is deferred.

A C++ type name must not automatically become the persistent event type identity.

---

# 8. Event Source

An Event identifies where its observation originated.

Examples:

```text id="gax6cb"
poker_client
hand_history
manual_input
simulation
network_capture
market_feed
imported_file
```

Source is not necessarily the same thing as the external system that physically transported the data.

The exact source model is deferred.

---

# 9. Source vs Provenance

Source identifies the origin associated with the Event.

Provenance describes how information was obtained, transformed, or derived.

Therefore:

```text id="2g5w4y"
source ≠ provenance
```

An event may have a source and still have a detailed provenance chain.

---

# 10. Event Time

An Event may contain event-time information.

Conceptually:

```text id="18y5qb"
event_time:
    timestamp
    status
    uncertainty?
    provenance?
```

The temporal model requires:

```text id="n5sl5v"
KNOWN
ESTIMATED
UNKNOWN
```

---

# 11. Event Time Is Not Ingestion Time

Ingestion may record when the Event entered EVolution.

For example:

```text id="0l2i0a"
event_time     = T1
ingestion_time = T2
```

These are distinct.

If event time is unknown:

```text id="4u9ehc"
event_time     = UNKNOWN
ingestion_time = T2
```

must remain valid.

Ingestion time must not silently replace event time.

---

# 12. Event Time and Sequence

An Event may also have a sequence value.

For example:

```text id="2i8l8w"
event_time = T
sequence   = 42
```

Time and sequence answer different questions.

```text id="o6k2qa"
time
    → when?

sequence
    → ordering position within a defined sequence scope
```

---

# 13. Event Sequence

Sequence numbers are optional because not every source provides a meaningful sequence.

When present, the sequence must have an explicit scope.

Examples:

```text id="xg8m5v"
source-local sequence
session sequence
stream sequence
partition sequence
```

A sequence value without a defined scope has little semantic meaning.

---

# 14. Sequence Ordering

A sequence may establish ordering when the Event source/domain contract says so.

Sequence ordering must not be confused with:

```text id="v5a2p9"
event time
ingestion order
processing order
execution order
storage order
```

These remain distinct.

---

# 15. Event Payload

The payload contains the domain-specific information carried by the Event.

Core must not assume a particular payload schema.

Conceptually:

```text id="k3pjz1"
Event
{
    ...
    payload
}
```

The payload belongs to the domain/event-type contract.

---

# 16. Core Must Not Interpret Payload

Core must not contain logic such as:

```text id="1h8rqd"
if event.type == PlayerAction
```

or:

```text id="9i5a8d"
if event.payload contains poker-specific field
```

Such interpretation belongs to the relevant domain.

---

# 17. Payload Type

The physical representation of the payload is deferred.

Possible implementation strategies include:

```text id="zq9kmb"
typed C++ value
type-erased value
serialized representation
variant
domain-specific object
```

No universal payload mechanism is selected by this ADR.

---

# 18. Event Type and Payload Compatibility

An Event's payload must be semantically compatible with its Event type.

For example:

```text id="0nyn1x"
EventType = Poker.PlayerAction
Payload   = compatible PlayerAction representation
```

A structurally valid payload with incompatible semantics is not a valid Event.

---

# 19. Event Validation

Event validation has at least two conceptual layers.

### Structural validation

Examples:

```text id="e8v2fa"
identity present where required
event type valid
payload structurally valid
temporal representation valid
```

### Domain validation

Examples:

```text id="t3lq4x"
action is legal
card belongs to deck
trade quantity is valid
```

Core owns generic structural mechanisms.

The domain owns domain-specific semantic validation.

---

# 20. Event Construction

An Event should be constructed in a valid state.

Conceptually:

```text id="8ujwta"
External Representation
       ↓
Parsing
       ↓
Normalization
       ↓
Validation
       ↓
Event Construction
```

An invalid event should not become a valid Event merely because construction succeeded technically.

---

# 21. Event Creation vs Ingestion

Not every Event must originate from an external ingestion pipeline.

Events may be produced by:

```text id="iqb5pr"
ingestion
simulation
application logic
domain processing
manual input
replay
test fixtures
```

The source/provenance must identify the appropriate origin.

---

# 22. Event Source Identity

The source itself may have an identity.

For example:

```text id="w9i3u0"
Source = Feed-17
```

The EventId and source identity are distinct.

---

# 23. External Identity

An external source may provide its own identifier.

For example:

```text id="x5x4na"
external_event_id = 12345
```

This must not automatically become `EventId`.

The ingestion/domain contract determines whether an external identity is mapped directly to an EVolution identity.

---

# 24. Duplicate Events

Identical content does not automatically mean identical Event identity.

Two Events may contain equal payloads while representing two separate occurrences.

Conversely, the same Event may be delivered twice.

Therefore:

```text id="2bl7a6"
duplicate content
≠
duplicate identity
```

and:

```text id="v0x5h0"
duplicate delivery
≠
new event occurrence
```

---

# 25. Deduplication

Deduplication is not a generic property of Event construction.

The ingestion/storage/application contract must define:

```text id="z3g8fc"
deduplication key
scope
retention period
duplicate behavior
```

An Event with an already-existing identity may be:

```text id="1u5g4m"
accepted as duplicate
ignored
rejected
reconciled
```

according to the relevant contract.

---

# 26. Event Ordering

Events may be ordered by:

```text id="1m6x7p"
event time
sequence
ingestion time
processing order
```

No one ordering is universally correct.

The consumer contract must state which ordering matters.

---

# 27. Out-of-Order Events

EVolution permits Events to arrive out of event-time order.

For example:

```text id="9mb3fe"
Event A: T=10:02
Event B: T=10:01

arrival:
A
B
```

This is not inherently invalid.

Processing behavior is determined by the processor/graph contract.

---

# 28. Event Corrections

Historical Event correction must be explicit.

Possible mechanisms include:

```text id="k8r4qj"
replacement event
correction event
new version
superseding relation
new logical identity
```

The generic Event model does not choose one universal correction mechanism.

Historical information must not be silently overwritten when the source-of-truth contract requires append-oriented history.

---

# 29. Event Version

An Event may have a version if its contract requires versioned representations.

Version is distinct from:

```text id="d4j7gp"
EventId
Event type
Sequence
Schema version
```

The exact version semantics are domain/storage concerns.

---

# 30. Event Provenance

Events should be capable of carrying or referencing provenance when required.

For an ingested event, provenance may identify:

```text id="5v5n2m"
source record
input representation
parser version
normalizer version
mapping version
configuration
ingestion run
```

The exact provenance representation follows the Provenance Model.

---

# 31. Event and Ingestion Provenance

An event created by ingestion may conceptually have:

```text id="k4j4g7"
Source Record
      ↓
Parser
      ↓
Normalizer
      ↓
Domain Mapping
      ↓
Event
```

The event's provenance should be able to describe this derivation when reproducibility or auditability requires it.

---

# 32. Event Immutability and Corrections

Immutability means that an accepted Event is not silently modified.

It does not prevent a later Event from saying:

```text id="w3x4qa"
"previous observation was incorrect"
```

Correction mechanisms operate through new information.

---

# 33. Event Persistence

Events are often historical source data and may therefore be persisted as authoritative information.

However, not every Event must be persisted.

Persistence policy depends on:

```text id="k5xv0m"
source-of-truth requirements
retention
reproducibility
storage policy
application requirements
```

---

# 34. Event Storage Order

Storage order is not automatically Event order.

For example:

```text id="gqf4cu"
database insertion order
```

must not silently become:

```text id="0b7w4f"
event sequence
```

or:

```text id="4r4yye"
event-time order
```

unless the storage contract explicitly guarantees that relationship.

---

# 35. Event Serialization

Serialization must preserve the semantic Event information required by its contract:

```text id="7p8o5c"
identity
type
temporal information
source
sequence
payload
provenance
version
```

The exact wire/storage representation is governed by the Serialization Model.

---

# 36. Event Ownership

An accepted Event is a value with explicit ownership semantics.

Consumers should not mutate the Event.

For asynchronous processing, the Event or its containing Envelope must remain valid for the entire lifetime promised by the processing contract.

---

# 37. Event and Envelope

A Processor may transport an Event using an `Envelope<Event>`.

The envelope may carry processing metadata such as:

```text id="3cegq9"
ingestion/delivery information
correlation
processing sequence
execution metadata
```

It must not duplicate or redefine Event semantics.

For example, the envelope must not introduce a second unrelated event timestamp.

---

# 38. Event and Context

An Event may reference relevant Context.

Context describes the circumstances in which the event exists.

Context does not change the meaning of the Event itself.

For example:

```text id="xq5w2p"
Event:
    PlayerAction(FOLD)

Context:
    Table=42
    Session=7
```

The domain defines the semantic relationship.

---

# 39. Event and Measurement

Measurements may be derived from Events.

For example:

```text id="z1lbrq"
Events
   ↓
Measurement
   ↓
fold_rate
```

The measurement must retain appropriate provenance when required.

---

# 40. Event and State

State reconstruction may consume an Event stream:

```text id="i6k8je"
Initial State
     +
Event
     ↓
New State
```

The Event itself remains historical input.

State does not replace the event stream.

---

# 41. Event Streams

An Event Stream is an ordered or orderable collection of related Events.

The stream contract defines:

```text id="25s9c1"
membership
ordering
partitioning
delivery
retention
identity scope
```

The Core Event type does not itself define a universal stream implementation.

---

# 42. Event Stream Ordering

A stream may define ordering using:

```text id="w72bvx"
sequence
event time
source order
explicit ordering key
```

The ordering mechanism must be explicit.

---

# 43. Event Sequence Scope

If an Event has sequence information, the sequence must be meaningful within a declared scope.

For example:

```text id="j4nq7c"
Sequence 42 in Stream A
```

is unrelated to:

```text id="1k1c58"
Sequence 42 in Stream B
```

unless a higher-level contract relates them.

---

# 44. Event Time Status Propagation

Processors must not silently transform:

```text id="x7ifp8"
ESTIMATED
```

into:

```text id="v0m7ly"
KNOWN
```

If processing derives a new temporal value, its knowledge status must follow the temporal contract.

---

# 45. Unknown Event Time

An Event with unknown event time may still be valid.

For example:

```text id="m0n3n8"
Event:
    type = ManualObservation
    event_time = UNKNOWN
```

A processor requiring event-time ordering may reject or defer it.

A processor that does not require event time may process it normally.

---

# 46. Event Payload and Unknown Values

Payload-level unknown values must remain distinct from:

```text id="hl0dcr"
zero
empty
false
default
```

The domain defines these semantics.

Core must not normalize unknown values into arbitrary defaults.

---

# 47. Event Type Registration

Domain Event types may participate in the Registration/Discovery model.

Registration may provide:

```text id="1b2t2p"
type identity
version
capabilities
schema
```

Registration does not itself create Event instances.

---

# 48. Event Type Compatibility

Changes to Event types must respect the Versioning Model.

Changing a field may be:

```text id="xjpqwy"
representation-compatible
schema-compatible
behaviorally-compatible
semantically-compatible
```

These are distinct compatibility questions.

---

# 49. Event API Shape

The initial API should conceptually resemble:

```cpp id="ajqkym"
namespace evolution::event
{

class Event;

class EventType;

class EventSource;

}
```

The exact payload mechanism and representation remain implementation decisions.

---

# 50. Generic Event Contract

Conceptually:

```text id="q3j50q"
Event
{
    id
    type
    event_time
    source
    sequence?
    payload
    provenance?
}
```

Not every field must necessarily be populated in every Event.

The Event contract determines which fields are required.

---

# 51. Event Construction Contract

Construction should ensure:

```text id="17m2hd"
valid identity where required
valid event type
valid temporal representation
valid source
payload compatible with type
valid sequence semantics when present
```

Construction failure uses:

```text id="n8g0kh"
Result<Event>
```

where construction can fail under the relevant contract.

---

# 52. Event Validation vs Processing

Event validation determines:

```text id="q7e5z0"
Is this a valid Event?
```

Processing determines:

```text id="9ip3u1"
What should happen when this Event is consumed?
```

These are different responsibilities.

---

# 53. Event Equality

Event equality is not universally defined as:

```text id="qyk7tb"
same EventId
```

or:

```text id="e0f0u2"
same complete payload
```

Each domain/operation must define whether it needs:

```text id="o6p9wh"
identity equality
structural equality
semantic equality
```

The generic Event API should not pretend these are universally equivalent.

---

# 54. Event Hashing

If Events are hashed, the hashing semantics must be explicit.

Hashing by EventId is different from hashing the complete Event content.

A content hash must not automatically replace logical identity.

---

# 55. Event Testing Requirements

Core/domain Event tests should cover:

```text id="0m8fjp"
Identity preservation
Event type validity
Payload/type compatibility
Known time
Estimated time
Unknown time
Sequence semantics
Source semantics
Immutability
Copy/move
Serialization round-trip
Provenance preservation
Out-of-order handling at consuming layers
Duplicate identity handling
Correction semantics
```

---

# 56. Deferred Decisions

This ADR does not select:

* exact `Event` C++ class/struct layout
* payload representation
* event-type representation
* event-source representation
* identity generation mechanism
* sequence numeric type
* event serialization format
* event storage implementation
* event-stream implementation
* deduplication implementation
* correction mechanism
* schema registry
* event version format
* zero-copy representation

---

# Decision Summary

```text
Event meaning:                  Domain-owned
Event structure:                Core-owned
Event identity:                 EventId
Event immutability:             Required after acceptance
Event time:                     Explicit
Temporal status:                KNOWN / ESTIMATED / UNKNOWN
Ingestion time:                 Separate
Sequence:                       Optional and scoped
Source:                         Explicit
Payload:                        Domain-specific
Payload interpretation:         Domain-owned
Event provenance:               Supported
Event vs state:                 Distinct
Event vs measurement:           Distinct
Event vs processing envelope:   Distinct
Duplicate delivery:             Distinct from new occurrence
Storage order:                  Not event order
Memory address:                 Not identity
Correction:                     Explicit
Core domain semantics:           Rejected
```

## Invariant

**An EVolution Event is an immutable representation of a historical observation: Core defines its structural contract—identity, temporal information, type, source, sequence, payload, and provenance—while domains define its meaning; event time, sequence, ingestion order, processing order, storage order, and identity remain distinct and must never be silently substituted for one another.**

