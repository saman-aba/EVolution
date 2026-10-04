# ADR 0057: Time API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will use `std::chrono` as the foundation of its temporal API.

The Core time model will explicitly distinguish:

```text
Absolute Time
Duration
Time Range
Temporal Information
Temporal Knowledge Status
```

and will distinguish the semantic clocks:

```text
Event Time
Ingestion Time
Processing Time
Measurement Time
Monotonic Execution Time
```

Absolute timestamps are represented in UTC. Elapsed-time measurement uses a monotonic clock.

The API must preserve the distinction between a timestamp being **known**, **estimated**, or **unknown**.

---

# 1. Absolute Time

Absolute historical time is conceptually represented by:

```cpp
std::chrono::system_clock::time_point
```

It represents a point on the wall-clock timeline.

EVolution's canonical absolute-time semantics are UTC.

The API must not depend on the host machine's local timezone for the meaning of an absolute timestamp.

---

# 2. Project Time Alias

EVolution may introduce a project-level alias such as:

```cpp
using TimePoint = std::chrono::system_clock::time_point;
```

The alias exists to make semantic intent explicit and to provide a future abstraction boundary.

It must not change the semantics of `std::chrono`.

---

# 3. Duration

Elapsed time is represented using:

```cpp
std::chrono::duration
```

or an EVolution alias over it.

A duration represents elapsed time, not a calendar timestamp.

For example:

```text
5 milliseconds
2 seconds
3 minutes
```

are durations.

They must not be interpreted as dates or wall-clock positions.

---

# 4. Time Point vs Duration

The API must preserve the distinction:

```text
TimePoint - TimePoint → Duration
TimePoint + Duration  → TimePoint
```

A duration cannot be used as an absolute timestamp without an explicit operation establishing the reference point.

---

# 5. Time Precision

The internal representation should preserve the precision supplied by the underlying `std::chrono` type.

EVolution must not silently round timestamps during normal processing.

If an external source provides only second precision, EVolution must not fabricate sub-second precision.

---

# 6. Temporal Knowledge

A temporal value has a knowledge status.

The conceptual model is:

```text
TemporalInformation
{
    timestamp?
    status
    uncertainty?
    provenance?
}
```

The status is:

```text
KNOWN
ESTIMATED
UNKNOWN
```

---

# 7. Known Time

`KNOWN` means the source or domain provides a temporal value that EVolution accepts as sufficiently authoritative for the relevant semantic purpose.

Known does not mean infinitely precise.

For example:

```text
2026-10-04 12:00:00
```

may be known only to one-second precision.

Precision and knowledge status are separate properties.

---

# 8. Estimated Time

`ESTIMATED` means the temporal value was inferred rather than directly established by the relevant source.

Examples include:

```text
Source provides only a date
Sequence information permits approximate reconstruction
Time inferred from neighboring observations
Time inferred from an external correlation
```

An estimated timestamp must remain marked as estimated.

---

# 9. Unknown Time

`UNKNOWN` means that no sufficiently supported temporal value is available.

An unknown timestamp must not be fabricated.

The following are invalid substitutes unless a domain explicitly defines such semantics:

```text
1970-01-01
0
current time
ingestion time
processing time
```

---

# 10. Missing vs Unknown

These concepts are distinct.

```text
Missing
    → representation/input condition

Unknown
    → semantic knowledge condition
```

For example, an input record may omit its event timestamp.

After ingestion, the semantic result may be:

```text
event_time = UNKNOWN
```

Missing input does not automatically imply a particular timestamp.

---

# 11. Unknown Event Time vs Ingestion Time

If event time is unknown but ingestion time is available, they remain separate:

```text
Event Time     = UNKNOWN
Ingestion Time = KNOWN
```

Ingestion time must not silently replace event time.

Both may be recorded when relevant.

---

# 12. Estimated Time Provenance

When an estimated timestamp materially affects processing, its provenance should explain:

```text
why it was estimated
what evidence was used
which estimation method was used
which configuration/version was used when relevant
```

This information must not be discarded merely because the timestamp itself is represented using the same `TimePoint` type as known time.

---

# 13. Estimated Time Is Not Uncertainty

These are separate concepts:

```text
Estimated
    → how the timestamp was obtained

Uncertainty
    → how precisely the timestamp is known
```

An estimated timestamp may have:

```text
±1 millisecond
```

or:

```text
±10 minutes
```

depending on the inference method.

If temporal uncertainty matters, it must be represented explicitly.

---

# 14. Temporal Uncertainty

When supported, uncertainty may conceptually be represented as:

```text
TemporalInformation
{
    timestamp
    status
    uncertainty
}
```

The exact uncertainty representation is deferred.

Uncertainty must not be inferred solely from `ESTIMATED`.

---

# 15. Event Time

Event time describes when the underlying domain event occurred.

For example:

```text
PlayerAction occurred at T
Trade occurred at T
Packet was observed at T
```

Event time belongs to the event's semantic representation.

---

# 16. Ingestion Time

Ingestion time describes when EVolution accepted or recorded an external observation.

