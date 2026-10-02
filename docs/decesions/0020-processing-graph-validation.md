# ADR 0020 — Processing Graph Validation

**Status:** Accepted
**Date:** 2026-10-03

## Context

ADR 0019 defines EVolution processing as an explicit graph of processor nodes and connections.

A graph can be structurally incorrect or semantically incompatible before execution begins.

Examples include:

```text
missing input
invalid connection
incompatible payload types
duplicate node identity
invalid processor configuration
unsupported cycle
ambiguous fan-in
invalid output contract
```

These errors should be detected before normal processing begins whenever they can be determined statically.

Graph validation therefore needs to be a distinct architectural responsibility.

## Decision

EVolution validates a processing graph through explicit validation stages before activation.

The conceptual lifecycle is:

```text
Graph Definition
      ↓
Structural Validation
      ↓
Contract Validation
      ↓
Configuration Validation
      ↓
Execution Validation
      ↓
VALID
      ↓
Initialization
      ↓
Activation
```

A graph must not become `ACTIVE` unless all required validation has succeeded.

Validation produces structured `Result`/`Error` information rather than relying on assertions for user/configuration mistakes.

## Validation Responsibilities

Graph validation is responsible for determining whether the graph can be meaningfully constructed and executed.

It validates:

```text
node structure
connection structure
processor contracts
configuration
graph topology
required execution capabilities
```

It does not execute normal processing.

It does not evaluate analytical results.

It does not prove that arbitrary runtime input will always be valid.

## Structural Validation

Structural validation checks the graph itself.

Examples:

```text
duplicate node identity
missing node
missing connection endpoint
duplicate connection
invalid graph identity
invalid graph configuration
```

For example:

```text
A → B
```

is invalid if `B` does not exist.

Structural validation should occur before deeper semantic validation.

## Node Identity

Every graph node requiring persistent graph identity must have a unique identifier within the graph.

For example:

```text
Node A
Node B
Node C
```

is valid.

```text
Node A
Node A
```

is invalid unless the graph format explicitly represents these as different identities.

Node identity must not depend on:

```text
memory address
container position
thread identity
execution order
```

## Connection Validation

Every connection must reference valid source and destination nodes.

Conceptually:

```text
source.output
      ↓
destination.input
```

Both endpoints must exist.

A connection to a nonexistent output or input is invalid.

## Duplicate Connections

Duplicate connections are not implicitly merged.

For example:

```text
A.output → B.input
A.output → B.input
```

must either:

* be explicitly permitted by the input contract, or
* produce a validation error.

The graph validator must not silently deduplicate them because doing so could change processing semantics.

## Contract Validation

After structural validity is established, the validator checks processor contracts.

This includes:

```text
input type
output type
cardinality
temporal semantics
ordering
identity
provenance
missing-data behavior
```

A connection is valid only when the source output satisfies the destination input contract.

## Type Compatibility

Type compatibility is necessary but not sufficient.

For example:

```text
Measurement<float>
```

and:

```text
Measurement<float>
```

may have compatible representations while having incompatible semantic units:

```text
milliseconds
seconds
```

The validator must therefore distinguish:

```text
representation compatibility
semantic compatibility
```

Where semantic compatibility cannot be established statically, the contract must explicitly identify the runtime validation requirement.

## Cardinality Validation

The graph must respect declared cardinality.

Examples:

```text
one → one
one → many
many → one
many → many
```

A processor requiring exactly one input cannot be connected to an output that can produce zero or many values unless the contract provides an explicit adaptation.

The validator must not silently invent such adaptation.

## Temporal Validation

Connections must preserve temporal semantics.

For example:

```text
Event Time
```

must not silently become:

```text
Processing Time
```

merely because the receiving processor operates asynchronously.

If a processor requires event time and the input does not provide it according to its contract, the graph is invalid or the processor must explicitly support the missing/estimated temporal state.

## Ordering Validation

If a destination processor requires ordered input, the connection must provide a compatible ordering guarantee.

For example:

```text
Destination:
    requires event sequence ordering

Connection:
    explicitly unordered
```

is incompatible.

The validator should detect this when the information is available statically.

## Identity Validation

If a processor requires stable logical identity, the connected input contract must provide the required identity semantics.

For example:

```text
Processor:
    requires EventId

Input:
    anonymous transient values
```

cannot be assumed compatible.

Identity semantics follow ADR 0010.

## Provenance Validation

If a processor requires provenance, the graph must ensure that the connection preserves the required provenance information.

A connection must not silently discard provenance required by downstream processing.

## Configuration Validation

