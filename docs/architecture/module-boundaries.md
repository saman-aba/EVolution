# Module Boundaries

## 1. Purpose

This document defines the responsibilities and dependency boundaries of the major EVolution modules.

The goal is not to prescribe programming language constructs, directory structure, or deployment topology.

The goal is to establish:

* what each module owns
* what each module may know
* what each module must not know
* how information crosses boundaries
* which dependencies are permitted

These boundaries should remain valid regardless of the eventual implementation technology.

---

## 2. Major Modules

The initial logical architecture consists of:

```text
Core
Processing
Domain
Analysis
Storage
Ingestion
Application
Interface
```

Conceptually:

```text
                         Application
                              │
              ┌───────────────┼───────────────┐
              ↓               ↓               ↓
          Interface         Domain         Analysis
              │               │               │
              └───────────────┼───────────────┘
                              ↓
                         Processing
                              ↓
                             Core
                              ↑
                              │
                           Storage
```

The diagram represents logical dependencies, not necessarily direct code dependencies.

---

# 3. Core

## Responsibility

Core defines the generic semantic building blocks of EVolution.

Core owns concepts such as:

```text
Event
State
Measurement
Metric
TimeSeries
Aggregation
Pattern
Analysis
Context
Provenance
Identity
```

It may also provide generic mechanisms required by these concepts.

Examples:

```text
time representation
identity handling
generic validation
generic configuration
generic data structures
generic processing contracts
generic error representation
```

## Core may know

Core may know:

```text
what an Event is
what a Measurement is
what provenance represents
how identities are represented
how generic objects relate
```

## Core must not know

Core must not know:

```text
what a poker hand is
what a blockchain transaction means
what a network protocol means
what a trading strategy is
what a domain-specific metric means
```

## Dependency rule

```text
Core
  ↓
No domain-specific dependency
```

Core is the lowest semantic layer of EVolution.

---

# 4. Processing

## Responsibility

Processing manages the execution of transformations over EVolution objects.

It owns mechanisms such as:

```text
processing graphs
processors
execution order
dependencies
stream processing
batch processing
replay
window management
incremental processing
failure propagation
execution state
```

## Processing may know

Processing may know:

```text
how a processor is executed
what inputs a processor requires
what outputs it produces
which processors depend on other processors
how processing progresses
```

## Processing must not know

Processing must not embed domain semantics.

For example, it should not contain special logic such as:

```text
if poker_hand:
    ...
```

or:

```text
if bitcoin_transaction:
    ...
```

A poker processor and a crypto processor should both be executable through the same generic processing mechanisms.

## Dependency rule

```text
Processing
    ↓
Core
```

Processing may execute domain-specific processors, but the generic processing engine should not contain those processors' semantic implementations.

---

# 5. Domain

## Responsibility

The Domain module gives meaning to generic EVolution concepts.

A domain defines:

```text
domain entities
domain events
domain state
domain measurements
domain metrics
domain patterns
domain analysis
domain rules
```

For Poker, this may include:

```text
Player
Table
Session
Hand
Action
Bankroll
```

and:

```text
HandStarted
CardsDealt
PlayerAction
CardRevealed
HandFinished
```

## Domain may know

A domain may know:

```text
Core
Processing contracts
domain-specific analysis
domain-specific storage requirements
```

## Domain must not know

A domain should not require a particular:

```text
database
GUI
HTTP framework
CLI
message broker
storage engine
deployment environment
```

unless a separate application/integration boundary explicitly requires it.

## Dependency rule

```text
Domain
   ↓
Core
```

A domain may specialize Core concepts but must not modify their generic meaning.

---

# 6. Analysis

## Responsibility

Analysis implements methods for deriving findings from available evidence.

Examples:

```text
statistical analysis
comparative analysis
temporal analysis
behavioral analysis
anomaly detection
trend analysis
predictive analysis
```

Analysis may consume:

```text
Events
State
Measurements
Time Series
Patterns
Context
Provenance
Historical Analysis
```

and produce:

```text
Findings
Analysis Results
Patterns
Models
Derived Measurements
```

## Analysis may know

Analysis may know the semantics required by a particular analytical method.

For example:

```text
Poker statistical analysis
```

may understand poker concepts.

However, generic analytical mechanisms should remain domain-independent.

## Analysis must not

Analysis must not silently turn findings into actions.

The distinction remains:

```text
Analysis
    ↓
Finding
    ↓
Policy
    ↓
Decision
    ↓
Action
```

Analysis produces evidence and interpretation.

Applications or policies determine what happens next.

---

# 7. Storage

## Responsibility

Storage provides persistence and retrieval capabilities.

Storage deals with:

```text
saving objects
loading objects
querying objects
versioning
retention
snapshots
checkpoints
caching
recovery
```

Storage must preserve the logical identity and provenance requirements defined by the system.

