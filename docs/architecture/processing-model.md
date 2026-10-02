# Processing Model

## 1. Purpose

The Processing Model defines how EVolution transforms information from one representation into another.

The conceptual data model defines what exists:

```text
Event
State
Measurement
Time Series
Aggregation
Pattern
Analysis
Decision
Context
Provenance
```

The Processing Model defines how these objects move through the system.

The fundamental relationship is:

```text
Input
   ↓
Processing
   ↓
Output
```

Processing may be performed continuously, periodically, on demand, or as a batch operation.

---

## 2. Processing Graph

EVolution should be modeled as a processing graph rather than as one fixed linear pipeline.

A simple example is:

```text
Events
   ↓
State Projection
   ↓
Measurements
   ↓
Time Series
   ↓
Aggregation
   ↓
Pattern Detection
   ↓
Analysis
```

However, multiple processors may consume the same input:

```text
                 ┌──→ Measurement A
                 │
Events ──────────┼──→ State
                 │
                 └──→ Measurement B
```

Likewise, one processor may require multiple inputs:

```text
Measurements ──┐
               ├──→ Analysis
Patterns ──────┘
```

Therefore the general model is:

```text
Input Nodes → Processing Nodes → Output Nodes
```

rather than a single predetermined sequence.

---

## 3. Processor

A processor transforms one or more inputs into one or more outputs.

Conceptually:

```text
Processor
{
    inputs
    configuration
    process()
    outputs
}
```

For example:

```text
Event
   ↓
StateProjection
   ↓
State
```

or:

```text
TimeSeries
   ↓
TrendDetector
   ↓
Pattern
```

The processor defines the transformation.

It does not necessarily own the data it processes.

---

## 4. Input and Output

Every processor should have explicit input and output semantics.

Conceptually:

```text
Processor
    Input:
        EventStream

    Output:
        State
```

Another processor may be:

```text
Processor
    Input:
        MeasurementStream

    Output:
        PatternStream
```

A processor may also have multiple inputs:

```text
Processor
    Inputs:
        Measurements
        Context
        Configuration

    Output:
        Analysis
```

Explicit input/output contracts allow processors to be composed without requiring knowledge of their internal implementation.

---

## 5. Processor Composition

Processors should be composable.

For example:

```text
Event
  ↓
Projection
  ↓
State
  ↓
Metric
  ↓
Measurement
  ↓
Aggregation
  ↓
Pattern Detector
  ↓
Analysis
```

Each component only needs to understand its own contract.

For example, a pattern detector should not need to know how a measurement was originally produced.

It receives measurements according to its input contract and produces patterns according to its output contract.

---

## 6. Processing Node

A processing graph consists of nodes connected by data dependencies.

Conceptually:

```text
Node
{
    id
    input_contract
    output_contract
    processor
    configuration
}
```

Connections represent dependencies:

```text
Node A
   │
   └────→ Node B
```

This means:

```text
B depends on the output of A
```

The graph itself describes relationships between processing stages.

The implementation of the graph remains undecided.

---

## 7. Data Dependency

A dependency exists when one processing operation requires the result of another.

For example:

```text
State
   ↓
PlayerMeasurement
   ↓
PlayerTrend
```

The trend detector cannot operate until the required measurement exists.

Therefore:

```text
Measurement → Pattern
```

creates a processing dependency.

Dependencies should be explicit rather than hidden inside processors.

---

## 8. Processing Order

For dependent operations, execution order follows the dependency graph.

For:

```text
A → B → C
```

the system must ensure:

```text
A
before B

B
before C
```

Independent operations do not necessarily require ordering.

For example:

```text
          ┌──→ B
A ────────┤
          └──→ C
```

B and C are both dependent on A, but there is no inherent dependency between B and C.

This distinction becomes important when execution is parallelized.

---

## 9. Streaming Processing

A processor may operate continuously as new data arrives.

For example:

```text
Event
  ↓
State Projection
  ↓
Measurement
  ↓
Pattern Detector
```

As each new event arrives, downstream processors may update their outputs.

