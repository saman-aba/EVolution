# ADR 0068: Analysis API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent an **Analysis** as an explicit, reproducible interpretation derived from observations, measurements, state, time series, patterns, context, and other analytical evidence.

Conceptually:

```text
Inputs + Analysis Definition + Configuration + Context
    ↓
Analyzer
    ↓
Analysis
```

An Analysis may produce findings, relationships, classifications, estimates, hypotheses, or other structured conclusions.

Analysis must remain distinct from:

```text
Measurement
Aggregation
Pattern
Decision
Action
```

The Core provides the structural contract for Analysis. Domain and analysis components define semantic interpretation.

---

# 1. Analysis

Analysis answers questions such as:

```text
What happened?
What changed?
How unusual was it?
What relationships exist?
What factors are associated?
What explanations are consistent with the evidence?
What might happen next?
```

Analysis is an interpretation of evidence rather than raw observation.

---

# 2. Analysis vs Measurement

A Measurement records a quantitative observation.

```text
Measurement:
    win_rate = 0.42
```

Analysis may interpret it:

```text
Analysis:
    win rate decreased relative to baseline
```

The measurement remains evidence for the analysis.

---

# 3. Analysis vs Aggregation

Aggregation defines how multiple inputs are combined.

Analysis determines what a collection of measurements or aggregated results means.

```text
Measurements
    ↓
Aggregation
    ↓
Statistics
    ↓
Analysis
```

An aggregation does not inherently produce an analytical conclusion.

---

# 4. Analysis vs Pattern

A Pattern identifies structure.

Analysis interprets that structure.

```text
Pattern:
    volatility increased

Analysis:
    volatility increase is unusual relative to historical baseline
```

Pattern detection may be an input to Analysis, but Analysis does not require a Pattern object.

---

# 5. Analysis vs Decision

Analysis produces evidence and interpretation.

Decision selects an outcome according to policy.

```text
Analysis
    ↓
Decision Policy
    ↓
Decision
    ↓
Action
```

Analysis must not silently become a decision.

---

# 6. Analysis Identity

A persistent Analysis may have an identity.

Conceptually:

```text
Analysis
{
    id
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

The exact representation remains deferred.

---

# 7. Analysis Type

Analysis type identifies the semantic category.

Examples:

```text
Descriptive
Comparative
Diagnostic
Statistical
Temporal
Causal
Predictive
Behavioral
```

The set is extensible.

Analysis type is distinct from the C++ implementation type.

---

# 8. Analyzer

An Analyzer is the mechanism that produces an Analysis.

Conceptually:

```text
Analyzer
{
    input
    configuration
    analyze()
}
```

The exact API is deferred.

---

# 9. Analyzer vs Analysis

The distinction is important:

```text
Analyzer:
    method / implementation

Analysis:
    resulting analytical object
```

One Analyzer may produce many Analyses.

Multiple Analyzer implementations may produce the same semantic Analysis type.

---

# 10. Analysis Inputs

Analysis may consume:

```text
Events
State
Measurements
Time Series
Aggregations
Patterns
Previous Analyses
Context
Domain Knowledge
External Data
```

The input contract must explicitly identify which are required.

---

# 11. Evidence

Analysis must distinguish its conclusion from the evidence supporting it.

Conceptually:

```text
Finding
{
    statement
    evidence
}
```

Evidence may reference:

```text
events
measurements
patterns
states
time series
other analyses
external sources
```

---

# 12. Finding

A Finding is a structured statement produced by Analysis.

Examples:

```text
trend increased
behavior differs from baseline
two variables are correlated
observed event frequency is unusually high
prediction exceeds threshold
```

The exact semantic form belongs to the analysis/domain layer.

---

# 13. Finding vs Evidence

The statement:

```text
frequency increased
```

is a Finding.

The observations showing that increase are Evidence.

The system must not conflate them.

---

# 14. Multiple Findings

One Analysis may produce multiple Findings.

For example:

```text
Analysis
    ├── Finding A
    ├── Finding B
    └── Finding C
