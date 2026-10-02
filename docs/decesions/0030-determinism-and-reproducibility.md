# ADR 0030 — Determinism and Reproducibility

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution is intended to support:

* live processing
* replay
* batch analysis
* experiments
* historical reconstruction
* debugging
* comparison of analytical results
* recovery and checkpoint restoration

These capabilities require clear semantics around determinism and reproducibility.

Previous decisions establish that results may depend on:

```text
Input
Configuration
Component / Algorithm Version
Execution Context
State
```

Execution may also involve:

```text
concurrency
ordering
resource limits
backpressure
randomness
current time
external dependencies
recovery
sampling
```

Without an explicit model, the system could incorrectly treat two executions as equivalent when they were not.

## Decision

EVolution distinguishes **determinism** from **reproducibility**.

### Determinism

A component is deterministic when, under its declared semantic inputs and relevant execution conditions, it produces the same defined result.

### Reproducibility

An execution is reproducible when sufficient information exists to reconstruct the conditions required to reproduce its result.

Conceptually:

```text
Inputs
  +
Effective Configuration
  +
Component / Algorithm Version
  +
Relevant Execution Context
  +
Required State
  ↓
Deterministic Processing
  ↓
Result
```

Reproducibility does not require every physical detail of an execution to be identical.

It requires all **semantically relevant** inputs and conditions to be identified and preserved.

## Determinism Is a Contract

Determinism is not assumed merely because code appears pure.

A processor contract should identify whether its result is:

```text
DETERMINISTIC
CONDITIONALLY_DETERMINISTIC
NON_DETERMINISTIC
```

The exact representation is deferred.

## Deterministic Processor

For a deterministic processor:

```text
Output = F(Input, Configuration, Relevant Context, State, Version)
```

Given equivalent values for all semantically relevant inputs, the processor should produce equivalent results.

## Conditional Determinism

A processor may be deterministic only under specified conditions.

For example:

```text
same input
same configuration
same algorithm version
same ordering
same random seed
```

may be required.

Changing any of these may legitimately produce a different result.

## Non-Deterministic Processing

Some algorithms are inherently or intentionally non-deterministic.

Examples may include:

```text
randomized algorithms
sampling
concurrent race-dependent algorithms
external stochastic systems
```

Non-deterministic behavior must not be falsely represented as deterministic.

## Semantic vs Bitwise Determinism

EVolution distinguishes semantic equivalence from byte-for-byte equality.

Two executions may produce:

```text
same analytical meaning
```

while differing in:

```text
serialization
floating-point representation
ordering of independent records
memory layout
```

Therefore deterministic guarantees must state the level of equivalence intended.

## Determinism Levels

Conceptually, a component may provide:

```text
Semantic determinism
    same logical result

Structural determinism
    same logical structure/order

Representation determinism
    same serialized representation
```

A stronger guarantee implies additional constraints.

The default requirement is semantic determinism where the component contract declares determinism.

## Floating-Point Operations

Floating-point arithmetic can produce different results because of:

```text
operation ordering
compiler behavior
hardware instructions
parallel reduction order
```

Therefore numerical algorithms requiring reproducibility must define appropriate tolerance or deterministic reduction semantics.

Exact bitwise equality must not be assumed unless explicitly required.

## Ordering

Ordering is a major source of nondeterminism.

These are distinct:

```text
event order
domain sequence
input order
processing order
execution order
completion order
storage order
```

A processor must identify which ordering is semantically relevant.

## Concurrent Processing

Concurrency does not automatically imply nondeterminism.

A concurrent processor can remain deterministic if:

```text
partitioning
ordering
state ownership
merge operation
```

are deterministic.

Conversely, serial execution can still be nondeterministic if it depends on:

```text
randomness
current time
external state
```

## Partitioned Processing

For partitioned processors:

```text
PartitionKey(Input)
```

must be deterministic when partitioning affects results.

Partition assignment must not depend on:

```text
thread ID
memory address
scheduler choice
process-local object address
```

unless the resulting behavior is explicitly declared non-deterministic.

