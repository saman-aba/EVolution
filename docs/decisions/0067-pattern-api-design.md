# ADR 0067: Pattern API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent a **Pattern** as an explicit description of a recognized structure, relationship, behavior, recurrence, or change within events, state, measurements, or time series.

Conceptually:

```text
Input Data + Pattern Definition + Configuration + Context
    ↓
Pattern Detection
    ↓
Pattern
```

A Pattern describes **what structure was detected**, not why it occurred and not what action should be taken.

Pattern detection is an analytical mechanism and remains separate from decision-making.

---

# 1. Pattern

A Pattern represents a detected structure in data.

Examples include:

```text
Trend
Reversal
Drawdown
Level Shift
Volatility Change
Cycle
Cluster
Anomaly
Correlation
Divergence
Persistence
Regime Change
```

The list is extensible.

The Core provides the structural model; domains and analysis components define specialized pattern semantics.

---

# 2. Pattern vs Measurement

A Measurement represents a quantitative observation.

For example:

```text
volatility = 0.25
```

A Pattern may interpret a sequence of measurements:

```text
volatility increased persistently
```

The measurement is the value.

The pattern is the detected structure.

---

# 3. Pattern vs Aggregation

Aggregation combines observations.

Pattern detection identifies structure within observations or derived results.

For example:

```text
Measurements
    ↓
rolling average
    ↓
Measurement / Time Series
    ↓
trend detector
    ↓
Trend Pattern
```

Aggregation does not imply that a pattern exists.

---

# 4. Pattern vs Analysis

Pattern detection identifies structure.

Analysis interprets that structure.

For example:

```text
Pattern:
    repeated drawdown

Analysis:
    drawdown is unusually persistent relative to baseline
```

The two remain separate.

---

# 5. Pattern vs Decision

A pattern does not imply an action.

For example:

```text
Pattern:
    volatility increased

Decision:
    reduce exposure
```

The decision requires a policy and additional context.

---

# 6. Pattern Identity

A persistent Pattern may have an identity.

Conceptually:

```text
Pattern
{
    id
    type
    scope
    time_range
    evidence
    confidence
    provenance
}
```

The exact representation remains deferred.

---

# 7. Pattern Type

Pattern type identifies the semantic category of detected structure.

Examples:

```text
Trend
Anomaly
RegimeChange
Cluster
Correlation
```

Pattern type is distinct from the C++ implementation type.

A persistent pattern type must not depend on compiler-generated C++ type names.

---

# 8. Pattern Scope

A Pattern applies to a defined scope.

Examples:

```text
session
player
hand
instrument
market
dataset
application
```

Scope semantics belong to the relevant domain.

---

# 9. Pattern Time Range

A Pattern may represent an interval:

```text
[start, end)
```

The interval identifies where the structure was observed.

A point-like pattern may instead have a single relevant timestamp.

The temporal representation must remain explicit.

---

# 10. Pattern Temporal Information

Pattern timestamps follow the Time Model.

Temporal information may be:

```text
KNOWN
ESTIMATED
UNKNOWN
```

A detector must not silently turn estimated input time into known pattern time.

---

# 11. Evidence

A Pattern should identify the evidence supporting its detection.

Evidence may reference:

```text
events
measurements
time series
states
other patterns
```

Evidence references should use logical identity where persistent objects have identity.

---

# 12. Evidence vs Provenance

Evidence answers:

> What observations support this pattern?

Provenance answers:

> How was this pattern produced?

They are related but distinct.

---

# 13. Confidence

A detector may attach confidence to a Pattern.

Confidence must have explicit semantics.

For example:

```text
confidence = 0.95
```

must not automatically be interpreted as:

```text
95% probability that the pattern is true
```

unless the detector explicitly defines that interpretation.

---

# 14. Strength

Some patterns may have a separate notion of strength.

For example:

```text
weak trend
strong trend
```

Strength and confidence are distinct.

A detector may be highly confident that a weak trend exists.

---

# 15. Significance

Statistical significance is distinct from confidence and strength.

If statistical significance is reported, the statistical definition must be explicit.

---