Processor configuration must be validated as part of graph preparation.

Conceptually:

```text
Node
  ↓
resolve configuration
  ↓
validate configuration
  ↓
effective configuration
```

Configuration errors are explicit errors.

Examples:

```text
InvalidConfiguration
Unsupported
InvalidState
```

The exact error code depends on the failure.

## Graph Configuration vs Processor Configuration

The validator distinguishes:

```text
Graph Configuration
```

from:

```text
Processor Configuration
```

Graph configuration describes composition:

```text
nodes
connections
graph-level policies
```

Processor configuration describes processor behavior:

```text
threshold
window size
metric
algorithm
```

Graph configuration must not silently override processor configuration.

## Topology Validation

The graph topology must be checked.

The default topology requirement is:

```text
directed acyclic graph
```

Therefore the validator must detect cycles.

Example:

```text
A → B → C
    ↑   |
    └───┘
```

is invalid under the default DAG model.

A future graph explicitly supporting cycles would require a different validation mode.

## Reachability

The validator should identify nodes that cannot participate in the graph's declared execution.

Examples:

```text
isolated node
unreachable processor
output with no consumer
required input with no producer
```

Whether an unused node is an error depends on the graph contract.

The important distinction is:

```text
structurally unused
```

versus:

```text
intentionally terminal
```

A graph must not silently execute nodes that have no meaningful path from a declared input.

## Graph Inputs

Graph input declarations must be validated.

For each required graph input:

```text
input identity
input type
input cardinality
input temporal semantics
input ordering
```

must be defined.

A required processor input that has neither:

```text
graph input
```

nor:

```text
upstream connection
```

is invalid.

## Graph Outputs

Graph outputs must be explicitly declared when the graph exposes results externally.

Validation checks:

```text
output identity
source node
source output
output contract
```

A processor producing output does not automatically mean that output is a graph output.

## Fan-In Validation

Fan-in requires explicit semantics.

For:

```text
A ──→
      \
       → D
      /
B ──→
```

the validator must determine whether D expects:

```text
independent inputs
correlated inputs
synchronized inputs
```

If the contract requires synchronization but no synchronization semantics are available, the graph is invalid.

## Fan-Out Validation

Fan-out is valid when the source output contract permits multiple consumers.

The validator must ensure that delivery semantics are compatible with each destination.

For example:

```text
A
├──→ B
├──→ C
└──→ D
```

may require different transformations or delivery guarantees.

Fan-out must not silently change ownership or lifetime semantics.

## Adapter Processors

When two contracts are incompatible, the graph must not silently insert conversion logic.

For example:

```text
A → B
```

where A produces:

```text
seconds
```

and B requires:

```text
milliseconds
```

must either be:

```text
A → ConversionProcessor → B
```

or explicitly supported by a declared compatibility rule.

Implicit semantic transformations are prohibited.

## Validation Levels

Validation is divided conceptually into:

```text
STATIC
INITIALIZATION
RUNTIME
```

### Static

Can be determined from the graph definition alone.

Examples:

```text
duplicate node ID
missing endpoint
cycle
invalid topology
obvious type mismatch
```

### Initialization

Requires instantiated processors or resolved configuration.

Examples:

```text
unsupported processor capability
resource requirement
external dependency availability
runtime configuration validity
```

### Runtime

Depends on actual input or execution conditions.

Examples:

```text
invalid event payload
unexpected runtime state
resource exhaustion
external service failure
```

Runtime validation cannot be eliminated by graph validation.

## Validation Does Not Prove Correctness

A valid graph means:

> The graph satisfies the architectural and declared contract constraints known at validation time.

It does not mean:

```text
all runtime inputs are valid
all external dependencies will remain available
all analyses are correct
all results are meaningful
```

Validation is therefore not a correctness proof.

## Validation Errors

Validation errors must use the Result/Error model.

Conceptually:

```text
validate(graph)
    ↓
Result<ValidatedGraph>
```

Possible errors include:

```text
InvalidInput
InvalidConfiguration
InvalidState
Unsupported
```

The error should identify the affected graph component where useful.

For example:

```text
node = "measurement_aggregator"
input = "price"
reason = "incompatible temporal semantics"
```

The exact diagnostic structure remains deferred.

## Validation and Lifecycle

Validation occurs before initialization/activation.

Conceptually:

```text
CREATED
   ↓
CONFIGURED
   ↓
VALIDATED
   ↓
INITIALIZED
   ↓
ACTIVE
```

`VALIDATED` is treated as a graph preparation stage rather than a new processor lifecycle state.