## Merge Operations

Parallel processing often produces partial results:

```text
Input
 ↓
Partition A → Result A
Partition B → Result B
Partition C → Result C
```

A merge operation can preserve determinism only when its mathematical and ordering properties permit it.

Associativity and commutativity should be explicit where relied upon.

## Randomness

Randomness must be explicit when it can affect results.

A processor should not silently use an uncontrolled global random generator.

Conceptually:

```text
Execution Context
{
    random_source
}
```

or:

```text
Configuration
{
    seed
}
```

may be used where appropriate.

## Random Seed

A seed may be sufficient to reproduce a pseudo-random execution if:

```text
algorithm
random generator
seed
random-consumption order
```

are also controlled.

A seed alone does not guarantee reproducibility.

## External Randomness

External stochastic systems may not be reproducible merely from EVolution's local configuration.

The external dependency must be treated as relevant execution context where necessary.

## Current Time

Current time is an implicit source of nondeterminism.

A deterministic processor should not silently depend on:

```text
system_clock::now()
```

for semantic processing.

Instead, relevant time should be supplied explicitly through:

```text
input
configuration
execution context
```

where appropriate.

## Monotonic Time

Monotonic clocks are appropriate for execution timing.

They generally should not become semantic historical inputs because their values are local execution measurements rather than portable historical timestamps.

## Event Time

Historical event time is semantic input.

Replay must preserve original event-time semantics.

A replay should not silently replace:

```text
event_time
```

with:

```text
replay_time
```

## Ingestion Time

Ingestion time may legitimately differ between executions.

If a processor's result depends on ingestion time, then ingestion timing becomes relevant execution/input information.

If it does not affect semantics, it need not affect analytical reproducibility.

## Processing Time

Processing time is normally execution metadata.

It becomes part of reproducibility only if processor semantics depend on it.

For example:

```text
rate-limit based on processing time
```

may make processing time semantically relevant.

## Execution Context

Execution context contains runtime conditions that materially affect processing.

Possible elements include:

```text
run identity
execution mode
time source
random source
resource limits
external dependency versions
environment capabilities
cancellation state
```

Not every runtime detail belongs in reproducibility.

## Relevant Context

The key distinction is:

```text
Relevant Context:
    materially affects semantic result

Incidental Context:
    does not affect semantic result
```

Reproducibility should capture relevant context without requiring the entire machine state to be serialized.

## Configuration

Effective configuration is part of reproducibility when it affects behavior.

The original source of configuration is not necessarily sufficient.

For example:

```text
config file
    ↓
defaults
    ↓
environment
    ↓
resolved configuration
```

The resolved effective configuration is what matters.

## Configuration Identity

A reproducible run should be able to identify the effective configuration.

Conceptually:

```text
ConfigurationId
```

may identify the configuration value/version.

The exact representation remains governed by ADR 0012.

## Component Version

The implementation/algorithm version must be identifiable when it can affect results.

For example:

```text
ProcessorVersion
AlgorithmVersion
LibraryVersion
```

may matter.

A source-code commit identifier may be useful, but the architecture does not require Git specifically.

## Dependency Versions

External dependencies can affect results.

Examples:

```text
database version
model version
parser version
numerical library version
```

Relevant dependency versions should therefore be captured when material.

## Input Identity

Reproducibility requires identifying the inputs.

Depending on the processing operation, this may mean:

```text
event identities
dataset identity
file identity
query definition
time range
graph input identity
```

The input identity must be stable enough to reconstruct the same logical input.

## Input Content vs Input Identity

An identity is not necessarily sufficient to reproduce content.

A persistent source may require:

```text
InputId
+
SourceVersion
```

or equivalent immutable content reference.

Reproducibility must ultimately identify the actual semantic input.

## State

State can affect processing results.

For stateful processors:

```text
Stateₙ₊₁ = F(Stateₙ, Inputₙ, ...)
```

Therefore reproducibility may require:

```text
initial state
```

or:

```text
complete event history required to reconstruct state
```

## State Snapshots

