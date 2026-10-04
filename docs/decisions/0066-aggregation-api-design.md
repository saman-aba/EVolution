# ADR 0066: Aggregation API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent an **Aggregation** as an explicit transformation that combines multiple inputs into a higher-level result according to a defined aggregation function, window, grouping, and configuration.

Conceptually:

```text
Inputs + Aggregation Function + Window + Grouping + Configuration
    ↓
Aggregation Result
```

Aggregation is a generic analytical mechanism.

The Core defines the structure and execution contract of aggregation, while domains and analysis components define domain-specific aggregation meaning.

---

# 1. Aggregation

Aggregation combines multiple observations or analytical objects into a result.

Examples:

```text
count
sum
average
minimum
maximum
median
percentile
variance
standard deviation
first
last
OHLC
```

Aggregation does not necessarily produce another Time Series.

It may produce:

```text
scalar
structured value
Measurement
Time Series
Pattern-supporting data
```

depending on the operation.

---

# 2. Aggregation vs Measurement

A Measurement is a quantitative observation.

Aggregation is the operation that produces a result from multiple inputs.

For example:

```text
Measurements
    ↓
average()
    ↓
Measurement
```

The aggregation operation and resulting Measurement remain conceptually distinct.

---

# 3. Aggregation vs Transformation

Not every transformation is an aggregation.

For example:

```text
x → x * 2
```

is a transformation.

Whereas:

```text
x1, x2, x3 → average(x1,x2,x3)
```

is an aggregation.

An aggregation reduces or combines multiple inputs according to defined semantics.

---

# 4. Aggregation Definition

Conceptually:

```text
Aggregation
{
    function
    input
    window
    grouping
    configuration
}
```

The exact C++ representation remains deferred.

---

# 5. Aggregation Function

The aggregation function defines how inputs are combined.

Conceptually:

```text
AggregationFunction
{
    initialize()
    update(input)
    finalize()
}
```

The exact interface is deferred.

---

# 6. Function State

An aggregation function may maintain internal state.

For example, an average may maintain:

```text
sum
count
```

rather than retaining every input.

The state is execution state and must not be confused with domain State.

---

# 7. Stateless Aggregation

Some aggregation operations may require no accumulated state beyond the input.

For example:

```text
first
last
```

depending on ordering and execution model.

---

# 8. Incremental Aggregation

An aggregation may consume inputs incrementally:

```text
input1 → update
input2 → update
input3 → update
...
finalize
```

This allows aggregation over streams without retaining all observations.

---

# 9. Mergeable Aggregation

An aggregation may support combining partial results:

```text
Partition A → partial result A
Partition B → partial result B

partial A + partial B
    ↓
final result
```

This is useful for partitioned and parallel processing.

Mergeability must be an explicit property of the aggregation function.

---

# 10. Associativity

Some aggregation functions are associative:

```text
(a + b) + c = a + (b + c)
```

Associativity allows different execution grouping without changing mathematical semantics.

Not every aggregation is associative.

---

# 11. Commutativity

Some aggregation functions are also commutative:

```text
a + b = b + a
```

Commutativity permits reordering inputs without changing the result.

Ordering-sensitive aggregations must not claim commutativity.

---

# 12. Ordering Requirements

Aggregation must declare whether input ordering matters.

Examples:

```text
sum:
    ordering independent

first:
    ordering required

last:
    ordering required

OHLC:
    ordering required for Open/Close
```

Execution must preserve the ordering contract.

---

# 13. Determinism

An aggregation is deterministic when equivalent inputs under the declared ordering/configuration produce an equivalent result.

Floating-point aggregation may depend on operation order.

If reproducibility requires a particular order, that order must be explicit.

---

# 14. Count

Count produces the number of accepted observations.

Conceptually:

```text
count(x1, x2, x3) = 3
```

The aggregation must define whether:

```text
missing
unknown
invalid
filtered
```

inputs contribute to the count.

---

# 15. Sum

Sum combines compatible numeric values.

```text
sum(x1, x2, ..., xn)
```

Unit compatibility must be respected.

For example:

```text
USD + USD
```

may be valid, while incompatible units are not.

---

# 16. Average

Average may be defined as:

```text
sum / count
```

The denominator semantics must be explicit.

An average of ratios is not automatically equivalent to a ratio of averages.

---

# 17. Weighted Average

Weighted average requires explicit weights:

```text
Σ(value × weight) / Σ(weight)
```

The weight Metric and semantics must be part of the aggregation contract.

---

# 18. Ratio Aggregation

Ratios require special treatment.

For example:

```text
rate = numerator / denominator
```

The correct aggregate may be:

```text
Σ numerator / Σ denominator
```

rather than:

```text
average(rate)
```

Aggregation must preserve numerator/denominator semantics where required.

---

# 19. Minimum and Maximum

