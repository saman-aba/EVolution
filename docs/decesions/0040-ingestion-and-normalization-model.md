# ADR 0040 — Ingestion and Normalization Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution receives information from external sources.

Examples include:

```text
files
network streams
databases
APIs
user input
logs
market feeds
game histories
simulations
other applications
```

These sources have representations and semantics that are external to EVolution.

The system therefore needs a clear boundary between:

```text
External Representation
        ↓
Ingestion
        ↓
Normalization
        ↓
EVolution-native Information
        ↓
Events / Domain Objects
```

Without such a boundary, external formats can leak into Core and Domain components.

The architecture also needs to distinguish:

```text
raw source data
parsed data
normalized data
domain events
```

These are not automatically the same thing.

## Decision

EVolution treats **Ingestion** as the boundary responsible for accepting external information and translating it into internal representations.

Ingestion may perform:

```text
acquisition
parsing
decoding
validation
normalization
mapping
deduplication where explicitly required
```

Ingestion does not define generic Core semantics.

The resulting information may be:

```text
raw source record
normalized record
domain event
domain object
```

depending on the source and application workflow.

## Ingestion Pipeline

Conceptually:

```text
External Source
      ↓
Acquisition
      ↓
Parsing / Decoding
      ↓
Structural Validation
      ↓
Normalization
      ↓
Semantic Validation
      ↓
Domain Mapping
      ↓
EVolution Information
```

Not every source requires every stage.

## External Representation

External representation is owned by the source/interface boundary.

Examples:

```text
JSON
CSV
binary file
database row
network packet
log line
protobuf message
custom text format
```

The internal architecture must not assume that the external representation is the canonical semantic representation.

## Acquisition

Acquisition obtains raw information from a source.

Examples:

```text
read file
receive network message
query database
consume API response
receive user-provided data
```

Acquisition may fail because of:

```text
network failure
file failure
permission failure
resource exhaustion
source unavailability
```

These are ingestion/infrastructure failures, not necessarily domain errors.

## Raw Data

Raw data is information preserved in its original or source-representative form.

Conceptually:

```text
Raw Data
{
    source
    representation
    received_at
    content
}
```

The exact representation is implementation-dependent.

Raw data may be retained for:

```text
reprocessing
debugging
audit
reconstruction
provenance
future normalization
```

Retention is governed by the Storage Model.

## Raw Data Is Not Automatically an Event

A raw source record is not necessarily a domain event.

For example:

```text
CSV row
```

may represent:

```text
PlayerAction
```

only after interpretation and normalization.

## Parsing

Parsing converts an external representation into a structurally understood representation.

For example:

```text
CSV bytes
    ↓
CSV fields
```

or:

```text
JSON
    ↓
parsed object
```

Parsing should not silently invent missing semantic information.

## Structural Validation

Structural validation verifies that the external representation is syntactically and structurally valid.

Examples:

```text
required field exists
integer is syntactically valid
message has valid framing
JSON is well formed
binary length is valid
```

Structural validation is distinct from domain validation.

## Semantic Validation

Semantic validation determines whether the parsed information makes sense according to the relevant domain or application contract.

Examples:

```text
timestamp is valid for the source
player action is valid in the current domain state
identifier format is supported
field combination is meaningful
```

Domain semantics belong to the domain layer.

## Normalization

Normalization converts source-specific representation into a stable internal representation.

For example:

```text
Source A:
    "player_id": "00123"

Source B:
    "player": 123
```

may normalize into:

```text
PlayerId(123)
```

if the domain defines those representations as equivalent.

## Normalization vs Interpretation

Normalization may remove representational differences.

Interpretation determines meaning.

These should not be conflated.

For example:

```text
"2026-10-03 12:30"
```

may be normalized into a UTC timestamp only if the source timezone is known or an explicit inference rule exists.

If timezone information is unavailable:

```text
time = UNKNOWN
```

may be more correct than inventing UTC.

## Temporal Normalization

Ingestion must follow ADR 0011.

Temporal information must preserve:

```text
KNOWN
ESTIMATED
UNKNOWN
```

A source timestamp may be:

```text
known
estimated
unknown
```

depending on available evidence.

## Ingestion Time

Ingestion should record ingestion time when available.

This is distinct from source/event time.

For example:

```text
Event Time:
    2026-10-03 10:00 UTC

Ingestion Time:
    2026-10-03 10:05 UTC
```

The latter must not replace the former.

## Missing Event Time

If the source provides no meaningful event time:

```text
event_time = UNKNOWN
```

must be allowed.

Ingestion must not silently use:

```text
current_time()
```

as event time.

## Estimated Event Time

If an explicit source/domain rule allows temporal inference:

```text
event_time = ESTIMATED
```

and the estimation method should be preserved when materially relevant.

For example:

```text
source provides date only
    ↓
time estimated from known session boundary
```

The result remains estimated.

## Source Identity

Ingestion should preserve source identity where relevant.

Examples:

```text
source file
source system
feed identifier
session
record identifier
```

Source identity is distinct from EVolution logical object identity.

## Source Record Identity

If the external source provides a stable record identifier, ingestion should preserve it where useful.

For example:

```text
source_record_id
```

can support:

```text
deduplication
provenance
correction
replay
traceability
```

It does not automatically become the EVolution object identity.

## Generated Identity

If the source has no identity, EVolution may generate one according to the relevant object identity contract.

The generated identity must not depend on:

```text
memory address
thread identity
incidental iteration order
```

unless the object explicitly defines such identity as temporary/local.

## Deduplication

Ingestion may perform deduplication when explicitly required.

Deduplication must define:

```text
identity key
scope
retention
duplicate behavior
```

It must not silently discard data merely because two records look similar.

## Duplicate Source Records

These are distinct cases:

```text
same source record delivered twice
```

and:

```text
two distinct source records containing identical content
```

Deduplication must distinguish them according to source identity semantics.

## Correction

If a source later provides a corrected record, ingestion should not silently overwrite historical truth when the source-of-truth model is append-oriented.

Possible semantics include:

```text
correction event
replacement relation
new source version
new normalized object
```

The exact behavior belongs to the source/domain contract.

## Source Versions

Sources may expose versions.

Examples:

```text
file version
dataset version
API revision
feed sequence
record revision
```

These may participate in provenance and reproducibility.

## Ingestion Provenance

Ingestion should be capable of recording:

```text
source
source identity
source version
raw record identity
ingestion time
parser
normalizer
configuration
component version
```

when required for reproducibility or traceability.

## Parser Identity

A parser implementation may affect interpretation.

Therefore, when material, provenance should identify:

```text
parser identity/version
```

## Normalizer Identity

Likewise, normalization logic may affect results.

Material normalization should be identifiable by:

```text
normalizer identity/version
configuration
```

## Ingestion Configuration

Ingestion behavior may depend on configuration such as:

```text
format
timezone
encoding
field mapping
validation policy
deduplication policy
normalization rules
```

Configuration must be explicit according to ADR 0012.

## External Environment

Ingestion may depend on environment such as:

```text
network availability
source service version
filesystem
locale
hardware
```

Only materially result-affecting conditions should become explicit execution context/provenance.

## Ingestion and Configuration Defaults

Defaults must be explicit.

For example:

```text
timezone = UTC
```

must not be silently assumed if doing so changes semantic interpretation.

A source without timezone information may instead require:

```text
timezone configuration
```

or produce estimated/unknown temporal information.

## Encoding

Text ingestion must identify the expected encoding where relevant.

Invalid encoding should produce an explicit error rather than silently replacing characters unless replacement behavior is explicitly part of the format contract.

## Units

External numeric values may use different units.

Normalization should explicitly convert units when required.

For example:

```text
milliseconds
    ↓
seconds
```

The conversion must preserve semantic meaning and sufficient precision.

## Numeric Precision

Normalization must not silently discard meaningful precision.

For example:

```text
64-bit source value
    ↓
32-bit internal value
```

requires an explicit compatibility decision if precision can be lost.

## Null, Missing, Empty, and Zero

Ingestion must distinguish where the source semantics require:

```text
missing
null
empty
zero
unknown
```

These are not universally interchangeable.

## Invalid Values

An invalid external value should not silently become a plausible valid value.

For example:

```text
invalid timestamp
    ↓
1970-01-01
```

must not be used as a generic error-handling strategy.

## Partial Records

Some sources may contain incomplete records.

The ingestion contract must define whether the record is:

```text
accepted
accepted with unknown fields
accepted with estimated fields
rejected
quarantined
```

depending on semantic requirements.

## Quarantine

Applications may choose to retain invalid or incomplete source records separately for later inspection.

For example:

```text
Source
 ├── valid records
 └── rejected/quarantined records
```

Quarantine is not silently successful ingestion.

## Ingestion Errors

Errors may occur at:

```text
acquisition
parsing
structural validation
normalization
semantic validation
domain mapping
persistence
```

Errors should identify the appropriate stage where useful.

## Error Granularity

For batch ingestion, the application may need per-record failures.

For example:

```text
100 records
97 accepted
3 rejected
```

The result must explicitly represent partial success.

## Batch Ingestion

Batch ingestion may conceptually produce:

```text
BatchResult
{
    accepted
    rejected
    failed
    metadata
}
```

The exact representation is deferred.

Batch ingestion is not automatically an aggregation.

## Streaming Ingestion

Streaming ingestion may continuously produce normalized information.

The contract must define:

```text
ordering
delivery
backpressure
failure
reconnection
duplicate handling
checkpointing
```

where relevant.

## Ingestion and Backpressure

Ingestion may be subject to backpressure.

For example:

```text
Source
    ↓
Ingestion
    ↓
Processing
```

If downstream capacity is exhausted, ingestion may:

```text
block
buffer
reject
drop
sample
spill
```

according to the explicit admission contract.

It must not silently lose source information unless lossy behavior is explicitly allowed.

## Lossless Ingestion

A lossless ingestion contract means every accepted source record receives an explicit disposition:

```text
accepted and processed
rejected
failed
cancelled
```

No silent disappearance is permitted.

## Lossy Ingestion

A source may intentionally support:

```text
sampling
dropping
filtering
```

when the application explicitly requires it.

Lossy behavior becomes part of configuration and reproducibility when it affects analytical results.

## Filtering

Ingestion may filter records when the filtering rule is part of the ingestion/application contract.

For example:

```text
accept only records for session X
```

Filtering should be distinguishable from accidental data loss.

## Source Ordering

Source order is not automatically semantic order.

The ingestion contract must distinguish:

```text
source sequence
event sequence
ingestion order
processing order
```

where relevant.

## Out-of-Order Source Data

Ingestion may receive records out of order.

The contract must define whether it:

```text
accepts
buffers
reorders
rejects
```

the records.

It must not silently claim event-time ordering if none exists.

## Event Mapping

When normalized information becomes a domain event, the mapping must be explicit.

Conceptually:

```text
Normalized Record
        ↓
Domain Mapping
        ↓
Event
```

The domain owns the meaning of the resulting event.

## Event Creation

Ingestion may construct domain events.

However, it must satisfy the event contract:

```text
identity
timestamp
type
source
sequence
payload
```

where applicable.

## Ingestion and Event Immutability

Once a domain event has been created as historical information, it should not be mutated silently.

Corrections should use the appropriate correction/version/provenance mechanism.

## Ingestion and State

Ingestion should normally produce information consumed by state reconstruction.

It should not directly modify domain state behind the domain's projection contract.

Conceptually:

```text
Ingestion
    ↓
Events
    ↓
Projection
    ↓
State
```

## Ingestion and Measurement

Ingestion may normalize a source measurement.

However, it must distinguish:

```text
source measurement
```

from:

```text
EVolution-derived measurement
```

The latter requires an explicit metric definition.

## Ingestion and Analysis

Ingestion should not perform analytical interpretation merely because it can.

For example:

```text
source contains suspicious behavior
```

should not automatically become:

```text
Pattern detected
```

unless the ingestion component explicitly owns that analytical responsibility.

## Ingestion and Storage

Applications may choose to persist:

```text
raw source data
normalized records
events
rejected records
provenance
```

according to retention/source-of-truth requirements.

Ingestion itself does not determine physical storage technology.

## Raw Data Retention

Raw source retention may be valuable when:

```text
normalization may change
parser may improve
source interpretation may be corrected
reprocessing is required
```

Retention policy remains a storage/application concern.

## Reprocessing

If raw data is retained, ingestion may be rerun with:

```text
new parser version
new normalization configuration
new domain mapping
```

