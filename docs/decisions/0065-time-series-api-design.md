# ADR 0065: Time Series API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent a **Time Series** as an ordered collection of observations associated with temporal positions.

```text
Time Series
{
    identity?
    metric?
    scope?
    dimensions?
    observations
}
```

A Time Series is a temporal organization of observations, not a replacement for Events, Measurements, Aggregations, or Patterns.

The Core provides generic temporal-series structures and operations.

Domains and analytical components define the meaning of the underlying observations.

---

# 1. Time Series

A Time Series represents observations ordered according to an explicit temporal contract.

Conceptually:

```text
Observation
{
    timestamp
    value
}
```

and:

```text
TimeSeries
{
    id
    metric
    scope
    dimensions
    observations
}
```

The exact C++ representation remains implementation-defined.

---

# 2. Observation

An Observation associates a value with temporal information.

At minimum:

```text
Observation
{
    time
    value
}
```

Additional metadata may be required by the series contract.

---

# 3. Observation Identity

An individual Observation does not automatically require a persistent identity.

A persistent ObservationId may be introduced when observations need independent references.

This ADR does not require:

```text
ObservationId
```

for every observation.

---

# 4. Time Series Identity

A persistent Time Series may have:

```cpp
using TimeSeriesId = evolution::Id<TimeSeriesTag>;
```

The identity identifies the logical series.

It does not identify:

```text
Observation
Metric
Event
Storage object
Processing run
```

---

# 5. Metric Association

A Time Series will normally identify the Metric represented by its observations when the series represents a single metric.

For example:

```text
TimeSeries:
    metric = win_rate
```

A series must not silently mix incompatible Metrics.

---

# 6. Multi-Value Series

Some series may contain structured values.

For example:

```text
OHLC
{
    open
    high
    low
    close
}
```

or:

```text
Statistics
{
    mean
    variance
}
```

Structured values are allowed when explicitly defined by the series contract.

---

# 7. Scalar Series

The simplest and most common form is:

```text
timestamp → scalar value
```

Examples:

```text
10:00 → 100
10:01 → 104
10:02 → 103
```

The Core should support scalar series efficiently.

---

# 8. Time Series vs Measurement

A Measurement is a quantitative semantic observation.

A Time Series organizes observations temporally.

For example:

```text
Measurement:
    win_rate = 0.42
    time = T1

Time Series:
    T1 → 0.42
    T2 → 0.45
    T3 → 0.41
```

A Measurement may exist without belonging to a Time Series.

---

# 9. Time Series vs Event Stream

An Event Stream represents historical Events.

A Time Series represents temporal observations.

```text
Event Stream:
    E1 → E2 → E3

Time Series:
    (T1,V1) → (T2,V2) → (T3,V3)
```

A Time Series may be derived from an Event Stream.

They are not interchangeable.

---

# 10. Time Series Ordering

Observations must have explicit ordering semantics.

The default conceptual ordering is temporal order, but the exact ordering contract must be declared.

Equal timestamps require explicit handling.

---

# 11. Equal Timestamps

Multiple observations may have the same timestamp.

For example:

```text
T1 → V1
T1 → V2
```

The Time Series contract must specify whether:

```text
multiple observations are allowed
```

and, if so, how they are ordered or identified.

---

# 12. Timestamp Ordering

Timestamp ordering does not necessarily imply a total ordering.

For example:

```text
T1
T1
T2
```

may be valid.

If exact ordering matters, an additional sequence or observation identity may be required.

---

# 13. Event Time

A Time Series may be based on Event Time.

For example:

```text
market price at event time
```

The event timestamp must remain distinct from:

```text
ingestion time
processing time
storage time
```

---

# 14. Measurement Time

A series may instead represent Measurement Time.

For example:

```text
measurement generated at T
```

The series contract must define which temporal concept its x-axis represents.

---

# 15. Temporal Information

Time Series observations follow the Time Model.

An observation may have:

```text
KNOWN
ESTIMATED
UNKNOWN
```

temporal status where required.

---

# 16. Unknown Time

An observation with unknown temporal position must not silently be assigned:

```text
epoch
zero
current time
ingestion time
```

