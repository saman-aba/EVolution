# EVolution Aggregation Model

## 1. Purpose

An **Aggregation** combines multiple observations, measurements, events, or states into a higher-level representation.

Conceptually:

```text id="7p3w4k"
Multiple Inputs
      ↓
  Aggregation
      ↓
 Single Result
```

Examples include:

```text
sum
count
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

Aggregation is one of the primary mechanisms through which EVolution changes analytical resolution.

---

## 2. Aggregation vs Measurement

A Measurement is a derived quantitative observation.

An Aggregation is a method for combining multiple inputs.

For example:

```text id="xj4r1k"
Inputs:
    +4 BB
    -2 BB
    +8 BB
    +3 BB

Aggregation:
    SUM

Result:
    +13 BB
```

The result is a measurement.

The aggregation describes how that measurement was produced.

Therefore:

```text id="j5v8q2"
Aggregation ≠ Measurement
```

---

## 3. Aggregation Inputs

An aggregation may consume different types of input.

### Events

```text id="8r5h9m"
Events
   ↓
count / duration / classification
```

### Measurements

```text id="1m6z8p"
Measurements
   ↓
sum / average / percentile
```

### Time-Series Observations

```text id="7k2q0s"
Time Series
   ↓
window aggregation
```

### State

State may be sampled or transformed into aggregate measurements, but aggregation should not mutate the state itself.

---

## 4. Aggregation Window

Aggregation requires a set of inputs over which the operation is performed.

This is the **Aggregation Window**.

Examples:

```text id="w9g2x1"
100 hands
1 minute
1 hour
1 session
1 day
2026-10-01 → 2026-10-02
```

The aggregation result is meaningless without knowing which inputs were included.

Conceptually:

```text id="3p5m7n"
Input Stream
───────────────────────────────
       │
       ├── Aggregation Window ──┤
       │                        │
       ▼                        ▼
     inputs                  result
```

---

## 5. Window Basis

An aggregation window may be based on different dimensions.

### Time-based

```text id="8c4j6x"
[10:00, 11:00)
```

### Observation-based

```text id="9v1m3a"
100 observations
```

### Event-based

```text id="f2k7q8"
all events belonging to one session
```

### Domain-based

```text id="t6r4w2"
one poker hand
one tournament
one network connection
```

The aggregation mechanism should not assume that time is always the primary boundary.

---

## 6. Aggregation Function

An aggregation function defines how inputs are combined.

Conceptually:

```text id="z7q3n1"
AggregationFunction(inputs) → result
```

Examples:

```text id="m8p2r4"
SUM
COUNT
MIN
MAX
FIRST
LAST
AVERAGE
MEDIAN
PERCENTILE
VARIANCE
STANDARD_DEVIATION
```

Different functions have different mathematical properties and therefore different streaming and parallelization characteristics.

---

## 7. Associativity

Some aggregation functions are associative.

For example:

```text id="5q1m8r"
SUM(a, b, c)
=
SUM(SUM(a, b), c)
```

This allows aggregation to be performed incrementally or in parallel.

For example:

```text id="4k9x2p"
Inputs
 ├── Partition A → sumA
 ├── Partition B → sumB
 └── Partition C → sumC
                  │
                  ▼
             sumA + sumB + sumC
```

Not every aggregation has this property.

The aggregation definition must therefore expose its combination semantics where required.

---

## 8. Incremental Aggregation

An aggregation may maintain an intermediate state.

For example, average can be represented by:

```text id="3h7w2c"
count
sum
```

Then:

```text id="9n4m6v"
average = sum / count
```

This allows:

```text id="v6k2p8"
new input
    ↓
update aggregation state
    ↓
new result
```

The aggregation state is an implementation detail unless it is explicitly exposed as part of the analytical model.

---

## 9. Mergeable Aggregations

Some aggregations can merge partial results.

For example:

```text id="q3r7m9"
Partition A:
    count = 100
    sum = 500

Partition B:
    count = 200
    sum = 900