Minimum and maximum require comparable values.

Missing values and invalid values must follow explicit policies.

---

# 20. First and Last

`first` and `last` depend on ordering.

They should not be implemented merely as:

```text
min(timestamp)
max(timestamp)
```

unless the aggregation contract explicitly defines this behavior.

---

# 21. Median

Median requires an ordering of values.

An implementation may:

```text
retain values
sort values
use a selection algorithm
```

without changing semantic meaning.

The implementation strategy remains deferred.

---

# 22. Percentile

Percentiles require explicit definition of:

```text
percentile level
interpolation method
sample/population semantics where relevant
missing-data policy
```

Different percentile definitions may produce different results.

The selected definition is part of configuration/algorithm identity.

---

# 23. Variance

Variance requires an explicit definition such as:

```text
population variance
sample variance
```

The distinction must not be hidden.

---

# 24. Standard Deviation

Standard deviation follows the variance definition.

The aggregation must preserve the selected statistical convention.

---

# 25. OHLC

OHLC is a structured aggregation:

```text
Open  = first
High  = maximum
Low   = minimum
Close = last
```

Conceptually:

```text
OHLC
{
    open
    high
    low
    close
}
```

The source ordering and window boundaries must be explicit.

---

# 26. OHLCV

An OHLCV aggregation may additionally calculate:

```text
volume = sum(volume)
```

Volume semantics remain domain-specific.

---

# 27. Structured Aggregation

An aggregation may produce multiple related values.

For example:

```text
Statistics
{
    count
    mean
    variance
    minimum
    maximum
}
```

The result should have an explicit semantic type rather than an arbitrary metadata dictionary.

---

# 28. Aggregation Result

Conceptually:

```text
AggregationResult
{
    value
    metadata
    provenance
}
```

The result may contain:

```text
value
measurement
time range
input count
quality information
provenance
```

as explicitly required by the contract.

---

# 29. Result vs Error

Aggregation execution may produce:

```text
Result<AggregationResult>
```

Failure is distinct from a valid aggregation result.

---

# 30. Empty Input

An aggregation must define behavior for an empty input.

Possible semantics include:

```text
valid identity value
no result
error
```

Examples:

```text
sum(empty) = 0
average(empty) = no result
minimum(empty) = no result
```

These are semantic decisions, not universal implementation rules.

---

# 31. Missing Input

Missing observations must not automatically become zero.

An aggregation must define its missing-data policy.

Possible policies include:

```text
IGNORE
PROPAGATE
REQUIRE_COMPLETE
IMPUTE
```

Any imputation must remain distinguishable from observed data where required.

---

# 32. Invalid Input

Invalid input should not silently enter an aggregation.

The aggregation contract must specify whether invalid values:

```text
reject
skip
propagate
```

and under what circumstances.

---

# 33. Unknown Values

Unknown values are distinct from zero and missing data.

An aggregation may:

```text
propagate unknown
ignore unknown
reject input
```

according to explicit semantics.

---

# 34. Window

Aggregation usually operates over a defined window.

Examples:

```text
time window
observation-count window
event window
session window
domain-defined window
```

The window must be explicit.

---

# 35. Time Window

A time window may use:

```text
[start, end)
```

according to the Time Model.

For example:

```text
[10:00, 11:00)
```

includes observations at 10:00 but not 11:00.

---

# 36. Observation Window

An observation-count window may be:

```text
last 100 observations
```

This is different from:

```text
last 100 seconds
```

The two must not be conflated.

---

# 37. Event Window

An aggregation may operate over a number of Events rather than Measurements.

For example:

```text
last 500 hands
```

The event definition and counting semantics belong to the relevant domain.

---

# 38. Session Window

A domain may define windows based on session boundaries.

For example:

```text
SessionStarted
    ↓
events
    ↓
SessionFinished
```

Session semantics belong to the domain.

---

# 39. Rolling Aggregation

A rolling aggregation evaluates overlapping windows.

Example:

```text
T1 → average(T1)
T2 → average(T1,T2)
T3 → average(T1,T2,T3)
T4 → average(T2,T3,T4)
```

Window size and boundary semantics must be explicit.

---

# 40. Expanding Aggregation

An expanding aggregation increases its historical window:

```text
T1 → aggregate(T1)
T2 → aggregate(T1,T2)
T3 → aggregate(T1,T2,T3)
```

Reset boundaries must be explicit.

---

# 41. Tumbling Windows

A tumbling aggregation partitions observations into non-overlapping windows:

```text
[00:00,01:00)
[01:00,02:00)
[02:00,03:00)
```

Boundary behavior follows the Time Model.

---

# 42. Sliding Windows

A sliding window may have:

```text
window size
slide interval
```

For example:

```text
window = 10 minutes
slide = 1 minute
```

The resulting windows overlap.

---

# 43. Grouping

