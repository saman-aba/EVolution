# Provenance Model

## 1. Purpose

Provenance describes the origin, history, and derivation of an object.

It answers questions such as:

```text
Where did this data come from?
Which inputs produced it?
Which transformation produced it?
Which configuration was used?
Which version of the algorithm produced it?
```

Provenance allows EVolution to trace derived information back toward its original observations.

The conceptual relationship is:

```text
Source
   ↓
Transformation
   ↓
Derived Object
```

For example:

```text
Events
   ↓
Projection
   ↓
State
   ↓
Measurement
   ↓
Aggregation
   ↓
Pattern
   ↓
Analysis
```

Each stage may preserve a reference to its origin.

---

## 2. Provenance vs Context

Context and provenance answer different questions.

Context:

```text
Under what circumstances does this object exist?
```

Provenance:

```text
How was this object produced?
```

For example:

```text
Measurement
    context:
        player = A
        session = 42

    provenance:
        source = hand_history
        derived_from = events 1000..2500
        metric = bb_per_100
        algorithm_version = 3
```

The context describes the measurement.

The provenance describes its derivation.

---

## 3. Source

A source identifies where information originated.

Possible sources include:

```text
live_stream
file
database
network_capture
user_input
simulation
sensor
external_api
previous_analysis
```

A source is not necessarily the physical origin of the information.

For example, a measurement may originate from:

```text
hand_history_file
```

while the actual historical source of the information was:

```text
poker_client
```

Multiple provenance layers may therefore exist.

---

## 4. Provenance Chain

Derived objects can form a provenance chain.

For example:

```text
Raw File
   ↓
Imported Event
   ↓
Normalized Event
   ↓
State
   ↓
Measurement
   ↓
Time Series
   ↓
Aggregation
   ↓
Pattern
   ↓
Analysis
```

An analysis should ideally be able to identify the relevant portion of this chain.

This makes it possible to trace:

```text
Analysis
   ↓
Evidence
   ↓
Measurements
   ↓
Events
   ↓
Original Source
```

---

## 5. Transformation

A transformation converts one representation into another.

Conceptually:

```text
Transformation
{
    type
    inputs
    outputs
    configuration
    version
}
```

Examples:

```text
event normalization
state projection
metric calculation
resampling
aggregation
pattern detection
statistical analysis
```

The transformation itself becomes part of provenance.

For example:

```text
Measurement
    derived_by:
        metric = bb_per_100
        version = 2.1
        configuration = ...
```

---

## 6. Direct and Indirect Provenance

An object may have direct provenance:

```text
Measurement
    ← Events 1000..1500
```

or indirect provenance:

```text
Analysis
    ← Pattern
        ← Time Series
            ← Measurements
                ← Events
```

The system should be able to preserve both the immediate derivation and, where required, the broader ancestry.

---

## 7. Provenance Graph

Provenance naturally forms a graph rather than a simple chain.

For example:

```text
             Event A
                │
                ├──────┐
                ↓      ↓
             State   Measurement
                │       │
                └───┬───┘
                    ↓
                 Pattern
                    │
              ┌─────┴─────┐
              ↓           ↓
          Analysis A   Analysis B
```

One input may contribute to many derived objects.

One derived object may depend on multiple inputs.

Therefore provenance should conceptually support:

```text
many inputs → one output
one input → many outputs
```

---

## 8. Provenance and Reproducibility

A derived result should contain enough provenance information to determine how it was produced.

At minimum, this may include:

```text
source
input references
transformation
configuration
version
timestamp
```

For example:

```text
Analysis
{
    input:
        measurement_series_42

    transformation:
        drawdown_analysis

    version:
        1.4

    configuration:
        threshold = 15 BB
}
```

Exact reproducibility may additionally require:

```text
software version
dataset version
environment
random seed
external dependencies
```

The required level depends on the application.

---

## 9. Versioning

Derived results can change when their producing logic changes.

For example:

```text
Metric v1
Metric v2
```

may produce different results from the same events.

The system must distinguish:

```text
same input
different transformation version
```

from:

```text
different input
same transformation version
```

This distinction is essential when comparing historical analyses.

---

## 10. Configuration Provenance

Algorithm identity alone may not be sufficient.

For example:

```text
PatternDetector v2
```

could produce different results with:

```text
threshold = 2%
```

versus:

```text
threshold = 5%
```

Therefore configuration should be part of provenance whenever it can affect the result.

Conceptually:

```text
Result
    algorithm = detector-v2
    configuration = config-17
```

---

## 11. Dataset Versioning

The same source may change over time.

For example:

```text
dataset-v1
dataset-v2
```

may contain different observations.

A derived result should therefore be able to identify which version of the input data it used.

