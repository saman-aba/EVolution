# Context Model

## 1. Purpose

Context describes the circumstances in which an observation, event, state, measurement, pattern, analysis, or decision exists.

Context answers questions such as:

```text
What is this about?
Who or what does it belong to?
Under what environment did it occur?
Which entities are involved?
What conditions surrounded it?
```

Context provides the information required to interpret data without becoming the interpretation itself.

The conceptual relationship is:

```text
Data
  +
Context
  ↓
Meaningful Interpretation
```

Context does not determine the interpretation. It provides the information required to perform one.

---

## 2. Context vs Data

Data describes an observation.

Context describes the circumstances surrounding that observation.

For example:

```text
Measurement:
    profit = +12 BB

Context:
    player = PlayerA
    table = Table7
    stake = 10 BB
    session = Session42
    game = NLHE
```

The measurement itself does not change.

Its interpretation may change substantially depending on its context.

---

## 3. Context Is Not a Single Object

Context may exist at different levels.

For example:

```text
System
 └── User
      └── Account
           └── Session
                └── Hand
                     └── Action
```

Or in another domain:

```text
Market
 └── Exchange
      └── Instrument
           └── Trading Session
                └── Order
                     └── Execution
```

These are domain-specific structures.

The core should provide mechanisms for representing relationships between contextual entities without assuming their meaning.

---

## 4. Contextual Entity

A contextual entity is something that provides identity or scope for observations.

Conceptually:

```text
Entity
{
    id
    type
    attributes
}
```

Examples include:

```text
Player
Table
Session
Game
Instrument
Exchange
Device
Experiment
Simulation
Dataset
```

The meaning of the entity type belongs to the domain.

The core only requires that entities can be identified and referenced.

---

## 5. Context Reference

Events and derived objects may reference contextual entities.

For example:

```text
Event
{
    type: PlayerAction
    timestamp: ...
    context:
        player: PlayerA
        table: Table7
        session: Session42
}
```

A measurement may use the same context:

```text
Measurement
{
    metric: win_rate
    value: 8.2
    context:
        player: PlayerA
        session: Session42
}
```

This allows different layers of the system to refer to the same contextual scope without duplicating its complete description.

---

## 6. Scope

Scope defines the boundary over which an object is meaningful.

Examples:

```text
single hand
session
player
table
day
week
account
dataset
experiment
```

Scope and context are related but not identical.

For example:

```text
Context:
    player = A
    table = 7
    session = 42

Scope:
    session 42
```

The context describes the circumstances.

The scope defines what portion of the system the object refers to.

---

## 7. Dimensions

Dimensions identify properties along which observations can be separated or grouped.

For example:

```text
metric:
    profit

dimensions:
    player = A
    stake = 10BB
    game_type = NLHE
    table_size = 6max
```

Dimensions are particularly important for:

* grouping
* filtering
* aggregation
* comparison
* time-series construction
* analysis

A dimension should represent an explicit property rather than an implicit assumption.

---

## 8. Context Hierarchy

Contexts may form hierarchical relationships.

For example:

```text
Account
   │
   └── Session
          │
          ├── Hand
          │     ├── Action
          │     └── Action
          │
          └── Hand
```

A child context may inherit some properties from its parent.

For example:

```text
Session
    player = A
    game = NLHE

Hand
    inherits player = A
    inherits game = NLHE
```

Inheritance should be explicit.

A value should not be assumed to remain constant merely because it was constant historically.

---

## 9. Context Changes

Context can change over time.

For example:

```text
Session
    stake = 10 BB

    ↓

Session
    stake = 20 BB
```

The system must distinguish:

```text
Current context
```

from:

```text
Historical context
```

When a contextual property changes, the change should be represented in a way that preserves temporal correctness.

This may be achieved through events, versions, intervals, or another mechanism.

The exact implementation remains undecided.

---

## 10. Static and Dynamic Context

Some contextual information changes rarely or never:

```text
instrument identity
player identity
device model
dataset identity
```

Other contextual information changes continuously:

```text
current stake
table composition
network conditions
market regime
session state
```

The system should not assume that all context is static metadata.

Dynamic context may itself require temporal representation.

---

## 11. Environment

Environment describes external conditions surrounding an observation.