Such an observation may be excluded from a temporal series if its ordering cannot be established.

---

# 17. Estimated Time

An observation may have an estimated timestamp.

The estimation method/provenance should be preserved when material.

An estimated timestamp must not be represented as directly observed.

---

# 18. Time Range

A Time Series query over time should use:

```text
[start, end)
```

by default.

For example:

```text
[T1, T2)
```

includes `T1` and excludes `T2`.

---

# 19. Observation Windows

A series operation may use:

```text
time window
observation-count window
event-based window
```

The window definition must be explicit.

---

# 20. Regular Sampling

A series is regularly sampled when observations occur according to a defined temporal interval.

For example:

```text
00:00
00:01
00:02
00:03
```

The sampling interval should be part of the series metadata or derivation contract where relevant.

---

# 21. Irregular Sampling

A series may be irregularly sampled:

```text
00:00:01
00:00:07
00:00:09
00:00:24
```

Irregular sampling is valid and must not be forced into regular intervals without an explicit resampling operation.

---

# 22. Sampling Interval vs Observation Difference

The actual difference between consecutive timestamps is not necessarily the intended sampling interval.

A series may have:

```text
intended interval = 1 second
actual observations = irregular
```

Missing observations must remain distinguishable from observations with value zero.

---

# 23. Missing Observations

Missing data is not automatically zero.

For example:

```text
T1 → 10
T2 → missing
T3 → 12
```

must not become:

```text
T2 → 0
```

unless the Metric explicitly defines zero as the correct representation.

---

# 24. Missing vs Unknown

The following may have different meanings:

```text
no observation exists
observation exists but value unknown
observation exists but value invalid
observation is not applicable
```

The series contract must preserve distinctions required by the Metric.

---

# 25. Duplicate Observations

Duplicate observations may mean:

```text
same value repeated
multiple legitimate observations
duplicate delivery
correction
conflicting observations
```

The series must not silently collapse them.

Deduplication requires an explicit contract.

---

# 26. Corrections

Historical observations may require correction.

Corrections should preserve traceability when historical integrity matters.

Possible mechanisms include:

```text
replacement version
correction record
superseding observation
new derived series
```

The implementation must not silently mutate authoritative history without an explicit semantic rule.

---

# 27. Series Provenance

A derived Time Series should identify its origin when required.

For example:

```text
Event Stream
    ↓
Aggregation
    ↓
Time Series
```

Provenance may identify:

```text
source series/stream
transformation
configuration
algorithm version
execution run
```

---

# 28. Series Configuration

A Time Series may depend on configuration such as:

```text
sampling interval
timezone/calendar semantics
resampling policy
missing-data policy
aggregation method
filtering
```

The exact configuration belongs to the series-producing operation.

---

# 29. Time Series Resolution

Resolution describes the temporal granularity represented by the series.

Examples:

```text
millisecond
second
minute
hour
day
```

Resolution does not necessarily mean regular sampling.

A minute-resolution series may still contain missing observations.

---

# 30. Source Resolution

The source data may have a different resolution from the resulting series.

For example:

```text
tick data
    ↓
1 minute aggregation
    ↓
minute series
```

The resulting series resolution is not the same as source resolution.

Provenance should preserve the relationship when required.

---

# 31. Resampling

Resampling transforms a series into another temporal resolution.

Examples:

```text
1 second
    ↓
1 minute

1 minute
    ↓
1 hour
```

Resampling must define:

```text
window boundaries
aggregation function
missing-data policy
alignment
```

---

# 32. Alignment

When combining multiple Time Series, their temporal boundaries may differ.

Alignment defines how observations correspond.

Possible policies include:

```text
exact timestamp
nearest
forward fill
backward fill
window overlap
intersection
union
```

No universal alignment policy is assumed.

---

# 33. Forward Fill

Forward filling is an analytical transformation.

For example:

```text
T1 → 10
T2 → missing
T3 → 12
```

may become:

```text
T2 → 10
```

only when explicitly requested.

A filled value must not be represented as if it were directly observed.

---

# 34. Backward Fill

Backward filling has the same requirement.

A derived filled value must retain provenance when its distinction from observed data matters.

