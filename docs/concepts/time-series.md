# EVolution Time Series Model

## 1. Purpose

A **Time Series** is an ordered sequence of observations associated with time.

In EVolution, a time series is a structural representation used to organize measurements and other time-dependent values.

The conceptual relationship is:

```text
Events
   ↓
State
   ↓
Measurements
   ↓
Time Series
```

A time series does not define what the measurements mean.

It defines how observations are organized along a temporal axis.

---

## 2. Time Series Is Not a Domain Object

A time series is a generic analytical structure.

For example, the following can all be time series:

```text
bankroll
profit
win_rate
latency
packet_rate
CPU_usage
temperature
price
```

The time-series layer must not assume that the underlying values represent money, poker, network traffic, or financial markets.

---

## 3. Observation

A time series consists of observations.

Conceptually:

```text
Observation
{
    timestamp
    value
}
```

For example:

```text
10:00 → 120
10:01 → 125
10:02 → 119
10:03 → 131
```

An observation may contain additional metadata when required.

The exact representation is intentionally undecided.

---

## 4. Ordering

Time series observations are ordered.

The primary ordering dimension is time.

For observations:

```text
O1.timestamp < O2.timestamp
```

then:

```text
O1 → O2
```

However, timestamps alone may not always provide a total ordering.

Two observations may have identical timestamps.

Therefore the time-series model must be compatible with an additional ordering mechanism when deterministic ordering is required.

Possible mechanisms include:

```text
sequence
event order
source sequence
ingestion order
```

The exact mechanism is left undecided.

---

## 5. Event Time vs Observation Time

EVolution must distinguish between different notions of time where necessary.

For example:

```text
event time
ingestion time
processing time
measurement time
```

An event may have occurred at:

```text
T1
```

but entered EVolution at:

```text
T2
```

and been processed at:

```text
T3
```

These timestamps have different meanings.

The time-series layer must not silently replace event time with processing time.

The relevant temporal semantics must be explicit.

---

## 6. Sampling

A time series may be:

### Regularly sampled

```text
10:00
10:01
10:02
10:03
```

### Irregularly sampled

```text
10:00:01
10:00:07
10:00:42
10:03:18
```

EVolution must support both.

The time-series model must therefore not assume a fixed interval.

---

## 7. Observation Frequency

Frequency describes how often observations occur.

Examples:

```text
per hand
per second
per minute
per 100 hands
per session
per day
```

Frequency is not necessarily intrinsic to the underlying data.

It may be produced by aggregation.

For example:

```text
individual hand results
        ↓
aggregate every 100 hands
        ↓
100-hand observations
```

---

## 8. Time Windows

A time window defines the range of observations considered.

Examples:

```text
[T0, T1]
last 100 hands
last 1 hour
current session
current day
```

Time windows are fundamental to temporal analysis.

Many measurements have different meanings depending on their window.

For example:

```text
win_rate over 100 hands
```

is different from:

```text
win_rate over 100,000 hands
```

---

## 9. Observation Windows vs Time Windows

Not every window is necessarily based on wall-clock time.

For poker, for example:

```text
last 100 hands
```

is an observation-count window rather than a clock-time window.

Therefore EVolution should conceptually distinguish:

```text
Temporal Window
    based on time

Observation Window
    based on number/order of observations

Domain Window
    based on domain-specific boundaries
```

This distinction allows the same analytical machinery to operate on domains where clock time is not the primary axis.

---

## 10. Series Identity

A time series must be identifiable.

Conceptually:

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

For example:

```text
id         = player-123.win_rate
metric     = win_rate
scope      = player-123
stake      = 10NL
game       = NLHE
```

The exact identity scheme is undecided.

---

## 11. Series Dimensions

Multiple series may represent the same metric under different dimensions.

For example:

```text
win_rate
    player=A
    stake=10NL

win_rate
    player=A
    stake=25NL

win_rate
    player=B
    stake=10NL
```

These are distinct series.

Dimensions therefore participate in series identity.

---

## 12. Missing Observations

A time series may contain gaps.

For example:

```text
10:00 → 120
10:01 → 124
10:02 → missing
10:03 → 130
```

A missing observation must not automatically be interpreted as:

```text
value = 0
```

Missing data has its own semantics.

The system must distinguish, where necessary:

```text
zero
missing
unknown
not applicable
not measured
```

---

## 13. Duplicate Observations

A source may produce multiple observations for the same timestamp.

For example:

```text
10:00 → 120
10:00 → 121
```

The system must not silently discard or overwrite one of them.

Possible interpretations include:

```text
multiple valid observations
duplicate data
correction
different dimensions
different sources
```

Resolution of duplicates belongs to the relevant ingestion or analytical policy.

---

## 14. Corrections

Historical observations may occasionally need correction.

The system should distinguish:

```text
original observation
corrected observation
```

rather than silently modifying historical information.

For example:

```text
Original:
10:03 → 130

Correction:
10:03 → 128
```

The mechanism for representing corrections is a separate architectural decision.

The important invariant is that historical analytical results should remain traceable to their inputs.

---

## 15. Immutable vs Mutable Series

A time series may be represented as an append-only sequence:

```text
O1 → O2 → O3 → O4
```

or as a mutable materialized representation.

The conceptual model must not depend on storage mutability.

The underlying historical observations should retain their provenance even when a materialized series is updated.

---

## 16. Derived Series

A time series may be derived from another time series.

For example:

```text
Bankroll Series
      │
      ├──→ Profit Series
      ├──→ Drawdown Series
      ├──→ Moving Average
      └──→ Volatility Series
```

Derived series must retain information about their source and calculation where provenance matters.

---