The processor lifecycle defined by ADR 0014 remains unchanged.

## Validation and Immutability

Once a graph has been successfully validated, its topology must not be modified without invalidating the validation result.

Therefore:

```text
validated graph
    +
topology mutation
    ↓
validation no longer valid
```

A modified graph must be validated again.

## Validation and Versioning

Validation should be associated with:

```text
graph definition version
processor version
effective configuration
```

If any material component changes, previous validation must not automatically be reused.

## Validation and Provenance

Where graph execution results depend on validation, provenance may record:

```text
graph identity/version
validation result
processor versions
effective configuration
```

This supports reproducibility and diagnosis.

## Validation and External Dependencies

Some requirements cannot be determined from graph structure alone.

For example:

```text
database available
storage capacity
external service reachable
required hardware capability
```

These are initialization/execution concerns.

The validator may establish that a dependency is required, while initialization determines whether it is currently available.

## Validation and Resource Requirements

The graph may declare resource requirements such as:

```text
memory
CPU
storage
external connections
```

Validation may determine whether the requirements are structurally coherent.

Actual resource availability may be checked during initialization.

The architecture must not confuse:

```text
required resource
```

with:

```text
currently available resource
```

## Validation and Backpressure

Backpressure policies are part of graph execution configuration.

Validation must detect incompatible combinations when statically identifiable.

For example, a graph claiming:

```text
LOSSLESS
```

while explicitly configuring:

```text
DROP
```

at a required lossless boundary is invalid.

## Validation and Concurrency

Concurrency declarations must also be compatible.

For example, a processor requiring:

```text
SERIAL
```

cannot be configured through a graph execution policy that requires unrestricted concurrent invocation of that processor.

The graph execution system may serialize that node, but it must not violate the processor contract.

## Validation and Determinism

If a graph is declared deterministic, validation should ensure that known configuration choices do not introduce explicitly forbidden nondeterministic behavior.

However, runtime scheduling cannot necessarily be proven deterministic during static validation.

Determinism remains a processor/execution contract.

## Consequences

### Positive

* Invalid graphs fail before normal execution.
* Structural and semantic errors are separated from runtime failures.
* Implicit semantic conversions are prevented.
* Graph configuration becomes reproducible and inspectable.
* Processor contracts can be checked before scheduling.
* Execution infrastructure receives a validated graph rather than discovering basic errors during processing.

### Negative

* Validation itself becomes a substantial subsystem.
* Some validation can only happen during initialization or runtime.
* Rich contracts require richer validation metadata.
* Dynamic graphs require repeated validation.

## Deferred Decisions

This ADR does not define:

```text
exact validation API
validation framework
graph serialization format
graph editor
schema language
runtime scheduling
dependency injection mechanism
resource scheduler
distributed graph validation
```

## Decision Summary

```text
Validation:
    Explicit graph responsibility

Required before:
    Activation

Validation levels:
    STATIC
    INITIALIZATION
    RUNTIME

Checks:
    Structure
    Node identity
    Connections
    Contract compatibility
    Configuration
    Topology
    Ordering
    Temporal semantics
    Identity
    Provenance
    Concurrency compatibility
    Backpressure compatibility

Default topology:
    DAG

Implicit adapters:
    Not allowed

Validation result:
    Explicit Result/Error

Topology mutation:
    Invalidates previous validation

Runtime validation:
    Still required where static validation is insufficient
```

## Invariants

1. A graph must be structurally valid before activation.
2. A graph must satisfy declared processor and connection contracts.
3. Duplicate node identities are invalid.
4. Invalid connection endpoints are invalid.
5. Semantic compatibility is required in addition to representation/type compatibility.
6. Required temporal semantics must be preserved.
7. Required ordering guarantees must be preserved.
8. Required identity semantics must be preserved.
9. Required provenance must not be silently discarded.
10. The default graph topology must be acyclic.
11. Implicit semantic conversion is not permitted.
12. Required graph inputs must have valid producers.
13. Validation errors use explicit `Result`/`Error` semantics.
14. Validation does not replace runtime validation.
15. Topology changes invalidate previous validation.
16. Material processor/configuration changes invalidate dependent validation.
17. Graph configuration and processor configuration remain distinct.
18. Validation must not execute normal analytical processing.
19. Resource requirements and resource availability remain distinct.
20. Graph validation must not redefine processor semantics.

## Invariant

> **A processing graph must satisfy its declared structural and semantic contracts before activation; validation detects what can be known before execution, while runtime validation remains responsible for conditions that depend on actual inputs or execution environment.**
