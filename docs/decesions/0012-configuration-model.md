## Configuration and Execution Context

Configuration and execution context are related but distinct.

### Configuration

**Configuration defines how a component is intended to behave.**

Examples:

```text id="j2l8k4"
window_size = 100
threshold = 0.75
aggregation = "mean"
processing_mode = "replay"
```

Configuration is an explicit input to the component.

Conceptually:

```text id="6x7v0m"
Component
    +
Configuration
    ↓
Configured behavior
```

### Execution Context

**Execution context describes the runtime conditions under which that configured behavior is executed.**

Examples include:

```text id="x5n4qr"
execution mode
current/replay time source
randomness source
resource limits
execution environment
component dependencies
run identity
cancellation state
```

Conceptually:

```text id="0c6y4e"
Configuration
      +
Execution Context
      +
Input
      ↓
Execution
      ↓
Result
```

Execution context therefore describes **how and where an already-defined configuration is being executed**, rather than defining the component's domain behavior itself.

### Configuration vs Execution Context

The distinction can be summarized as:

```text id="4qv7sa"
Configuration
    → What behavior should the component use?

Execution Context
    → Under what runtime conditions is that behavior executed?
```

For example:

```text id="n8f2pj"
Configuration:
    window_size = 100
    threshold = 0.75

Execution Context:
    mode = REPLAY
    time_source = historical replay clock
    run_id = R123
```

The configuration defines the analytical behavior.

The execution context defines the conditions under which that behavior is run.

### Execution Context Is Not Configuration

Runtime conditions must not be silently converted into configuration.

For example:

```text id="q4m8ty"
current system time
CPU architecture
thread count
memory availability
process ID
run identifier
```

are not automatically configuration values.

Likewise, configuration must not be inferred from arbitrary runtime state.

For example:

```text id="w7v3ks"
CPU count = 16
    ↓
window_size = 16
```

would introduce an implicit configuration dependency unless explicitly defined by the component contract.

### Execution Context Is Not State

Execution context is also distinct from component state.

```text id="8r2d1m"
Configuration
    → intended behavior

Execution Context
    → conditions of execution

State
    → accumulated/runtime information produced while executing
```

For example:

```text id="x0k5qy"
Configuration:
    window_size = 100

Execution Context:
    mode = REPLAY

State:
    current_window_count = 73
```

The current window count is state, not configuration and not execution context.

### Execution Context and Time

The Time Model defines several temporal concepts.

Execution context may provide the time source used by a component.

For example:

```text id="1y8p6z"
LIVE execution
    → system/realtime clock

REPLAY execution
    → historical/replay clock

TEST execution
    → deterministic fixed clock
```

The component should depend on an explicit time-source abstraction where current execution time affects behavior.

It must not silently call the host system clock when deterministic behavior is required.

Therefore:

```text id="3b6n8v"
Time Source
    → execution context

Timestamp
    → data / event / measurement

Duration
    → temporal value
```

These remain separate concepts.

### Execution Context and Randomness

If a component uses randomness, the source of randomness belongs to execution context when it represents a runtime facility.

For example:

```text id="g0w2dz"
Execution Context
    random source
        ↓
Component
```

The configuration may specify intended randomization behavior:

```text id="5c8p1q"
randomization_algorithm = X
```

while execution context provides the actual source or seed:

```text id="9h3m4k"
random source
seed
run identity
```

The exact randomness architecture is deferred.

If randomness materially affects a result, the relevant information must be available for reproducibility.

### Execution Context and Environment

The execution environment is part of the broader execution context when it can materially affect behavior.

Examples:

```text id="v2r6s8"
operating system
CPU architecture
available hardware capabilities
library versions
external service versions
locale
timezone
resource limits
```

However, not every environmental property must be captured.

The relevant rule is:

> **Environment information belongs in execution context or provenance when it can materially affect the behavior or interpretation of the result.**

The system should avoid recording arbitrary environmental information merely for completeness.

### Execution Context and Resources

Resources available to a component may form part of execution context.

Examples:

```text id="6z4p1n"
memory limit
CPU allocation
worker count
I/O availability
storage endpoint
network availability
```

Resource availability does not necessarily determine analytical semantics.

For example:

```text id="7s3k5q"
available CPUs = 16
```

should not change the analytical result merely because more CPUs are available, unless the component explicitly defines behavior that depends on it.

Resource information is primarily relevant to execution, diagnostics, reproducibility, and performance analysis.

### Execution Context and Processing Mode

Processing mode is an execution-context concern when it describes how an already-configured component is being run.

Examples:

```text id="8f4m2x"
LIVE
REPLAY
BATCH
EXPERIMENT
```

A processor may therefore have:

```text id="1a9k7c"
Configuration:
    threshold = 0.75

Execution Context:
    mode = REPLAY
```

The same configuration can be executed in different modes.

If the mode changes the semantic result, that fact must be explicit in the component contract and provenance.

### Execution Context and Run Identity

An execution context may have a **run identity**.

For example:

```text id="q6p2w9"
RunId = R12345
```

A run identifies a particular execution instance.