The resulting objects must remain distinguishable through provenance/versioning.

## Reprocessing and Identity

Reprocessing does not automatically mean that the newly generated object has the same identity.

Identity semantics depend on the relevant object contract.

For example:

```text
same source record
+
different normalization version
```

may produce:

```text
same logical object
```

or:

```text
new derived object
```

depending on the domain contract.

## Replay

Replay of already-normalized events differs from re-ingestion of raw source data.

```text
Raw Replay:
    raw source
    → parse
    → normalize
    → map

Event Replay:
    existing events
    → processing
```

These must not be conflated.

## Ingestion Determinism

Given the same:

```text
source data
parser version
normalization configuration
domain mapping version
relevant execution context
```

deterministic ingestion should produce semantically equivalent normalized results.

## Current Time

Ingestion must not silently use current time to fill missing source time.

Current time may be recorded separately as:

```text
ingestion_time
```

## Randomness

If ingestion uses randomness, such as sampling, the relevant randomness source must be explicit when reproducibility matters.

## Locale

Locale must not silently change semantic parsing.

For example:

```text
1,234
```

may have different meanings in different formats.

The source format or explicit configuration must define the interpretation.

## Timezone

Timezone interpretation must be explicit.

A timestamp without timezone information cannot automatically be treated as UTC unless the source/domain contract explicitly establishes that interpretation.

## External Source Reliability

Source reliability is context/provenance information, not automatically an analytical conclusion.

Ingestion may record source quality metadata where useful.

## Source Quality

Quality metadata may include:

```text
completeness
precision
temporal quality
validation status
source confidence
```

Such metadata must have explicit semantics.

It must not silently become analytical confidence.

## Ingestion Confidence

If ingestion assigns confidence to an inferred interpretation, the meaning must be explicit.

For example:

```text
timestamp estimated from sequence
```

may carry temporal estimation information.

This is distinct from confidence in a later analytical conclusion.

## Ingestion and Provenance

Every derived internal object should be capable of identifying its ingestion origin when provenance is required.

Conceptually:

```text
External Source
    ↓
Raw Record
    ↓
Parser
    ↓
Normalizer
    ↓
Domain Mapping
    ↓
Event
```

## Ingestion Provenance Chain

A provenance chain may identify:

```text
source
raw record
parser
normalizer
mapping
configuration
versions
timestamps
```

This supports backward tracing:

```text
Event
    ↓
Normalized Record
    ↓
Raw Record
    ↓
Source
```

and forward tracing:

```text
Raw Record
    ↓
Event
    ↓
Measurement
    ↓
Analysis
```

## Ingestion Identity

Ingestion component identity is distinct from:

```text
source identity
event identity
record identity
application identity
run identity
```

## Ingestion Run

A batch or streaming ingestion execution may have a run identity.

For example:

```text
IngestionRunId
```

This can group records processed during one execution.

It must not replace individual record/event identity.

## Ingestion Configuration and Reproducibility

If normalization behavior changes results, reproducibility requires:

```text
input
parser version
normalizer version
mapping version
effective configuration
relevant execution context
```

## Security

Ingestion is an untrusted-input boundary when data originates externally.

It should enforce appropriate:

```text
size limits
resource limits
encoding validation
parser safety
input validation
```

It must not assume that syntactically valid input is semantically safe.

## Resource Exhaustion

Oversized or malicious input must not silently cause:

```text
unbounded memory allocation
unbounded CPU consumption
unbounded buffering
```

Resource limits should follow the resource model.

## External Side Effects

Ingestion should avoid unintended external side effects.

For example:

```text
parsing a file
```

should not unexpectedly modify unrelated application state.

Side effects that are required by an ingestion workflow must be explicit.

## Testing

Ingestion tests should include:

```text
valid source
invalid syntax
invalid semantics
missing fields
unknown fields
duplicate records
out-of-order records
unknown timestamps
estimated timestamps
invalid encoding
oversized input
partial batches
source failures
reprocessing
```

## Golden Source Data

Representative source files/messages may be retained as golden fixtures.

Each fixture should identify:

```text
source format
version
expected normalization
expected temporal semantics
```

## Parser Contract Tests

Parser implementations supporting the same external format may share contract tests.

For example:

```text
ParserContractTests
    ├── ParserA
    └── ParserB
```

