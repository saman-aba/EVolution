# ADR 0019 — Processing Graph Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution processing is intentionally modeled as a graph rather than as one fixed linear pipeline.

Previous decisions define the semantics of individual processors and their execution:

```text
Processor
    ↓
Execution
    ↓
Concurrency
    ↓
Work Admission
```

A complete analytical workflow, however, normally consists of multiple processors:

```text
A → B → C
```

or:

```text
        → B →
A →                 D
        → C →
```

The architecture therefore needs explicit semantics for:

* processor nodes
* connections
* inputs and outputs
* dependencies
* graph validation
* graph lifecycle
* graph state
* graph failures
* graph backpressure
* fan-in/fan-out
* cycles
* graph identity
* graph configuration

## Decision

EVolution represents composed processing workflows as an explicit **Processing Graph**.

A processing graph consists of:

```text id="ywh8oa"
Graph
 ├── Processor Nodes
 ├── Connections
 ├── Inputs
 ├── Outputs
 └── Graph Configuration
```

Conceptually:

```text id="9nq0ip"
Input
  ↓
[Processor A]
  ↓
[Processor B]
  ↓
[Processor C]
  ↓
Output
```

The graph is responsible for composition and execution relationships.

Processors remain responsible for their own transformation semantics.

The graph must not redefine the meaning of individual processor inputs or outputs.

## Graph Node

A processor instance participating in a graph is represented as a node.

Conceptually:

```text id="l8p2eg"
Node
{
    identity
    processor
    configuration
    inputs
    outputs
}
```

The node identity identifies the processor instance within the graph.

It is distinct from:

```text id="5nxx4p"
processor type
processor implementation
processor object memory address
```

Multiple nodes may use the same processor implementation:

```text id="0v49a1"
Parser A → Parser
Parser B → Parser
Parser C → Parser
```

Each is a distinct graph node.

## Graph Identity

A processing graph has logical identity.

Conceptually:

```text id="1wpgvl"
ProcessingGraph
{
    id
    version?
    nodes
    connections
    configuration
}
```

Graph identity is independent of:

* memory address
* process ID
* thread ID
* storage location
* execution instance

Graph identity follows the identity model defined by ADR 0010.

## Processor Identity vs Node Identity

A processor implementation and its graph node are different concepts.

For example:

```text id="l5e3ko"
Processor Type:
    "OHLC Aggregator"

Graph:
    Node A → OHLC Aggregator
    Node B → OHLC Aggregator
```

The implementation/type identifies what behavior is provided.

The node identifies one particular instance of that behavior within the graph.

## Connection

A connection defines a data dependency between nodes.

Conceptually:

```text id="o5b6ud"
Connection
{
    source
    source_output
    destination
    destination_input
    contract
}
```

Example:

```text id="6q07cw"
A.output
    ↓
B.input
```

A connection is not merely a queue.

It defines:

* source
* destination
* compatible data contract
* delivery/admission boundary
* ordering requirements where relevant

The physical transport mechanism is implementation-defined.

## Typed Connections

Connections must be semantically compatible.

For example:

```text id="s3qjvw"
Event
    ↓
EventProcessor
```

may be valid while:

```text id="e0vyn5"
Analysis
    ↓
RawEventProcessor
```

may be invalid.

Compatibility includes more than C++ type compatibility.

It may include:

```text id="c4l2f9"
payload type
cardinality
temporal semantics
ordering
identity semantics
missing-data semantics
provenance requirements
```

## Graph Validation

A graph should be validated before activation.

Validation should establish at least:

```text id="l3i9rc"
node identity validity
connection validity
input/output compatibility
required inputs
graph structure
cycle validity
configuration validity
```

Invalid graphs must not enter normal execution.

Conceptually:

```text id="c4e7a6"
Graph Definition
      ↓
Validation
      ↓
Valid
      ↓
Initialization
      ↓
Activation
```

or:

```text id="7p2f4a"
Graph Definition
      ↓
Validation
      ↓
Error
```

## Graph Lifecycle

The graph has a lifecycle that coordinates its processors.

Conceptually:

```text id="6i1cws"
CREATED
    ↓
CONFIGURED
    ↓
VALIDATED
    ↓
INITIALIZED
    ↓
ACTIVE
    ↓
STOPPING
    ↓
STOPPED
```

Graph lifecycle is not necessarily identical to processor lifecycle.

The graph coordinates processors but does not replace their individual lifecycle state.

## Graph Activation

A graph may only become `ACTIVE` when all required processors and connections satisfy their activation requirements.

Conceptually:

```text id="h5ghg1"
Graph
  ├── Processor A ACTIVE
  ├── Processor B ACTIVE
  └── Processor C ACTIVE
          ↓
      Graph ACTIVE
```

If required initialization fails, the graph must not report successful activation.

Optional components may have different semantics if the graph contract explicitly permits degraded operation.