A snapshot may be used to reproduce or recover a state efficiently.

The snapshot must identify:

```text
processor version
configuration
state version
input/event position
```

where necessary.

A snapshot without compatible semantic context is not sufficient for reproducibility.

## Replay

Replay reconstructs processing from retained information.

A replay should preserve:

```text
event identity
event time
event sequence
required state
configuration
processor version
relevant execution context
```

where those values affect semantics.

## Replay Time vs Historical Time

Replay execution time is not historical event time.

For example:

```text
Historical event:
    2026-01-01 12:00 UTC

Replay:
    2026-10-03 15:00 UTC
```

The processor must not silently reinterpret the historical event as occurring at replay time.

## Batch Reproducibility

A batch result should identify:

```text
input dataset
effective configuration
algorithm version
relevant execution context
```

where applicable.

Changing batch boundaries can itself change results for windowed/incremental algorithms.

## Streaming Reproducibility

Streaming systems may depend on:

```text
arrival order
event-time order
watermarks
late data
window closure
```

Therefore reproducibility requires the stream-processing contract to define which of these are semantically relevant.

## Backpressure and Reproducibility

Admission behavior may affect which work is processed.

For example:

```text
DROP
SAMPLE
DEGRADE
```

can materially change results.

Therefore any admission policy that affects semantic input must be identifiable in reproducibility information.

A lossless BLOCK policy may affect timing without necessarily affecting semantic results.

## Queue Ordering

Queue ordering is not automatically semantic.

If a processor requires FIFO processing, queue behavior becomes relevant.

If independent inputs may be processed in any order, physical queue order may be incidental.

## Scheduler Determinism

The scheduler does not need to be globally deterministic unless scheduling order affects semantics.

For example:

```text
independent operations
```

may execute in different orders while producing the same result.

If completion order changes results, then ordering becomes a semantic contract and must be controlled or captured.

## Execution Backend

Changing execution backends should not change results when the processor contract promises deterministic semantics.

For example:

```text
single-threaded backend
```

and:

```text
thread-pool backend
```

should produce equivalent semantic output for a processor whose contract guarantees backend-independent determinism.

If they do not, the relevant execution behavior must be declared.

## Resource Conditions

Resource availability can affect results when resource pressure causes:

```text
drop
sample
degrade
timeout
```

Therefore resource configuration and relevant runtime resource conditions may become part of reproducibility.

Purely incidental differences such as:

```text
unused CPU capacity
```

do not need to be recorded if they cannot affect semantic output.

## Cancellation

Cancellation normally produces a different execution outcome.

A cancelled execution is not equivalent to a successfully completed execution.

Partial output must not be represented as a complete deterministic result.

## Failure

A failed execution is not a successful result.

Reproducibility should distinguish:

```text
successful result
failed execution
cancelled execution
```

A deterministic processor may deterministically fail for the same invalid input/configuration.

## Error Determinism

Where practical, deterministic components should produce the same error category/code for equivalent invalid conditions.

Human-readable diagnostic text does not need to be byte-for-byte identical.

## Recovery

Recovery may affect execution path without changing semantic output.

For example:

```text
failure
 ↓
checkpoint restore
 ↓
replay
 ↓
same result
```

If recovery changes the semantic input or configuration, however, the resulting execution is no longer equivalent.

## Retry

Retries can produce duplicate side effects.

A deterministic processor's pure analytical result may remain deterministic while external effects differ.

Therefore reproducibility of analytical output and reproducibility of external side effects are separate concerns.

## External Dependencies

External systems can make reproducibility difficult.

Examples:

```text
live database
network service
external API
market data source
remote model
```

If their state affects the result, reproducibility requires identifying or capturing the relevant external state.

## Snapshotting External Dependencies

An external dependency may require:

```text
snapshot
version
immutable dataset
recorded response
```

for true replay.

Simply recording the endpoint URL is not sufficient.

## Environment

The physical environment may affect results through:

```text
compiler
CPU architecture
floating-point behavior
library versions
filesystem behavior
locale
timezone
```