---

# 35. Interpolation

Interpolation may create estimated observations between known values.

For example:

```text
T1 → 10
T3 → 14

T2 → 12
```

The interpolated value is derived.

It must not silently become a directly observed value.

---

# 36. Extrapolation

Extrapolated observations extend a series beyond the known observation range.

They are derived predictions/estimates and must be distinguishable from historical observations.

---

# 37. Smoothing

Smoothing transforms a series to reduce variation or noise.

Examples include:

```text
moving average
EWMA
median filter
```

A smoothed series is derived data.

It must retain provenance to the source series and transformation configuration when required.

---

# 38. Normalization

A series may be normalized:

```text
z-score
min-max scaling
percentage change
baseline-relative value
```

Normalization changes representation and possibly interpretation.

The resulting series must identify its transformation.

---

# 39. Differencing

A series may be transformed into differences:

```text
Vt - Vt-1
```

The resulting metric/series semantics must explicitly identify that it represents differences rather than original values.

---

# 40. Cumulative Series

A cumulative series may represent:

```text
V1
V1 + V2
V1 + V2 + V3
```

The accumulation rule must be explicit.

Reset boundaries must also be explicit.

---

# 41. Rolling Series

A rolling transformation applies a moving window.

Examples:

```text
rolling mean
rolling variance
rolling percentile
rolling volatility
```

The window definition must be part of the resulting series contract.

---

# 42. Expanding Series

An expanding transformation grows its historical window:

```text
[1]
[1,2]
[1,2,3]
```

The initial boundary and reset semantics must be explicit.

---

# 43. Time Series Aggregation

Aggregation may produce a Time Series.

For example:

```text
Events
    ↓
1-minute aggregation
    ↓
Time Series
```

Aggregation semantics are governed by the Aggregation Model.

---

# 44. OHLC Series

OHLC is a derived temporal representation.

For a window:

```text
Open  = first value
High  = maximum
Low   = minimum
Close = last value
```

An OHLC representation is therefore not a primitive event type.

It is an aggregation of ordered observations.

---

# 45. Volume

Volume may accompany an OHLC series.

For example:

```text
OHLCV
{
    open
    high
    low
    close
    volume
}
```

Volume semantics must be defined by the Metric/domain.

---

# 46. Candles

A candle is a domain/analytical representation of a temporal aggregation.

Conceptually:

```text
Time Series
    ↓
Window
    ↓
Aggregation
    ↓
Candle
```

The Core should not treat "candle" as a universal primitive.

---

# 47. Multi-Series

An analytical operation may combine multiple Time Series.

For example:

```text
price series
volume series
volatility series
```

Each series retains its own:

```text
metric
scope
dimensions
temporal semantics
provenance
```

---

# 48. Multi-Series Alignment

Combining series requires explicit:

```text
timestamp alignment
missing-data policy
resolution
scope compatibility
metric compatibility
```

The operation must reject incompatible combinations where required.

---

# 49. Series Scope

A Time Series may have a scope:

```text
player
session
table
instrument
market
system
```

The scope is part of its semantic identity when required.

---

# 50. Series Dimensions

Dimensions may distinguish multiple series for the same Metric.

For example:

```text
Metric:
    price

Dimensions:
    instrument = BTC
    venue = X
```

This allows:

```text
price(BTC, X)
price(BTC, Y)
```

to remain distinct series.

---

# 51. Series Identity and Dimensions

Two Time Series with different semantic dimensions should not automatically share the same logical identity.

The exact identity construction is deferred.

---

# 52. Series Version

A Time Series may have a version if its definition or contents are versioned.

Version is distinct from:

```text
TimeSeriesId
MetricId
Observation timestamp
Processing RunId
```

---

# 53. Series Equality

Two series may contain equivalent observations while having different identities or provenance.

Structural equality and semantic equivalence are distinct.

The Core does not impose universal series equality semantics.

---

# 54. Series Ownership

A Time Series may be:

```text
owned
borrowed
view
materialized
```

according to the API.

Views must not outlive the underlying storage/data.

---

# 55. Series Views

A view may expose a subset of a series:

```text
full series
    ↓
time range
    ↓
view
```