```

Each finding may have separate evidence and limitations.

---

# 15. Finding Identity

A finding may or may not require persistent identity.

If findings are independently referenced, corrected, or persisted, they should receive logical identity according to the Identity Model.

---

# 16. Reasoning

Analysis may record the reasoning mechanism used to derive a Finding.

Examples:

```text
rule
statistical test
mathematical model
simulation
machine-learning model
graph analysis
program
human-authored interpretation
```

Reasoning should be represented sufficiently to support reproducibility where required.

---

# 17. Reasoning vs Explanation

The reasoning mechanism describes how the result was derived.

An explanatory statement describes what the result means.

They are related but distinct.

---

# 18. Analysis Configuration

Configuration may include:

```text
thresholds
parameters
models
baselines
windows
statistical methods
feature definitions
comparison groups
confidence levels
```

Configuration follows the Configuration Model.

---

# 19. Analysis Context

Analysis may depend on Execution Context:

```text
analysis mode
run identity
randomness
clock
resource limits
dependency versions
security context where relevant
```

Relevant context must be explicit.

---

# 20. Scope

Analysis must define what it applies to.

Examples:

```text
session
player
hand
instrument
market
dataset
population
application
```

Scope semantics belong to the domain.

---

# 21. Time Range

An Analysis may apply to a temporal range:

```text
[start, end)
```

The range identifies the evidence period or analysis period according to the analysis contract.

---

# 22. Analysis Time vs Evidence Time

These may differ.

For example:

```text
Evidence:
    events from January

Analysis executed:
    February
```

The analysis should preserve this distinction where relevant.

---

# 23. Descriptive Analysis

Descriptive Analysis summarizes observed behavior.

Examples:

```text
average value
distribution
frequency
range
trend summary
```

Descriptive Analysis should not imply explanation or causality.

---

# 24. Comparative Analysis

Comparative Analysis evaluates differences between defined groups or periods.

For example:

```text
Group A vs Group B
Before vs After
Current vs Historical
```

The comparison baseline must be explicit.

---

# 25. Diagnostic Analysis

Diagnostic Analysis investigates possible factors associated with an observed result.

It may produce:

```text
candidate explanation
associated variable
contributing factor
alternative explanation
```

Diagnostic findings do not automatically establish causation.

---

# 26. Statistical Analysis

Statistical Analysis applies statistical methods to evidence.

Examples:

```text
hypothesis testing
confidence intervals
distribution analysis
correlation
regression
variance analysis
```

The statistical methodology must be identifiable.

---

# 27. Temporal Analysis

Temporal Analysis studies behavior over time.

Examples:

```text
trend
seasonality
change point
persistence
periodicity
regime transition
```

Temporal semantics follow the Time Series and Pattern models.

---

# 28. Causal Analysis

Causal Analysis attempts to establish causal relationships.

Causality requires stronger assumptions and methodology than correlation.

A causal finding should identify:

```text
causal model
assumptions
identification method
evidence
limitations
```

A generic correlation must never automatically become a causal conclusion.

---

# 29. Predictive Analysis

Predictive Analysis estimates future or otherwise unobserved outcomes.

Conceptually:

```text
Historical Evidence
    ↓
Predictive Model
    ↓
Prediction
```

A prediction must remain distinguishable from an observation.

---

# 30. Prediction

A Prediction may contain:

```text
target
prediction
prediction_time
horizon
uncertainty
model
provenance
```

The exact representation is deferred.

---

# 31. Prediction vs Decision

A prediction does not determine an action.

```text
Prediction:
    probability of outcome = X

Policy:
    if probability > threshold
        take action
```

The policy remains separate.

---

# 32. Behavioral Analysis

Behavioral Analysis identifies behavior characteristics of entities or systems.

Examples:

```text
activity patterns
response behavior
strategy changes
repeated actions
state transitions
```

Domain semantics remain outside Core.

---

# 33. Hypothesis

Analysis may explicitly represent a Hypothesis.

Conceptually:

```text
Hypothesis
{
    statement
    evidence
    assumptions
    test
    result
}
```

A hypothesis is not automatically a conclusion.

---

# 34. Claim Strength

Analytical claims should be distinguishable by strength.

Conceptually:

```text
Observation
Association
Hypothesis
Causal Conclusion
```

The system should not silently promote a weaker claim to a stronger one.

---

# 35. Confidence

Analysis may include confidence.

Confidence semantics must be explicitly defined.

Possible interpretations include:

```text
statistical confidence
model confidence
detector confidence
analyst confidence
```

These must not be conflated.

---

# 36. Probability

A probability is a quantitative property with defined semantics.

It should not be inferred merely from a field named `confidence`.

---

# 37. Uncertainty

Analysis may represent uncertainty.

Uncertainty may arise from:

```text
measurement error
sampling
model uncertainty
parameter uncertainty
input uncertainty
temporal uncertainty
```

Uncertainty is distinct from confidence.

---

# 38. Assumptions

Analysis should record assumptions when they materially affect interpretation.

Examples:

```text
stationarity
independent observations
representative sample
regular sampling
known timestamps
valid model assumptions
```

---

# 39. Limitations

Analysis should be able to record limitations.

Examples:

```text
insufficient data
missing observations
sampling bias
model assumptions
confounding
measurement uncertainty
limited historical range
```

Limitations are part of analytical interpretation, not merely logging.

---

# 40. Baseline

Many analyses require a baseline.

Examples:

```text
historical baseline
population baseline
peer baseline
model baseline
expected value
```

Baseline identity and construction must be explicit.

---

# 41. Counterfactual

Some analyses compare observed outcomes with a counterfactual.

Conceptually:

```text
Observed:
    what happened