This is streaming processing.

Streaming does not necessarily mean that every processor executes immediately.

The system may buffer, batch, aggregate, or schedule processing while maintaining the required semantics.

---

## 10. Batch Processing

A processor may instead operate on a finite collection of data.

For example:

```text
Historical Events
      ↓
Replay
      ↓
Measurements
      ↓
Analysis
```

Batch processing is useful for:

* historical analysis
* backtesting
* recomputation
* importing datasets
* experiments
* validation
* model evaluation

The conceptual processor contract should not require streaming or batch execution unless the specific operation needs it.

---

## 11. Incremental Processing

Some processors can update their output incrementally.

For example:

```text
Existing State
      +
New Event
      ↓
Updated State
```

Similarly:

```text
Existing Aggregation
      +
New Measurement
      ↓
Updated Aggregation
```

Incremental processing can avoid recomputing the complete history.

Whether a processor supports incremental processing should be an explicit capability.

---

## 12. Replay

Because EVolution is event-driven, historical events can be replayed through processors.

Conceptually:

```text
Historical Events
      ↓
Replay
      ↓
Processing Graph
      ↓
Reconstructed Results
```

Replay can be used for:

* state reconstruction
* historical analysis
* testing
* debugging
* recomputation
* validating new algorithms

Replay should preserve the semantics of the original event stream.

---

## 13. Deterministic Processing

Where a processor is deterministic:

```text
same input
+
same configuration
+
same processor version
=
same output
```

This property is important for:

* testing
* replay
* debugging
* reproducibility
* comparing algorithm versions

Processors involving randomness, external data, or nondeterministic behavior must explicitly account for those dependencies in their provenance.

---

## 14. Stateful Processors

Not every processor can operate independently on each input.

Some processors maintain state.

For example:

```text
Events
   ↓
State Projection
```

requires previous state to process the next event.

Conceptually:

```text
Processor State
      +
Input
      ↓
Updated Processor State
      +
Output
```

A stateful processor should make its state semantics explicit.

This is distinct from domain state.

A processor may maintain internal execution state without that state necessarily being part of the domain model.

---

## 15. Stateless Processors

A stateless processor does not require historical internal state between processing operations.

Conceptually:

```text
Input
  ↓
Process
  ↓
Output
```

For example, a transformation that converts one representation into another may be stateless.

Stateless processors are generally easier to:

* test
* replay
* parallelize
* reuse

The architecture should support both stateful and stateless processing.

---

## 16. Windowed Processing

Many analytical operations require a limited window of data.

Examples:

```text
last 100 observations
last 1000 hands
last 24 hours
current session
current trading day
```

Conceptually:

```text
Stream
  ↓
Window
  ↓
Processor
  ↓
Output
```

The window definition should be explicit.

A window may be:

```text
time-based
count-based
event-based
context-based
```

Window semantics belong to the processor or processing configuration, not to the underlying observations themselves.

---

## 17. Event Time vs Processing Time

Processing must distinguish when an event occurred from when EVolution processed it.

For example:

```text
Event time:
    12:00:01

Ingestion time:
    12:00:04

Processing time:
    12:00:05
```

These timestamps describe different things.

Analytical calculations should use the appropriate temporal semantics rather than assuming processing time is event time.

This is especially important for:

* delayed data
* imported historical data
* replay
* out-of-order events
* real-time streams

---

## 18. Out-of-Order Data

Events may arrive in a different order from their original occurrence.

For example:

```text
Event A
event_time = 10:00:01

Event B
event_time = 10:00:03

Arrival:
    B
    A
```

The processing system must distinguish:

```text
arrival order
```

from:

```text
event order
```

The appropriate handling depends on the processor.

Possible strategies include:

```text
buffering
reordering
late-event correction
recomputation
ignoring late data
```

No universal strategy should be imposed at the conceptual level.

---

## 19. Backpressure

A streaming processor may produce data faster than its downstream processor can consume it.

Conceptually:

```text
Producer
    ↓
████████████
    ↓
Slow Consumer
```