```

can produce:

```text id="n8p4x2"
count = 300
sum = 1400
average = 4.666...
```

This property is important for:

* parallel processing
* distributed processing
* incremental updates
* historical recomputation

Whether a particular aggregation supports merging is part of its definition.

---

## 10. Aggregation Output

An aggregation does not necessarily produce a single scalar.

For example:

```text id="b6q1t9"
OHLC
```

produces:

```text id="k3m8v5"
open
high
low
close
```

A statistical aggregation may produce:

```text id="r2x7c4"
mean
variance
sample_count
```

Therefore:

```text id="j8p5n2"
Aggregation → Result
```

where `Result` may be scalar or structured.

---

## 11. Count

Count measures the number of inputs.

Example:

```text id="4v8m1q"
100 poker hands
```

produces:

```text id="s6r2k9"
count = 100
```

Count is one of the simplest aggregations but is fundamental to many other measurements.

---

## 12. Sum

Sum combines additive values.

Example:

```text id="p8n3v6"
+4
-2
+8
+3
```

produces:

```text id="c1m7x5"
sum = 13
```

Sum is only semantically valid when the input values are additive.

For example, adding:

```text id="r9w4k2"
latency = 20 ms
latency = 30 ms
```

may produce `50 ms`, but that does not necessarily represent a useful latency measurement.

The aggregation function must therefore have semantic compatibility with the metric.

---

## 13. Average

Average combines values into their arithmetic mean.

```text id="q5m2v8"
average = sum(values) / count(values)
```

However, averages must be used carefully when inputs have different weights.

For example:

```text id="a8k4p1"
Session A:
100 hands
+10 BB/100

Session B:
1000 hands
-2 BB/100
```

The overall win rate should be derived from the underlying total profit and total hands rather than blindly averaging the two rates.

Therefore EVolution should distinguish:

```text id="j7r3n9"
average of measurements
```

from:

```text id="c5x8q2"
weighted / ratio-derived measurement
```

---

## 14. Minimum and Maximum

Minimum and maximum select boundary values.

For example:

```text id="h4n9p2"
values:
120
125
119
131
127

min = 119
max = 131
```

These operations are fundamental for:

* range calculations
* drawdowns
* volatility-related analysis
* OHLC construction
* anomaly detection

---

## 15. First and Last

`FIRST` and `LAST` select observations based on the aggregation order.

For example:

```text id="x2m7q5"
120
125
119
131
```

produces:

```text id="n8c4r1"
FIRST = 120
LAST  = 131
```

These operations are important for constructing state transitions and OHLC representations.

Their ordering semantics must therefore be explicit.

---

## 16. OHLC Aggregation

OHLC is a specialized aggregation over an ordered value series.

Given:

```text id="m7p2x9"
120
125
119
131
127
```

the result is:

```text id="q4n8c3"
Open  = 120
High  = 131
Low   = 119
Close = 127
```

Conceptually:

```text id="y3r6k1"
FIRST → Open
MAX   → High
MIN   → Low
LAST  → Close
```

A candle may additionally contain volume or other measurements.

OHLC therefore does not require a special primitive data source.

It is an aggregation of an ordered series.

---

## 17. Rolling Aggregation

A **Rolling Aggregation** continuously applies an aggregation over a moving window.

For example:

```text id="p6w3m8"
Values:
1 2 3 4 5 6 7

Window = 3
```

produces:

```text id="z2k9q4"
1 2 3 → 2
2 3 4 → 3
3 4 5 → 4
4 5 6 → 5
5 6 7 → 6
```

Rolling aggregations are important for:

* moving averages
* rolling volatility
* rolling win rate
* rolling drawdown
* trend analysis

---

## 18. Expanding Aggregation

An **Expanding Aggregation** grows from the beginning of the selected range.

For example:

```text id="h7m4x1"
Values:
1 2 3 4
```

with cumulative sum:

```text id="c5n8q3"
1
3
6
10
```

This is useful for cumulative measurements such as:

```text id="w2p9r6"
cumulative profit
cumulative volume
cumulative events
```

---

## 19. Grouped Aggregation

An aggregation may group observations by one or more dimensions.

Example:

```text id="g8x3m5"
Hands
 ├── NLHE
 ├── NLHE
 ├── PLO
 ├── NLHE
 └── PLO
```

Grouping by game produces:

```text id="q1r7k4"
NLHE → aggregate
PLO  → aggregate
```

Grouping may be based on:

```text id="m6v2p8"
player
stake
table
game
session
day
source
event type
```

This allows one input stream to produce many related aggregate series.

---

## 20. Hierarchical Aggregation

Aggregations may be performed at multiple levels.

For example:

```text id="f4n8q2"
Hands
   ↓
Sessions
   ↓
Days
   ↓
Weeks
   ↓
