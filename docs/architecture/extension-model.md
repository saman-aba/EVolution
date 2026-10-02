# Extension Model

## 1. Purpose

The Extension Model defines how EVolution can be extended with new functionality without modifying the generic Core for every new domain or algorithm.

EVolution is intended to evolve through additions such as:

```text
new domains
new event types
new state projections
new measurements
new aggregations
new pattern detectors
new analysis algorithms
new processors
new storage backends
new ingestion sources
new interfaces
```

The architecture should make these additions possible while preserving the existing boundaries.

This document defines the logical extension model.

It does not decide:

```text
static vs dynamic linking
shared libraries
plugin ABI
C vs C++
scripting language
process boundaries
IPC mechanism
```

Those decisions are deferred.

---

# 2. Extension Principle

The primary extension principle is:

> **New capabilities should normally be added by implementing existing contracts rather than modifying generic infrastructure.**

Conceptually:

```text
Existing Contract
       ↑
       │
   New Implementation
```

rather than:

```text
New Feature
     ↓
Modify Core
     ↓
Modify Existing Components
     ↓
Modify Applications
```

The second approach creates unnecessary coupling.

---

# 3. Extension Categories

EVolution has several distinct extension categories.

```text
Domain Extensions
Processing Extensions
Analysis Extensions
Storage Extensions
Ingestion Extensions
Interface Extensions
Application Extensions
```

Each category has different responsibilities.

---

# 4. Domain Extensions

A domain extension introduces a new analytical domain.

For example:

```text
domains/
├── poker/
├── crypto/
├── simulation/
└── future-domain/
```

A domain may define:

```text
entities
events
state
metrics
patterns
analysis
policies
```

A domain must use the generic Core concepts rather than creating an unrelated parallel object model.

For example:

```text
PokerEvent
```

should conceptually be:

```text
Event
    +
Poker-specific semantics
```

rather than a completely independent event abstraction.

---

# 5. Adding a New Domain

Adding a domain should conceptually require:

```text
1. Define domain entities
2. Define domain events
3. Define state projections
4. Define domain measurements
5. Define domain-specific patterns
6. Define domain analyses
7. Register required processors
8. Connect ingestion sources
9. Connect applications
```

The Core should remain unchanged unless the new domain exposes a genuinely missing generic capability.

---

# 6. Domain Extension Example

Suppose a new Crypto domain is introduced.

The system might contain:

```text
CryptoTransaction
CryptoBlock
Wallet
Token
Exchange
```

and events such as:

```text
TransactionObserved
BlockProduced
TransferDetected
WalletBalanceChanged
```

The generic pipeline remains:

```text
Events
   ↓
State
   ↓
Measurements
   ↓
Time Series
   ↓
Patterns
   ↓
Analysis
```

Only the semantics change.

---

# 7. Core Extension vs Domain Extension

A common architectural mistake is adding a feature to Core simply because it is needed by one domain.

The following question should be asked:

> Is this concept meaningful independently of the domain?

If yes, it may belong in Core.

If no, it probably belongs in the Domain or Analysis layer.

For example:

```text
Rolling Average
    → Generic

Player VPIP
    → Poker Domain

Time Window
    → Generic

Poker Session
    → Poker Domain
```

---

# 8. Processor Extensions

Processors implement transformations that participate in the Processing Model.

A processor should have an explicit contract describing:

```text
inputs
outputs
configuration
state requirements
execution behavior
failure behavior
```

Conceptually:

```text
Processor
{
    input_contract
    output_contract
    configuration
    process()
}
```

A processor may be:

```text
stateless
stateful
incremental
batch-oriented
stream-oriented
```

depending on its contract.

---

# 9. Processor Example

A generic processor:

```text
RollingAverageProcessor
```

could accept:

```text
TimeSeries
```

and produce:

```text
TimeSeries
```

A domain processor:

```text
PokerHandStateProcessor
```

could accept:

```text
Poker Events
```

and produce:

```text
Poker Hand State
```

Both can participate in the same Processing Engine.

---

# 10. Analysis Extensions

Analysis algorithms should also follow explicit contracts.

Conceptually:

```text
Analyzer
{
    inputs
    configuration
    analyze()
    outputs
}
```

An analyzer may produce:

```text
Analysis
Pattern
Measurement
Model
```

depending on its purpose.

For example:

```text
Input:
    Player action measurements

Analyzer:
    Behavioral Analysis

Output:
    Behavioral Pattern
```

The Processing Engine does not need to understand the internal algorithm.

---

# 11. Analysis as a Replaceable Component

Different algorithms may analyze the same evidence.

For example:

```text
Measurements
      │
      ├── Statistical Analyzer
      │
      ├── Rule-Based Analyzer
      │
      └── Machine-Learning Analyzer
```

The architecture should permit all three to coexist.

The choice of algorithm is a configuration/application concern unless the domain explicitly requires one.

---

# 12. Pattern Detector Extensions

Pattern detectors are a specialized form of analytical processor.

Examples:

```text
TrendDetector
AnomalyDetector
ChangePointDetector
CorrelationDetector
RegimeDetector
```

Each detector should explicitly define:

```text
input
configuration
detection criteria
output pattern
confidence semantics
provenance
```

The detector should not silently convert a detected pattern into a decision.

---

# 13. Measurement Extensions

New measurements should normally be introduced by defining new metrics rather than modifying the generic Measurement representation.

For example:

```text
Metric:
    average_hand_duration

Measurement:
    18.4 seconds
```

The generic representation remains:

```text
Measurement
{
    metric
    value
    scope
    dimensions
    time_range
    provenance
}
```

The metric definition provides the domain meaning.

---

# 14. Storage Extensions

Storage backends should implement the persistence contract.

Possible implementations could eventually include:

```text
File Storage
SQL Database
Key-Value Store
Columnar Storage
Time-Series Database
Object Storage
In-Memory Storage
```

The architecture should not require a particular backend.

For example:

```text
             Storage Contract
              /      |      \
             /       |       \
          File      SQL     Memory
```

A storage backend should not leak its internal representation into the rest of the system.

---

# 15. Ingestion Extensions

New data sources should be added through ingestion adapters.

Examples:

```text
Poker hand-history importer
CSV importer
JSON importer
Network stream
Exchange API
Database importer
Simulation source
```

The generic ingestion flow remains:

```text
External Representation
        ↓
      Parser
        ↓
   Normalization
        ↓
       Event
```

The source-specific implementation remains outside Core.

---

# 16. Interface Extensions

External interfaces may also be added independently.

Examples:

```text
CLI
HTTP API
gRPC API
IPC
Message Broker
GUI backend
```

An interface should translate between its external protocol and an application-level contract.

For example:

```text
HTTP Request
     ↓
HTTP Adapter
     ↓
Application Service
     ↓
EVolution
```

The analytical engine should not need to know whether the request originated from HTTP, CLI, or another interface.

---

# 17. Application Extensions

Applications compose existing components into specific workflows.

For example:

```text
Poker Analysis CLI
Research Application
Simulation Application
Batch Analysis Tool
```

An application may select:

```text
domain
processors
analyses
storage
interfaces
configuration
```

The application therefore acts as a composition boundary.

---

# 18. Registration

Extensions eventually need a way to become visible to the system.

Conceptually:

```text
Extension
    ↓
Registration
    ↓
Registry
    ↓
Processing / Application
```

A registry might contain:

```text
processor types
analyzer types
domain types
storage providers
ingestion providers
interface providers
```

The exact registration mechanism is intentionally undecided.

---

# 19. Discovery

Registration and discovery are related but distinct.

Registration answers:

> What extensions are available?

Discovery answers:

> Which extension should be used for this operation?

For example:

```text
Registry
   ↓
Find Analyzer("trend")
   ↓
Analyzer Instance
```

Discovery may eventually use:

```text
name
type
capability
version
configuration
domain
```

---

# 20. Capability Model

Extensions should be able to describe their capabilities.

Conceptually:

```text
Capability
{
    type
    name
    version
    inputs
    outputs
    requirements
}
```

For example:

```text
Analyzer:
    name = "trend"
    input = TimeSeries
    output = Pattern
```

Capabilities allow applications and processing systems to reason about available components without depending on their implementation.

---

# 21. Versioning

Extensions should have explicit versions where compatibility matters.

Potential versioned objects include:

```text
processor
analyzer
domain
storage backend
schema
configuration
algorithm
```

Versioning is particularly important when derived results depend on algorithm behavior.

For example:

```text
TrendDetector v1
```

and:

```text
TrendDetector v2
```

may produce different results from the same input.

Provenance should therefore record the version when required for reproducibility.

---

# 22. Extension Identity

An extension should have a stable logical identity.

Conceptually:

```text
Extension Identity
{
    type
    name
    version
}
```

The identity should not depend on:

```text
memory address
process ID
file path
database row ID
```

unless one of these is explicitly part of a separate implementation identity.

---

# 23. Configuration

Extensions should receive configuration explicitly.

Avoid hidden dependencies such as:

```text
global configuration
environment variables
global mutable state
implicit singleton objects
```

when they affect analytical behavior.

Prefer:

```text
Application
    ↓
Configuration
    ↓
Extension
```

This improves:

```text
testing
reproducibility
provenance
experimentation
```

---

# 24. Extension Lifecycle

Extensions may have a lifecycle such as:

```text
DISCOVERED
    ↓
REGISTERED
    ↓
CONFIGURED
    ↓
INITIALIZED
    ↓
ACTIVE
    ↓
STOPPED
```

Not every extension requires every state.

A stateless analysis function may require almost no lifecycle management.

A storage backend may require substantial initialization and shutdown behavior.

The lifecycle contract should therefore be capability-dependent rather than unnecessarily universal.

---

# 25. Isolation

An extension should not assume that it owns the entire application.

For example, a processor must not:

```text
terminate the process
modify unrelated processors
change global configuration
modify another processor's state
```