Counterfactual:
    what would have happened under another condition
```

Counterfactual assumptions must be explicit.

---

# 42. Statistical Significance

If an Analysis reports statistical significance, the statistical test and assumptions must be identifiable.

A p-value alone is not an explanation or causal conclusion.

---

# 43. Effect Size

Where applicable, analysis should distinguish:

```text
statistical significance
```

from:

```text
practical / effect magnitude
```

A statistically significant effect may have negligible practical significance.

---

# 44. Analysis Composition

An Analysis may consume other Analyses.

For example:

```text
Analysis A
Analysis B
    ↓
Meta Analysis
    ↓
Analysis C
```

The dependency relationship should be represented through evidence/provenance.

---

# 45. Analysis Graph

Complex analyses may form a graph:

```text
Measurements
    ↓
Aggregation
    ↓
Pattern
    ↓
Analysis A
    ↓
Analysis B
    ↓
Analysis C
```

The graph is analytical dependency structure, not necessarily the same as the Processing Graph.

---

# 46. Processing Graph vs Analysis Graph

Processing Graph answers:

> How is computation executed?

Analysis Graph answers:

> How are analytical conclusions dependent on other analytical objects?

They must remain distinct.

---

# 47. Analysis Execution

An Analyzer may execute through the Processing Model.

For example:

```text
Processor
    input: Envelope<AnalysisInput>
    state: AnalyzerState
    output: Envelope<Analysis>
```

Execution semantics remain the responsibility of Processing.

---

# 48. Stateless Analysis

Some analysis can execute entirely from supplied inputs.

For example:

```text
compare(group_a, group_b)
```

No persistent analyzer state is required.

---

# 49. Stateful Analysis

Other analyses may require state.

Examples:

```text
online model
incremental statistics
adaptive baseline
streaming predictor
```

The state is execution state unless explicitly modeled as domain State.

---

# 50. Incremental Analysis

Analysis may update as new evidence arrives.

Conceptually:

```text
Evidence 1
    ↓
partial analysis

Evidence 2
    ↓
updated analysis
```

The distinction between evolving execution state and persistent analytical history must be explicit.

---

# 51. Analysis Versioning

Analytical results may depend on:

```text
analysis definition
configuration
algorithm version
model version
input versions
dependency versions
```

Changes that alter semantics require appropriate version distinction.

---

# 52. Model Identity

If Analysis uses a model, the model must have identifiable version/identity where reproducibility requires it.

Model identity is distinct from Analysis identity.

---

# 53. External Models

External models are dependencies.

Their:

```text
identity
version
configuration
availability
reproducibility
```

follow the External Dependency Model.

---

# 54. Randomness

Randomized analysis must receive randomness explicitly.

Examples:

```text
Monte Carlo
random sampling
stochastic optimization
randomized algorithms
```

Random seeds or equivalent reproducibility information should be captured where necessary.

---

# 55. Current Time

Analysis must not silently depend on wall-clock time when deterministic replay is required.

If current time is semantically relevant, it must be supplied through Execution Context.

---

# 56. Evidence Alignment

When multiple evidence sources are combined, temporal and semantic alignment must be explicit.

For example:

```text
Series A
    +
Series B
    ↓