Months
```

Each level is a derived representation.

Higher-level aggregation should preserve sufficient provenance to explain how the result was produced.

---

## 21. Aggregation of Ratios

Ratios require special treatment.

For:

```text id="x7m3p9"
rate = numerator / denominator
```

the correct aggregate is generally:

```text id="b2q8n5"
total_numerator / total_denominator
```

rather than:

```text id="k6r1v4"
average(individual_rates)
```

unless the individual rates are equally weighted by definition.

This rule is important for metrics such as:

```text id="t9w3m7"
BB/100
success rate
packet rate
requests/second
error rate
```

The aggregation definition must specify the correct mathematical behavior.

---

## 22. Aggregation and Missing Data

Missing values must have explicit semantics.

For example:

```text id="c4p8x2"
10
missing
20
```

may produce different results depending on the policy.

Possible policies include:

```text id="q7m3n9"
ignore missing values
treat missing as zero
invalidate result
propagate missing
```

No universal policy exists.

Therefore missing-data behavior belongs to the aggregation configuration or metric definition.

---

## 23. Aggregation and Units

An aggregation must respect the units of its inputs.

Examples:

```text id="r8k2m5"
BB + BB → BB
hands + hands → hands
bytes + bytes → bytes
```

But:

```text id="y4n7q1"
BB + milliseconds
```

is invalid.

Some operations change units.

For example:

```text id="p6m2x8"
profit / hands → BB/hand
```

and:

```text id="v9r3k7"
profit / time → BB/second
```

Unit semantics therefore belong to the metric/calculation system rather than being ignored by the aggregation layer.

---

## 24. Aggregation Provenance

An aggregate should be traceable to the inputs and configuration that produced it.

Conceptually:

```text id="n5q8m2"
Aggregation Result
{
    function
    input_scope
    input_window
    configuration
    result
}
```

For example:

```text id="a3r7x9"
function = SUM
metric = profit
window = session-123
result = +42 BB
```

This is important for reproducibility and debugging.

---

## 25. Aggregation Is Not Interpretation

Aggregation describes how values are combined.

It does not explain what the result means.

For example:

```text id="w8m4p2"
maximum_drawdown = 37 BB
```

is an aggregate/measurement.

Whether:

```text id="q3n7x5"
37 BB represents unusual behavior
```

is an analytical interpretation.

That distinction belongs to the Pattern and Analysis layers.

---

## 26. Aggregation Pipeline

A typical EVolution pipeline may therefore look like:

```text id="f7r2m9"
Events
   ↓
State
   ↓
Measurements
   ↓
Time Series
   ↓
Window
   ↓
Aggregation
   ↓
Derived Measurement / Series
```

For example:

```text id="b8x4q1"
Poker Hands
    ↓
Profit per Hand
    ↓
100-hand Window
    ↓
OHLC Aggregation
    ↓
100-hand Candle Series
```

---

## 27. Conceptual Contract

The initial conceptual model is:

```text id="v3m7q9"
Aggregation
{
    function
    input
    window
    grouping
    configuration
}
```

with:

```text id="k8p2x5"
AggregationResult
{
    value
    metadata
    provenance
}
```

An aggregation function conceptually provides:

```text id="r4n9m6"
initialize()
update(input)
merge(partial)
finalize()
```

Not every implementation must expose all of these operations.

They describe the capabilities the analytical engine may eventually require.

---

## 28. Core Invariant

The EVolution core understands **how collections of observations can be reduced or transformed into higher-level representations**, but it must not assume the domain-specific meaning of the result.

Therefore:

```text id="x5q8m2"
Core:
    aggregation mechanics
    windows
    grouping
    incremental processing
    mergeability
    provenance

Domain / Metric:
    semantic validity
    units
    weighting
    mathematical definition
```

---

## 29. Current Conceptual Pipeline

The EVolution model now contains:

```text id="m2r7v9"
                 ┌───────────────┐
                 │     Events    │
                 └───────┬───────┘
                         ↓
                 ┌───────────────┐
                 │     State     │
                 └───────┬───────┘
                         ↓
                 ┌───────────────┐
                 │  Measurements │
                 └───────┬───────┘
                         ↓
                 ┌───────────────┐
                 │  Time Series  │
                 └───────┬───────┘
                         ↓
                 ┌───────────────┐
                 │  Aggregation  │
                 └───────┬───────┘
                         ↓
                 Derived Measurements
```

Aggregation is therefore a transformation mechanism rather than another permanent layer of semantic data.