It is an EVolution processing-boundary concept.

It must not overwrite the event's original event time.

---

# 17. Processing Time

Processing time describes when EVolution performs a processing operation.

It is useful for:

```text
latency
execution metrics
operational timing
scheduling
performance analysis
```

Processing time must not silently become domain event time.

---

# 18. Measurement Time

Measurements may have temporal semantics distinct from the event that caused them.

A measurement can represent:

```text
a point
an interval
a window
```

For example:

```text
Measurement:
    metric = win_rate
    interval = [T1, T2)
```

Measurement time must be explicitly represented according to the measurement contract.

---

# 19. Monotonic Time

Elapsed execution timing must use a monotonic clock where possible.

Conceptually:

```cpp
std::chrono::steady_clock
```

is appropriate for:

```text
timeouts
latency
elapsed duration
performance measurements
deadline measurement
```

Wall-clock adjustments must not cause elapsed-time calculations to become negative or otherwise semantically invalid.

---

# 20. Wall Clock vs Monotonic Clock

The distinction is:

```text
system_clock
    → historical/absolute time

steady_clock
    → elapsed execution time
```

Neither clock replaces the other.

---

# 21. Temporal Semantics Are Not Clock Types

The semantic concepts:

```text
Event Time
Ingestion Time
Processing Time
Measurement Time
```

are not separate physical clocks.

For example, both event time and ingestion time may be represented by:

```cpp
std::chrono::system_clock::time_point
```

while retaining different semantic meanings.

---

# 22. Time Range

EVolution will use an explicit time-range abstraction where a range is semantically required.

Conceptually:

```text
TimeRange
{
    start
    end
}
```

The default interval convention is:

```text
[start, end)
```

where the start is inclusive and the end is exclusive.

---

# 23. Half-Open Ranges

For:

```text
[start, end)
```

an observation at:

```text
timestamp == start
```

belongs to the range.

An observation at:

```text
timestamp == end
```

does not.

This convention should be used consistently for temporal windows unless a contract explicitly requires different semantics.

---

# 24. Empty Ranges

A range where:

```text
start == end
```

represents an empty half-open interval.

Whether empty ranges are accepted by a particular operation is determined by that operation's contract.

---

# 25. Invalid Ranges

Normally:

```text
start > end
```

is an invalid range.

Operations constructing a range should reject invalid temporal relationships rather than silently swapping endpoints.

---

# 26. Open-Ended Ranges

Some queries or processing operations may require:

```text
[start, ∞)
(-∞, end)
```

or equivalent open-ended semantics.

The time API may support optional boundaries where required.

The representation is deferred.

---

# 27. Time Ordering

Time ordering is not automatically a total ordering of domain events.

Two events may have:

```text
same timestamp
```

without having a defined order.

Sequence information must be used when the domain or processing contract requires additional ordering.

---

# 28. Timestamp Equality

Two timestamps are equal when their represented temporal points are equal at the representation's precision.

Equality does not imply:

```text
same event
same object
same observation
same source
```

Identity remains separate.

---

# 29. Time and Sequence

A domain object may contain both:

```text
timestamp
sequence
```

These answer different questions:

```text
timestamp
    → when?

sequence
    → what ordering position within this sequence?
```

Neither should silently replace the other.

---

# 30. Out-of-Order Time

EVolution must support data arriving in an order different from event time.

For example:

```text
Event A: event_time = 10:00:02
Event B: event_time = 10:00:01

arrival:
A
B
```

This is not automatically an error.

The relevant processor or graph contract determines whether data is:

```text
accepted
buffered
reordered
rejected
processed independently
corrected later
```

---

# 31. Late Data

An observation arriving after its semantic processing window may be considered late.

Lateness is a processing concept, not a property of the timestamp type.

The time API must not embed universal watermark or lateness rules.

---

# 32. Current Time

A processor requiring the current wall-clock time must receive it through an explicit execution dependency/context.

Core analytical APIs must not silently call:

```cpp
std::chrono::system_clock::now()
```

when doing so would make the result depend on uncontrolled current time.

---

# 33. Deterministic Processing

A deterministic processor should use explicit time inputs.

For replay:

```text
historical event time
    ≠
replay execution time
```

Replay should preserve historical temporal semantics while allowing execution timing to differ.

---

# 34. Deadlines and Timeouts

Operational deadlines and timeouts are execution concerns.

They should use appropriate monotonic timing for elapsed-time enforcement.

They must not be interpreted as domain timestamps.

For example:

```text
request deadline
```

does not become:

```text
event time
```

---

# 35. Time Zones

Core absolute timestamps use UTC semantics.

Local timezone interpretation belongs at an external/interface/domain boundary where calendar semantics are actually required.

Core analytical processing should not depend on the host machine's local timezone.

---

# 36. Calendar Time

Calendar concepts such as:

```text
day
month
year
business day
local midnight
```

may be required by domain applications.

They must not be confused with arbitrary fixed durations.

For example:

```text
1 calendar month
```

is not necessarily equivalent to:

```text
30 days
```