Aggregation may group inputs by dimensions.

For example:

```text
group by player
group by instrument
group by session
```

Conceptually:

```text
inputs
    ↓
group
    ↓
aggregate each group
```

---

# 44. Grouping Dimensions

Grouping keys must have explicit semantic identity.

Two fields with the same textual representation are not necessarily semantically compatible.

---

# 45. Hierarchical Grouping

Multiple grouping levels may be supported:

```text
market
  └── instrument
       └── venue
```

Hierarchical aggregation must define whether parent results are independently calculated or derived from child results.

---

# 46. Grouped Result

A grouped aggregation produces multiple results.

Conceptually:

```text
Group A → Result A
Group B → Result B
Group C → Result C
```

Each result must retain the grouping identity.

---

# 47. Partitioning

Aggregation may execute independently on partitions.

For example:

```text
Partition A → partial A
Partition B → partial B
Partition C → partial C
```

A final merge is valid only if the aggregation supports the required merge semantics.

---

# 48. Parallel Aggregation

Parallel execution is an implementation concern.

The semantic result must remain compatible with the aggregation's ordering and determinism contract.

---

# 49. Merge Contract

A mergeable aggregation conceptually provides:

```text
initialize()
update(input)
merge(partial)
finalize()
```

The merge operation must preserve the semantics of the aggregation.

---

# 50. Non-Mergeable Aggregations

An aggregation that cannot safely merge partial state must be executed according to its ordering/state requirements.

The processing system must not automatically parallelize it.

---

# 51. Aggregation State

Aggregation execution state may include:

```text
running count
partial sum
minimum
maximum
statistical accumulators
window state
```

This is processing state, not domain State.

---

# 52. State Reset

Reusable aggregation instances may need explicit reset semantics.

Reset must establish the defined initial aggregation state.

---

# 53. Incremental Updates

Incremental aggregation may update a result when new observations arrive without recomputing all historical inputs.

This is an optimization only when semantic equivalence is guaranteed.

---

# 54. Retraction

Some streaming aggregations may need to remove an observation from an existing window.

For example:

```text
add(x)
remove(x)
```

Support for retraction must be explicitly declared.

Not every aggregation is retractable.

---

# 55. Window Expiration

When an observation leaves a rolling window, the aggregation must define how its contribution is removed.

Possible approaches include:

```text
retract
recompute
retain sufficient state
```

The implementation remains deferred.

---

# 56. Aggregation Configuration

Configuration may include:

```text
window
grouping
missing-data policy
ordering
percentile definition
weighting
precision
reset behavior
```

Configuration must be explicit.

---

# 57. Aggregation Identity

A persistent aggregation definition may have an identity.

This identity is distinct from:

```text
AggregationResult identity
MetricId
TimeSeriesId
ProcessorId
RunId
```

The exact identity model follows the general Identity Model.

---

# 58. Aggregation Version

Changes to aggregation semantics should produce a distinct version when they can alter results.

For example:

```text
percentile algorithm v1
percentile algorithm v2
```

must not silently appear equivalent.

---

# 59. Provenance

Aggregation results should identify their inputs and transformation when required.

Conceptually:

```text
inputs
+
aggregation definition
+
configuration
+
algorithm version
+
execution context
    ↓
result
```

---

# 60. Reproducibility

A reproducible aggregation requires sufficient information to reconstruct:

```text
input identity/version
aggregation definition
configuration
component/algorithm version
relevant execution context
required state
```

---

# 61. Aggregation Over Events

Aggregations may consume Events directly.

Example:

```text
PlayerAction events
    ↓
count
    ↓
action count
```

The aggregation does not thereby become domain-specific.

The meaning of `PlayerAction` belongs to the domain.

---

# 62. Aggregation Over State

Aggregations may consume State snapshots.

For example:

```text
PlayerState snapshots
    ↓
aggregate bankroll
```

The State semantics remain domain-specific.

---

# 63. Aggregation Over Measurements

Measurements are a natural aggregation input.

For example:

```text
Measurement stream
    ↓
rolling average
    ↓
Time Series
```

---

# 64. Aggregation Over Time Series

A Time Series may be aggregated into:

```text
scalar
structured value
coarser Time Series
```

For example:

```text
1-minute series
    ↓
1-hour OHLC
```

---

# 65. Aggregation Output Time

When aggregation produces a temporal result, the resulting timestamp must have explicit semantics.

Possible choices include:

```text
window start
window end
window center
last observation
domain-defined timestamp
```

No universal choice is assumed.

---

# 66. Aggregation Output Interval

An aggregated result may represent a time interval.

For example:

```text
[10:00, 11:00) → average = 42
```

The interval should be represented explicitly rather than pretending the value was observed at an arbitrary point.

---

# 67. Aggregation and Temporal Status

If input observations have estimated or unknown time, the resulting temporal information must not silently become known.

