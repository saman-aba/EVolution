# EVolution Analysis Model

## 1. Purpose

An **Analysis** is a structured interpretation derived from observations, measurements, states, time series, patterns, and contextual information.

The conceptual pipeline is:

```text id="a1"
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
```

Analysis answers questions such as:

* What does the observed structure indicate?
* Is the observed behavior unusual?
* Which factors are associated with the observed result?
* Does the evidence support a particular hypothesis?
* Has the behavior of the system changed?
* What explanations are consistent with the evidence?

Analysis is therefore a higher abstraction than Pattern.

---

## 2. Analysis vs Pattern

A Pattern describes observed structure.

An Analysis interprets that structure.

For example:

```text id="a2"
Pattern:

    session duration increased
    while profit per hand decreased
```

An analysis may produce:

```text id="a3"
Analysis:

    Longer sessions are associated with
    lower observed performance in the
    analyzed dataset.
```

The pattern describes the data.

The analysis gives the pattern contextual meaning.

---

## 3. Analysis vs Measurement

A measurement provides a quantitative value.

For example:

```text id="a4"
average_session_length = 3.2 hours
```

An analysis may use that measurement together with others:

```text id="a5"
average_session_length = 3.2 hours
profit_rate = -1.8 BB/100
decision_error_rate = +34%
```

and produce:

```text id="a6"
Analysis:

    Performance deterioration is concentrated
    in sessions longer than approximately
    three hours.
```

The conclusion is not itself a measurement.

---

## 4. Analysis vs State

State represents the current or reconstructed condition of a domain.

Analysis interprets state.

For example:

```text id="a7"
State:
    bankroll = 120 BB
    session_duration = 4 hours
```

Analysis:

```text id="a8"
The current session satisfies the conditions
associated with the previously observed
high-duration performance pattern.
```

State remains descriptive.

Analysis is interpretive.

---

## 5. Analysis Inputs

An analysis may consume multiple types of information:

```text id="a9"
Events
State
Measurements
Time Series
Patterns
Historical Analyses
Domain Knowledge
Configuration
External Context
```

For example:

```text id="a10"
                  ┌── Measurements
                  ├── Time Series
Inputs ───────────┼── Patterns
                  ├── State
                  └── Domain Context
                         │
                         ▼
                      Analysis
```

The analysis layer should not require every input type.

---

## 6. Analysis Output

An analysis produces a structured analytical result.

Conceptually:

```text id="a11"
Analysis
{
    type
    scope
    time_range

    findings
    evidence
    reasoning

    confidence?
    limitations
    provenance
}
```

The exact representation remains undecided.

The output should contain enough information to understand what was concluded and why.

---

## 7. Finding

A **Finding** is a statement produced by an analysis.

Examples:

```text id="a12"
Performance decreased during long sessions.

The observed decline is concentrated
in the final third of sessions.

The observed bankroll decline is not
accompanied by a corresponding decline
in EV.
```

A finding should be distinguishable from the evidence supporting it.

Conceptually:

```text id="a13"
Evidence
   ↓
Reasoning
   ↓
Finding
```

---

## 8. Evidence

An analysis must be able to identify the evidence supporting a finding.

Evidence may include:

```text id="a14"
measurements
patterns
events
state snapshots
time ranges
historical comparisons
statistical results
external observations
```

For example:

```text id="a15"
Finding:
    performance decreases after 3 hours.

Evidence:
    sessions 10–42
    session duration measurements
    profit measurements
    performance time series
```

The evidence should be traceable to underlying data.

---

## 9. Reasoning

Reasoning describes how evidence supports a finding.

For example:

```text id="a16"
Evidence:

    sessions > 3h:
        -2.4 BB/100

    sessions <= 3h:
        +1.7 BB/100

    sample size:
        42 sessions

Reasoning:

    The observed performance difference
    is concentrated in sessions exceeding
    three hours.
```

Reasoning is not necessarily natural-language text.

It may eventually be represented by:

```text id="a17"
rules
expressions
statistical models
graphs
programs
machine-learning models
```

The architecture must not assume one reasoning mechanism.

---

## 10. Analysis Types

Analysis may take different forms.

Examples:

```text id="a18"
Descriptive Analysis
Comparative Analysis
Diagnostic Analysis
Statistical Analysis
Temporal Analysis
Causal Analysis
Predictive Analysis
Anomaly Analysis
Behavioral Analysis
```

These categories describe analytical intent rather than implementation.

---

## 11. Descriptive Analysis

Descriptive analysis summarizes what happened.

Example:

```text id="a19"
The player completed 4,200 hands
over 17 sessions with a total result
of +83 BB.
```

This does not attempt to explain the result.

---

## 12. Comparative Analysis

Comparative analysis compares two or more populations, periods, states, or conditions.

Example:

```text id="a20"
Sessions shorter than three hours:
    +2.1 BB/100

Sessions longer than three hours:
    -1.4 BB/100
```

The comparison identifies a difference.