Views should avoid copying data when practical.

The ownership/lifetime contract must remain explicit.

---

# 56. Materialized Series

A transformed series may be materialized when:

```text
repeated access
performance
persistence
recovery
```

justify storage.

Materialization does not change semantic provenance.

---

# 57. Lazy Series

A series may be represented as a lazy transformation.

For example:

```text
source series
    ↓
filter
    ↓
transform
    ↓
view
```

The transformation must preserve deterministic and provenance semantics where required.

---

# 58. Series Iteration

The conceptual API should support ordered observation traversal.

For example:

```cpp
for (const auto& observation : series)
{
    ...
}
```

The exact iterator/view implementation remains deferred.

---

# 59. Range Access

A Time Series should conceptually support querying:

```text
[start, end)
```

and, where appropriate:

```text
first N observations
last N observations
```

The latter is an observation-count operation rather than a temporal range.

---

# 60. Append Semantics

A mutable series implementation may support appending observations.

Append behavior must define whether observations must be:

```text
strictly increasing
non-decreasing
arbitrarily ordered
```

and whether late insertion is supported.

---

# 61. Ordered Append

For efficient series construction, an implementation may require:

```text
T1 ≤ T2 ≤ T3
```

This is an implementation/contract choice.

The semantic model itself does not require append-only temporal order.

---

# 62. Out-of-Order Insertion

If out-of-order observations are accepted, the series implementation must define:

```text
ordering
indexing
duplicate handling
correction behavior
```

It must not silently corrupt temporal order.

---

# 63. Late Observations

A late observation may arrive after a series has already been materialized.

Possible handling includes:

```text
reject
insert
recompute
mark stale
create corrected version
```

The behavior belongs to the series/update contract.

---

# 64. Series Staleness

A materialized series may become stale when its source changes.

Staleness must be distinguishable from validity.

A stale derived series may remain useful as a historical result but must not silently claim to represent the latest source state.

---

# 65. Series Freshness

If freshness is exposed, it should identify what boundary it represents.

For example:

```text
latest processed event
latest event-time watermark
latest source position
```

"Current" must not be ambiguous.

---

# 66. Time Zone and Calendar

The Core Time Series model uses absolute temporal values.

Time zones and calendar semantics may be required for operations such as:

```text
day
week
month
trading session
```

These semantics are not selected universally by this ADR.

They may be domain/application-specific.

---

# 67. Calendar Boundaries

A calendar-based window must explicitly identify its calendar/time-zone semantics when those boundaries affect results.

For example:

```text
day
```

is not universally equivalent to:

```text
24 hours
```

---

# 68. Business/Trading Sessions

Domain-specific sessions may define temporal boundaries that differ from ordinary calendar periods.

Such semantics belong to the relevant domain.

---

# 69. Numerical Precision

Time Series operations must preserve appropriate value precision.

Transformations must not silently round values merely for storage/display convenience.

---

# 70. Floating-Point Determinism

Operations involving floating-point values may produce different results if operation order changes.

When reproducibility matters, the relevant calculation and ordering must be explicit.

---

# 71. Time Series Determinism

A deterministic transformation requires:

```text
input observations
+
ordering
+
configuration
+
algorithm/component version
+
relevant context
```

to determine the resulting series.

---

# 72. Provenance

Derived Time Series should preserve provenance sufficient to identify:

```text
source series
source range
transformation
configuration
algorithm version
execution run
```

when required.

---

# 73. Serialization

Serialization must preserve the semantics necessary to reconstruct the series.

Depending on the contract this may include:

```text
TimeSeriesId
MetricId/version
scope
dimensions
timestamps
temporal status
values
ordering
series version
provenance
```

---

# 74. Persistence

A Time Series may be:

```text
authoritative
derived
cache
recoverable
ephemeral
```

according to the Storage Model.

Persistence does not determine semantic authority.

---

# 75. Querying

Queries over Time Series should explicitly define:

```text
range
ordering
resolution
consistency
missing-data behavior
version
scope
dimensions
```

---

# 76. Query vs Transformation

A query selects information.

A transformation creates a different analytical representation.

For example:

```text
query:
    values from T1 to T2

transformation:
    resample to 1-minute intervals
```

These must remain separate concepts.

---

# 77. Time Series and Aggregation

Aggregation can consume a Time Series and produce:

```text
scalar Measurement
Time Series
structured analytical value
```

The aggregation function defines the result semantics.

---

# 78. Time Series and Pattern Detection

Pattern detectors may consume Time Series.

For example:

```text
Time Series
    ↓
Trend Detector
    ↓
Pattern
```

The resulting Pattern retains its own identity and provenance.

---

# 79. Time Series and Analysis

Analysis may consume Time Series directly.

For example:

```text
price series
    ↓
statistical analysis
    ↓
finding
```

The Time Series remains an input representation rather than becoming the analysis itself.

---

# 80. API Shape

The initial conceptual API should resemble:

```cpp
namespace evolution::timeseries
{

class Observation;
class TimeSeries;

}
```

The exact relationship between TimeSeries and Measurement remains implementation-defined.

---

# 81. Typed Series

Where possible, series should preserve the type of their values.

Conceptually:

```text
TimeSeries<double>
TimeSeries<Price>
TimeSeries<MeasurementValue>
```

rather than requiring all values to use a universal dynamic type.

---

# 82. Type Compatibility

Combining two series requires compatible value semantics.

For example:

```text
price + packet_count
```

must not be permitted merely because both happen to use `double`.

Semantic compatibility is more important than representation compatibility.

---

# 83. Unit Compatibility

Operations involving series values must respect unit semantics.

For example:

```text
meters + seconds
```

is invalid unless the operation explicitly defines a compatible transformation.

A universal dimensional-analysis framework is not selected by this ADR.

---

# 84. Series Construction

Conceptually:

```text
create series
    → define metric/scope/dimensions
    → append observations
    → validate ordering/values
```

Construction may return:

```text
Result<TimeSeries>
```

when validation can fail.

---

# 85. Series Validation

Validation may include:

```text
metric compatibility
value type
unit
temporal validity
ordering
scope
dimensions
duplicate policy
```

Domain-specific validation remains outside Core.

---

# 86. Testing Requirements

Tests must cover:

```text
observation construction
series identity
ordering
equal timestamps
known/estimated/unknown time
regular sampling
irregular sampling
missing values
duplicates
corrections
range queries
append
out-of-order handling
resampling
alignment
interpolation
smoothing
rolling operations
cumulative operations
OHLC aggregation
multi-series operations
provenance
serialization
determinism
ownership/lifetime
```

---

# 87. Deferred Decisions

This ADR does not select:

* exact TimeSeries C++ representation
* exact Observation representation
* storage layout
* columnar vs row-oriented representation
* lazy vs eager transformation implementation
* iterator/view implementation
* universal unit library
* calendar library
* timezone library
* interpolation framework
* numerical precision policy
* compression
* time-series database
* indexing strategy
* distributed series processing

---

# Decision Summary

```text
Time Series:
    Ordered temporal organization of observations

Observation:
    Value associated with temporal information

Time:
    Explicit and follows the Time Model

Ordering:
    Explicit

Equal timestamps:
    Supported only according to contract

Missing:
    Not automatically zero

Unknown:
    Distinct from missing and zero

Estimated:
    Must remain distinguishable when material

Regular sampling:
    Supported

Irregular sampling:
    Supported

Resampling:
    Explicit transformation

Interpolation:
    Derived data

Smoothing:
    Derived data

OHLC:
    Aggregated representation

Candles:
    Domain/analytical representation

Series identity:
    Distinct from Metric/Event/Measurement identity

Scope:
    Explicit

Dimensions:
    Explicit

Provenance:
    Required for derived series when material

Persistence:
    Storage concern

Query:
    Selection, not transformation

Unit compatibility:
    Required for semantic operations

Universal unit framework:
    Deferred
```

## Invariant

**A Time Series is an explicitly ordered temporal organization of observations; its temporal semantics, ordering, missing-data behavior, transformations, identity, scope, dimensions, and provenance remain explicit, while the series abstraction stays independent of storage, Event Streams, Measurements, and domain-specific analytical meaning.**