## Graph Shutdown

Graph shutdown follows processor shutdown semantics.

For orderly stop:

```text id="b5q3ks"
Graph ACTIVE
    ↓
Graph STOPPING
    ↓
stop processors
    ↓
drain connections
    ↓
Graph STOPPED
```

For cancellation:

```text id="w2v8bj"
Graph ACTIVE
    ↓
Graph STOPPING
    ↓
cancel processors
    ↓
Graph STOPPED
```

For abort:

```text id="kjd0ck"
Graph ACTIVE
    ↓
Graph FAILED
```

The graph must coordinate shutdown without violating individual processor contracts.

## Dependency Ordering

Processor dependencies establish execution constraints.

For:

```text id="84eywh"
A → B
```

B depends on data produced by A.

The graph execution system must ensure that the connection semantics are respected.

This does not necessarily mean B and A execute on different threads or in separate processes.

The dependency is semantic, not a threading decision.

## Fan-Out

A processor may produce output consumed by multiple processors:

```text id="7x1tdk"
             → B
A → output →
             → C
```

The graph must define whether the output is:

```text id="brp85h"
copied
shared
referenced
multicast
independently materialized
```

The physical mechanism remains deferred.

Each destination receives data according to the connection contract.

## Fan-In

Multiple processors may feed one processor:

```text id="ez8gkh"
A ──→
      \
       → D
      /
B ──→
```

Fan-in requires explicit semantics for:

* ordering
* synchronization
* correlation
* missing inputs
* partial availability
* termination

The receiving processor must not assume that inputs from different connections are naturally synchronized.

## Correlation

When multiple inputs represent related information, correlation must use explicit identity or correlation metadata.

For example:

```text id="w85vry"
A.output ──┐
           ├──→ D
B.output ──┘
```

D may need to determine which A and B outputs correspond to one another.

Correlation is distinct from:

```text id="d8l6nt"
logical identity
processing sequence
execution order
```

## Multiple Inputs

A processor may require:

```text id="0g6h3v"
Input A
Input B
Input C
```

The graph contract must define whether those inputs are:

```text id="xq1t9p"
independent
correlated
synchronized
optional
required
```

The graph must not invent synchronization semantics that the processor contract does not define.

## Fan-Out and Failure

If one destination fails:

```text id="2s0t0p"
A
├──→ B
└──→ C
```

failure of B does not automatically imply failure of C.

Graph-level failure policy must determine whether:

```text id="j9j5tg"
B failure
    ↓
continue C
```

or:

```text id="qf7h5v"
B failure
    ↓
stop graph
```

or another policy is appropriate.

There is no universal propagation policy.

## Failure Domains

Graphs should distinguish:

```text id="k1pxp0"
operation failure
processor failure
connection failure
graph failure
```

A local failure should not automatically become a graph failure unless the graph contract requires it.

For example:

```text id="x39z8d"
Input
  ↓
Processor A
  ↓ Error(InvalidInput)
Processor A remains ACTIVE
```

does not require the graph to fail.

By contrast:

```text id="2sv1t6"
Processor B state corruption
  ↓
B → FAILED
  ↓
graph cannot satisfy required dependency
  ↓
Graph → FAILED
```

may require graph termination.

## Backpressure Between Nodes

Each connection represents an admission boundary.

For:

```text id="5d0v0w"
A → B
```

A's output must not automatically imply that B can accept unlimited data.

The connection therefore participates in:

```text id="f5pxa7"
capacity
admission
buffering
ordering
loss
cancellation
shutdown
```

This follows ADR 0018.

## Connection Failure

Connections may fail independently of processor logic.

Examples include:

```text id="4e7ikd"
buffer exhaustion
transport failure
serialization failure
storage failure
external IPC failure
```

The graph must distinguish connection failure from processor failure.

The resulting propagation policy is graph-specific.

## Cycles

A processing graph may contain cycles only when the execution semantics explicitly support them.

For example:

```text id="8q5dkn"
A → B → C
    ↑     │
    └─────┘
```

Cycles introduce requirements for:

* buffering
* termination
* feedback semantics
* convergence
* deadlock avoidance
* state handling

Therefore the default graph model does **not** assume arbitrary cycles are valid.

A graph containing a cycle must explicitly declare that the cycle is supported and define its execution semantics.

## Acyclic Default

The default graph structure is a directed acyclic graph:

```text id="1r7vdy"
DAG
```

This simplifies:

* validation
* dependency analysis
* scheduling
* lifecycle ordering
* replay
* reasoning about completion

Cycles remain possible as a future explicit graph capability.

## Graph Inputs and Outputs

A graph may expose external inputs and outputs.

Conceptually:

```text id="d4cqg9"
External Input
      ↓
   Graph Input
      ↓
   Processors
      ↓
   Graph Output
      ↓
External Output
```

Graph inputs and outputs are explicit boundaries.

