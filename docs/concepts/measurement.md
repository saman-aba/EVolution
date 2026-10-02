# EVolution Measurement Model

## 1. Purpose

A **Measurement** is a derived quantitative observation about events, state, or a combination of both.

The conceptual relationship is:

```text
Events → State → Measurements
```

However, a measurement may also be calculated directly from events when reconstructing state is unnecessary:

```text
Events ─────────────→ Measurement
```

A measurement answers questions such as:

* How many events occurred?
* How long did something take?
* How much did a value change?
* What was the average?
* What was the maximum or minimum?
* How frequently did something occur?
* How volatile was a value?

A measurement does not, by itself, explain why something happened.

---

## 2. Measurement vs State

State describes a condition.

Measurement quantifies a property.

For example:

```text
State:
    bankroll = 150 BB

Measurement:
    bankroll_change = +25 BB
```

Another example:

```text
State:
    hands_played = 1000

Measurement:
    win_rate = 4.2 BB/100
```

State may be required to calculate a measurement, but the two concepts remain separate.

---

## 3. Measurement vs Event

An event represents something that happened.

A measurement represents information calculated from one or more observations.

For example:

```text
Event:
    HandFinished
    profit = +12 BB
```

is an observation.

From a sequence of such events:

```text
+12 BB
-8 BB
+4 BB
+20 BB
```

EVolution may calculate:

```text
total_profit = +28 BB
average_profit = +7 BB
maximum_profit = +20 BB
minimum_profit = -8 BB
```

These are measurements.

They are not historical events.

---

## 4. Measurement Identity

A measurement must identify what was measured.

Conceptually:

```text
Measurement
{
    metric
    value
    unit
    scope
    time_range
}
```

For example:

```text
metric    = "bankroll"
value     = 150
unit      = "BB"
scope     = "poker.session"
time_range = [T0, T1]
```

The exact representation is intentionally undecided.

---

## 5. Metric

A **Metric** defines what a measurement means.

For example:

```text
Metric:
    name = "profit"

Measurement:
    value = +32 BB
```

The metric defines the semantic meaning.

The measurement provides a concrete value for a particular scope and time.

This distinction allows the same metric to be calculated repeatedly:

```text
Profit(Session A) = +32 BB
Profit(Session B) = -14 BB
Profit(Session C) = +8 BB
```

---

## 6. Units

Measurements must have explicit units whenever the value has meaningful dimensional semantics.

Examples:

```text
BB
USD
seconds
milliseconds
hands
events
bytes
packets
percentage
ratio
```

The unit must not be inferred solely from a metric name when ambiguity is possible.

For example:

```text
100
```

is ambiguous.

Whereas:

```text
100 BB
100 hands
100 ms
100 USD
```

has an explicit meaning.

---

## 7. Time Range

Measurements may describe a point in time or an interval.

Point measurement:

```text
bankroll_at(T)
```

Interval measurement:

```text
profit_between(T0, T1)
```

Therefore a measurement may conceptually have:

```text
time_start
time_end
```

For point measurements:

```text
time_start == time_end
```

or an equivalent point-in-time representation may be used.

The exact timestamp representation is left undecided.

---

## 8. Scope

A measurement must have a scope.

Examples:

```text
player
hand
session
table
day
week
stake
tournament
dataset
```

For example:

```text
win_rate
scope = player-123
range = session-456
```

The scope prevents identical metric names from being interpreted as globally interchangeable values.

---

## 9. Measurement Dimensions

A metric may require additional dimensions.

For example:

```text
win_rate
    player = PlayerA
    game = NLHE
    stake = 10NL
    table_type = cash
```

This allows EVolution to represent multidimensional measurements without requiring separate metric definitions for every combination.

Conceptually:

```text
Measurement
{
    metric
    value
    unit

    dimensions:
        player
        game
        stake
        ...
}
```

The set of dimensions is domain-dependent.

---

## 10. Raw vs Derived Measurements

Not every numerical value should automatically be considered a measurement.

EVolution distinguishes between:

### Direct measurement

A value directly observed from incoming data.

Example:

```text
Event:
    latency = 18 ms
```

### Derived measurement

A value calculated from other observations.

Example:

```text
average_latency = 23.4 ms
```

Both are measurements, but their provenance is different.

The system should preserve this distinction where provenance matters.

---

## 11. Aggregation

An **Aggregation** combines multiple observations or measurements.

Examples:

```text
sum
count
average
minimum
maximum
median
percentile
standard deviation
variance
```

For example:

```text
Hand Results
    │
    ├── +4 BB
    ├── -2 BB
    ├── +8 BB
    └── +3 BB
          │
          ▼
      Aggregation
          │
          ▼
      +13 BB
```

An aggregation is therefore a mechanism for producing measurements from multiple observations.

---

## 12. Measurement Windows

Measurements may be calculated over explicit windows.

Examples:

```text
last 100 hands
last 1000 hands
current session
current day
current week
2026-09-01 → 2026-09-30
```

The window is part of the semantics of the measurement.

For example:

```text
win_rate = 4.2 BB/100
window = last 1000 hands
```

is materially different from:

```text
win_rate = 4.2 BB/100
window = lifetime
```

The value alone is insufficient to fully describe the measurement.

---

## 13. Incremental Measurements

Measurements should not necessarily require processing the complete event history every time.

A measurement may be maintained incrementally.

For example:

```text
previous:
    hands = 999
    profit = +42 BB

new event:
    profit = -3 BB

updated:
    hands = 1000
    profit = +39 BB
```

This allows large streams to be processed efficiently.

However, incremental calculation must preserve the same semantic result as recalculating from the underlying data.

---

## 14. Measurement Reproducibility

A measurement should be reproducible from its inputs and calculation definition.

Conceptually:

```text
Input Data
+
Metric Definition
+
Window
+
Dimensions
+
Configuration
        │
        ▼
   Measurement
```

Changing any of these may legitimately change the result.

Therefore a measurement should not be treated as an unexplained number.

---

## 15. Measurement Provenance

A measurement may need to record how it was produced.

Conceptually:

```text
Measurement
{
    metric
    value
    unit

    source
    calculation
    time_range
    scope
}
```

For example:

```text
metric      = "win_rate"
value       = 3.8
unit        = "BB/100"

source      = "poker.hand_history"
calculation = "profit / hands * 100"

scope       = "player-123"
time_range  = "session-456"
```

The exact provenance model is not yet defined.

The important principle is that analytical results should not become opaque numbers without a traceable origin.

---

## 16. Measurement Precision

Measurements may have different precision requirements.

For example:

```text
hands = 1000
```

may be exact.

While:

```text
average_profit = 0.037482 BB
```

may be an approximation.

The system must distinguish, where necessary, between:

* exact values
* approximate values
* estimated values
* sampled values

Precision and rounding must not silently alter the semantic result.

---

## 17. Measurement Uncertainty

Some measurements represent inherently uncertain quantities.

For example:

```text
estimated_win_rate
estimated_EV
probability
confidence_interval
```

These must not be represented as ordinary deterministic values if doing so would hide important uncertainty.

For example:

```text
win_rate = 4.2 BB/100
```

may be a measured historical rate.

Whereas:

```text
estimated_true_win_rate
```

is a statistical estimate and has different semantics.

EVolution must keep these concepts distinguishable.

---

## 18. Measurement Does Not Imply Interpretation

A measurement should describe a property without automatically assigning meaning to it.

For example:

```text
Measurement:
    win_rate = -3.2 BB/100
```

does not itself mean:

```text
player is getting worse
```

Similarly:

```text
volatility = high
```

is already an interpretation unless "high" has been explicitly defined by a metric or classification system.

Interpretation belongs to higher-level analytical concepts.

---

## 19. Measurements as Time-Series Data

Measurements naturally form time series.

For example:

```text
Time       Profit
T1         +5 BB
T2         -2 BB
T3         +7 BB
T4         -4 BB
```

A sequence of measurements can therefore become:

```text
Measurement Stream
        │
        ▼
    Time Series
```

This is important because EVolution's later analytical layers will operate heavily on temporal relationships.

The time-series abstraction should therefore remain independent from any particular visualization.

---

## 20. Measurements and Candles

A candle is not a fundamental data type.

It is an aggregation of measurements over a time or observation window.

For example:

```text
Measurements
    │
    │  window
    ▼
Aggregation
    │
    ▼
OHLC representation
```

For a bankroll series:

```text
Open   = value at beginning
High   = maximum value
Low    = minimum value
Close  = value at end
```

Volume may represent:

```text
hands played
```

The candle is therefore a **derived analytical representation**, not a primitive domain object.

This distinction keeps the core useful for applications that do not use financial-style visualization.

---

## 21. Measurement Streams

Measurements may themselves form streams:

```text
Event Stream
     │
     ▼
Projection
     │
     ▼
State
     │
     ▼
Measurement Stream
```

For example:

```text
Session State
     │
     ├── bankroll measurement
     ├── profit measurement
     ├── hands measurement
     └── win-rate measurement
```

Different analysis components may consume these streams independently.

---

## 22. Measurement Definition vs Measurement Value

The system must distinguish between:

```text
Metric Definition
```

and:

```text
Measurement Instance
```

For example:

```text
Metric Definition:
    name = "profit_per_100_hands"
    formula = profit / hands * 100
    unit = BB/100
```

and:

```text
Measurement:
    metric = profit_per_100_hands
    value = 4.2
    window = last 1000 hands
```

The definition describes the calculation.

The instance contains its result.

---

## 23. Conceptual Contract

The initial conceptual model is:

```text
Metric
{
    name
    unit
    definition
}
```

and:

```text
Measurement
{
    metric
    value

    scope
    dimensions

    time_start
    time_end

    provenance
}
```

These are conceptual models only.

The following remain intentionally undecided:

* numeric representation
* floating-point vs fixed-point vs arbitrary precision
* unit system
* serialization
* storage
* metric registration
* query language
* aggregation engine
* statistical library
* streaming implementation
* caching

---

## 24. Core Invariant

The EVolution core understands **what a measurement is and how measurements can be represented and combined**, but it must not assume the meaning of domain-specific metrics.

Therefore:

```text
Core:
    Measurement
    Metric
    Window
    Aggregation
    Provenance

Domain:
    defines meaningful metrics
    defines domain-specific units
    defines domain-specific calculations
```

The resulting conceptual pipeline is:

```text
Events
   │
   ▼
State
   │
   ▼
Measurements
   │
   ▼
Time Series / Aggregations
   │
   ▼
Patterns
   │
   ▼
Analysis
```

Measurement is therefore the boundary between **derived quantitative information** and higher-level **interpretation**.
