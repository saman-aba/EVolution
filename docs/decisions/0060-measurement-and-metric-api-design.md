# ADR 0060: Measurement and Metric API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will distinguish between a **Metric** and a **Measurement**.

```text
Metric
    → defines what is being measured

Measurement
    → records an observed/derived value of that metric
```

A Measurement is a derived analytical observation and may be produced from Events, State, Time Series, other Measurements, or external observations according to an explicit contract.

The Core provides generic representation and mechanisms.

Domains and analytical components define metric meaning.

---

# 1. Metric

A Metric defines the semantic meaning of a quantitative observation.

Conceptually:

```text
Metric
{
    id
    name
    unit?
    definition
}
```

The exact representation remains implementation-defined.

---

# 2. Metric Identity

A Metric may have a logical identity:

```cpp
using MetricId = evolution::Id<MetricTag>;
```

The identity identifies the metric definition.

It does not identify an individual measurement.

Therefore:

```text
MetricId ≠ MeasurementId
```

---

# 3. Metric vs Measurement

For example:

```text
Metric:
    name = win_rate

Measurement:
    value = 0.42
    metric = win_rate
    time_range = [T1, T2)
```

The Metric defines what `win_rate` means.

The Measurement records one result.

---

# 4. Metric Definition

A Metric definition should specify, where relevant:

```text
name
definition
unit
value domain
scope
dimensions
temporal semantics
calculation
precision
missing-data semantics
```

Not every metric requires every property.

---

# 5. Metric Meaning

Core must not define domain meaning.

For example:

```text
Poker:
    VPIP

Markets:
    realized_volatility

Networking:
    packets_per_second
```

may all be Metrics, but their semantic definitions belong to their respective domains or analytical modules.

---

# 6. Metric Unit

A Metric should identify its unit when a meaningful unit exists.

Examples:

```text
seconds
milliseconds
bytes
packets
hands
USD
BB
ratio
percentage
count
```

Unit semantics must not be inferred solely from the metric name.

---

# 7. Dimensionless Metrics

Some Metrics are dimensionless.

Examples:

```text
ratio
probability
percentage
correlation coefficient
```

The absence of a physical unit must not imply that the metric has no semantic definition.

---

# 8. Unit vs Scale

These are separate.

For example:

```text
seconds
milliseconds
microseconds
```

may represent the same dimensional quantity at different scales.

A metric definition should establish the intended unit and representation.

---

# 9. Quantity Conversion

Unit conversion may be supported by future infrastructure.

However, numerical conversion must preserve semantic meaning.

For example:

```text
1 second = 1000 milliseconds
```

does not mean that arbitrary numeric values may be converted without knowing their unit.

A universal quantity/unit framework is not selected by this ADR.

---

# 10. Measurement Identity

A persistent Measurement may have:

```cpp
using MeasurementId = evolution::Id<MeasurementTag>;
```

Measurement identity is distinct from:

```text
MetricId
EventId
ProcessorId
RunId
```

---

# 11. Measurement Value

A Measurement contains a value associated with its Metric.

Conceptually:

```text
Measurement
{
    id
    metric
    value
    scope
    dimensions?
    temporal_information?
    provenance?
}
```

The exact numeric representation depends on the Metric.

---

# 12. Value Type

Measurements are not restricted to one universal numeric type.

Possible values include:

```text
integer
floating-point
fixed-point
boolean
distribution
vector
structured statistical value
```

However, the value type must be part of the Metric/Measurement contract.

A universal `variant<anything>` is not the default design.

---

# 13. Scalar Measurements

The most common form is a scalar:

```text
Metric:
    packet_rate

Measurement:
    value = 120000
    unit = packets/second
```

The Core should support scalar measurements efficiently.

---

# 14. Structured Measurements

Some analytical outputs naturally contain multiple related values.

Examples:

```text
mean
variance
sample_count
confidence_interval
```

A structured measurement is valid when the structure is part of the metric's contract.

It must not be used as a generic escape hatch for arbitrary unrelated values.

---

# 15. Measurement Time