unless explicitly authorized through a system contract.

Extensions operate within the boundaries defined by the host system.

---

# 26. Failure Isolation

An extension failure should have explicitly defined consequences.

Possible policies include:

```text
FAIL_PIPELINE
SKIP_INPUT
RETRY
DISABLE_EXTENSION
DEGRADE
REPORT_AND_CONTINUE
```

The Processing Model determines how these policies are applied.

An extension reports failure; the execution environment determines the broader response according to its contract.

---

# 27. Extension Composition

Extensions should be composable.

For example:

```text
Event Source
     ↓
State Projection
     ↓
Measurement Processor
     ↓
Aggregation
     ↓
Pattern Detector
     ↓
Analysis
```

Each component can be independently replaced.

This is one of the primary reasons to maintain explicit contracts.

---

# 28. Extension Graph

Extensions may form a graph rather than a hierarchy.

For example:

```text
                 ┌── Measurement A
Events → State ──┼── Measurement B
                 └── Measurement C
                         │
                  ┌──────┴──────┐
                  ↓             ↓
              Pattern A      Pattern B
                  │             │
                  └──────┬──────┘
                         ↓
                      Analysis
```

The architecture should support multiple consumers of the same output.

---

# 29. No Mandatory Plugin Mechanism Yet

The architecture deliberately does not require:

```text
dlopen()
shared libraries
dynamic loading
plugin directories
ABI versioning
RPC
separate processes
```

These mechanisms may become appropriate later.

The architectural requirement is simply:

> **Extensions must interact through stable contracts and must not require unrelated components to know their implementation details.**

---

# 30. Static and Dynamic Extensions

The same logical extension model should permit different deployment strategies.

For example:

```text
Static:
    Extension compiled into executable

Dynamic:
    Extension loaded at runtime

Remote:
    Extension executed by another process
```

The choice can be made independently for different extension categories if necessary.

---

# 31. Scripting as an Extension Mechanism

Scripting is potentially another extension mechanism.

Conceptually:

```text
Script
   ↓
Scripting API
   ↓
EVolution Contracts
```

A script could eventually define:

```text
custom analysis
custom transformation
experiment
query
automation
```

However, scripting should not bypass architectural boundaries.

For example, a script should not directly manipulate internal storage structures merely because the implementation happens to expose them.

---

# 32. Domain Independence

Adding a new domain should not require modifying unrelated domains.

For example:

```text
Poker
   │
   └── Core

Crypto
   │
   └── Core
```

rather than:

```text
Poker
   ↔
Crypto
   ↔
Core
```

Cross-domain interaction may exist, but it should be explicit.

---

# 33. Cross-Domain Extensions

Some future applications may combine multiple domains.

For example:

```text
Market Data
     +
Blockchain Data
     +
User Activity
```

Such combinations should occur at an application or higher analytical layer unless there is a genuinely shared domain model.

This prevents the Core from becoming a universal semantic model for every possible domain.

---

# 34. Extension Testing

Every extension should be testable against its contract.

A processor should be testable using:

```text
input → expected output
```

An analyzer should be testable using:

```text
evidence → expected finding
```

A storage backend should be testable using:

```text
store → retrieve → compare
```

An ingestion adapter should be testable using:

```text
external input → normalized representation
```

Contract tests can later verify that multiple implementations behave consistently where the contract requires it.

---

# 35. Extension Documentation

Each extension should document at least:

```text
Name
Purpose
Type
Version
Inputs
Outputs
Configuration
Dependencies
Lifecycle
Failure behavior
Determinism
Provenance requirements
```

This information may eventually become machine-readable.

---

# 36. Extension Boundary Invariant

The fundamental invariant is:

> **An extension adds behavior through an explicit contract without changing the meaning of unrelated existing components.**

Therefore:

```text
New Domain
    → implements domain contracts

New Processor
    → implements processing contracts

New Analyzer
    → implements analysis contracts

New Storage Backend
    → implements storage contracts

New Ingestion Source
    → implements ingestion contracts

New Interface
    → implements interface contracts
```

The Core changes only when a genuinely generic capability is missing.

---

# 37. Architectural Consequence

This model gives EVolution two distinct forms of evolution:

```text
Vertical Evolution
    Improve existing concepts and mechanisms

Horizontal Evolution
    Add new domains, processors, analyses, and integrations
```

The architecture should support both without forcing every new feature into the Core.

---

# 38. Final Extension Principle

The extension architecture can be summarized as:

```text
                 EVolution Core
                       │
        ┌──────────────┼──────────────┐
        ↓              ↓              ↓
     Domains       Processing      Analysis
        │              │              │
        └──────────────┼──────────────┘
                       ↓
                 Applications
                       │
          ┌────────────┴────────────┐
          ↓                         ↓
      Interfaces                 Storage
```

Extensions attach to explicit boundaries.

They do not redefine those boundaries.

> **EVolution should grow by adding implementations and capabilities, not by continuously expanding the semantic responsibility of the Core.**
