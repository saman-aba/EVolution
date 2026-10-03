# ADR 0029 — Processing Observability Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processing consists of interacting components:

```text
Graph
 ├── Scheduler
 ├── Queues
 ├── Processors
 ├── Execution Backend
 ├── Storage
 └── Recovery / Supervision
```

These components need operational visibility.

Examples include:

* processor state
* queue depth
* processing latency
* throughput
* failures
* retries
* resource usage
* recovery attempts
* dropped work
* backpressure
* checkpoint duration

The EVolution analytical model already contains:

```text
Event
State
Measurement
Time Series
Pattern
Analysis
```

Operational telemetry could easily become confused with those concepts.

The architecture therefore needs an explicit boundary between **observability** and **analytical data**.

## Decision

EVolution treats **observability as a separate cross-cutting concern**.

Observability provides information about system execution and operational behavior.

It does not automatically become part of the domain analytical model.

The primary conceptual observability signals are:

```text
Logs
Metrics
Traces
Health / Status
Events
```

These signals may reference the analytical model when appropriate, but they remain operational information unless explicitly promoted into domain data.

## Observability Goals

Observability should allow an operator or application to determine:

```text
What happened?
Where did it happen?
When did it happen?
How long did it take?
How often does it happen?
What resources were involved?
What is the current state?
What work is affected?
```

It should support diagnosis without changing processing semantics.

## Observability vs Analytical Data

The distinction is:

```text
Observability:
    How is EVolution operating?

Analytical data:
    What does the processed domain information represent?
```

For example:

```text
queue_depth = 5000
```

is normally operational telemetry.

A domain measurement such as:

```text
player_action_rate = 12 actions/minute
```

is analytical data.

The same physical measurement mechanism may be used to represent both, but their semantic ownership differs.

## Logs

Logs represent human-oriented or diagnostic execution information.

Examples:

```text
processor initialization failed
checkpoint restored
connection rejected input
recovery attempt started
```

Logs may contain:

```text
timestamp
severity
component
message
structured fields
correlation information
```

The exact logging framework is deferred.

## Logs Are Not Errors

An `Error` is a structured result representing an unsuccessful operation.

A log is an observability output describing execution.

Therefore:

```text
Error
```

does not automatically imply:

```text
log entry
```

and:

```text
log entry
```

does not automatically imply:

```text
Error
```

A caller may choose to log an error.

The lower-level component should avoid automatically logging every propagated error, preventing duplicate logging.

## Log Levels

A logging system may conceptually support:

```text
TRACE
DEBUG
INFO
WARN
ERROR
FATAL
```

The exact levels are implementation-dependent.

Severity must not change processing semantics.

## Structured Logging

Operational logs should preferably expose structured fields rather than requiring consumers to parse arbitrary text.

Conceptually:

```text
LogRecord
{
    timestamp
    severity
    component
    message?
    fields?
}
```

Human-readable messages are diagnostic and are not machine contracts.

## Sensitive Information

Observability output must not automatically expose secrets or sensitive configuration.

Examples requiring protection may include:

```text
credentials
authentication tokens
private keys
connection secrets
personal data
```

Redaction policy belongs to the observability/configuration boundary.

## Metrics

Metrics represent quantitative operational observations.

Examples:

```text
processing_rate
processing_latency
queue_depth
queue_bytes
error_count
retry_count
checkpoint_duration
memory_usage
cpu_usage
```

Metrics should have explicit identity and units.

Conceptually:

```text
Metric
{
    name
    unit
    description
    dimensions
}
```

This is related to the generic analytical `Metric` concept but does not imply that every operational metric belongs to the domain analytical model.

## Operational Metric vs Analytical Metric

The distinction is semantic.

```text
Operational metric:
    describes EVolution execution

Analytical metric:
    describes the subject being analyzed
```

For example:

```text
processor_latency
```

is operational.

```text
player_win_rate
```

is analytical.

A system may expose both through a common metric infrastructure while preserving their semantic ownership.

## Metric Identity

Operational metrics should have stable logical identity.

A metric should not be identified solely by its display name if multiple dimensions distinguish instances.

For example:

```text
processor_latency
processor = poker_hand_processor
```

is different from:

```text
processor_latency
processor = market_tick_processor
```

The exact metric registry mechanism is deferred.

## Metric Dimensions

Operational metrics may use dimensions such as:

```text
component
processor
graph
connection
partition
resource
execution_mode
```

Dimensions should remain bounded and meaningful.

Uncontrolled high-cardinality dimensions can create significant storage and memory costs.

## Metric Units

Metrics must define units explicitly where applicable.

Examples:

```text
seconds
milliseconds
bytes
items
operations
requests
percent
ratio
```

A metric called:

```text
latency
```

without a defined unit is insufficient as a stable contract.

## Counters

Counters represent monotonically increasing quantities within their defined lifetime.

Examples:

```text
processed_total
errors_total
retries_total
dropped_total
```

Counter reset behavior must be understood when interpreting time series.

A process restart may reset an in-memory counter.

## Gauges

Gauges represent a current or sampled value.

Examples:

```text
queue_depth
memory_usage
active_processors
```

A gauge may increase or decrease.

## Histograms and Distributions

Latency and resource measurements often require distributions rather than a single average.

Conceptually:

```text
latency
    ↓
distribution
    ↓
count / buckets / quantiles
```

The exact representation is deferred.

## Rates

Rates must preserve their temporal basis.

For example:

```text
events_per_second
```

requires knowledge of:

```text
event count
time interval
```

A rate must not be blindly averaged across intervals when that changes its meaning.

This follows the general measurement/aggregation model.

## Histograms vs Percentiles

A percentile is a derived statistical value.

The system must distinguish:

```text
raw observations
distribution representation
percentile estimate
```

An approximate percentile must not be presented as exact without preserving that distinction.

## Latency Measurement

Operational latency should use an appropriate monotonic clock for elapsed time.

For example:

```text
start = monotonic_clock()
operation()
end = monotonic_clock()

latency = end - start
```

Wall-clock timestamps should not be used as the sole mechanism for elapsed-time measurement.

This follows the time model.

## Throughput

Throughput may be measured as:

```text
items / second
bytes / second
operations / second
```

The counting population and time interval must be explicit.

For processing systems, throughput should distinguish at least where relevant:

```text
submitted
admitted
processed
completed
failed
dropped
```

These quantities are not interchangeable.

## Queue Observability

Queues may expose:

```text
current_items
current_bytes
capacity
admission_rejections
drops
blocked_producers
oldest_item_age
```

These are operational observations.

They must not automatically become domain measurements.

## Backpressure Observability

Backpressure may expose:

```text
blocked_time
rejected_items
dropped_items
sampled_items
degraded_operations
spill_count
```

This allows operators to determine whether processing capacity is limiting throughput.

## Delivery Observability

Delivery semantics may produce operational telemetry:

```text
delivery_attempts
duplicate_executions
acknowledgements
unknown_completions
redeliveries
```

These observations are especially important for at-least-once processing.

## Recovery Observability

Recovery may expose:

```text
recovery_attempts
restarts
checkpoint_restores
replay_items
replay_duration
recovery_failures
recovery_successes
```

Recovery telemetry must not silently alter recovery policy.

## Supervisor Observability

Supervision may expose:

```text
processor_state
failure_count
recovery_count
cooldown_time
escalation_count
```

These are operational signals.

## Health

Health represents whether a component can currently perform its required role.

Conceptually:

```text
Health
{
    status
    reason?
    details?
}
```

Possible conceptual states:

```text
HEALTHY
DEGRADED
UNAVAILABLE
FAILED
UNKNOWN
```

The exact state set is not universal.

Health is an operational abstraction, not a processor lifecycle replacement.

## Health vs Lifecycle State

These are different:

```text
Lifecycle:
    What state is the processor in?

Health:
    Can the component currently fulfill its intended role?
```

For example:

```text
Processor lifecycle:
    ACTIVE

Health:
    DEGRADED
```

may be valid.

Likewise:

```text
Processor lifecycle:
    STOPPED

Health:
    UNAVAILABLE
```

may be expected rather than a failure.

## Health vs Error

An error describes a particular unsuccessful operation.

Health describes an ongoing or current operational condition.

For example:

```text
Error(StorageUnavailable)
```

may occur during one operation while the component remains operational.

Repeated storage failures may eventually cause:

```text
Health = UNAVAILABLE
```

These are separate concepts.

## Status

Components may expose status information such as:

```text
current lifecycle state
current execution mode
active configuration identity
current checkpoint
current recovery phase
```

Status is operational information.

## Tracing

Tracing represents relationships between processing operations.

Conceptually:

```text
Graph Input
    ↓
Processor A
    ↓
Processor B
    ↓
Processor C
```

A trace can connect these operations through a correlation/trace identity.

Tracing is especially useful for diagnosing:

```text
latency
dependency chains
retries
fan-out
fan-in
failures
```

## Trace Identity

A trace identity is distinct from logical object identity.

For example:

```text
TraceId
```

does not replace:

```text
EventId
ProcessorId
AnalysisId
```

A trace answers:

> Which execution activities belong to this operational flow?

