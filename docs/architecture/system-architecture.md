# System Architecture

## 1. Purpose

The System Architecture defines the major boundaries of EVolution and the relationships between them.

The conceptual models define the meaning of individual objects.

The Processing Model defines how those objects are transformed.

The Storage Model defines how they are retained.

This document combines those concepts into a system-level architecture.

The architecture must remain independent of implementation details such as:

```text
C vs C++
database technology
serialization format
threading model
IPC mechanism
GUI framework
scripting language
deployment model
```

Those decisions belong to later architectural decisions.

---

## 2. Architectural Goal

EVolution should provide a generic analytical engine capable of transforming observations into progressively higher-level representations.

The primary conceptual flow is:

```text
Raw Data
   ↓
Events
   ↓
State
   ↓
Measurements
   ↓
Time Series
   ↓
Aggregation
   ↓
Patterns
   ↓
Analysis
   ↓
Decision / Application
```

Context and provenance apply across this pipeline:

```text
                         Context
                            │
                            ▼
Events → State → Measurements → ... → Analysis
   │                                  │
   └──────────── Provenance ──────────┘
```

---

## 3. Major Architectural Boundaries

The system is divided into several major areas:

```text
┌──────────────────────────────────────────────┐
│                  Applications                │
├──────────────────────────────────────────────┤
│             Domain / Analysis                │
├──────────────────────────────────────────────┤
│              Processing Engine               │
├──────────────────────────────────────────────┤
│        Core Models and Infrastructure        │
├──────────────────────────────────────────────┤
│                Persistence                   │
└──────────────────────────────────────────────┘
```

External systems provide observations and consume results.

---

## 4. Core

The Core contains generic concepts and mechanisms that are independent of any particular domain.

Examples include:

```text
Event
State
Measurement
Time Series
Aggregation
Pattern
Analysis
Context
Provenance
Identity
```

The Core may also provide generic mechanisms for:

```text
processing
data flow
validation
configuration
serialization
logging
error handling
time
identity
```

However, Core must not contain domain-specific semantics.

For example:

```text
Core:
    Measurement

Domain:
    PokerWinRate
```

The Core knows what a measurement is.

It does not know what winning at poker means.

---

## 5. Domain Layer

The Domain Layer defines concepts specific to a particular analytical domain.

For example, a Poker domain may define:

```text
Player
Table
Session
Hand
Action
Bankroll
Stake
```

and domain events such as:

```text
SessionStarted
HandStarted
CardsDealt
PlayerAction
CardRevealed
HandFinished
SessionFinished
```

Another domain could define completely different objects.

The domain layer gives meaning to the generic Core abstractions.

---

## 6. Domain as a Separate Boundary

The Core should not contain:

```text
Poker
Crypto
Trading
Network Protocols
Game Rules
```

as built-in concepts.

Instead:

```text
                 ┌── Poker
                 │
Core ────────────┼── Simulation
                 │
                 ├── Network Analysis
                 │
                 └── Future Domain
```

This allows EVolution to evolve without turning the Core into a collection of unrelated domain-specific features.

---

## 7. Processing Engine

The Processing Engine executes transformations defined by the system.

It is responsible for concepts such as:

```text
processor
processing graph
dependencies
execution
streaming
batch processing
replay
windowing
failure handling
```

The engine should not determine the semantic meaning of the data.

For example:

```text
Processing Engine
    executes:
        PokerStateProjection
        WinRateCalculator
        DrawdownDetector
```

but does not need to understand poker itself.

---

## 8. Analysis Layer

Analysis components transform measurements, patterns, state, context, and other evidence into analytical results.

Examples include:

```text
Trend Analysis
Risk Analysis
Behavior Analysis
Statistical Analysis
Comparative Analysis
```

Analysis is distinct from the Processing Engine.

The Processing Engine answers:

> How is this computation executed?

The Analysis Layer answers:

> What analytical operation is being performed?

---

## 9. Policy and Decision Layer

Decision logic sits downstream of analysis.

Conceptually:

```text
Analysis
   ↓
Policy
   ↓
Decision
```

This layer is domain/application dependent.

For example:

```text
Poker Analysis
    ↓
Bankroll Policy
    ↓
Stake Eligibility Decision
```

The Core should provide mechanisms for representing decisions and policies without embedding domain-specific objectives.

---

## 10. Persistence Layer

