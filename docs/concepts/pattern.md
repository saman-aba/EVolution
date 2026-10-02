# EVolution Pattern Model

## 1. Purpose

A **Pattern** is a recognized structure, relationship, behavior, or recurrence in observations, measurements, states, or time series.

A pattern describes **what structure exists in the data**.

It does not necessarily explain why the structure exists.

The conceptual relationship is:

```text id="p1"
Measurements / Time Series
          ↓
       Pattern
```

Examples:

```text
increasing trend
decreasing trend
repeated cycle
sudden change
volatility increase
persistent deviation
correlation
regime change
drawdown
cluster
anomaly
```

---

## 2. Pattern vs Measurement

A measurement gives a value.

A pattern describes a relationship among values.

For example:

```text id="p2"
Measurement:
    win_rate = 2.1 BB/100
```

versus:

```text id="p3"
Pattern:
    win_rate has declined continuously
    over the last 20 sessions
```

The first is a value.

The second describes structure across multiple observations.

---

## 3. Pattern vs Aggregation

Aggregation combines observations.

Pattern identifies structure in observations.

For example:

```text id="p4"
1000 hands
    ↓
average win rate = 3.2 BB/100
```

is aggregation.

Whereas:

```text id="p5"
win rate has remained below
the long-term average for
the last 5000 hands
```

is a pattern.

Aggregation produces quantitative summaries.

Pattern detection produces structural descriptions.

---

## 4. Pattern vs Analysis

A pattern describes observed structure.

Analysis interprets that structure in a particular context.

For example:

```text id="p6"
Pattern:
    bankroll declined 18%
    while EV remained approximately stable
```

This is an observation about relationships in the data.

An analysis may subsequently state:

```text id="p7"
The observed decline is consistent with
an increased variance regime.
```

The second statement is an interpretation.

EVolution must keep these concepts separate.

---

## 5. Pattern Identity

A pattern should have an identity.

Conceptually:

```text id="p8"
Pattern
{
    type
    scope
    time_range
    evidence
    confidence
    provenance
}
```

For example:

```text id="p9"
type       = "downtrend"
scope      = "player-123"
time_range = session-120 → session-140
```

The exact representation remains undecided.

---

## 6. Pattern Type

A pattern type defines the structural property being detected.

Examples:

```text id="p10"
Trend
Reversal
Breakout
Drawdown
VolatilityChange
LevelShift
Cycle
Cluster
Anomaly
Correlation
Divergence
Persistence
RegimeChange
```

Pattern types may be generic or domain-specific.

The core should provide generic mechanisms while domains may define specialized pattern semantics.

---

## 7. Pattern Evidence

A pattern must be supported by observations.

Conceptually:

```text id="p11"
Pattern
   │
   └── Evidence
          │
          ├── observation range
          ├── measurements
          ├── thresholds
          └── detection parameters
```

For example:

```text id="p12"
Pattern:
    sustained decline

Evidence:
    18 consecutive observations
    slope < threshold
    minimum sample size = 10
```

The pattern should not exist as an unexplained label.

---

## 8. Pattern Confidence

Some pattern detection methods produce uncertainty.

For example:

```text id="p13"
Pattern:
    possible trend

Confidence:
    0.73
```

Confidence is not universally required.

A deterministic rule may simply produce:

```text id="p14"
Pattern:
    drawdown detected
```

Therefore confidence must be optional and its semantics must be explicitly defined.

A numerical confidence value must not automatically be interpreted as a probability unless the detection method establishes that meaning.

---

## 9. Pattern Strength

Pattern strength describes how strongly the observed data satisfies the pattern definition.

This is different from confidence.

For example:

```text id="p15"
strength = 0.82
confidence = 0.91
```

could theoretically mean:

```text
strength:
    degree to which observations exhibit the pattern

confidence:
    certainty of the detector's conclusion
```

These concepts must not be conflated.

Not every pattern detector needs either value.

---

## 10. Pattern Detection

A **Pattern Detector** identifies patterns from input data.

Conceptually:

```text id="p16"
Input Data
    ↓
Pattern Detector
    ↓
Pattern
```

A detector may consume:

```text id="p17"
events
state
measurements
time series
other patterns
```

This allows detectors to operate at different abstraction levels.

---

## 11. Deterministic Pattern Detection

A deterministic detector produces the same result for the same inputs and configuration.

```text id="p18"
Input
+
Detector Configuration
+
Version
        ↓
      Pattern
```

This is important for:

* reproducibility
* testing
* debugging
* historical analysis
* comparing detector versions

---

## 12. Statistical Pattern Detection

Some patterns require statistical inference.

Examples:

```text id="p19"
trend significance
change-point detection
correlation
anomaly probability
distribution shift
```

These patterns may contain statistical information such as:

```text id="p20"
sample_size
p_value
confidence_interval
effect_size
```

Such values must retain their statistical semantics.

A statistical result must not be reduced to an arbitrary "confidence" score without preserving what that score actually represents.

---

## 13. Threshold Patterns