An object identity answers:

> Which logical object is this?

## Span

A trace may consist of spans representing bounded operations.

Conceptually:

```text
Trace
 ├── Span: graph admission
 ├── Span: processor A
 ├── Span: processor B
 └── Span: storage write
```

The exact tracing implementation is deferred.

## Correlation

Correlation identifiers may connect:

```text
submission
execution
retry
completion
recovery
```

Correlation is not logical identity.

An operation can have:

```text
input identity
execution identity
trace identity
correlation identity
```

simultaneously.

## Observability and Provenance

Observability and provenance overlap but are not identical.

```text
Provenance:
    Where did this information come from?

Observability:
    What happened during system execution?
```

Provenance is part of the meaning/reproducibility of derived information.

Observability primarily supports operational diagnosis.

A provenance record may reference observability information where useful, but the two systems remain conceptually separate.

## Observability and Configuration

Effective configuration may be exposed operationally:

```text
processor
configuration_id
version
execution_mode
```

However, secrets must be redacted.

Configuration observability must not silently expose protected values.

## Observability and Execution Context

Execution context may contain information useful for diagnosis:

```text
run_id
execution_mode
resource limits
dependency versions
```

Only relevant information should be exposed.

Incidental runtime details should not become mandatory analytical metadata.

## Observability and Provenance

If an operational condition materially affects a derived result, provenance should capture the relevant semantic fact.

For example:

```text
processing degraded due to resource limit
```

may matter to interpreting the result.

However, provenance should not require storing every debug log line.

## Logs, Metrics, and Traces Are Complementary

The three primary telemetry forms answer different questions:

```text
Logs:
    What happened?

Metrics:
    How much / how often?

Traces:
    Where did the operation spend time / flow?
```

None universally replaces the others.

## Observability Does Not Control Processing

Telemetry must not silently change processing behavior.

For example:

```text
metric collection enabled
```

must not change:

```text
processor semantics
ordering
delivery guarantees
analytical result
```

unless an explicit execution configuration says otherwise.

## Observability Overhead

Instrumentation consumes resources.

Therefore observability may require:

```text
sampling
aggregation
buffering
rate limiting
```

These are implementation concerns.

Observability must not create uncontrolled resource consumption.

## Observability Sampling

Trace or metric sampling is allowed.

However, sampled telemetry must not be presented as a complete record.

For example:

```text
10% trace sampling
```

does not mean:

```text
all operations traced
```

Sampling configuration may be relevant when interpreting operational statistics.

## Observability Failure

Telemetry failure should not normally cause analytical processing to fail.

For example:

```text
metrics backend unavailable
```

should not automatically imply:

```text
processor FAILED
```

unless the component explicitly requires telemetry for correctness.

## Telemetry Backpressure

Observability pipelines may themselves experience overload.

Telemetry must therefore have explicit:

```text
capacity
buffering
sampling
drop
spill
```

semantics.

Observability loss must not silently become analytical data loss.

## Logging Failure

Failure to write a log should normally not terminate processing.

Logging infrastructure may have its own bounded buffers and loss policy.

Critical operational information that is required for correctness should not rely solely on best-effort logs.

## Metrics Failure

Similarly, failure to export a metric should not normally alter processor results.

If a metric is required for an operational control mechanism, that dependency must be explicit.

## Tracing Failure

Tracing should normally be non-fatal.

A missing trace span must not alter processor semantics.

## Observability and Security

Telemetry can expose sensitive information.

Observability boundaries should support:

```text
redaction
access control
field filtering
sampling
retention limits
```

The exact security model is deferred.

## Observability Retention

Logs, metrics, and traces have different retention characteristics.

They are not automatically retained with the same policy as domain events.

Retention belongs to the relevant storage/observability configuration.

## Observability Storage

Observability data may use separate storage from analytical data.

For example:

```text
Domain Data
    ↓
Analytical Storage

Operational Telemetry
    ↓
Observability Storage
```

The generic Storage Model does not require one physical backend.

## Operational Metrics as Analytical Inputs

An application may intentionally ingest operational metrics into EVolution's analytical pipeline.

For example:

```text
CPU usage
    ↓
Event / Measurement
    ↓
Time Series
    ↓
Pattern Detection
```

When this happens, the data explicitly crosses the boundary.

It is no longer merely incidental telemetry.

## Domain Events vs Operational Events

An operational event such as:

```text
ProcessorRestarted
```

is not automatically a domain event.

A domain may intentionally define:

```text
SystemRestartObserved
```

and ingest it as domain information.

That conversion must be explicit.

## Observability Identity

Observability objects may require their own identifiers:

```text
TraceId
SpanId
LogRecordId
MetricId
```

These identifiers must not be confused with domain identifiers.

## Observability Time

Observability records may contain multiple temporal values:

```text
event time
record creation time
processing time
export time
```

The time model applies.

For latency measurements, elapsed time should use a monotonic source.

## Clock Synchronization

Distributed tracing may depend on clocks from different systems.

The architecture must not assume that wall-clock timestamps alone provide a perfect global ordering.

Correlation and explicit causal relationships may be required.

## Observability and Determinism

Instrumentation should not change deterministic processing semantics.

For example:

```text
debug logging enabled
```

must not alter the result of a deterministic processor.

If instrumentation changes scheduling or timing in a way that materially affects semantics, that dependency must be explicit.

## Observability and Reproducibility

Observability configuration may affect execution performance but does not automatically become part of analytical reproducibility.

It becomes relevant when it materially changes semantic results.

For example:

```text
trace sampling
```

normally does not change a processor result.

A resource-intensive instrumentation mode that changes scheduling enough to alter nondeterministic output may require explicit execution-context representation.

## Observability API Boundary

Components should expose observability through explicit interfaces.

Conceptually:

```text
Processor
    ↓
Status / Metrics / Events
    ↓
Observability Adapter
    ↓
External Telemetry System
```

The processor should not depend directly on a specific monitoring backend.

## External Telemetry Systems

Possible external systems include:

```text
metrics backend
logging backend
distributed tracing backend
monitoring dashboard
alerting system
```

No particular vendor or protocol is selected.

## Observability and Alerts

Alerts are an operational policy built on telemetry.

For example:

```text
queue_depth > threshold
```

may trigger an operational alert.

The alert policy does not belong to the generic analytical Core.

## Observability and Recovery

A supervisor may consume health/status/metrics to inform recovery policy.

However:

```text
metric
```

does not automatically prescribe:

```text
recovery action
```

Recovery remains policy-driven.

## Consequences

### Positive

* Operational telemetry has a clear architectural boundary.
* Errors, logs, metrics, traces, health, and provenance are not conflated.
* Analytical data remains separate from incidental system telemetry.
* Observability failures do not automatically become processing failures.
* Operational metrics can intentionally cross into the analytical model when required.

### Negative

* Two related but distinct metric concepts must be maintained.
* Telemetry systems require their own capacity and retention management.
* Distributed tracing introduces additional identity and timing concepts.
* Instrumentation itself consumes resources.

## Deferred Decisions

This ADR does not select:

```text
logging framework
metrics library
tracing protocol
telemetry backend
dashboard
alerting system
log format
metric transport
trace transport
sampling implementation
telemetry storage
```

## Decision Summary

```text
Observability:
    Cross-cutting operational concern

Primary signals:
    Logs
    Metrics
    Traces
    Health
    Status

Error:
    Processing failure representation

Log:
    Diagnostic execution information

Metric:
    Quantitative operational observation

Trace:
    Execution relationship

Health:
    Current operational capability

Provenance:
    Origin and derivation of information

Analytical data:
    Domain information intentionally processed by EVolution

Telemetry failure:
    Normally non-fatal

Sampling:
    Explicit and visible

Physical telemetry backend:
    Deferred
```

## Invariants

1. Observability is separate from analytical semantics.
2. Errors and logs are distinct concepts.
3. Operational metrics and analytical metrics are distinct by semantic ownership.
4. Trace identity must not replace logical object identity.
5. Health must not be treated as a replacement for lifecycle state.
6. Observability must not silently modify processing semantics.
7. Telemetry failure must not normally cause processing failure.
8. Observability buffering and capacity must be bounded.
9. Telemetry loss must not silently become analytical data loss.
10. Sampled telemetry must not be represented as complete telemetry.
11. Sensitive configuration and data must not be exposed unintentionally through observability.
12. Operational telemetry is not automatically retained as domain history.
13. Operational events do not automatically become domain events.
14. Operational metrics may become analytical data only through explicit modeling/ingestion.
15. Provenance and observability remain distinct even when they reference related execution information.
16. Elapsed-time measurements must use appropriate monotonic timing semantics.
17. Wall-clock timestamps must not be assumed to provide perfect global ordering.
18. Observability must not become an implicit recovery policy.
19. External telemetry systems must remain replaceable through explicit interfaces.
20. Observability configuration must not silently change analytical results.

## Invariant

> **EVolution treats observability as a separate operational layer: logs, metrics, traces, health, and status describe system execution without silently becoming domain data or changing processing semantics, while explicit interfaces allow operational information to be consumed or promoted into the analytical model when intentionally required.**