Only materially relevant environment properties should become reproducibility requirements.

## Locale and Timezone

Core temporal semantics use UTC.

Therefore locale/timezone should not silently affect historical analytical results.

If a domain explicitly uses calendar-local semantics, the relevant timezone/calendar must become explicit context.

## Serialization

Serialization can affect representation without necessarily affecting semantic meaning.

For reproducibility:

```text
semantic equality
```

is generally more important than:

```text
byte equality
```

unless a serialization format is itself part of the contract.

## Canonical Representation

Where byte-level reproducibility is required, the component must define canonical serialization.

This ADR does not select a serialization format.

## Hashes

Content hashes may be useful for identifying:

```text
dataset content
configuration content
serialized representation
algorithm artifact
```

A hash is not automatically the logical identity of the object.

This follows the identity model.

## Run Identity

Every reproducible execution may have a distinct run identity.

Conceptually:

```text
RunId
```

Run identity identifies an execution instance.

It is distinct from:

```text
EventId
ProcessorId
GraphId
ConfigurationId
AlgorithmVersion
```

Two runs can therefore use identical inputs/configuration while having different `RunId` values and equivalent results.

## Reproducibility Record

A run may produce a reproducibility record:

```text
ReproducibilityRecord
{
    run_id
    input_identity
    configuration_identity
    component_versions
    execution_context
    required_state
    relevant_admission_policy
    relevant ordering
    result_identity
}
```

This is conceptual.

The exact storage format is deferred.

## Reproducibility Levels

Not every execution needs the same guarantee.

Conceptually:

```text
LEVEL 0:
    no reproducibility guarantee

LEVEL 1:
    logical input/configuration identifiable

LEVEL 2:
    result reproducible under declared environment/context

LEVEL 3:
    deterministic semantic reproduction

LEVEL 4:
    canonical/bitwise reproduction
```

These levels are conceptual rather than a mandatory public enumeration.

A component should state the guarantee it provides.

## Reproducibility vs Repeatability

Repeatability means reproducing results under substantially the same environment.

Reproducibility can mean reconstructing results using recorded inputs/configuration/version/context.

The exact terminology may vary across domains.

EVolution focuses on explicit conditions rather than relying on terminology alone.

## Reproducibility vs Provenance

Provenance records where a result came from.

Reproducibility records enough information to reproduce it.

They overlap:

```text
Provenance
    ↓
origin / derivation

Reproducibility
    ↓
reconstructable conditions
```

A provenance record may be insufficient for reproduction if required execution context is missing.

## Reproducibility vs Audit

Audit answers questions about what happened and who/what performed an operation.

Reproducibility answers whether the result can be reconstructed.

They are related but distinct concerns.

## Reproducibility and Configuration Changes

A configuration change creates a materially different execution condition if the changed configuration affects semantics.

Therefore results should remain associated with the configuration used to produce them.

## Reproducibility and Algorithm Changes

Changing algorithm implementation may change results even if the public processor interface remains unchanged.

Algorithm/component version therefore participates in reproducibility where relevant.

## Reproducibility and Graph Changes

Changing:

```text
graph topology
processor configuration
connection semantics
```

can change results.

A graph definition/version should therefore be identifiable for reproducible graph executions.

## Reproducibility and Observability

Observability should normally not affect semantic output.

Therefore:

```text
logging enabled
tracing enabled
metrics exported
```

do not normally belong in the semantic reproducibility record.

If instrumentation materially changes execution semantics, the relevant condition must be captured.

## Reproducibility and Hardware

Hardware differences should not matter to semantic results unless the implementation contract allows hardware-dependent behavior.

Numerical or accelerator-based algorithms may require explicit hardware/environment constraints.

## Reproducibility and Parallelism

Parallelism is compatible with reproducibility when the algorithm defines deterministic ordering/merge semantics.

If result depends on race timing, the component is not deterministic under that execution model.

## Reproducibility Testing

Tests should explicitly verify declared determinism.

For deterministic components:

```text
same input
same configuration
same version
same relevant context
    ↓
equivalent result
```

Repeated execution should be used to detect accidental nondeterminism.

## Differential Testing

Two implementations or execution backends may be compared:

```text
Implementation A
       ↓
    Result A

Implementation B
       ↓
    Result B
```

The comparison must use the declared equivalence level.

## Replay Testing

Replay tests should verify:

```text
historical input
    ↓
original result

same historical input
    ↓
replay
    ↓
equivalent result
```

where the processor contract promises replay reproducibility.

## Reproducibility Failure

If required reproducibility information is missing, the system must not falsely claim that the result is reproducible.

The result may still be valid.

The distinction is:

```text
valid result
```

versus:

```text
reproducible result
```

## Reproducibility Metadata

Reproducibility metadata should be compact enough to retain with results where required.

Not every debug-level execution detail should become mandatory metadata.

## Consequences

### Positive

* Determinism becomes an explicit contract.
* Replay and historical analysis have clear requirements.
* Configuration and algorithm versions are properly connected to results.
* Randomness and current time become explicit dependencies.
* Parallel execution can remain compatible with deterministic semantics.
* The system can distinguish valid results from reproducible results.

### Negative

* Full reproducibility can require substantial metadata or snapshots.
* External dependencies can make exact reproduction difficult.
* Floating-point and hardware differences complicate numerical reproduction.
* Some inherently stochastic algorithms cannot provide deterministic guarantees.

## Deferred Decisions

This ADR does not select:

```text
reproducibility metadata format
run identifier representation
content hash algorithm
canonical serialization
random-number-generator implementation
snapshot format
environment capture mechanism
floating-point reproducibility strategy
distributed replay protocol
```

## Decision Summary

```text
Determinism:
    Explicit processor/algorithm contract

Reproducibility:
    Reconstruction of semantically relevant conditions

Required factors:
    Input
    Effective Configuration
    Component / Algorithm Version
    Relevant Execution Context
    Required State

Randomness:
    Explicit when semantically relevant

Current time:
    Explicit when semantically relevant

Ordering:
    Explicit when semantically relevant

Resource conditions:
    Relevant only when they affect semantics

Backpressure:
    Relevant when admission changes semantic input

Run identity:
    Distinct from object identity

Provenance:
    Origin/derivation

Reproducibility:
    Reconstruction conditions

Bitwise equality:
    Not assumed

Semantic equivalence:
    Default target for deterministic analytical processing
```

## Invariants

1. Determinism is a contract, not an assumption.
2. Reproducibility and determinism are distinct concepts.
3. A deterministic processor must define which inputs and execution conditions are semantically relevant.
4. Effective configuration must be identifiable when it affects results.
5. Component/algorithm version must be identifiable when it affects results.
6. Relevant execution context must be identifiable when it affects results.
7. Randomness must be explicit when it affects results.
8. Current time must not silently affect deterministic processing.
9. Historical event time must not be replaced by replay time.
10. Ordering must be explicit whenever it affects results.
11. Partitioning must be deterministic whenever it affects results.
12. Parallel execution must not silently introduce result nondeterminism where deterministic semantics are promised.
13. Resource pressure must not silently alter results.
14. Admission behavior that changes semantic input must be reproducible.
15. Cancellation and failure are not equivalent to successful completion.
16. A valid result may exist without being fully reproducible.
17. Provenance alone does not guarantee reproducibility.
18. Reproducibility does not require every incidental physical execution detail to be identical.
19. Semantic equivalence is the default deterministic guarantee; bitwise equality requires an explicit contract.
20. External dependencies that materially affect results must be identifiable or captured for reproducibility.
21. Reproducibility metadata must not silently become a substitute for the actual semantic input.
22. Observability should not alter semantic results.
23. A reproducibility claim must not be made when required reproducibility information is unavailable.

## Invariant

> **EVolution treats determinism as an explicit processing contract and reproducibility as the ability to reconstruct all semantically relevant inputs and execution conditions; incidental implementation details must not be required for reproduction unless they materially affect the result.**