## Storage may know

A storage implementation may know:

```text
database schemas
file formats
indexes
serialization
transactions
compression
physical storage
```

## Storage must not define

Storage must not define the meaning of domain concepts.

For example:

```text
SQLite schema
```

must not become the definition of:

```text
Poker Hand
```

The logical model exists independently of the storage implementation.

## Dependency rule

Storage implementations depend on generic storage contracts.

```text
Core / Storage Contract
          ↑
          │
     Storage Backend
```

Possible implementations remain undecided.

---

# 8. Ingestion

## Responsibility

Ingestion converts external information into EVolution representations.

Examples:

```text
file parser
network decoder
API importer
database importer
simulation input
manual input
```

Typical flow:

```text
External Data
     ↓
Parser / Decoder
     ↓
Normalization
     ↓
Event
```

## Ingestion may know

An ingestion component may know:

```text
external file format
external protocol
external API
source-specific quirks
source-specific timestamps
```

## Ingestion must not

It should not force source-specific representation into the generic Core model.

For example, an external poker hand-history format should not cause the Core Event model to become a hand-history parser.

## Dependency rule

```text
Ingestion
    ↓
Core
    +
Domain
```

Domain dependency is required when external information must be translated into domain-specific events.

---

# 9. Application

## Responsibility

Applications compose EVolution capabilities into a usable product or workflow.

An application may determine:

```text
which domain is active
which processors are enabled
which analyses are executed
which data sources are used
which outputs are exposed
how users interact with the system
```

Examples:

```text
CLI application
research application
interactive analysis tool
simulation application
server
GUI
```

## Application may know

Application code may know about:

```text
Core
Domain
Processing
Analysis
Storage
Interfaces
```

It is therefore an integration boundary.

## Application should not

Application code should not reimplement domain semantics merely to present them.

For example:

```text
Application
    → request PlayerStatistics
```

is preferable to:

```text
Application
    → manually calculate PlayerStatistics
```

---

# 10. Interface

## Responsibility

Interfaces expose EVolution functionality to external consumers.

Examples:

```text
CLI
HTTP
gRPC
message bus
IPC
file export
GUI backend
```

An interface translates between an external interaction model and an application/service model.

## Interface must not

An interface should not contain the actual analytical implementation.

For example:

```text
HTTP Handler
    ↓
Application Service
    ↓
Analysis
```

rather than:

```text
HTTP Handler
    ↓
Statistical Algorithm
```

This keeps transport technology separate from analytical logic.

---

# 11. Dependency Direction

The following dependency direction is preferred:

```text
External Systems
       ↓
   Interfaces
       ↓
  Applications
       ↓
 ┌─────┴─────┐
 ↓           ↓
Domain     Analysis
 └─────┬─────┘
       ↓
   Processing
       ↓
      Core
```

Storage attaches through defined persistence boundaries:

```text
        Application
             │
             ↓
       Storage Contract
             ↑
             │
      Storage Backend
```

The exact physical dependency graph may differ, but the semantic dependency direction should remain intact.

---

# 12. Dependency Restrictions

The following dependencies are prohibited at the architectural level.

## Core → Domain

Not allowed.

```text
Core → Poker
```

would make the generic platform dependent on one domain.

---

## Core → Application

Not allowed.

```text
Core → CLI
Core → GUI
Core → HTTP
```

The Core must remain usable without any particular application.

---

## Core → Storage Technology

Not allowed.

```text
Core → PostgreSQL
Core → SQLite
Core → Redis
```

Storage technology is an implementation detail.

---

## Domain → Application

Generally not allowed.

A domain should define semantics independently of how they are presented.

---

## Domain → Interface Technology

Not allowed.

For example:

```text
Poker Domain → HTTP
```

would unnecessarily couple domain semantics to transport.

---

## Analysis → Presentation

Not allowed.

An analysis should produce structured analytical results, not terminal output, HTML, GUI widgets, or HTTP responses.

---

# 13. Allowed Dependency Example

A valid dependency chain could be:

```text
Poker CLI
   ↓
Poker Application
   ↓
Poker Domain
   ↓
EVolution Core
```

while the processing engine executes the required domain processors:

```text
Poker Application
       ↓
Processing Engine
       ↓
Poker Processor
       ↓
Core
```

Storage can independently participate:

```text
Poker Application
       ↓
Storage Contract
       ↓
PostgreSQL Backend
```

The PostgreSQL backend is replaceable without changing the conceptual Poker model.

---

# 14. Interfaces Between Modules

Modules should communicate through explicit contracts.

A contract should define:

```text
inputs
outputs
configuration
errors
identity
lifecycle
provenance requirements
```

The implementation behind the contract remains replaceable.

For example:

```text
Processor
{
    inputs
    outputs
    configuration
    process()
}
```

The architecture does not yet determine whether this becomes:

```text
C struct + function pointers
C++ interface
message
RPC
plugin ABI
```

---

# 15. Ownership of Semantics

Every concept should have a clearly defined owner.

| Concept               | Owner                   |
| --------------------- | ----------------------- |
| Event structure       | Core                    |
| Event meaning         | Domain                  |
| State mechanism       | Core                    |
| State semantics       | Domain                  |
| Measurement structure | Core                    |
| Metric meaning        | Domain / Analysis       |
| Processing execution  | Processing              |
| Analytical method     | Analysis                |
| Persistence mechanism | Storage                 |
| External format       | Ingestion / Interface   |
| User workflow         | Application             |
| Presentation          | Interface / Application |
| Domain policy         | Domain / Application    |

This table defines semantic ownership rather than source-code ownership.

---

# 16. Generic vs Specialized Components

A component should be generic only when its behavior can be defined without domain assumptions.

For example:

```text
Generic:
    rolling average
    event ordering
    time-window management
    provenance tracking
    aggregation
```

Potentially domain-specific:

```text
Poker:
    VPIP
    PFR
    bankroll policy
    hand classification
```

The distinction should be made based on semantics, not implementation difficulty.

A complicated algorithm can still be generic.

A trivial algorithm can still be domain-specific.

---

# 17. Boundary Crossing

When information crosses a module boundary, the receiving module should receive an explicit representation.

Avoid implicit coupling such as:

```text
Module A knows the internal memory layout of Module B.
```

Prefer:

```text
Module A
    ↓
Explicit Contract
    ↓
Module B
```

This is particularly important for:

```text
Processing
Storage
Plugins
External APIs
Scripting
```

---

# 18. Shared State

Modules should not rely on uncontrolled shared mutable state.

If shared state is required, its ownership must be explicit.

For example:

```text
State Owner
    ↓
Controlled Access
    ↓
Consumers
```

rather than:

```text
Module A ─┐
Module B ─┼→ global mutable state
Module C ─┘
```

The exact concurrency model will be defined later.

---

# 19. Error Boundaries

Errors should remain meaningful across module boundaries.

An error should distinguish, where applicable:

```text
invalid input
invalid domain state
processing failure
analysis failure
storage failure
configuration failure
external system failure
```

An interface may translate an internal error into an external representation, but should not destroy the underlying cause.

For example:

```text
Storage Error
     ↓
Application Error
     ↓
HTTP Error Response
```

The HTTP representation is not the source of truth for the error.

---

# 20. Lifecycle Ownership

Each major object or subsystem should have a clear lifecycle owner.

Examples:

```text
Application
    owns system lifecycle

Processing Engine
    owns processing lifecycle

Processor
    owns processor-local execution state

Storage
    owns storage resources

Domain
    owns domain semantics

Interface
    owns external connection lifecycle
```

No module should silently assume ownership of resources controlled by another module.

---

# 21. Testability Requirement

Every boundary should permit independent testing.

For example:

```text
Core
    ↓
test without domain

Domain
    ↓
test with deterministic events

Processor
    ↓
test with controlled inputs

Analysis
    ↓
test against known datasets

Storage
    ↓
test persistence independently

Application
    ↓
integration test modules together
```

This requirement should influence future implementation decisions.

---

# 22. Replaceability

A module should be replaceable when its externally visible contract remains unchanged.

Examples:

```text
Storage:
    Backend A → Backend B

Interface:
    CLI → HTTP

Analysis:
    Algorithm A → Algorithm B

Ingestion:
    File Format A → File Format B
```

Replacement does not mean every implementation must support every possible feature.

It means the rest of the architecture should depend on the contract rather than unnecessary implementation details.

---

# 23. Physical Deployment Is Not Yet Defined

The logical modules do not imply processes.

For example:

```text
Core
Processing
Domain
Analysis
Storage
```

could eventually exist:

```text
in one executable
```

or:

```text
as libraries
```

or:

```text
as separate processes
```

or:

```text
as dynamically loaded modules
```

or some combination.

This document intentionally does not decide that question.

---

# 24. Physical Threading Is Not Yet Defined

Likewise, the architecture does not currently require:

```text
one thread per processor
worker pools
event loops
actors
parallel pipelines
```

Those are execution-model decisions.

The logical boundary must exist independently of the concurrency implementation.

---

# 25. Architectural Rule

The most important rule of this document is:

> **A module owns a semantic responsibility, and other modules interact with that responsibility through an explicit contract rather than through its implementation details.**

The resulting separation is:

```text
Core
    defines generic concepts

Domain
    defines domain meaning

Processing
    executes transformations

Analysis
    derives interpretations

Storage
    persists information

Ingestion
    converts external information

Application
    composes system capabilities

Interface
    exposes capabilities externally
```

This separation is the foundation on which future implementation decisions should be made.