where both claim the same parsing contract.

## Normalizer Contract Tests

Normalization implementations may likewise share tests where the semantic normalization contract is common.

## Ingestion Integration Tests

Integration tests should verify:

```text
source
    ↓
acquisition
    ↓
parser
    ↓
normalizer
    ↓
domain mapping
    ↓
event
```

without requiring the entire application stack.

## Consequences

### Positive

* External formats remain isolated from Core.
* Raw, normalized, and semantic information remain distinguishable.
* Temporal uncertainty is preserved correctly.
* Reprocessing becomes possible without redefining historical semantics.
* Parser and normalizer versions can participate in reproducibility.
* Domain event creation remains explicit.
* Lossy ingestion cannot happen accidentally.

### Negative

* More explicit stages may require additional code and metadata.
* Maintaining raw source data may increase storage requirements.
* Normalization and domain mapping boundaries require careful design.
* Reprocessing can produce multiple semantic versions of derived data.

## Deferred Decisions

This ADR does not select:

```text id="h0f8p1"
file format
parser library
serialization format
message broker
streaming framework
ingestion framework
database
raw-data storage technology
schema registry
data lake
ETL framework
```

## Decision Summary

```text id="4n0d8f"
Ingestion:
    External → Internal boundary

Acquisition:
    Obtains source information

Parsing:
    Converts external representation

Structural Validation:
    Checks representation

Normalization:
    Removes source-specific representational differences

Semantic Validation:
    Checks meaning

Domain Mapping:
    Creates domain-specific information/events

Raw Data:
    Source-representative information

Normalized Data:
    Internal representation

Domain Event:
    Semantically meaningful historical fact

Event Time:
    KNOWN / ESTIMATED / UNKNOWN

Ingestion Time:
    Separate temporal dimension

Current Time:
    Must not silently replace missing event time

Deduplication:
    Explicit

Filtering:
    Explicit

Loss:
    Explicit

Provenance:
    Source → parser → normalizer → mapping

Replay:
    Distinct from re-ingestion

Storage:
    Selected independently

Physical format:
    Deferred
```

## Invariants

1. Ingestion is the boundary between external representations and EVolution-native information.
2. External representation must not define Core semantics.
3. Raw source data, normalized data, and domain events are distinct concepts.
4. Parsing and semantic interpretation are distinct responsibilities.
5. Structural validation must not be confused with domain validation.
6. Normalization must not silently invent semantic information.
7. Missing temporal information must not be replaced with current time.
8. Event time and ingestion time must remain distinct.
9. Known, estimated, and unknown temporal information must remain distinguishable.
10. Estimated temporal information must retain its estimated status.
11. Source identity and EVolution logical identity are distinct.
12. Source record identity must not automatically become EVolution object identity.
13. Deduplication must have explicit identity and scope semantics.
14. Duplicate source delivery and identical source content are distinct conditions.
15. Invalid source data must not silently become valid data.
16. Partial batch ingestion must explicitly represent accepted, rejected, failed, and cancelled records where relevant.
17. Lossy ingestion must be explicit.
18. Source ordering must not automatically become semantic event ordering.
19. Out-of-order behavior must be explicitly defined where ordering matters.
20. Domain events created by ingestion must satisfy the Event contract.
21. Ingestion must not directly bypass domain state/projection semantics.
22. Ingestion must not silently perform analytical interpretation.
23. Material parser, normalization, mapping, and configuration differences must participate in provenance/reproducibility.
24. Reprocessing must remain distinguishable from ordinary historical processing.
25. Ingestion resource limits must prevent uncontrolled resource consumption.
26. External input must be treated according to the relevant trust/security boundary.
27. Physical ingestion technologies remain replaceable implementation concerns.
28. Ingestion components must communicate failures through the established error model.
29. Raw-data retention is a storage/application decision, not an inherent requirement of ingestion.
30. Ingestion transforms external information into internal semantics but does not redefine the meaning of generic Core concepts.

## Invariant

> **EVolution treats ingestion as an explicit translation boundary from external representations to internal information: acquisition, parsing, validation, normalization, and domain mapping remain distinguishable responsibilities, while source identity, temporal knowledge, provenance, loss behavior, and reproducibility are preserved rather than hidden by the ingestion implementation.**
