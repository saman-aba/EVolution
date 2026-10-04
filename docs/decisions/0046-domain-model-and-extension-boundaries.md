# ADR 0046: Domain Model and Domain Extension Boundaries

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution separates **generic analytical infrastructure** from **domain meaning**.

The Core defines generic concepts and mechanisms such as:

* Event
* State
* Measurement
* Metric
* Time Series
* Aggregation
* Pattern
* Analysis
* Context
* Provenance
* Identity

A domain defines what those concepts mean for a particular subject.

Conceptually:

```text
                    EVolution Core
                         │
             Generic analytical concepts
                         │
          ┌──────────────┼──────────────┐
          ↓              ↓              ↓
       Poker          Markets       Simulation
       Domain          Domain          Domain
```

Adding a domain must normally require implementing domain extensions rather than modifying the meaning of existing Core concepts.

---

## 1. Domain Responsibility

A domain owns semantic knowledge about a particular problem space.

A domain may define:

* entities
* domain events
* domain state
* domain measurements
* metrics
* domain-specific patterns
* analytical models
* domain rules
* policies
* domain-specific configuration
* domain-specific ingestion mappings
* domain-specific serialization where required

For example, the Poker domain may define:

```text
Player
Table
Session
Hand
Action
Tournament
```

while Core only provides generic mechanisms for representing and processing such information.

---

## 2. Generic Concept vs Domain Meaning

The same Core concept may have different meanings in different domains.

For example:

```text
Core:
    Event

Poker:
    PlayerAction

Market:
    Trade

Simulation:
    SimulationStep
```

All are events, but Core must not know what distinguishes them semantically.

Likewise:

```text
Core:
    Measurement

Poker:
    VPIP

Market:
    Volatility

Network:
    PacketRate
```

The Core provides the measurement mechanism.

The domain defines the metric meaning.

---

## 3. Domain Model

A domain model describes the meaningful objects and relationships of a domain.

Conceptually:

```text id="c2p0dq"
Domain Model
{
    entities
    events
    states
    measurements
    patterns
    analyses
    policies
    relationships
}
```

Not every domain requires every category.

A domain may be event-heavy, measurement-heavy, simulation-oriented, or otherwise structured.

The architecture must not force every domain into an identical semantic hierarchy.

---

## 4. Domain Entities

Domains may define entities with stable logical identity.

For example:

```text
PlayerId
TableId
HandId
SessionId
```

These use the generic Identity Model while remaining domain-specific types.

A domain identifier must not be placed into Core merely because multiple domains happen to use an identifier.

Core provides identity mechanisms; the domain provides identity meaning.

---

## 5. Domain Events

Domain events represent meaningful occurrences within the domain.

For Poker:

```text
SessionStarted
HandStarted
CardsDealt
PlayerAction
CardRevealed
HandFinished
SessionFinished
```

For a market domain:

```text
TradeExecuted
OrderCreated
OrderCancelled
BookUpdated
```

The Event concept belongs to Core.

The event types and payload semantics belong to the domain.

Therefore:

```text
Core knows:
    Event has identity, temporal information, source, sequence, type, payload.

Domain knows:
    PlayerAction means a player performed a particular action.
```

---

## 6. Domain State

Domains define the meaning of reconstructed state.

For example:

```text
Poker:
    TableState
    HandState
    PlayerState

Market:
    OrderBookState
    PositionState

Simulation:
    SimulationState
```

Core provides mechanisms for state representation and projection.

The domain defines:

```text
Event → State transition
```

semantics.

A Core component must not contain assumptions such as:

```text
if event.type == PlayerAction
```

because that would make Core domain-aware.

---

## 7. Domain Measurements and Metrics

Domains define metric semantics.

For example:

```text
Poker:
    VPIP
    PFR
    Aggression Factor

Market:
    Volatility
    Spread
    Volume

Network:
    Packet Loss
    Throughput
    Latency
```

The generic `Measurement` representation remains reusable.

The domain or analysis layer defines:

* what is measured
* how it is calculated
* what unit it has
* what scope it applies to
* what assumptions it requires

Metric identity must therefore remain distinct from the generic Measurement representation.

---

## 8. Domain Patterns

Domains may define specialized patterns.

For example, Poker may define:

```text
Repeated Betting Pattern
Aggression Pattern
Positional Pattern
Range Pattern
```

A market domain may define:

```text
Breakout
Reversal
Regime Change
Liquidity Shift
```

The generic Pattern concept provides structure for representing detected patterns.

The domain defines what constitutes a meaningful domain-specific pattern.

A domain pattern must not be hard-coded into generic pattern infrastructure.

---

## 9. Domain Analysis

Domains may define analytical questions that have no meaning outside the domain.

Examples:

```text
Poker:
    How does position affect profitability?

Market:
    Does volatility regime change strategy performance?

Simulation:
    Which parameter produces the best outcome distribution?
```

Generic analysis infrastructure can execute analytical methods.

The domain defines:

* meaningful inputs
* domain assumptions
* domain-specific hypotheses
* interpretation
* relevant baselines
* valid conclusions

---

## 10. Domain Policies and Decisions

Domain-specific policies belong outside Core.

For example:

```text
Poker:
    bankroll management policy

Trading:
    position sizing policy

Simulation:
    experiment selection policy
```

The generic Decision model provides structural representation.

The domain defines what decisions mean and what constraints apply.

Applications may then compose those policies into executable workflows.

---

## 11. Domain Invariants

A domain owns invariants that depend on domain semantics.

For example:

```text
Poker:
    A hand cannot have an action before the hand starts.

Market:
    An order cannot be filled after it has been cancelled.

Simulation:
    A completed simulation cannot accept another simulation step.
```

These invariants must not be generalized into Core unless they are genuinely domain-independent.

Core may enforce structural invariants.

Domain code enforces semantic invariants.

---

## 12. Domain Validation

Validation occurs at multiple levels.

```text
Representation validation
        ↓
Core structural validation
        ↓
Domain semantic validation
        ↓
Application policy validation
```

For example:

```text
Core:
    timestamp representation is valid

Domain:
    action is legal for the current poker state

Application:
    user is permitted to execute the requested operation
```

A structurally valid object can therefore still be semantically invalid for its domain.

---

## 13. Domain Configuration

Domain configuration belongs to the domain or consuming application.

Examples:

```text
Poker:
    blind structure
    tournament rules
    player roles

Market:
    exchange configuration
    instrument definitions
    session rules
```

Generic Core configuration mechanisms may represent configuration values, but Core must not define domain-specific configuration semantics.

---

## 14. Domain Extensions

A new domain should normally be added as an extension.

For example:

```text
domains/
├── poker/
├── market/
├── simulation/
└── network/
```

A domain extension may provide:

```text
domain/
├── entities
├── events
├── state
├── measurements
├── patterns
├── analysis
├── policies
└── ingestion
```

The exact physical structure remains an implementation concern.

---

## 15. Domain Dependency Direction

The dependency direction must remain:

```text
Core
  ↑
Domain
  ↑
Application
```

More precisely:

```text
Application
    ↓
Domain
    ↓
Core
```

A domain may depend on generic Core contracts.

Core must not depend on a domain.

Therefore:

```text
Core → Poker
```

is forbidden architecturally.

```text
Poker → Core
```

is valid.

---

## 16. Cross-Domain Concepts

Two domains may share concepts.

For example:

```text
Time
Identity
Quantity
Price
Location
Actor
```

A concept should move into Core only when its semantics are genuinely domain-independent.

Similarity is not sufficient.

The following is therefore discouraged:

```text
PokerPlayer
CryptoTrader
NetworkActor
        ↓
GenericActor
```

merely because all three happen to represent "actors."

The abstraction should be introduced only when a stable generic contract actually exists.

---

## 17. Cross-Domain Data

A processing graph may connect different domains.

For example:

```text
Market Data
    ↓
Generic Measurement
    ↓
Analysis
    ↓
Decision
```

or:

```text
Poker Events
    ↓
Behavioral Analysis
    ↓
Generic Statistical Analysis
```

Cross-domain composition should occur through explicit contracts.

A domain must not reach into another domain's internal implementation.

---

## 18. Domain Adapters

When two domains need to communicate but their semantic models differ, an explicit adapter should translate between them.

Conceptually:

```text
Domain A
   ↓
Domain Adapter
   ↓
Domain B
```

The adapter must make the semantic translation visible.

The architecture must not silently reinterpret one domain's object as another domain's object merely because their physical representations are compatible.

---

## 19. Domain Extension vs Generic Core Evolution

When implementing a new domain, the default question is:

> Can this be implemented using existing Core contracts?

If yes, the change belongs in the domain.

If not, determine whether the missing capability is genuinely generic.

Only then should Core evolve.

For example:

```text
Need:
    Poker-specific range representation

Correct:
    Add to Poker domain.

Need:
    Generic interval representation useful across domains

Potential:
    Extend Core.
```

A domain must not modify Core merely to make its own implementation convenient.

---

## 20. Domain-Specific Serialization

Domains may define serialization for domain-specific objects.

For example:

```text
PokerHand
MarketInstrument
SimulationScenario
```

Core serialization mechanisms remain generic.

A domain serializer must preserve the domain object's:

* logical identity
* temporal semantics
* domain meaning
* version
* required provenance

External representation remains separate from the internal domain model.

---

## 21. Domain Ingestion

External data often requires domain-specific interpretation.

For example:

```text
Poker hand history
        ↓
Parsing
        ↓
Normalization
        ↓
Poker domain mapping
        ↓
Poker Events
```

The generic ingestion model defines the stages.

The domain provides the mapping rules that assign meaning to the normalized information.

---

## 22. Domain Analysis Extensions

Domain analyses should implement generic analytical contracts where possible.

For example:

```text
Generic:
    Analyzer

Poker:
    PositionProfitabilityAnalyzer
    RangeAnalyzer

Market:
    VolatilityRegimeAnalyzer
```

The generic Analysis model should not need modification merely because a new analytical algorithm is introduced.

This follows the Extension Model.

---

## 23. Domain Testing

Domain tests must establish semantic correctness that generic Core tests cannot establish.

For example:

```text
Core test:
    Event identity behaves correctly.

Poker test:
    PlayerAction produces the correct HandState transition.
```

Domain testing should include:

* domain invariants
* valid transitions
* invalid transitions
* domain event semantics
* metric definitions
* domain patterns
* domain analyses
* domain serialization
* domain ingestion
* domain policies

Generic contract tests may be reused where applicable.

---

## 24. Domain Versioning

Domain semantics can evolve independently of Core.

Examples:

```text
Core version
Poker domain version
Poker metric definition version
Poker ruleset version
```

A domain change that alters analytical meaning must be versioned appropriately.

Historical results must not silently acquire new meaning because the current domain implementation changed.

This is particularly important for:

* metric definitions
* event interpretation
* state transitions
* pattern detectors
* analytical algorithms
* domain rules

---

## 25. Domain Provenance

Domain-derived objects must preserve enough provenance to determine which domain semantics produced them when that distinction matters.

For example:

```text
Measurement
    ↓
Metric Definition Version
    ↓
Poker Domain Version
    ↓
Analysis Version
```

The exact amount of captured version information depends on reproducibility requirements.

---

## 26. Domain Registration

Domains may participate in the Registration and Discovery Model.

A domain definition may expose:

```text
Domain Identity
Domain Version
Supported Concepts
Supported Events
Supported Processors
Supported Analyses
Supported Serializers
Supported Ingestion Sources
```

Registration does not automatically activate a domain.

Applications explicitly select which domains they use.

---

## 27. Domain Isolation

A domain should be independently testable and, where practical, independently buildable.

A domain should not require:

* a GUI
* a particular database
* a particular HTTP server
* a particular deployment model

unless the domain contract genuinely requires such an external capability.

This preserves portability and allows the same domain to be used by multiple applications.

---

## 28. Domain Boundary and Core Purity

The Core must remain domain-neutral.

The following are prohibited in Core:

```text
Poker-specific event types
Poker rules
Trading rules
Market-specific metrics
Domain-specific state transitions
Domain-specific policies
Domain-specific UI behavior
```

Generic infrastructure may provide mechanisms used by these concepts, but must not encode their meaning.

The Core boundary should therefore be reviewed whenever a new domain capability is proposed.

---

## 29. When a Domain Concept Becomes Generic

A domain concept may eventually become generic if:

1. multiple domains require it;
2. its semantics are stable;
3. the abstraction is genuinely domain-independent;
4. its contract can be defined without referencing the original domain;
5. moving it into Core does not introduce hidden assumptions.

Migration should be explicit.

A concept should not be generalized merely because it is convenient to reuse its implementation.

---

## 30. Deferred Decisions

This ADR does not select:

* domain-specific class hierarchy
* inheritance model
* domain aggregate model
* domain-driven-design methodology
* event sourcing
* domain package structure
* domain serialization format
* domain registry implementation
* dynamic domain loading
* scripting interface
* generic quantity/unit framework
* generic actor/entity framework
* cross-domain ontology
* domain-specific database schema
* domain-specific GUI/API

These remain future decisions.

---

## Decision Summary

```text
Domain meaning:                 Owned by Domain
Generic mechanisms:             Owned by Core
Domain events:                  Domain
Domain state semantics:         Domain
Metric meaning:                 Domain / Analysis
Domain patterns:                Domain / Analysis
Domain policies:                Domain / Application
Domain invariants:              Domain
Structural invariants:          Core
Domain ingestion mapping:       Domain
Domain-specific serialization: Domain
Cross-domain translation:       Explicit adapters
Core → Domain dependency:       Forbidden
Domain → Core dependency:       Allowed
New domain:                     Extension
Core modification for domain:   Only when capability is genuinely generic
Domain versioning:              Independent
Domain provenance:              Required when materially relevant
```

## Invariant

**EVolution keeps domain meaning outside generic Core: domains define the semantics of entities, events, state, metrics, patterns, analyses, and policies, while Core provides reusable mechanisms and contracts; new domains must extend those contracts rather than modify generic infrastructure to encode domain-specific assumptions.**