# 16. Pattern Status

Patterns that persist over time may have lifecycle state.

Conceptually:

```text
NOT_DETECTED
DETECTED
ACTIVE
ENDED
```

A pattern may also be represented as a completed immutable observation rather than a mutable lifecycle object.

The chosen representation depends on the processing context.

---

# 17. Pattern Detection

A PatternDetector conceptually performs:

```text
PatternDetector
{
    input
    configuration
    detect()
}
```

The exact C++ API remains deferred.

---

# 18. Stateless Detection

Some detectors can inspect a finite input and produce a result without persistent execution state.

Example:

```text
detectTrend(time_series)
```

The detector is stateless from the processing perspective.

---

# 19. Stateful Detection

Some detectors need historical state.

For example:

```text
previous observations
current regime
active pattern
threshold history
```

Such state belongs to detector execution and must not be confused with domain State.

---

# 20. Incremental Detection

A detector may process observations incrementally:

```text
observation
    ↓
update detector state
    ↓
pattern state
```

This avoids recomputing the complete historical input.

Incremental semantics must remain equivalent to the declared detection contract.

---

# 21. Pattern Detection Over Time Series

Time Series are a common input.

For example:

```text
Time Series
    ↓
Trend Detector
    ↓
Trend Pattern
```

The detector must define:

```text
ordering
window
sampling assumptions
missing-data behavior
thresholds
```

---

# 22. Pattern Detection Over Measurements

Patterns may be detected directly from Measurement streams.

For example:

```text
Measurement stream
    ↓
anomaly detector
    ↓
Anomaly Pattern
```

---

# 23. Pattern Detection Over Events

Events may also be the direct input.

For example:

```text
Events
    ↓
repeated-action detector
    ↓
behavioral pattern
```

The detector may interpret domain-specific Event semantics outside Core.

---

# 24. Pattern Detection Over State

Patterns may be identified from state transitions or snapshots.

For example:

```text
State history
    ↓
state-change detector
    ↓
LevelShift Pattern
```

---

# 25. Pattern Windows

Pattern detection may use windows.

Examples:

```text
last 100 observations
last 1 hour
current session
between two state transitions
```

Window semantics follow the Aggregation and Time models.

---

# 26. Threshold-Based Detection

A detector may identify a pattern when a value crosses a threshold.

Example:

```text
x > threshold
```

The threshold must be part of configuration or detector definition.

---

# 27. Persistence-Based Detection

A pattern may require a condition to remain true.

For example:

```text
condition true
for N observations
```

or:

```text
condition true
for T seconds
```

The persistence requirement must be explicit.

---

# 28. Trend Detection

A trend detector may identify sustained directional movement.

Possible configuration includes:

```text
window
minimum slope
minimum duration
minimum magnitude
noise tolerance
```

The mathematical definition of trend must be explicit.

---

# 29. Reversal Detection

A reversal identifies a change in direction.

Conceptually:

```text
increasing
    ↓
turning point
    ↓
decreasing
```

The detector must define how turning points and noise are handled.

---

# 30. Anomaly Detection

An anomaly identifies an observation or sequence that differs from a defined baseline or expected behavior.

Anomaly detection must define the reference model.

For example:

```text
historical distribution
rolling baseline
statistical model
domain rule
```

---

# 31. Baseline

A Pattern detector may require a baseline.

A baseline may be:

```text
historical
rolling
population-based
peer-based
model-derived
```

The baseline itself should be identifiable and reproducible.

---

# 32. Regime Change

A regime-change detector identifies a transition between statistically or behaviorally distinct regimes.

For example:

```text
Regime A
    ↓
transition
    ↓
Regime B
```

The definition of regime belongs to the detector or analysis domain.

---

# 33. Correlation Pattern

A correlation pattern identifies a statistical relationship between variables.

Correlation does not imply causation.

The detector must specify:

```text
correlation measure
window
significance method
missing-data policy
alignment
```

---

# 34. Divergence Pattern

A divergence pattern may identify different behavior between related series.

For example:

```text
Series A ↑
Series B ↓
```

The detector must define:

```text
series relationship
alignment
window
threshold
direction
```