It does not automatically establish why the difference exists.

---

## 13. Diagnostic Analysis

Diagnostic analysis attempts to identify factors associated with an observed result.

Example:

```text id="a21"
Observed:
    performance declined.

Relevant patterns:
    increased session duration
    increased decision-error frequency
    increased variance
```

The analysis may conclude:

```text id="a22"
The observed deterioration is associated
with increased session duration and
decision-error frequency.
```

Association must not automatically be presented as causation.

---

## 14. Statistical Analysis

Statistical analysis applies statistical methods to data.

Examples:

```text id="a23"
hypothesis testing
confidence intervals
regression
correlation
distribution comparison
change-point detection
variance analysis
sampling analysis
```

Statistical results must preserve their statistical meaning.

For example:

```text id="a24"
p-value = 0.03
```

must not be described simply as:

```text id="a25"
confidence = 97%
```

unless that interpretation is mathematically justified.

---

## 15. Causal Analysis

Causal analysis attempts to determine whether a relationship represents a causal effect.

This requires stronger assumptions and methodology than ordinary pattern detection or correlation.

For example:

```text id="a26"
Pattern:
    longer sessions correlate with worse results.
```

does not establish:

```text id="a27"
longer sessions cause worse results.
```

A causal analysis may require:

```text id="a28"
controlled experiments
causal models
confounder analysis
natural experiments
domain assumptions
```

Causal conclusions must therefore explicitly identify their assumptions and limitations.

---

## 16. Predictive Analysis

Predictive analysis estimates future or unknown values based on historical information.

Examples:

```text id="a29"
expected future value
probability of an event
forecasted metric
classification
```

A prediction is fundamentally different from an observed measurement.

For example:

```text id="a30"
Observed:
    current bankroll = 120 BB

Prediction:
    estimated bankroll distribution
    after the next 1000 hands
```

Predictions must therefore be explicitly identified as predictions.

---

## 17. Hypothesis

An analysis may formulate a **Hypothesis**.

Conceptually:

```text id="a31"
Hypothesis
{
    statement
    evidence
    assumptions
    test
    result
}
```

For example:

```text id="a32"
Hypothesis:

    Performance decreases after
    prolonged sessions.

Test:

    Compare performance across
    session-duration groups.
```

A hypothesis is not automatically true merely because a pattern supports it.

---

## 18. Analysis Claims

An analysis may contain claims with different evidentiary strength.

For example:

```text id="a33"
Observation:
    A and B occurred together.

Association:
    A and B are statistically associated.

Hypothesis:
    A may influence B.

Causal conclusion:
    A causes B.
```

These statements have materially different meanings.

EVolution should preserve these distinctions rather than collapsing them into a generic "insight."

---

## 19. Confidence and Uncertainty

An analysis may contain uncertainty.

Conceptually:

```text id="a34"
Analysis
{
    finding
    confidence?
    uncertainty?
    assumptions
    limitations
}
```

Confidence must have defined semantics.

Possible sources include:

```text id="a35"
statistical confidence
model probability
rule certainty
detector confidence
expert assessment
```

These must not be treated as interchangeable.

---

## 20. Assumptions

Analyses often depend on assumptions.

For example:

```text id="a36"
Assumption:

    Imported hand histories are complete.

Assumption:

    All recorded sessions belong to
    the same player.

Assumption:

    The sampled population is
    representative of the analyzed period.
```

Assumptions should be explicit when they materially affect the conclusion.

---

## 21. Limitations

An analysis should be able to describe limitations.

Examples:

```text id="a37"
small sample size
missing observations
selection bias
measurement error
unknown variables
model limitations
insufficient historical data
```

This prevents analytical conclusions from appearing more certain than the evidence supports.

---

## 22. Analysis Provenance

An analysis must be traceable to the information from which it was produced.

Conceptually:

```text id="a38"
Analysis
   │
   ├── inputs
   ├── detector / analyzer
   ├── version
   ├── configuration
   ├── assumptions
   └── output
```

For example:

```text id="a39"
analyzer:
    long_session_analysis

version:
    1.2

input:
    sessions 100–250

configuration:
    duration_threshold = 3h
```

This is essential for reproducibility.

---

## 23. Analysis Reproducibility

Given:

```text id="a40"
same input data
+
same analyzer
+
same version
+
same configuration
```

a deterministic analysis should produce the same result.

If the analysis uses stochastic methods, randomness and model state must be controlled or recorded where reproducibility matters.

---

## 24. Analysis Composition

Analyses may consume previous analyses.

For example:

```text id="a41"
Measurements
      ↓
Pattern Detection
      ↓
Analysis A
      ↓
Analysis B
```

Example:

```text id="a42"
Analysis A:
    identifies a persistent performance decline.

Analysis B:
    compares that decline with session duration,
    decision errors, and historical regimes.
```

This allows analytical reasoning to be constructed hierarchically.

---

## 25. Analysis Graph

Complex analyses may be represented as a dependency graph.