This prevents a common ambiguity:

```text
"Why can't I reproduce yesterday's analysis?"
```

when the underlying dataset has since changed.

---

## 12. Immutable Historical Results

Provenance becomes unreliable if historical derived objects are silently modified.

For example:

```text
Analysis A
```

should not silently become:

```text
Analysis B
```

while retaining the same identity and provenance.

Corrections or recomputation should produce a distinguishable version or new result.

This does not require immutable storage everywhere.

It requires that historical identity and derivation remain understandable.

---

## 13. Provenance of Corrections

Corrections are themselves events in the history of data.

For example:

```text
Original Event
      ↓
Correction
      ↓
Corrected Representation
```

The corrected representation should retain knowledge that a correction occurred.

Similarly:

```text
Original Measurement
      ↓
Recalculation
      ↓
Updated Measurement
```

should not erase the fact that the original result existed.

The exact correction mechanism remains an architectural decision for later.

---

## 14. Provenance and Confidence

Provenance does not determine confidence.

For example:

```text
Source:
    imported_file

Confidence:
    high
```

or:

```text
Source:
    imported_file

Confidence:
    low
```

are both possible.

Confidence depends on the semantics and quality of the evidence or method.

Provenance merely records where the information came from and how it was produced.

---

## 15. Provenance and Uncertainty

Similarly, provenance does not eliminate uncertainty.

A result may be:

```text
exact
estimated
sampled
predicted
simulated
```

while still having complete provenance.

For example:

```text
Prediction
    source = historical_dataset
    model = predictor-v5
    result = estimated_probability
```

The provenance explains the derivation.

The uncertainty describes the nature of the result.

---

## 16. Provenance and External Dependencies

Some analyses depend on information outside EVolution.

Examples:

```text
external market data
exchange metadata
software configuration
reference datasets
user-provided parameters
```

Such dependencies should be identifiable when they materially affect the result.

Conceptually:

```text
Analysis
    ├── internal measurements
    ├── external dataset
    └── configuration
```

This is particularly important for reproducibility.

---

## 17. Provenance Queries

A provenance system should conceptually support questions such as:

```text
What produced this result?
```

```text
Which inputs contributed to this result?
```

```text
Which analyses depend on this event?
```

```text
Which version of the algorithm produced this pattern?
```

```text
Which source file contained the original observation?
```

```text
What changed between these two analyses?
```

These queries should be possible without requiring every analytical component to implement its own unrelated provenance mechanism.

---

## 18. Forward and Backward Tracing

Provenance supports two important directions.

### Backward tracing

Start with a result and find its origin:

```text
Analysis
   ↓
Pattern
   ↓
Measurement
   ↓
Event
   ↓
Source
```

This answers:

> Why does this result exist?

### Forward tracing

Start with an input and find its consequences:

```text
Event
   ↓
Measurement
   ↓
Pattern
   ↓
Analysis A
   ↓
Decision
```

This answers:

> What was affected by this input?

Both directions are useful.

---

## 19. Provenance Identity

A conceptual provenance record may look like:

```text
Provenance
{
    source
    inputs
    transformation
    configuration
    version
    timestamp
    environment?
}
```

This is not intended to prescribe a concrete storage representation.

The important requirement is that derived objects can reference their derivation.

---

## 20. Provenance Graph and Reproducibility

The combination of provenance and versioned transformations creates a reproducibility path:

```text
Source Dataset
      +
Source Version
      +
Transformation
      +
Transformation Version
      +
Configuration
      +
Environment
      ↓
Reproducible Result
```

Perfect reproducibility may not always be possible.

For example, external systems may change or stochastic algorithms may depend on unavailable randomness.

In those cases, provenance should still document the known dependencies and limitations.

---

## 21. Provenance Is Not Audit

Provenance and audit trails are related but different.

Provenance answers:

```text
How was this object produced?
```

Audit answers:

```text
Who performed this operation?
When?
What changed?
```

An application may require both.

EVolution should not automatically equate the two concepts.

---

## 22. Provenance Is Not Storage History

Database history, filesystem history, and provenance are also different concepts.

A database may know:

```text
row updated at 12:30
```

but that does not necessarily tell us:

```text
which measurement produced the row
which algorithm generated it
which events contributed to it
```

Provenance describes semantic derivation rather than merely physical storage changes.

---

## 23. Core Architectural Invariant

The core should provide mechanisms for representing and tracing derivation.

It should not impose a particular storage system, database, serialization format, or version-control mechanism.

The fundamental relationship is:

```text
Source
   ↓
Input
   ↓
Transformation
   ↓
Derived Object
```

Therefore:

> **Every derived object should be capable of identifying where it came from and, when required, how it was produced.**