Persistence provides storage and retrieval of logical EVolution objects.

Conceptually:

```text
Processing
    ↓
Persistence
```

and:

```text
Persistence
    ↓
Processing
```

when historical information must be loaded.

Persistence may store:

```text
events
snapshots
measurements
time series
patterns
analyses
provenance
configuration
```

The physical storage implementation remains separate.

---

## 11. Applications

Applications are the outermost layer that uses EVolution.

Examples might include:

```text
CLI
GUI
research tool
data importer
interactive analysis tool
automated service
simulation
```

Applications determine how users interact with the system.

They may combine:

```text
domain
analysis
processing
storage
policy
```

according to their requirements.

---

## 12. External Inputs

EVolution may receive information from external systems.

Examples:

```text
file
network stream
database
API
sensor
simulation
user input
```

These inputs should pass through an ingestion boundary.

Conceptually:

```text
External Source
      ↓
Ingestion
      ↓
Normalization
      ↓
Event
      ↓
EVolution
```

The Core should not assume that all external data already conforms to the internal event model.

---

## 13. Ingestion

Ingestion converts external representations into EVolution's internal representations.

For example:

```text
Hand History
     ↓
Parser
     ↓
Normalized Event
```

or:

```text
Network Data
     ↓
Decoder
     ↓
Normalized Event
```

Ingestion is therefore a boundary between external data formats and the internal event model.

---

## 14. Normalization

Different sources may describe the same conceptual information differently.

For example:

```text
Source A:
    player_action = "raise"

Source B:
    action = "R"

Internal:
    PlayerAction(type=RAISE)
```

Normalization creates a consistent internal representation.

Normalization belongs near ingestion because the Core should not need to understand every external representation.

---

## 15. Output Boundary

EVolution may produce information for external consumers.

Conceptually:

```text
Analysis
   ↓
Output Adapter
   ↓
External Consumer
```

Possible outputs include:

```text
file
API
database
message stream
CLI
GUI
report
```

Output formatting should remain separate from analytical semantics.

---

## 16. Architectural Dependency Direction

Dependencies should generally point toward more generic abstractions.

Conceptually:

```text
Applications
     ↓
Domain / Analysis
     ↓
Processing
     ↓
Core
     ↓
Infrastructure Interfaces
```

Persistence implementations and external adapters should attach to defined interfaces rather than forcing their implementation details into the Core.

The important principle is:

> **Generic components should not depend on domain-specific components.**

---

## 17. Dependency Example

A Poker application may depend on:

```text
Poker Domain
     ↓
EVolution Core
```

while the Core should not depend on:

```text
Poker Domain
```

Similarly:

```text
SQLite Storage
```

may implement a generic persistence boundary.

The Core should not become dependent on SQLite-specific concepts.

---

## 18. Domain Plugins

Domains may eventually be loaded or selected independently.

Conceptually:

```text
EVolution
   │
   ├── Core
   │
   ├── Processing
   │
   └── Domains
        ├── Poker
        ├── Simulation
        └── Other
```

Whether domains become dynamic plugins, static modules, libraries, or separate processes is intentionally undecided.

The architectural requirement is modularity, not a particular plugin mechanism.

---

## 19. Analysis Plugins

Analysis algorithms should similarly be replaceable.

For example:

```text
Measurement Series
       │
       ├── Trend Detector
       ├── Volatility Analyzer
       ├── Anomaly Detector
       └── Custom Analyzer
```

The Processing Engine should be able to execute different analysis implementations through explicit contracts.

---

## 20. Configuration Boundary

Configuration should be separate from implementation.

Conceptually:

```text
Configuration
      ↓
Processor / Domain / Analysis
```

Configuration may define:

```text
thresholds
windows
enabled processors
domain parameters
analysis parameters
storage policies
runtime options
```

Configuration that affects derived output must participate in provenance.

---

## 21. Execution vs Definition

The architecture should distinguish between defining a processing operation and executing it.

For example:

```text
Definition:
    calculate rolling volatility
    window = 100 observations
```

versus:

```text
Execution:
    calculate it now
```

This distinction allows the same analytical definition to be used for:

```text
live processing
historical replay
batch analysis
experiments
tests
```

---

## 22. Data Plane and Control Plane

EVolution may eventually need to distinguish between:

```text
Data Plane
    observations
    events
    measurements
    processing results

Control Plane
    configuration
    processor lifecycle
    graph management
    execution control
```