comparison
```

requires compatible time semantics.

---

# 57. Missing Evidence

Missing evidence must not automatically become a negative finding.

For example:

```text
missing observation
```

is not equivalent to:

```text
observation = 0
```

The analysis must define its missing-data policy.

---

# 58. Unknown Evidence

Unknown values remain distinguishable from known values.

An analysis must not silently convert unknown evidence into a concrete value.

---

# 59. Insufficient Evidence

Insufficient evidence may produce:

```text
no conclusion
inconclusive result
confidence limitation
explicit analysis failure
```

depending on the contract.

It must not automatically produce a positive or negative conclusion.

---

# 60. Analysis Result

An Analysis may contain:

```text
findings
evidence
reasoning
assumptions
confidence
uncertainty
limitations
provenance
```

Not every analysis requires every field.

The semantic contract determines which are required.

---

# 61. Analysis Finding Structure

Conceptually:

```text
Finding
{
    statement
    evidence
    reasoning?
    confidence?
    limitations?
}
```

The exact C++ representation remains deferred.

---

# 62. Structured Findings

Findings should be structured where downstream consumers need machine-readable interpretation.

For example:

```text
Finding
{
    type: "trend"
    direction: "increasing"
    magnitude: ...
    evidence: ...
}
```

The specific structure belongs to the domain/analysis implementation.

---

# 63. Natural-Language Findings

Human-readable explanations may accompany structured findings.

Natural-language text must not be the only representation when machine-readable semantics are required.

---

# 64. Explanation

An Analysis may provide an explanation describing the reasoning in human-readable form.

The explanation should not replace structured evidence or provenance.

---

# 65. Analysis Provenance

Analysis provenance should identify, where relevant:

```text
inputs
analysis definition
configuration
algorithm/model version
execution/run
external dependencies
```

---

# 66. Analysis Reproducibility

Reproduction should follow:

```text
Inputs
+
Effective Configuration
+
Analysis/Algorithm Version
+
Relevant Execution Context
+
Required State
    ↓
Analysis
```

---

# 67. Analysis Determinism

A deterministic Analyzer should produce equivalent results from equivalent semantic inputs and relevant conditions.

Representation-level differences do not necessarily imply semantic differences.

---

# 68. Analysis Quality

Analysis quality should be evaluated separately from Analysis representation.

Possible evaluation measures include:

```text
accuracy
precision
recall
calibration
prediction error
stability
robustness
reproducibility
```

The relevant measure depends on analysis type.

---

# 69. Analysis Evaluation

An Analysis may itself be evaluated by a later Analysis or evaluation component.

For example:

```text
Prediction
    ↓
Outcome
    ↓