The derivation rules must define how temporal uncertainty/status propagates.

---

# 68. Aggregation and Units

Aggregation must preserve compatible units.

Examples:

```text
USD + USD → USD
USD / USD → ratio
USD / hour → USD/hour
```

Exact dimensional analysis remains deferred.

---

# 69. Aggregation and Precision

The aggregation implementation must not silently reduce precision beyond its declared contract.

For floating-point calculations, numerical error may be relevant to reproducibility.

---

# 70. Aggregation and Sampling

Aggregating sampled measurements may produce biased results if sampling is non-uniform.

The aggregation must not assume uniform sampling unless the contract establishes it.

---

# 71. Weighted Temporal Aggregation

Some temporal metrics require weighting by elapsed time.

For example:

```text
time-weighted average
```

must not be implemented as a simple arithmetic mean unless those semantics are equivalent.

---

# 72. Rate Aggregation

Rates require denominator semantics.

For example:

```text
events / second
```

may require:

```text
total events / total elapsed time
```

rather than an arithmetic average of instantaneous rates.

---

# 73. Aggregation and Domain Semantics

The Core may provide generic functions such as:

```text
sum
count
mean
min
max
```

but it must not define domain-specific meanings such as:

```text
VPIP
PFR
Sharpe Ratio
Win Rate
Expected Value
```

Those belong to domain/analysis layers.

---

# 74. Aggregation as Processor

An aggregation may be implemented as a Processor.

For example:

```text
Processor
    input: Measurement
    state: AggregationState
    output: AggregationResult
```

This connects aggregation semantics with the Processing Model.

The Aggregation concept itself remains independent of execution infrastructure.

---

# 75. Aggregation Lifecycle

Conceptually:

```text
initialize
    ↓
update*
    ↓
merge*
    ↓
finalize
```

Not every implementation needs every operation.

For example, a non-mergeable aggregation may omit `merge`.

---

# 76. Error Handling

Aggregation failures use the established `Result<T>` / `Error` model.

Possible errors include:

```text
invalid configuration
incompatible input
invalid value
unsupported operation
resource failure
internal invariant violation
```

Aggregation must not silently turn invalid input into valid output.

---

# 77. Cancellation

Long-running aggregation may be cancellable.

Cancellation is distinct from aggregation failure.

A cancelled operation must not be represented as a successful aggregation result.

---

# 78. Partial Results

Partial aggregation results may be exposed when explicitly supported.

A partial result must not be represented as a final result.

Its completeness/status must be explicit.

---

# 79. Persistence

Aggregation definitions and materialized results may be persisted.

Persistence semantics follow the Persistence Interface Model.

The aggregation itself does not require persistent storage.

---

# 80. Serialization

Serialization of an aggregation definition must preserve all configuration that affects semantic results.

Serialization of results must preserve sufficient metadata to interpret the result.

---

# 81. Testing

Aggregation tests must cover:

```text
empty input
single input
multiple inputs
ordering
equal values
negative values
zero
missing values
unknown values
invalid values
units
precision
floating-point behavior
rolling windows
tumbling windows
sliding windows
grouping
partitioning
merge
retraction
incremental updates
OHLC
weighted averages
ratios
rates
provenance
determinism
cancellation
errors
```

Property-based tests should be used where algebraic properties apply.

For example:

```text
sum(a,b,c) == sum(sum(a,b),c)
```

where the declared numerical semantics permit that comparison.

---

# 82. Deferred Decisions

This ADR does not select:

* exact C++ Aggregation API
* type-erasure mechanism
* aggregation state representation
* numerical library
* decimal/fixed-point implementation
* percentile algorithm library
* vectorized/SIMD implementation
* parallel execution mechanism
* window engine implementation
* persistent aggregation state format
* distributed aggregation protocol
* universal unit framework

---

# Decision Summary

```text
Aggregation:
    Explicit combination of multiple inputs

Function:
    Defines aggregation semantics

Window:
    Defines which inputs participate

Grouping:
    Defines independent aggregation domains

Ordering:
    Explicitly declared

Missing:
    Explicit policy

Unknown:
    Distinct from missing and zero

Empty input:
    Explicit semantics

Incremental:
    Supported where function permits

Merge:
    Supported only where semantics permit

Retraction:
    Optional and explicit

Associativity:
    Declared property

Commutativity:
    Declared property

Determinism:
    Explicit

Result:
    May be scalar or structured

Temporal result:
    Explicit timestamp/interval semantics

Provenance:
    Preserved where required

Domain meaning:
    Outside Core

Execution:
    Processing concern
```

## Invariant

**An Aggregation is an explicit, contract-defined combination of inputs whose function, ordering, window, grouping, missing-data behavior, result semantics, and reproducibility requirements are visible; the Core provides aggregation mechanisms without defining domain-specific analytical meaning.**