```text id="a43"
Measurements
   │
   ├────→ Pattern A ────┐
   │                    │
   ├────→ Pattern B ────┼──→ Analysis A
   │                    │
   └────→ Pattern C ────┘
                            │
                            ▼
                       Analysis B
```

This is preferable to treating analysis as an opaque block.

The graph makes dependencies explicit.

---

## 26. Analysis Context

The same pattern may have different meanings in different contexts.

For example:

```text id="a44"
Pattern:
    bankroll declined 20 BB
```

may occur during:

```text id="a45"
normal variance
```

or:

```text id="a46"
unusual decision behavior
```

depending on the surrounding evidence.

Therefore analysis may include contextual information:

```text id="a47"
domain
time period
population
historical baseline
current state
configuration
```

---

## 27. Baselines

Many analyses require a baseline.

For example:

```text id="a48"
Current win rate:
    -1.2 BB/100

Historical baseline:
    +2.8 BB/100
```

The analysis can then compare current behavior against historical behavior.

A baseline must have explicit scope and time range.

A poorly defined baseline can produce misleading conclusions.

---

## 28. Counterfactuals

Some analyses may eventually use counterfactual reasoning.

For example:

```text id="a49"
Observed:
    player continued playing for 4 hours.

Counterfactual:
    what would performance have been
    if the session had ended after 3 hours?
```

A counterfactual is not an observation.

It depends on a model or set of assumptions.

Therefore counterfactual results must be clearly identified as modeled quantities.

---

## 29. Analysis Result vs Action

An analysis may produce information that can be used by another component to make a decision.

For example:

```text id="a50"
Analysis:
    sustained negative performance
    under condition X.
```

A separate policy may then decide:

```text id="a51"
Decision:
    stop processing condition X.
```

The analysis itself should not inherently perform the action.

This separation allows the same analysis to be consumed by:

* a UI
* a report
* an automation system
* another analysis
* a decision engine
* a human

---

## 30. Analysis vs Decision

This is a fundamental boundary.

```text id="a52"
Analysis:
    describes what the evidence indicates.

Decision:
    chooses what should happen.
```

For example:

```text id="a53"
Analysis:
    Performance has deteriorated
    during sessions exceeding three hours.
```

A separate decision policy may choose:

```text id="a54"
Decision:
    limit future sessions to three hours.
```

EVolution should not embed the second statement into the first.

---

## 31. Analysis vs Recommendation

A recommendation is even more policy-dependent than an analysis.

For example:

```text id="a55"
Analysis:
    condition X is associated with
    a 2.3 BB/100 lower result.
```

Recommendation:

```text id="a56"
Recommendation:
    avoid condition X.
```

The recommendation requires a policy or decision criterion.

Therefore recommendation generation should remain separate from the analytical core.

---

## 32. Analysis Types Are Extensible

The core should not contain a fixed exhaustive list of analysis types.

New domains may introduce new forms of analysis.

For example:

```text id="a57"
Poker:
    TiltAnalysis
    SessionAnalysis

Network:
    CongestionAnalysis
    FailureAnalysis

Scientific:
    ExperimentAnalysis

Financial:
    RegimeAnalysis
    RiskAnalysis
```

The core provides the analytical framework.

Domains provide semantic analyzers.

---

## 33. Conceptual Contract

The initial conceptual model is:

```text id="a58"
Analysis
{
    type
    scope
    time_range

    findings
    evidence
    reasoning

    assumptions
    limitations

    provenance

    confidence?
}
```

An analyzer is conceptually:

```text id="a59"
Analyzer
{
    input
    configuration
    analyze()
}
```

A finding is conceptually:

```text id="a60"
Finding
{
    statement
    evidence
    reasoning
}
```

These are conceptual models only.

The following remain intentionally undecided:

* rule engine
* statistical framework
* machine-learning integration
* natural-language generation
* analysis persistence
* analyzer scheduling
* analyzer composition
* causal inference framework
* prediction framework
* decision integration

---

## 34. Core Invariant

The EVolution core may provide mechanisms for constructing, executing, and representing analyses, but it must not decide what a domain should conclude or what action a user should take.

The fundamental distinction is:

```text id="a61"
Measurement:
    what value exists?

Aggregation:
    how were values combined?

Pattern:
    what structure exists?

Analysis:
    what does the available evidence indicate?

Decision:
    what should be done?
```

The last step is outside the analytical core.

---

## 35. Current Conceptual Pipeline

The architecture now becomes:

```text id="a62"
                    RAW OBSERVATIONS
                          │
                          ▼
                       Events
                          │
                          ▼
                        State
                          │
                          ▼
                    Measurements
                          │
                          ▼
                     Time Series
                          │
                          ▼
                     Aggregation
                          │
                          ▼
                       Patterns
                          │
                          ▼
                       Analysis
                          │
                          ▼
                 Decision / Application
```

Each layer should have a clearly defined semantic responsibility.

The system should preserve provenance between layers so that a high-level analysis can be traced back to the observations that produced it.