This distinction is not yet an implementation requirement.

It exists to prevent operational control information from being confused with analytical data.

---

## 23. Application Boundary

Applications should consume EVolution through explicit interfaces.

For example:

```text
CLI
 │
 ├── submit data
 ├── query analysis
 ├── inspect state
 └── control processing
```

The CLI should not directly manipulate internal processing structures.

Likewise, a GUI should use the same logical interfaces rather than requiring special analytical semantics.

---

## 24. External API Boundary

If EVolution later exposes an API, the API should sit outside the analytical Core.

Conceptually:

```text
Client
   ↓
API
   ↓
Application Service
   ↓
EVolution
```

This prevents transport concerns such as HTTP, gRPC, or messaging protocols from becoming part of the Core model.

No specific API technology is selected at this stage.

---

## 25. Scripting Boundary

EVolution may eventually expose scripting capabilities.

Possible scripting consumers include:

```text
analysis scripts
experiments
custom processors
data transformation
automation
```

A scripting environment should interact with explicit EVolution interfaces.

Conceptually:

```text
Script
   ↓
Scripting API
   ↓
EVolution
```

The choice of scripting language remains undecided.

---

## 26. Security Boundary

Security-sensitive operations should be isolated from analytical semantics.

Potential concerns include:

```text
external input validation
access control
configuration permissions
script execution
external APIs
persistent data access
```

The Core should not assume that every input is trusted.

However, security architecture should be defined separately once the deployment and threat model are understood.

---

## 27. Observability

The system itself may produce operational information.

Examples:

```text
processing latency
queue depth
processor failures
throughput
storage errors
resource usage
```

These are different from domain measurements.

For example:

```text
processing latency = 20 ms
```

is an operational metric about EVolution.

It should not automatically become a domain measurement such as:

```text
player performance = ...
```

The architecture should therefore distinguish:

```text
System Observability
```

from:

```text
Domain Analytics
```

---

## 28. Testing Boundary

Each architectural layer should be testable independently.

For example:

```text
Core
    unit tests

Domain
    domain behavior tests

Processor
    transformation tests

Analysis
    analytical correctness tests

Persistence
    storage tests

Application
    integration tests
```

The architecture should allow components to be tested without requiring the complete system.

---

## 29. Architectural Dependency Graph

The resulting conceptual dependency structure is:

```text
                         Applications
                              │
               ┌──────────────┴──────────────┐
               │                             │
          Domain / Analysis             Interfaces
               │                             │
               └──────────────┬──────────────┘
                              ↓
                       Processing Engine
                              ↓
                            Core
                              ↑
                              │
                    Persistence / Infrastructure
```

External adapters connect at the boundaries:

```text
External Sources
       ↓
   Ingestion
       ↓
      Core

Core
       ↓
 Output Adapters
       ↓
External Consumers
```

The diagram is conceptual rather than a prescribed package dependency graph.

---

## 30. Core Architectural Rule

The most important architectural dependency rule is:

```text
Specific
   ↓
Generic
```

not:

```text
Generic
   ↓
Specific
```

Therefore:

```text
Poker
    may depend on Core

Core
    must not depend on Poker
```

and:

```text
SQLite Adapter
    may implement Storage interface

Core
    must not depend on SQLite
```

and:

```text
HTTP API
    may expose Application functionality

Core
    must not depend on HTTP
```

---

## 31. Initial Module Model

At the conceptual level, EVolution can therefore be divided into:

```text
core/
    generic semantic models and mechanisms

processing/
    processing graph and execution

domains/
    domain-specific semantics

analysis/
    analytical algorithms

storage/
    persistence implementations

ingestion/
    external data conversion

applications/
    user-facing applications

interfaces/
    APIs and integration boundaries
```

This is a conceptual decomposition.

The physical repository structure does not need to exactly match these names yet.

---

## 32. Architectural Invariant

EVolution should remain a platform rather than becoming a single application.

The platform provides:

```text
generic models
processing mechanisms
analytical mechanisms
persistence boundaries
provenance
context
identity
```

Domains provide:

```text
meaning
domain events
domain state
domain metrics
domain analysis
domain policies
```

Applications provide:

```text
interaction
workflow
configuration
presentation
integration
```

Therefore:

> **EVolution Core provides generic analytical infrastructure; domains provide meaning; applications provide purpose and interaction.**