A Measurement may have temporal semantics.

It may represent:

```text
point
interval
window
```

Conceptually:

```text
Measurement
{
    ...
    time_range?
}
```

The exact representation follows the Time API.

---

# 16. Point Measurement

A point measurement represents a value associated with a particular temporal point.

For example:

```text
balance at T
price at T
state value at T
```

---

# 17. Interval Measurement

An interval measurement represents a value derived over:

```text
[start, end)
```

For example:

```text
average_latency
    over [T1, T2)
```

The boundaries must be explicit.

---

# 18. Window Measurement

A measurement may be based on a processing/analytical window.

Examples:

```text
last 100 hands
last 5 minutes
current session
current trading period
```

The window definition must be part of the measurement's provenance or semantic contract when necessary.

---

# 19. Event-Based Windows

Not all windows are time-based.

A measurement may cover:

```text
last 100 events
last 50 hands
one session
one game
```

The measurement must identify its relevant scope and window semantics.

---

# 20. Scope

Measurements must identify the scope to which they apply when relevant.

Examples:

```text
player
session
table
game
market
instrument
system
processor
```

Scope semantics are domain-owned.

---

# 21. Dimensions

Dimensions distinguish multiple measurements of the same Metric.

For example:

```text
Metric = win_rate

Dimensions:
    player = Alice
    table = T1
```

A measurement's dimensions must have explicit semantic meaning.

---

# 22. Dimension Identity

Dimension values may reference domain entities.

For example:

```text
player_id
market_id
session_id
```

These identities must use the appropriate strongly typed identity types.

A generic string field must not silently replace a semantic identity.

---

# 23. Measurement vs Context

Context describes circumstances.

Dimensions identify how a measurement is scoped or partitioned.

They may overlap conceptually but are not automatically interchangeable.

For example:

```text
player = Alice
```

may be a measurement dimension.

The current table configuration may be contextual information.

---

# 24. Measurement Provenance

A derived Measurement should be able to identify where it came from.

For example:

```text
Measurement
    metric = win_rate
    provenance:
        events E1..E500
        processor P3
        configuration C7
        run R2
```

The exact provenance representation follows the Provenance Model.

---

# 25. Direct Measurements

Some Measurements may originate directly from an external observation.

For example:

```text
external sensor reading
market feed price
recorded packet count
```

Such measurements still require explicit provenance identifying their source.

---

# 26. Derived Measurements

Other Measurements are derived:

```text
Events
    ↓
State
    ↓
Measurement
```

or:

```text
Measurements
    ↓
Aggregation
    ↓
Measurement
```

Derived status should be clear through provenance and/or the relevant metric contract.

---

# 27. Measurement vs Aggregation

Aggregation is a transformation.

Measurement is the resulting quantitative observation.

For example:

```text
100 values
    ↓ average aggregation
42.5
```

`average` is the aggregation operation.

`42.5` is the resulting measurement value.

---

# 28. Aggregation Result

An aggregation may produce:

```text
scalar measurement
structured measurement
time series
other analytical representation
```

Therefore not every aggregation result must automatically be stored as a Measurement.

The output contract determines its semantic type.

---

# 29. Rates

Rates require explicit temporal or denominator semantics.

For example:

```text
packets / second
hands / hour
events / minute
```

A rate must retain enough information to establish its denominator.

Blindly averaging rates may produce an incorrect result.

---

# 30. Ratios

Ratios require similar care.

For:

```text
successes / attempts
```

aggregating:

```text
success_rate_1
success_rate_2
```

by arithmetic mean is not generally equivalent to:

```text
total_successes / total_attempts
```

The Metric definition must specify the correct aggregation semantics.

---

# 31. Percentages

Percentage and ratio are related but not interchangeable representations.

For example:

```text
0.42
```

may represent:

```text
42%
```

only when the Metric contract defines that interpretation.

The unit/scale must be explicit.

---

# 32. Counts

Counts are quantitative Measurements.

Examples:

```text
events = 100
hands = 500
errors = 12
packets = 100000
```

Count semantics should identify what is being counted.