---

# 35. Cluster Pattern

A cluster pattern identifies a group of observations with similar characteristics.

Clustering requires explicit:

```text
distance/similarity definition
feature representation
algorithm
configuration
```

The algorithm version may affect the resulting pattern.

---

# 36. Composite Patterns

A Pattern may be defined from other Patterns.

For example:

```text
Trend
+
Volatility Change
+
Volume Increase
    ↓
Composite Pattern
```

The component patterns become evidence for the composite pattern.

---

# 37. Pattern Relationships

Patterns may have explicit relationships.

Examples:

```text
PRECEDES
FOLLOWS
OVERLAPS
CONTAINS
CONTAINED_BY
REINFORCES
CONTRADICTS
```

Relationships are analytical structures and must not be confused with causality.

---

# 38. Pattern Overlap

Multiple patterns may coexist over the same time range.

For example:

```text
Trend
VolatilityChange
Anomaly
```

may all be active simultaneously.

The system must not assume that one pattern invalidates another.

---

# 39. Pattern Boundaries

A detector should define how pattern start and end boundaries are determined.

For example:

```text
first condition satisfying threshold
last condition satisfying threshold
confirmed turning point
window boundary
```

Boundary semantics affect reproducibility.

---

# 40. Pattern Confirmation

Some detectors may initially detect a candidate and later confirm it.

Conceptually:

```text
CANDIDATE
    ↓
CONFIRMED
```

Candidate and confirmed detection should remain distinguishable when the distinction matters.

---

# 41. Pattern Invalidation

A previously detected pattern may later be invalidated when additional information becomes available.

This must not silently rewrite historical analytical records.

Instead, the system should represent the correction/update explicitly according to the relevant versioning and provenance contract.

---

# 42. Pattern Confidence Updates

If confidence changes as more evidence arrives, the system must define whether this represents:

```text
same evolving execution state
```

or:

```text
new Pattern version/result
```

Persistent analytical history should preserve the distinction where required.

---

# 43. Pattern Evidence Window

Evidence should identify the relevant input range.

For example:

```text
Pattern:
    Trend

Evidence:
    TimeSeriesId = X
    [10:00, 12:00)
```

This enables reproducibility and later inspection.

---

# 44. Pattern Provenance

A Pattern should be able to identify:

```text
source inputs
detector
configuration
algorithm version
execution/run
```

where required.

---

# 45. Pattern Reproducibility

Reproducing a Pattern requires sufficient information to reconstruct:

```text
Input identity/version
Pattern definition
Configuration
Detector/algorithm version
Relevant execution context
Required state
```

---

# 46. Pattern Determinism

A deterministic detector should produce equivalent results from equivalent inputs and relevant conditions.

Nondeterministic algorithms must explicitly expose the relevant source of nondeterminism.

Examples:

```text
random initialization
parallel reduction order
external model
current time
```

---

# 47. Randomized Detection

A randomized detector must receive randomness explicitly through execution context or configuration.

Hidden global randomness is prohibited.

The random seed or equivalent reproducibility information must be captured when necessary.

---

# 48. Pattern Quality

Pattern detectors should be evaluated independently from Pattern representation.

Possible evaluation measures include:

```text
precision
recall
false-positive rate
false-negative rate
detection delay
coverage
stability
```

These are evaluation measurements, not intrinsic properties of every Pattern.

---

# 49. Pattern Detection vs Pattern Evaluation

Detection asks:

> Did the detector identify a pattern?

Evaluation asks:

> How well does the detector perform?

These are separate concerns.

---

# 50. Pattern Configuration

Configuration may include:

```text
thresholds
windows
minimum duration
sampling requirements
algorithm parameters
baseline
confidence requirements
missing-data policy
```

Configuration follows the Configuration Model.

---

# 51. Pattern Context

Detection may require execution context such as:

```text
analysis mode
run identity
randomness
clock
resource limits
dependency versions
```

Relevant context should be captured for reproducibility.

---

# 52. Pattern Identity vs Detector Identity

The detector implementation and detected Pattern are separate objects.

```text
DetectorId ≠ PatternId
```