This is distinct from:

```text id="r3d8x1"
ConfigurationId
ComponentVersion
Input identity
Output identity
```

A run may use the same configuration and inputs as another run while remaining a separate execution.

Run identity is particularly useful for:

* reproducibility
* provenance
* diagnostics
* experiment tracking
* comparing executions

The exact `RunId` representation is deferred.

### Execution Context and Determinism

For a deterministic component, the relevant execution context must either:

1. be controlled so that repeated execution produces equivalent results, or
2. be explicitly treated as an input to the result.

Conceptually:

```text id="5u7j2p"
Input
+
Configuration
+
Component Version
+
Relevant Execution Context
    ↓
Result
```

Therefore, the earlier reproducibility model is refined to:

```text id="q8m4v1"
Result reproducibility
    depends on
        Input
        Configuration
        Component Version
        Relevant Execution Context
```

Not every execution-context attribute necessarily affects the result.

For example, a processor may produce identical analytical results regardless of:

```text id="d1v6s9"
CPU count
thread scheduling
memory address
```

provided its implementation is deterministic.

Such incidental runtime details do not need to become analytical inputs.

### Relevant vs Incidental Execution Context

The system must distinguish **relevant execution context** from incidental runtime information.

Relevant context:

```text id="5h8q2r"
changes or can materially affect the result
```

Incidental context:

```text id="3k7m9p"
describes execution but does not affect semantic output
```

For example:

```text id="p5z8y2"
random seed
```

may be relevant.

Whereas:

```text id="v1q6k4"
memory address of an allocated buffer
```

is normally incidental.

This distinction prevents provenance and configuration from becoming uncontrolled dumps of runtime state.

### Execution Context and Provenance

Execution context is an important input to provenance, but it should not be confused with provenance itself.

```text id="6j3m8q"
Execution Context
    → describes conditions of execution

Provenance
    → records origin and derivation of a result
```

For example:

```text id="k4p9s2"
Analysis
    ↓
produced during Run R123
    ↓
using Configuration C42
    ↓
with Component Version V7
    ↓
in REPLAY mode
```

The provenance record can reference the relevant execution context rather than copying every runtime property into every derived object.

### Execution Context and Configuration Provenance

Configuration provenance answers:

```text id="n6c2x8"
What effective configuration was used?
```

Execution context answers:

```text id="m9r4t1"
Under what execution conditions was it used?
```

Both may be required to reproduce a result.

For example:

```text id="u7q3p5"
Configuration:
    window_size = 100

Execution Context:
    mode = REPLAY
    replay_clock = T
    run_id = R123
```

A provenance record can then distinguish:

```text id="e4k8s2"
configuration_id = C42
run_id           = R123
```

rather than incorrectly treating `R123` as part of configuration identity.

### Execution Context Must Be Explicit at Relevant Boundaries

A component must not silently obtain semantically relevant execution context from unrelated global facilities.

For example, if processing depends on a clock, the component should receive access to the appropriate time source through its execution environment.

Conceptually:

```text id="x2p7m5"
Application
    ↓
Execution Context
    ↓
Processor
```

rather than:

```text id="s8v3k1"
Processor
    ↓
global system clock
global random generator
global configuration
global environment
```

This keeps execution dependencies visible and testable.

### Execution Context Does Not Become a Universal Parameter Bag

Execution context must not become an unrestricted key-value container containing arbitrary runtime information.

Avoid a design equivalent to:

```text id="c5r9w2"
ExecutionContext
{
    map<string, any> everything;
}
```

Instead, execution-context capabilities should have explicit semantics.

Conceptually:

```text id="m8q4z6"
ExecutionContext
    ├── time source
    ├── randomness source
    ├── run identity
    ├── execution mode
    └── explicitly defined runtime capabilities
```

The exact structure remains deferred.

## Refined Reproducibility Model

The reproducibility model is therefore:

```text id="a2f7k9"
Input
    +
Effective Configuration
    +
Component / Algorithm Version
    +
Relevant Execution Context
    ↓
Deterministic Processing
    ↓
Result
```

Where relevant execution context may include:

```text id="j5m3x8"
time source
randomness
processing mode
external dependency versions
other explicitly documented runtime inputs
```

Incidental execution details do not become part of the semantic input merely because they are observable.

## Decision Summary

```text id="q8v4m1"
Configuration:
    Defines intended component behavior

Execution Context:
    Defines runtime conditions of execution

State:
    Represents accumulated/runtime information

Configuration source:
    Where configuration originated

Environment:
    Runtime conditions that may form part of execution context

Run identity:
    Identifies an execution instance

Time source:
    Execution-context capability

Random source:
    Execution-context capability

Relevant context:
    Can materially affect result

Incidental context:
    Does not materially affect semantic result

Provenance:
    Records origin/derivation and may reference execution context

Universal context bag:
    Forbidden as the default architectural model
```

## Invariant

> **Configuration defines a component's intended behavior; execution context defines the runtime conditions under which that behavior executes; state records what occurs during execution; and any execution-context property that materially affects a result must be explicit enough to support reproducibility and provenance.**