---

# 33. Missing Measurement

A missing measurement is not automatically zero.

For example:

```text
no observation
```

must not silently become:

```text
value = 0
```

unless the Metric explicitly defines zero as the correct semantic representation.

---

# 34. Unknown Measurement

A Measurement may have insufficient information to produce a value.

Unknown value semantics must be explicitly supported by the relevant metric.

The implementation must distinguish:

```text
zero
missing
unknown
not applicable
invalid
```

when the metric contract requires these distinctions.

---

# 35. Invalid Measurement

A value that violates the Metric contract must not become a valid Measurement.

For example:

```text
probability = 3.7
```

would be invalid if the Metric contract requires:

```text
0 ≤ probability ≤ 1
```

The appropriate response is validation failure.

---

# 36. Precision

Measurements must preserve appropriate numerical precision.

The implementation must not silently round values merely for storage or display convenience.

Display formatting is an interface concern.

---

# 37. Floating-Point Measurements

Floating-point equality must not automatically be treated as exact semantic equality.

Metrics requiring numerical comparison must define appropriate tolerance or comparison semantics.

This is particularly important for:

```text
statistics
probabilities
correlations
financial values
physical measurements
```

---

# 38. NaN and Infinity

Whether `NaN`, positive infinity, or negative infinity are valid depends on the Metric contract.

They must not be silently converted into:

```text
0
null
missing
```

without explicit semantic rules.

---

# 39. Measurement Confidence

A Measurement may require confidence information.

For example:

```text
estimate = 0.42
confidence = 0.95
```

Confidence is not automatically part of every Measurement.

When used, its interpretation must be defined by the Metric/Analysis contract.

---

# 40. Confidence vs Uncertainty

Confidence and numerical/temporal uncertainty are different concepts.

For example:

```text
measurement uncertainty
confidence interval
temporal uncertainty
model confidence
```

must not be collapsed into one generic field.

---

# 41. Measurement Quality

A measurement may have quality information such as:

```text
exact
estimated
sampled
approximate
partial
```

The exact quality model is deferred.

Quality information must not silently alter the measurement's semantic value.

---

# 42. Measurement Temporal Status

If a Measurement has temporal information, it follows the Time Model:

```text
KNOWN
ESTIMATED
UNKNOWN
```

An estimated measurement timestamp must not become known merely because the Measurement itself was successfully constructed.

---

# 43. Measurement Dependencies

A Measurement may depend on:

```text
Events
State
Other Measurements
Time Series
Context
External Data
```

Its provenance should identify relevant dependencies when reproducibility requires them.

---

# 44. Measurement Determinism

For a deterministic metric calculation:

```text
Inputs
+
Metric Definition
+
Configuration
+
Component/Algorithm Version
+
Relevant Context
```

must determine the measurement.

---

# 45. Measurement Recalculation

Recomputing a Measurement does not necessarily mean producing the same logical object.

Possible semantics include:

```text
same identity + new version
new identity
same result attached to a new run
```

The calculation contract must determine the appropriate identity semantics.

---

# 46. Measurement Versioning

A Metric definition may evolve.

A Measurement should identify the Metric definition/version used when that distinction materially affects interpretation.

A change in metric semantics must not silently reinterpret historical measurements.

---

# 47. Metric Compatibility

Changing a Metric definition may affect:

```text
unit
calculation
scope
dimensions
temporal semantics
aggregation
interpretation
```

Semantic compatibility must be evaluated rather than inferred from the metric name remaining unchanged.

---

# 48. Measurement Serialization

Serialization must preserve, when required:

```text
Measurement identity
Metric identity/version
value
unit
scope
dimensions
temporal information
quality
provenance
```

The serialization format remains implementation-defined.

---

# 49. Measurement Persistence

Measurements may be:

```text
authoritative
derived
cached
recomputable
ephemeral
```

according to the Storage Model.

Derived measurements should normally retain enough provenance to support reconstruction or interpretation.

---

# 50. Measurement Querying

Queries for Measurements must preserve:

```text
Metric
Scope
Dimensions
Temporal semantics
Units
Version
Provenance
```

A query must not silently mix semantically incompatible metric versions.

---

# 51. Measurement and Time Series

A Time Series can organize Measurements or scalar values over time.

The distinction is:

```text
Measurement
    → quantitative semantic observation

Time Series
    → temporal organization of observations
```

A Measurement does not automatically imply a Time Series.

---

# 52. Measurement and Pattern

Patterns may be detected from Measurements.

For example:

```text
Measurements
    ↓
Trend detector
    ↓
Pattern
```

The Pattern describes structure.

The Measurement remains the underlying quantitative observation.

---

# 53. Measurement and Analysis

Analysis may interpret Measurements.

For example:

```text
Measurement:
    win_rate = 0.42

Analysis:
    win rate significantly below baseline
```

The analysis does not redefine the Measurement.

---

# 54. Metric Registration

Metrics may participate in the Registration/Discovery model.

A Metric definition can expose:

```text
identity
version
unit
value type
scope requirements
dimensions
capabilities
```

Registration does not itself produce measurements.

---

# 55. Initial API Shape

The initial API should conceptually resemble:

```cpp
namespace evolution::measurement
{

class Metric;
class Measurement;

}
```

The exact value representation and generic quantity model remain implementation decisions.

---

# 56. Initial Scalar Measurement

The Core should support a simple scalar representation without forcing every measurement into a complex object hierarchy.

Conceptually:

```text
Measurement
{
    id
    metric
    value
    scope
    dimensions?
    temporal_information?
    provenance?
}
```

---

# 57. Generic Core vs Domain Metrics

Core should provide:

```text
Measurement structure
Metric structure
Value handling mechanisms
Identity
Time
Provenance
Validation
```

Domains should provide:

```text
metric meaning
calculation
valid ranges
domain units
dimensions
aggregation rules
interpretation
```

---

# 58. No Universal Metric Registry Requirement

EVolution does not require a single global registry containing every Metric.

Metric discovery may be scoped to:

```text
domain
application
graph
analysis package
extension
```

according to the application.

---

# 59. Testing Requirements

Tests must cover:

```text
Metric identity
Measurement identity
Metric/measurement association
Units
Scalar values
Structured values where supported
Point time
Interval time
Unknown time
Estimated time
Dimensions
Scope
Missing vs zero
Invalid values
Precision
Serialization
Copy/move
Provenance
Metric version compatibility
```

---

# 60. Deferred Decisions

This ADR does not select:

* exact Measurement C++ representation
* exact Metric C++ representation
* universal quantity/unit library
* fixed-point vs floating-point policy
* decimal representation
* uncertainty framework
* confidence framework
* structured-value representation
* metric registry implementation
* measurement storage backend
* time-series representation
* statistical distribution type
* serialization format

---

# Decision Summary

```text
Metric:                       Defines what is measured
Measurement:                  Records a value of a metric
Metric identity:              MetricId
Measurement identity:         MeasurementId
Metric meaning:               Domain / Analysis
Unit:                         Explicit where meaningful
Scope:                        Explicit where relevant
Dimensions:                   Explicit where relevant
Time:                         Explicit where relevant
Temporal status:              KNOWN / ESTIMATED / UNKNOWN
Provenance:                   Supported
Missing ≠ zero:               Yes
Unknown ≠ zero:               Yes
Invalid values:               Rejected
Precision:                    Preserved
Rates:                        Denominator semantics required
Ratios:                       Aggregation semantics required
Measurement ≠ aggregation:    Yes
Measurement ≠ time series:    Yes
Measurement ≠ pattern:        Yes
Measurement ≠ analysis:       Yes
Universal unit framework:     Deferred
Universal metric registry:    Not required
```

## Invariant

**EVolution separates metric meaning from measured values: a Metric defines what a quantitative observation means, while a Measurement records an instance of that metric with explicit identity, value, scope, dimensions, temporal semantics, and provenance; units, missing values, uncertainty, aggregation, and interpretation must remain explicit rather than being inferred from representation or metric names.**