Prediction Evaluation
```

Evaluation is not automatically part of the original Analysis.

---

# 70. Analysis and Context

The same evidence may produce different interpretations under different contexts.

Context may include:

```text
population
environment
market regime
session conditions
domain configuration
```

Context must remain explicit.

---

# 71. Domain Knowledge

Analysis may use domain knowledge.

Examples:

```text
poker rules
financial market conventions
physical constraints
protocol semantics
```

Such meaning belongs to the Domain or Analysis layer, not generic Core.

---

# 72. Analysis Dependencies

Analysis may depend on:

```text
statistics libraries
models
datasets
external services
domain rules
previous analytical results
```

Dependencies follow the External Dependency Model.

---

# 73. Resource Requirements

Analysis may require:

```text
CPU
memory
GPU
storage
execution slots
```

These requirements belong to the Resource Model.

---

# 74. Error Handling

Analyzer execution uses `Result<T>` / `Error`.

Possible errors include:

```text
invalid input
insufficient required data
invalid configuration
unsupported analysis
dependency failure
resource failure
processing failure
```

No failure may silently become a valid analytical conclusion.

---

# 75. Cancellation

Long-running analysis may be cancelled cooperatively.

Cancellation is distinct from:

```text
successful analysis
inconclusive analysis
analysis failure
```

---

# 76. Partial Analysis

Some analyses may produce partial findings.

Partial results must be explicitly distinguishable from complete results.

For example:

```text
status = PARTIAL
```

may be part of the contract where necessary.

---

# 77. Persistence

Analysis results may be persisted according to their role:

```text
AUTHORITATIVE
DERIVED
RECOVERABLE
CACHE
EPHEMERAL
```

Persistence does not change the semantic meaning of Analysis.

---

# 78. Serialization

Serialization must preserve all semantic information necessary to interpret the Analysis.

Where applicable:

```text
identity
type
scope
time range
findings
evidence
reasoning
assumptions
limitations
confidence
provenance
version
```

---

# 79. Security

Analyses may contain sensitive information.

Security and access control remain separate from analytical semantics.

Sensitive information must not be accidentally exposed through:

```text
logs
errors
metrics
provenance
debug output
```

---

# 80. Testing

Analysis implementations should test:

```text
known inputs
known conclusions
negative cases
boundary cases
missing evidence
unknown values
insufficient evidence
configuration changes
baseline changes
time alignment
out-of-order data
determinism
randomness
model versions
dependency versions
provenance
replay
cancellation
partial results
errors
```

Analytical correctness tests should be separated from infrastructure tests.

---

# 81. Regression Tests

Important analytical cases should be preserved as regression datasets.

Changes to algorithms should be evaluated against known historical inputs.

---

# 82. Golden Results

Where deterministic analytical results are expected, golden results may be stored.

Golden data must include sufficient version/configuration information to determine whether a changed result is expected.

---

# 83. Analysis API Boundary

The future Core API should conceptually expose:

```text
evolution::analysis::Analysis
evolution::analysis::Finding
evolution::analysis::Analyzer
```

Potential supporting types may include:

```text
AnalysisType
Evidence
Reasoning
Confidence
```

but exact decomposition remains deferred.

---

# 84. Generic Core vs Specialized Analysis

Generic Core should provide the structural mechanisms required for Analysis.

Specialized analysis components should define:

```text
domain metrics
statistical methods
domain findings
domain reasoning
domain models
domain interpretation
```

---

# 85. No Universal Analysis Object

EVolution should not create a universal Analysis type containing arbitrary:

```text
map<string, any>
```

for every possible analytical output.

Analysis should remain strongly structured.

---

# 86. Analysis Registration

Analysis implementations may participate in the Registration and Discovery Model.

A registered analyzer may expose:

```text
identity
version
capabilities
input requirements
configuration schema
output type
resource requirements
```

Registration does not imply activation.

---

# 87. Analysis and Processing

An Analyzer may be executed by the Processing layer.

Processing controls:

```text
scheduling
concurrency
backpressure
delivery
recovery
resources
lifecycle
```

Analysis controls:

```text
analytical semantics
```

---

# 88. Analysis and Storage

Analysis must not directly depend on a particular storage implementation.

It should receive data through defined query/read interfaces.

---

# 89. Analysis and Interfaces

CLI, GUI, HTTP, RPC, or other interfaces may expose Analysis results.

Interfaces must not implement analytical semantics.

---

# 90. Analysis and Applications

Applications determine why an analysis is executed and how its result is used.

For example:

```text
Application
    ↓
select Analyzer
    ↓
execute
    ↓
consume Analysis
```

The application does not redefine the meaning of the analysis.

---

# 91. Analysis and Decision Policy

A Policy may consume analytical findings:

```text
Analysis
    ↓
Policy
    ↓
Decision
```

The policy determines the action semantics.

---

# 92. Deferred Decisions

This ADR does not select:

* exact C++ Analysis API
* Finding type hierarchy
* universal confidence representation
* universal uncertainty representation
* statistical library
* ML framework
* model format
* explanation format
* causal inference framework
* prediction representation
* analysis graph implementation
* distributed analysis execution
* result persistence format
* query language
* visualization framework

---

# Decision Summary

```text
Analysis:
    Structured interpretation of evidence

Analyzer:
    Mechanism producing Analysis

Finding:
    Explicit analytical statement

Evidence:
    Inputs supporting a Finding

Reasoning:
    Method used to derive a Finding

Assumptions:
    Conditions required for interpretation

Limitations:
    Known boundaries of the conclusion

Confidence:
    Explicit detector/analysis-defined semantics

Uncertainty:
    Explicit uncertainty representation

Baseline:
    Reference used for comparison

Hypothesis:
    Testable analytical proposition

Prediction:
    Estimate about an unobserved/future outcome

Causality:
    Stronger claim requiring explicit methodology

Provenance:
    Origin and derivation of Analysis

Context:
    Circumstances in which Analysis applies

Decision:
    Separate policy-driven layer
```

## Invariant

**An Analysis is a structured and reproducible interpretation of explicit evidence; its findings, reasoning, assumptions, uncertainty, limitations, and provenance remain distinguishable from the underlying observations, the analytical algorithm, and any decision or action derived from the result.**