The system therefore needs a concept of flow control.

Possible strategies include:

```text
buffer
block
drop
sample
batch
slow producer
```

The correct behavior depends on the semantics of the data.

For example, silently dropping a critical historical event may be unacceptable, while dropping redundant telemetry may be acceptable.

The processing model should therefore make loss behavior explicit.

---

## 20. Failure

Processors can fail.

Examples:

```text
invalid input
configuration error
resource exhaustion
external dependency failure
algorithm failure
corrupted data
```

A processing graph should distinguish between:

```text
successful processing
processing failure
invalid input
missing input
temporary unavailability
```

Failure handling is part of execution semantics, not domain analysis.

---

## 21. Error Propagation

A failure in one node may affect downstream nodes.

For example:

```text
Events
  ↓
State Projection
  X
  ↓
Measurements
```

The system must determine whether downstream processing should:

```text
stop
continue with previous state
produce partial output
retry
mark output unavailable
```

This decision should be defined by the processor and execution policy rather than being an implicit side effect.

---

## 22. Processing Modes

The same processing graph may eventually support multiple execution modes.

For example:

```text
LIVE
    process new observations as they arrive

REPLAY
    process historical observations

BATCH
    process a finite dataset

EXPERIMENT
    process data under alternative configurations
```

The semantic meaning of the processors should remain consistent across modes.

Only the execution environment should change where possible.

---

## 23. Configuration

Processing behavior may depend on configuration.

For example:

```text
Pattern Detector
    threshold = 2%

Aggregation
    window = 100 observations

Analysis
    baseline = previous session
```

Configuration is part of the processing definition and should be included in provenance when it affects output.

Configuration should not be hidden inside processor implementation.

---

## 24. Processing Graph vs Data Graph

The processing graph describes computation:

```text
A → B → C
```

The data/provenance graph describes information relationships:

```text
Event → Measurement → Pattern
```

They may look similar, but they are conceptually different.

The processing graph answers:

> Which computation depends on which computation?

The provenance graph answers:

> Which output was derived from which input?

Keeping these concepts separate prevents execution details from becoming confused with historical data lineage.

---

## 25. Processing Contract

A processor should conceptually define:

```text
Processor
{
    input_contract
    output_contract
    configuration
    capabilities
    process()
}
```

Capabilities may eventually describe properties such as:

```text
stateless
stateful
streaming
batch
incremental
mergeable
deterministic
```

These are conceptual capabilities, not yet an implementation interface.

---

## 26. Processing Graph Lifecycle

A processing graph may conceptually move through:

```text
DEFINED
   ↓
VALIDATED
   ↓
INITIALIZED
   ↓
RUNNING
   ↓
PAUSED
   ↓
STOPPED
```

A graph may also be reconstructed or restarted from persisted state or from replaying its inputs.

The lifecycle of the processing graph is separate from the lifecycle of domain objects processed by it.

---

## 27. Dynamic Graphs

The processing graph may eventually need to change while the system is running.

For example:

```text
Graph V1
    Events → Measurement → Analysis

Graph V2
    Events → Measurement → Pattern → Analysis
```

Changing a graph can change the meaning of derived results.

Therefore graph configuration and version should participate in provenance when they affect output.

Dynamic graph modification is not required for the initial implementation.

---

## 28. Processing Isolation

Processors should communicate through explicit contracts.

A processor should not implicitly depend on internal implementation details of another processor.

For example:

```text
PatternDetector
```

should depend on:

```text
Measurement contract
```

rather than:

```text
internal memory layout of MeasurementCalculator
```

This allows individual processors to be replaced without redesigning the entire system.

---

## 29. Core Architectural Invariant

The processing layer defines **how information is transformed and propagated**.

It should not define:

* domain-specific meaning
* domain-specific policies
* user objectives
* application behavior

The fundamental separation is:

```text
Semantic Model
    ↓
Processing Model
    ↓
Application
```

Therefore:

> **A processor transforms information according to an explicit contract, and a processing graph composes those transformations according to their dependencies.**