The simplest pattern detectors use explicit thresholds.

For example:

```text id="p21"
IF
    drawdown > 20 BB
THEN
    DrawdownPattern
```

Another:

```text id="p22"
IF
    latency > 100 ms
    for 10 consecutive observations
THEN
    SustainedLatencyPattern
```

Thresholds must be part of detector configuration rather than hidden inside unrelated code.

---

## 14. Trend Patterns

A trend describes directional behavior over an interval.

Examples:

```text id="p23"
uptrend
downtrend
flat
accelerating
decelerating
```

A trend detector may use:

```text id="p24"
slope
moving averages
regression
monotonicity
relative change
```

A trend is not simply a positive or negative value.

It describes behavior across multiple observations.

---

## 15. Persistence Patterns

A persistence pattern describes a condition that remains present over an interval.

For example:

```text id="p25"
win_rate < 0
for 5000 hands
```

The important property is duration or observation count.

Persistence can be expressed using:

```text id="p26"
time duration
observation count
consecutive occurrences
percentage of observations
```

---

## 16. Change Patterns

A change pattern describes a significant alteration in behavior.

Examples:

```text id="p27"
level shift
variance increase
variance decrease
distribution shift
sudden jump
sudden drop
```

For example:

```text id="p28"
Before:
    average latency ≈ 20 ms

After:
    average latency ≈ 80 ms
```

The pattern identifies the change.

It does not determine its cause.

---

## 17. Reversal Patterns

A reversal describes a change in direction.

For example:

```text id="p29"
increasing
    ↓
peak
    ↓
decreasing
```

or:

```text id="p30"
decreasing
    ↓
trough
    ↓
increasing
```

Reversal detection generally requires an ordered time series.

---

## 18. Divergence Patterns

A divergence describes two related series behaving differently.

For example:

```text id="p31"
Bankroll:
    ↓

EV:
    →
```

Or:

```text id="p32"
Metric A:
    ↑

Metric B:
    ↓
```

A divergence does not automatically imply causation.

It only identifies a relationship between observed trajectories.

---

## 19. Correlation Patterns

A correlation pattern describes statistical association between variables.

For example:

```text id="p33"
session_duration
        ↕
loss_per_session
```

A correlation pattern must not be interpreted as causal evidence.

The pattern means:

```text id="p34"
the variables exhibit a measured statistical relationship
```

not:

```text id="p35"
one variable causes the other
```

Causal reasoning belongs to a higher analytical layer.

---

## 20. Anomaly Patterns

An anomaly is an observation or sequence of observations that differs materially from an expected or established baseline.

Conceptually:

```text id="p36"
Normal:
    10
    11
    9
    10
    12

Observed:
    87
```

The anomaly detector may use:

```text id="p37"
statistical deviation
historical baseline
local neighborhood
model prediction
domain rules
```

Anomaly does not necessarily mean error.

It means the observation is unusual according to the specified detection model.

---

## 21. Regime Patterns

A **Regime** describes a period during which the statistical or behavioral characteristics of a system are relatively stable.

For example:

```text id="p38"
Regime A
──────────────
low volatility

Regime B
──────────────
high volatility
```

A regime change occurs when the characteristics of the data shift sufficiently to classify the system differently.

Regimes are therefore higher-level temporal patterns.

---

## 22. Composite Patterns

Patterns may be constructed from other patterns.

For example:

```text id="p39"
Downtrend
    +
Increasing Volatility
    +
Negative Performance
        ↓
Composite Pattern
```

A composite pattern does not necessarily require raw observations.

It may consume already detected patterns.

This allows hierarchical analysis:

```text id="p40"
Measurements
      ↓
Basic Patterns
      ↓
Composite Patterns
      ↓
Higher-Level Analysis
```

---

## 23. Pattern Lifecycle

A pattern may have a lifecycle.

Conceptually:

```text id="p41"
NOT_DETECTED
     ↓
DETECTED
     ↓
ACTIVE
     ↓
ENDED
```

For example:

```text id="p42"
Downtrend begins
       ↓
Downtrend continues
       ↓
Downtrend ends
```

Some patterns are instantaneous:

```text id="p43"
Anomaly detected at T
```

while others persist over intervals.

The pattern model must support both.

---

## 24. Pattern Boundaries

A pattern may have:

```text id="p44"
start
end
```

For example:

```text id="p45"
Downtrend:
    start = session 120
    end   = session 137
```

Some patterns may not have a known end yet:

```text id="p46"
Downtrend:
    start = session 120
    end   = ongoing
```

The distinction between an active and completed pattern must be explicit.

---

## 25. Pattern Overlap

Multiple patterns may exist simultaneously.

For example:

```text id="p47"
Time ──────────────────────────>

Downtrend
     ├─────────────────────┤

High Volatility
          ├───────────────┤

Anomaly
                  ├───┤
```

Patterns must therefore not require exclusive ownership of a time interval.

A single observation may contribute evidence to multiple patterns.

---

## 26. Pattern Relationships

Patterns may have relationships.

Examples:

```text id="p48"
Pattern A precedes Pattern B

Pattern A overlaps Pattern B

Pattern A contains Pattern B

Pattern A contradicts Pattern B

Pattern A reinforces Pattern B
```

For example:

```text id="p49"
HighVolatility
      ↓
      precedes
      ↓
LargeDrawdown
```

This relationship itself is an analytical fact and may later be used to construct higher-level models.

---

## 27. Pattern Detection vs Pattern Storage

A detector produces a pattern.

A pattern repository or stream stores detected patterns.

Conceptually:

```text id="p50"
Data
 ↓
Detector
 ↓
Pattern
 ↓
Pattern Stream / Store
```

Detection does not imply persistence.

Some patterns may be transient and only needed during analysis.

Others may need to be persisted for historical analysis.

---

## 28. Pattern Provenance

Every pattern should be traceable to the information and detector that produced it.

Conceptually:

```text id="p51"
Pattern
{
    type
    scope
    time_range

    evidence
    detector
    detector_version
    configuration

    confidence
    strength
}
```

This is particularly important when pattern definitions evolve.

A historical result produced by detector version `1.0` should not silently appear identical to a result produced by version `2.0`.

---

## 29. Pattern Versioning

Pattern definitions may evolve.

For example:

```text id="p52"
Downtrend v1:
    slope < -0.1

Downtrend v2:
    slope < -0.1
    AND
    minimum duration >= 10 observations
```

These are different detection definitions.

The detector version must therefore be part of provenance where reproducibility matters.

---

## 30. Pattern Quality

A pattern detector may produce false positives or false negatives.

EVolution should therefore allow detector quality to be evaluated separately from the pattern itself.

For example:

```text id="p53"
Pattern:
    anomaly detected

Detector evaluation:
    precision
    recall
    false-positive rate
```

These are measurements about the detector.

They are not properties of the underlying phenomenon itself.

---

## 31. Pattern Does Not Imply Causation

This is a fundamental invariant.

If EVolution detects:

```text id="p54"
longer sessions correlate with larger losses
```

the system must not automatically conclude:

```text id="p55"
long sessions cause larger losses
```

The first is a detected relationship.

The second is a causal hypothesis.

Causal analysis belongs to a separate higher-level analytical layer.

---

## 32. Pattern Does Not Imply Importance

A pattern can be statistically detectable without being practically important.

For example:

```text id="p56"
metric increased by 0.01%
```

may satisfy a detector but have negligible practical impact.

Therefore:

```text id="p57"
Pattern detection ≠ importance assessment
```

Importance belongs to the analysis layer.

---

## 33. Pattern Does Not Imply Action

Similarly:

```text id="p58"
Pattern:
    volatility increased
```

does not imply:

```text id="p59"
Action:
    stop
```

or:

```text id="p60"
Action:
    change strategy
```

Action generation is a separate concern.

---

## 34. Pattern Streams

Patterns may themselves form streams.

For example:

```text id="p61"
Time
 │
 ├── Downtrend
 ├── VolatilityIncrease
 ├── Anomaly
 ├── Reversal
 └── Downtrend
```

This allows EVolution to analyze patterns at a higher abstraction level.

For example:

```text id="p62"
Pattern Stream
      ↓
Composite Pattern
      ↓
Regime
```

---

## 35. Pattern Hierarchy

Patterns can exist at different abstraction levels.

```text id="p63"
Level 1:
    primitive patterns

Level 2:
    composite patterns

Level 3:
    regimes

Level 4:
    behavioral / domain patterns
```

For example:

```text id="p64"
Negative Results
       +
Increasing Session Duration
       +
Increasing Decision Errors
       ↓
Possible Performance Deterioration Pattern
```

The final interpretation remains outside the primitive pattern detector.

---

## 36. Conceptual Contract

The initial conceptual model is:

```text id="p65"
Pattern
{
    type
    scope
    time_range

    evidence
    provenance

    confidence?
    strength?
    status
}
```

A detector is conceptually:

```text id="p66"
PatternDetector
{
    input
    configuration
    detect()
}
```

These are conceptual models only.

The following remain intentionally undecided:

* detector API
* rule engine
* statistical libraries
* machine-learning integration
* pattern persistence
* detector scheduling
* online vs batch detection
* detector composition
* confidence representation
* pattern ontology
* pattern query language

---

## 37. Core Invariant

The EVolution core may provide mechanisms for representing and processing patterns, but it must not assume the domain-specific meaning of those patterns.

The fundamental distinction is:

```text id="p67"
Measurement:
    what value was observed?

Aggregation:
    how were observations combined?

Pattern:
    what structure exists?

Analysis:
    what does that structure mean?
```

---

## 38. Current Conceptual Pipeline

The EVolution model now becomes:

```text id="p68"
Events
   ↓
State
   ↓
Measurements
   ↓
Time Series
   ↓
Aggregation
   ↓
Patterns
   ↓
Analysis
```

Each layer raises the abstraction level while preserving the ability to trace the result back toward the underlying observations.