Examples:

```text
software version
hardware
network
location
market
weather
simulation configuration
game configuration
```

Environment can materially affect measurements and analysis.

For example:

```text
Measurement:
    processing_latency = 80 µs

Environment:
    CPU = ...
    workload = ...
    packet_size = ...
    configuration = ...
```

The measurement remains 80 µs, but the environmental context determines how that measurement should be interpreted or compared.

---

## 12. Context and Provenance

Context and provenance are related but distinct.

Context answers:

```text
Under what circumstances does this object exist?
```

Provenance answers:

```text
Where did this object come from?
How was it produced?
```

For example:

```text
Measurement
    context:
        session = 42
        player = A

    provenance:
        source = hand_history
        derived_from = events 1000..2500
        calculation = metric-v3
```

Both may be required to reproduce or understand an analytical result.

---

## 13. Contextual Identity

An object may be uniquely identified by both its own identity and its context.

For example:

```text
profit
    player = A
    session = 42
```

is different from:

```text
profit
    player = B
    session = 42
```

Therefore contextual identity may participate in:

* storage
* indexing
* aggregation
* comparison
* caching
* lookup

The core should support contextual identity without imposing a particular storage mechanism.

---

## 14. Context and Time

Context may itself be temporal.

Conceptually:

```text
Context
{
    entity
    attributes
    valid_from
    valid_until
}
```

This allows questions such as:

```text
What was the context when this event occurred?
```

rather than merely:

```text
What is the context now?
```

Historical correctness is particularly important when reconstructing state or reproducing analysis.

---

## 15. Context Resolution

Different analyses may require different levels of contextual detail.

For example:

```text
Analysis A:
    player-level

Analysis B:
    player + stake

Analysis C:
    player + stake + table size + session

Analysis D:
    individual hand
```

The system should therefore support context at multiple resolutions.

More context does not automatically produce better analysis.

Unnecessary dimensions can increase complexity, reduce comparability, and fragment datasets.

---

## 16. Contextual Filtering

Context can be used to select observations.

Conceptually:

```text
Input:
    all measurements

Filter:
    player = A
    stake = 10BB
    game = NLHE

Output:
    matching measurements
```

Filtering should operate on explicit contextual properties.

It should not require domain-specific knowledge inside the generic filtering mechanism.

---

## 17. Context and Comparison

Comparisons should account for contextual differences.

For example:

```text
Measurement A:
    win_rate = 8 BB/100
    stake = 10BB

Measurement B:
    win_rate = 8 BB/100
    stake = 100BB
```

The numerical values are identical, but their contexts differ.

A comparison system must therefore preserve context rather than treating measurements as isolated numbers.

Whether two contextual groups are comparable is an analytical question.

---

## 18. Context Does Not Imply Causation

Context should never automatically be interpreted as a cause.

For example:

```text
Context:
    high_stakes = true

Observation:
    performance decreased
```

This does not establish:

```text
high_stakes caused performance to decrease
```

Context identifies circumstances.

Analysis determines relationships between those circumstances and observations.

---

## 19. Context Lifecycle

Context may have its own lifecycle.

A simplified model is:

```text
CREATED
   ↓
ACTIVE
   ↓
CHANGED
   ↓
ENDED
```

Not every contextual entity requires all stages.

For example:

```text
Session
    CREATED
    ACTIVE
    ENDED
```

while:

```text
Player
    CREATED
    ACTIVE
```

may have a substantially different lifecycle.

Lifecycle semantics belong to the domain.

---

## 20. Contextual Model

A generic conceptual representation is:

```text
Context
{
    scope
    entities
    dimensions
    environment
    validity
}
```

Where:

```text
scope
    defines what the object refers to

entities
    identify relevant objects

dimensions
    describe grouping/filtering properties

environment
    describes surrounding conditions

validity
    defines when the context applies
```

This is a conceptual model rather than a required data structure.

---

## 21. Core Architectural Invariant

The core should understand how context can be identified, attached, queried, and related.

It should not decide what contextual entities mean.

The fundamental separation is:

```text
Entity
    ↓
Context
    ↓
Observation
    ↓
Analysis
```

Therefore:

> **Context describes the circumstances and scope of information; it does not determine its interpretation.**