A graph should not require callers to know internal node identities merely to submit or consume normal graph data.

## Graph Configuration

Graph configuration defines composition.

Examples include:

```text id="e2s9yy"
nodes
connections
processor configurations
admission policies
graph-level execution mode
optional-node policy
```

Processor-specific configuration remains owned by the processor.

Graph configuration must not silently override processor semantics.

## Graph Version

A graph definition may be versioned.

Changing:

```text id="2q4s3v"
nodes
connections
processor versions
configuration
```

may create a different executable graph definition.

Graph versioning is distinct from:

```text id="l3n4q6"
graph execution instance
```

An execution instance records one actual run of a graph definition.

## Graph Execution Instance

A graph definition may execute multiple times:

```text id="2gq7jz"
Graph Definition v3
    ├── Run 001
    ├── Run 002
    └── Run 003
```

Each execution may have its own:

```text id="yq0r2f"
run identity
execution context
state
provenance
outputs
lifecycle
```

This follows the configuration and execution-context decisions.

## Graph and Provenance

The graph itself participates in provenance.

A derived output should be able to identify:

```text id="b2w1sm"
graph identity/version
node
processor version
configuration
input
execution context
```

This allows analytical results to be traced through the processing graph.

## Graph and Replay

A graph should be replayable when its processors and execution context support deterministic replay.

Conceptually:

```text id="e3t6ny"
Historical Inputs
      ↓
Graph Definition
      ↓
Replay
      ↓
Derived Outputs
```

Replay must preserve relevant historical temporal semantics.

The graph must not silently replace event time with current processing time.

## Graph Mutation

A running graph must not be structurally modified implicitly.

Changing:

```text id="8v0qpg"
nodes
connections
processor semantics
```

during active execution would make reproducibility and lifecycle semantics ambiguous.

Therefore graph topology is immutable while the graph is active.

A future dynamic graph mechanism may explicitly support controlled topology changes.

## Dynamic Graphs

Dynamic graph modification is deferred.

If supported later, it must define:

* topology versioning
* migration of in-flight work
* state ownership
* connection draining
* lifecycle semantics
* provenance
* failure behavior

No implicit dynamic topology is permitted.

## Consequences

### Positive

* Processing composition becomes explicit.
* Graph topology is separate from processor implementation.
* Fan-in/fan-out have clear architectural boundaries.
* Graph-level and processor-level failures remain distinguishable.
* Backpressure naturally has a defined location at connections.
* DAG execution provides a predictable default.
* Graph definitions can be versioned and replayed.

### Negative

* Graph validation becomes a distinct responsibility.
* Fan-in and fan-out require additional execution semantics.
* Failure propagation requires explicit graph policy.
* Cyclic processing requires substantially more machinery.

## Deferred Decisions

This ADR does not select:

```text id="o2y4p5"
graph scheduler
queue implementation
thread assignment
executor
serialization
IPC
distributed graph execution
dynamic topology implementation
graph persistence format
```

## Decision Summary

```text
Graph:
    Explicit processing composition

Node:
    Processor instance within graph

Connection:
    Explicit data dependency and admission boundary

Default topology:
    Directed acyclic graph

Cycles:
    Explicitly supported only with defined semantics

Fan-out:
    Supported

Fan-in:
    Supported

Ordering:
    Explicit

Correlation:
    Explicit

Graph lifecycle:
    Coordinates processor lifecycles

Graph failure:
    Explicit propagation policy

Backpressure:
    Connection-level concern

Graph topology:
    Immutable while active

Graph definition:
    Versionable

Graph execution:
    Separate execution instance

Provenance:
    Graph and node participation required where material
```

## Invariants

1. A processing graph explicitly represents processor composition.
2. A node identifies a processor instance within a graph.
3. Node identity is distinct from processor implementation identity.
4. Connections explicitly define data dependencies.
5. Connection compatibility includes semantic, not merely type, compatibility.
6. Graphs are validated before activation.
7. The default graph topology is acyclic.
8. Cycles require explicit support and termination semantics.
9. Fan-in synchronization is never assumed implicitly.
10. Fan-out delivery semantics must be explicit.
11. Domain identity and correlation are distinct.
12. Processor failure and graph failure are distinct.
13. Local operation failure does not automatically terminate the graph.
14. Connection boundaries participate in backpressure.
15. Active graph topology is immutable.
16. Graph definition identity is distinct from graph execution identity.
17. Graph configuration is distinct from processor configuration.
18. Graph lifecycle must respect processor lifecycle contracts.
19. Graph execution must preserve relevant provenance.
20. Graph topology must not silently change the meaning of processor inputs or outputs.

## Invariant

> **EVolution represents processing composition as an explicit graph whose nodes own processor semantics and whose connections define compatible data dependencies, while graph execution coordinates lifecycle, admission, ordering, and failure without redefining the meaning of individual processors.**