A single detector may produce many Patterns.

Different detectors may produce Patterns of the same type.

---

# 53. Pattern Type vs Algorithm

The semantic type:

```text
Trend
```

is distinct from the algorithm:

```text
LinearRegressionTrendDetector
```

Multiple algorithms may detect the same semantic pattern.

---

# 54. Pattern Algorithm Version

Algorithm changes that can alter detection results must be versioned.

For example:

```text
TrendDetector v1
TrendDetector v2
```

must remain distinguishable when reproducibility requires it.

---

# 55. Pattern Output

A detector may produce:

```text
zero patterns
one pattern
multiple patterns
candidate patterns
pattern updates
```

The output cardinality must be explicit.

---

# 56. Empty Detection Result

No detected pattern is a valid result.

It is not an error.

Conceptually:

```text
Result<vector<Pattern>>
```

may therefore represent:

```text
success + empty collection
```

rather than treating "no pattern" as failure.

---

# 57. Detection Failure

Failure is distinct from no detection.

For example:

```text
no anomaly detected
```

is not equivalent to:

```text
anomaly detector failed because input was invalid
```

---

# 58. Invalid Input

Pattern detection may reject:

```text
incompatible metric
insufficient observations
invalid configuration
invalid ordering
unsupported sampling
```

The failure follows the Error Model.

---

# 59. Insufficient Evidence

Insufficient evidence may be represented as a valid "no detection" result or an explicit status depending on the detector contract.

The distinction must be documented where it affects interpretation.

---

# 60. Missing Data

Pattern detection must define missing-data behavior.

Possible policies include:

```text
IGNORE
BREAK_PATTERN
PROPAGATE
IMPUTE
REQUIRE_COMPLETE
```

The detector must not silently treat missing observations as ordinary values.

---

# 61. Sampling Assumptions

A detector may depend on regular sampling.

If so, this requirement must be explicit.

A detector designed for:

```text
one observation per second
```

must not silently assume the same semantics for:

```text
irregular observations
```

---

# 62. Multi-Series Patterns

Patterns may involve multiple series.

Examples:

```text
correlation
divergence
relative movement
spread
co-movement
```

Series identity, alignment, and temporal compatibility must be explicit.

---

# 63. Alignment

When multiple inputs are combined, the detector must define alignment:

```text
exact timestamp
nearest
interpolated
overlap
union
intersection
```

Interpolation or extrapolation creates derived data and must remain identifiable.

---

# 64. Pattern Values

A Pattern may carry quantitative characteristics.

For example:

```text
slope
magnitude
duration
peak
distance
correlation coefficient
```

These are descriptive properties of the detected pattern.

They must not be confused with confidence.

---

# 65. Pattern Metadata

Pattern representation should remain strongly typed.

The Core should not introduce an unrestricted:

```text
map<string, any>
```

as a universal pattern-property mechanism.

Domain-specific pattern data should use explicit types or domain-owned extensions.

---

# 66. Pattern Serialization

Serialization must preserve semantic information required to reconstruct the Pattern.

At minimum, where applicable:

```text
identity
type
scope
time range
evidence
status
confidence
configuration/algorithm references
provenance
```

---

# 67. Pattern Persistence

Patterns may be:

```text
EPHEMERAL
CACHE
DERIVED
RECOVERABLE
AUTHORITATIVE
```

according to their application.

A pattern is not inherently persistent.

---

# 68. Pattern Corrections

If a persisted Pattern is found to be incorrect, the system should not silently mutate historical analytical history.

Correction semantics must follow identity, versioning, provenance, and persistence rules.

---

# 69. Pattern Processing

Pattern detection may be implemented as a Processor.

Conceptually:

```text
Processor
    input: Envelope<Input>
    state: DetectorState
    output: Envelope<Pattern>
```

The Processing Model determines execution.

The Pattern model determines semantic meaning.

---

# 70. Streaming Pattern Detection

Streaming detection may maintain active patterns:

```text
observations
    ↓
detector state
    ↓
pattern started
    ↓
pattern updated
    ↓
pattern ended
```