Calendar-specific semantics belong outside the minimal absolute-time foundation unless explicitly required.

---

# 37. Duration Arithmetic

Duration arithmetic should use `std::chrono` semantics.

The API should not introduce custom duration arithmetic merely for convenience.

Where units matter, conversions must be explicit enough to avoid accidental loss of precision.

---

# 38. Time and Measurements

Measurements must explicitly state their temporal meaning when relevant.

For example:

```text
Measurement
{
    metric = win_rate
    value = 0.42
    time_range = [T1, T2)
}
```

The measurement must not rely on an implicit timestamp hidden elsewhere.

---

# 39. Time and Aggregation

Aggregation windows must explicitly define which temporal dimension is used.

For example:

```text
event-time window
ingestion-time window
processing-time window
measurement-time window
```

An implementation must not silently substitute one for another.

---

# 40. Time and Queries

Queries must explicitly identify their temporal semantics.

For example:

```text
query by event time
query by ingestion time
query by measurement time
```

Storage order or insertion time must not silently determine the meaning of a temporal query.

---

# 41. Time and Persistence

Persistence must preserve temporal knowledge status when it is semantically relevant.

For example:

```text
timestamp = T
status = ESTIMATED
```

must not deserialize as:

```text
timestamp = T
status = KNOWN
```

merely because the timestamp itself survived serialization.

---

# 42. Time and Serialization

Serialized temporal values must preserve:

```text
timestamp
precision
temporal meaning
knowledge status
uncertainty where required
```

The serialization format is implementation-defined.

---

# 43. Time and Provenance

Temporal values that are estimated or derived may require provenance identifying their origin and derivation.

The provenance timestamp itself must not be confused with the timestamp being described.

---

# 44. Time and Identity

Time does not identify an object.

Multiple objects may share exactly the same timestamp.

Therefore:

```text
timestamp ≠ identity
```

even when a timestamp is unique in a particular dataset.

---

# 45. Time and Error Handling

Invalid temporal input should produce an appropriate `Result`/`Error` where the contract considers it invalid.

Examples:

```text
invalid range
invalid encoding
unsupported precision
invalid temporal relationship
```

Unknown time is not automatically an error.

It is a valid semantic state when the domain permits missing temporal knowledge.

---

# 46. Time API Shape

The initial Core API should conceptually expose:

```cpp
namespace evolution::time
{

using TimePoint = std::chrono::system_clock::time_point;

using Duration = std::chrono::system_clock::duration;

enum class TemporalStatus
{
    Known,
    Estimated,
    Unknown
};

struct TemporalInformation;

struct TimeRange;

}
```

The exact class/struct definitions remain implementation work.

---

# 47. Clock Access

Clock access should be abstracted where deterministic testing or replay requires it.

Conceptually:

```text
Clock
├── wall-clock source
└── monotonic source
```

The exact clock interface is deferred.

Production code should not make deterministic components directly depend on uncontrolled global clock access.

---

# 48. Testing Requirements

The implementation must test:

```text
TimePoint construction
Duration arithmetic
TimeRange boundaries
[start, end) semantics
Known temporal information
Estimated temporal information
Unknown temporal information
Serialization round-trip
Precision preservation
Time-zone independence
Monotonic elapsed-time behavior
Deterministic clock injection where required
```

Tests must explicitly verify that:

```text
UNKNOWN ≠ current time
UNKNOWN ≠ ingestion time
ESTIMATED ≠ KNOWN
```

---

# 49. Deferred Decisions

This ADR does not select:

* exact timestamp precision
* exact `TimePoint` alias
* exact `Duration` alias
* timezone library
* calendar library
* open-ended range representation
* uncertainty representation
* clock interface
* clock injection mechanism
* serialization format
* timestamp wire encoding
* watermark implementation
* late-data policy
* event-time ordering implementation
* distributed clock synchronization strategy

---

# Decision Summary

```text
Absolute time:                 std::chrono::system_clock
Absolute semantic timezone:    UTC
Elapsed time:                  std::chrono duration
Monotonic timing:              steady_clock concept
Event time:                    Explicit semantic field
Ingestion time:                Explicit semantic field
Processing time:               Explicit semantic field
Measurement time:              Explicit semantic field
Temporal status:               KNOWN / ESTIMATED / UNKNOWN
Missing vs unknown:            Distinct
Estimated vs known:            Distinct
Temporal uncertainty:          Separate concept
Default time range:            [start, end)
Timestamp ≠ identity:          Yes
Timestamp ≠ sequence:          Yes
Unknown time fabrication:      Rejected
Current time in deterministic
processing:                    Must be explicit
Host timezone dependency:      Rejected
Calendar semantics:            Deferred/domain-specific
Late-data policy:              Deferred
Watermarks:                    Deferred
```

## Invariant

**EVolution treats time as explicit semantic information: absolute timestamps use UTC-based `std::chrono` semantics, elapsed execution uses monotonic timing, event/ingestion/processing/measurement times remain distinct, and temporal knowledge must be represented as known, estimated, or unknown without silently replacing missing information with fabricated or unrelated timestamps.**