## 17. Transformations

A time series may be transformed.

Examples:

```text
resampling
filtering
smoothing
normalization
differencing
cumulative sum
rolling calculation
aggregation
```

For example:

```text
Raw observations
      ↓
100-hand aggregation
      ↓
100-hand series
      ↓
20-point moving average
```

Each transformation creates a new analytical representation rather than changing the meaning of the original observations.

---

## 18. Resampling

Resampling changes the observation frequency.

Example:

```text
1-minute series
      ↓
5-minute series
```

Or:

```text
individual poker hands
      ↓
100-hand observations
```

Resampling must explicitly define how values are combined.

For example:

```text
sum
average
minimum
maximum
first
last
OHLC
count
```

The correct aggregation depends on the metric.

---

## 19. Cumulative Series

Some measurements naturally form cumulative series.

For example:

```text
Hand     Profit     Cumulative
1        +4 BB       +4 BB
2        -2 BB       +2 BB
3        +8 BB      +10 BB
4        -3 BB       +7 BB
```

The cumulative representation is derived from the underlying observations.

It is therefore not a replacement for the original hand-level measurements.

---

## 20. Rate Series

Rates require special care because their denominator is part of their meaning.

For example:

```text
profit = +40 BB
hands  = 1000

profit rate = +4 BB/100
```

A rate series must preserve the relevant denominator or calculation definition.

Simply averaging rates can produce incorrect results when the underlying sample sizes differ.

For example:

```text
Session A:
100 hands → +10 BB/100

Session B:
1000 hands → -2 BB/100
```

The combined rate cannot necessarily be calculated as:

```text
(10 + -2) / 2
```

The aggregation must operate on the underlying quantities when appropriate.

---

## 21. Time Series and Candles

Candles are a specialized transformation of an underlying ordered series.

For a value series:

```text
120
125
119
131
127
```

an aggregation window may produce:

```text
Open  = 120
High  = 131
Low   = 119
Close = 127
```

Conceptually:

```text
Value Series
      ↓
Window
      ↓
OHLC Aggregation
      ↓
Candle
```

A candle therefore belongs to the analytical representation layer.

It is not a primitive EVolution data type.

---

## 22. Volume

Some time-series representations contain a volume-like dimension.

For example:

```text
Poker:
    hands played

Network:
    packets

Trading:
    traded quantity

Events:
    event count
```

Volume is not universally meaningful.

Therefore the core must not require every time series to contain volume.

Where present, volume should be an explicitly defined measurement.

---

## 23. Multi-Series Analysis

Multiple series may share a common temporal axis.

For example:

```text
Time
 │
 ├── bankroll
 ├── profit
 ├── EV
 ├── hands
 └── win_rate
```

This allows analysis of relationships between measurements.

For example:

```text
Bankroll ↓
EV      →
```

may represent a different situation from:

```text
Bankroll ↓
EV      ↓
```

The time-series layer provides the alignment structure.

Interpretation belongs to higher layers.

---

## 24. Alignment

Two series may not have identical observation timestamps.

For example:

```text
Series A:
10:00
10:01
10:02

Series B:
10:00
10:02
10:04
```

Analysis may require alignment.

Possible strategies include:

```text
exact timestamp matching
nearest observation
forward fill
interpolation
window-based alignment
event-based alignment
```

No single strategy is universally correct.

The alignment policy must therefore be explicit.

---

## 25. Temporal Aggregation

A time series can be aggregated into larger windows.

For example:

```text
Hand Results
      ↓
Session
      ↓
Day
      ↓
Week
```

Each level can preserve different information.

For example, daily aggregation may produce:

```text
Open
High
Low
Close
Volume
```

while a session-level representation may contain additional domain-specific measurements.

---

## 26. Series Resolution

EVolution should distinguish between:

```text
source resolution
```

and:

```text
analytical resolution
```

For example:

```text
Source:
    every poker hand

Analysis:
    every 100 hands
```

The higher-level series is derived from the lower-resolution data.

The system should avoid destroying source resolution merely because a particular analysis does not require it.

---

## 27. Time Series as an Analytical Boundary

The time-series layer sits between raw quantitative measurements and temporal analysis.

Conceptually:

```text
Events
   ↓
State
   ↓
Measurements
   ↓
Time Series
   ↓
Temporal Analysis
```

The time-series layer provides:

* ordering
* windows
* alignment
* sampling
* resampling
* temporal transformations
* derived series

It does not determine whether a temporal pattern is meaningful.

---

## 28. Conceptual Contract

The initial conceptual model is:

```text
TimeSeries
{
    id
    metric
    scope
    dimensions
    axis
    observations
}
```

Where an observation is conceptually:

```text
Observation
{
    timestamp
    value
    metadata
}
```

And a window is conceptually:

```text
Window
{
    start
    end
    basis
}
```

These are conceptual models only.

The following remain intentionally undecided:

* storage format
* in-memory representation
* indexing
* timestamp precision
* timezone handling
* interpolation
* alignment algorithms
* resampling implementation
* streaming vs batch processing
* database requirements
* query language

---

## 29. Core Invariant

The EVolution core understands **temporal organization of observations**, but does not assume what temporal behavior means.

Therefore:

```text
Core:
    TimeSeries
    Observation
    Window
    Alignment
    Transformation

Domain:
    defines metric semantics
    defines meaningful windows
    defines valid transformations
    defines interpretation
```

The resulting architecture is now:

```text
Events
   ↓
State
   ↓
Measurements
   ↓
Time Series
   ↓
Patterns
   ↓
Analysis
```

A visualization such as a line chart, candlestick chart, heatmap, or dashboard consumes these representations but does not define them.