Streaming lifecycle semantics must remain distinct from persistent Pattern identity/version semantics.

---

# 71. Batch Pattern Detection

Batch detection may inspect a complete historical dataset:

```text
dataset
    ↓
detector
    ↓
patterns
```

The detector may have different performance characteristics but should preserve the same declared semantic contract.

---

# 72. Replay

Pattern detection may be replayed from historical Events or Measurements.

Replay should preserve:

```text
original event time
input identity
configuration
detector version
relevant execution context
```

Replay execution receives a distinct RunId.

---

# 73. Pattern and Analysis

Analysis may consume Patterns:

```text
Patterns
    ↓
Analyzer
    ↓
Analysis
```

Analysis may also bypass Patterns and operate directly on Measurements or State.

Patterns are useful analytical representations, not mandatory pipeline stages.

---

# 74. Pattern and Decision

A Pattern may become evidence for a Decision.

However:

```text
Pattern → Decision
```

is not an implicit rule.

A Policy must explicitly define how the Pattern contributes to a decision.

---

# 75. Pattern and Context

The same pattern may have different significance in different contexts.

For example:

```text
Pattern:
    high volatility

Context:
    normal market conditions
```

versus:

```text
Pattern:
    high volatility

Context:
    known scheduled event
```

Context does not itself establish causation.

---

# 76. Pattern Detection Dependencies

A detector may depend on:

```text
baseline data
historical data
statistical libraries
models
external data
```

Dependencies follow the External Dependency Model.

Their identity, version, availability, and reproducibility requirements must be explicit.

---

# 77. Resource Requirements

Pattern detection may have resource requirements such as:

```text
memory
CPU
GPU
storage
execution slots
```

These belong to the Resource Model rather than Pattern semantics.

---

# 78. Error Handling

Pattern detection uses `Result<T>` / `Error`.

Expected failures are explicit.

Programming invariant violations remain distinct from ordinary detection failures.

The detector must not silently convert processing failure into "no pattern."

---

# 79. Cancellation

Long-running pattern detection may support cooperative cancellation.

Cancellation is not equivalent to:

```text
no pattern detected
```

and must remain distinguishable.

---

# 80. Testing

Pattern detectors should test:

```text
known positive cases
known negative cases
boundary conditions
threshold boundaries
minimum evidence
missing data
unknown values
out-of-order input
duplicate input
equal timestamps
sampling changes
window boundaries
pattern start/end
overlapping patterns
multiple simultaneous patterns
algorithm determinism
configuration changes
randomness
provenance
replay
cancellation
invalid input
```

Synthetic and real-world fixtures should both be used where appropriate.

---

# 81. Pattern Detector Evaluation

Detector implementations should have independent benchmark/evaluation datasets.

Evaluation should not alter the semantic representation of Pattern.

---

# 82. Deferred Decisions

This ADR does not select:

* exact C++ Pattern API
* pattern type registry implementation
* detector registration mechanism
* statistical library
* machine-learning framework
* model representation
* approximate detection algorithms
* streaming state implementation
* pattern persistence format
* universal confidence representation
* universal feature/vector representation
* GPU execution
* distributed detection mechanism

---

# Decision Summary

```text
Pattern:
    Recognized structure or behavior

Pattern Type:
    Semantic category of structure

Detector:
    Mechanism that recognizes patterns

Evidence:
    Observations supporting detection

Confidence:
    Detector-defined confidence semantics

Strength:
    Magnitude/degree of detected structure

Time Range:
    Where the pattern occurs

Scope:
    What the pattern applies to

Provenance:
    How the pattern was produced

Configuration:
    Detection parameters

Algorithm Version:
    Version of detection semantics

Status:
    Candidate/active/completed semantics where applicable

Relationships:
    Explicit relationships between patterns

Domain Meaning:
    Outside generic Core semantics

Decision:
    Separate policy-driven layer
```

## Invariant

**A Pattern represents explicitly detected structure in data, with its type, scope, temporal boundaries, evidence, detection semantics, and provenance distinguishable from the underlying measurements, the detector that produced it, the analysis that interprets it, and any decision made from it.**

