# ADR 0069: Decision API Design

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution will represent a **Decision** as an explicit selection of an outcome based on evidence, context, objectives, constraints, and a defined policy.

Conceptually:

```text
Evidence + Context + Policy + Objectives + Constraints
    ↓
Decision Evaluation
    ↓
Decision
```

A Decision is downstream of analysis and is distinct from:

```text
Event
State
Measurement
Pattern
Analysis
Action
```

EVolution may provide generic mechanisms for representing and evaluating decisions, but domain-specific decision policies remain outside generic Core semantics.

---

# 1. Decision

A Decision represents a selected outcome.

Conceptually:

```text
Decision
{
    id
    policy
    scope
    inputs
    outcome
    reasoning
    confidence?
    constraints
    provenance
}
```

The exact C++ representation remains deferred.

---

# 2. Decision vs Analysis

Analysis answers:

> What does the evidence indicate?

Decision answers:

> Given the evidence, objectives, constraints, and policy, what should be selected?

For example:

```text
Analysis:
    risk increased

Decision:
    reduce exposure
```

The decision requires policy.

---

# 3. Decision vs Action

A Decision selects or recommends an outcome.

An Action performs an operation.

```text
Decision
    ↓
Action
    ↓
New Events
```

A Decision does not imply that the action was executed.

---

# 4. Recommendation

A Decision may be a recommendation rather than an executed action.

For example:

```text
Recommendation:
    do not enter position
```

The recommendation does not establish that the position was actually avoided.

---

# 5. Policy

A Policy defines how decisions should be made under specified conditions.

Conceptually:

```text
Policy
{
    name
    scope
    conditions
    objectives
    constraints
    actions
}
```

The exact representation remains deferred.

---

# 6. Policy vs Decision

The distinction is:

```text
Policy:
    rule / strategy for making decisions

Decision:
    result of applying the policy
```

A single Policy may produce many Decisions.

---

# 7. Policy Identity

A persistent Policy should have logical identity.

Policy identity is distinct from:

```text
DecisionId
AnalysisId
RunId
ApplicationId
ActionId
```

---

# 8. Policy Version

Policy changes that can alter decision outcomes must be versioned.

For example:

```text
RiskPolicy v1
RiskPolicy v2
```

must remain distinguishable where reproducibility matters.

---

# 9. Policy Configuration

Policy configuration may include:

```text
thresholds
weights
objectives
constraints
allowed outcomes
risk limits
priority rules
```

Configuration follows the Configuration Model.

---

# 10. Policy Scope

A Policy may apply to a defined scope.

Examples:

```text
player
session
portfolio
instrument
market
application
system
```

Domain semantics belong outside Core.

---

# 11. Decision Scope

A Decision records the scope to which the selected outcome applies.

The decision scope may differ from the scope of individual evidence objects.

---

# 12. Decision Inputs

A Decision may consume:

```text
Analysis
Pattern
Measurement
State
Context
Policy
External Information
Previous Decisions
```

The required inputs must be explicit.

---

# 13. Evidence

Decision inputs should distinguish between:

```text
evidence
```

and:

```text
policy
```

Evidence describes the situation.

Policy determines how that situation should influence the decision.

---

# 14. Decision Outcome

The outcome is the selected result.

Examples:

```text
APPROVE
REJECT
WAIT
SELECT_A
SELECT_B
NO_ACTION
RECOMMEND
```

Outcome semantics are domain-specific.

---

# 15. No Universal Outcome Enumeration

The Core must not define a universal list of domain decisions.

For example:

```text
FOLD
CALL
RAISE
```

belongs to a Poker domain, not generic EVolution Core.

---

# 16. Objective

A Policy may define objectives.

Examples:

```text
maximize return
minimize risk
minimize latency
maximize reliability
preserve resources
```

Objectives must be explicit when they influence selection.

---

# 17. Multiple Objectives

Policies may have multiple objectives.

Their relationship must be defined.

Possible semantics include:

```text
priority
weighted optimization
lexicographic ordering
constraint-first selection
Pareto optimization
```

No universal optimization strategy is assumed.

---

# 18. Constraint

Constraints define conditions that a Decision must satisfy.

Examples:

```text
maximum risk
maximum resource usage
minimum confidence
budget
time limit
legal restriction
```

Constraints are separate from objectives.

---

# 19. Hard Constraints

A hard constraint must not be violated by a valid Decision.

For example:

```text
risk <= maximum_allowed_risk
```

Failure to satisfy a hard constraint may make the decision infeasible.

---

# 20. Soft Constraints

A soft constraint may be violated with an explicit penalty or tradeoff.

The policy must define how the tradeoff works.

---

# 21. Feasibility

A policy may produce:

```text
feasible
infeasible
undetermined
```

before selecting an outcome.

Infeasibility is not necessarily an execution error.

---

# 22. No Valid Decision

A policy may legitimately conclude:

```text
NO_DECISION
```

when evidence or constraints do not permit a valid outcome.

This is distinct from policy execution failure.

---

# 23. Insufficient Evidence

Insufficient evidence may result in:

```text
WAIT
REQUEST_MORE_INFORMATION
NO_DECISION
```

depending on policy.

The policy determines the semantics.

---

# 24. Decision Confidence

A Decision may have confidence when the policy supports such a concept.

Confidence semantics must be explicit.

Decision confidence must not automatically inherit Analysis confidence.

---

# 25. Decision Uncertainty

A Decision may depend on uncertain evidence.

The uncertainty should remain visible where material.

A policy may explicitly account for uncertainty.

---

# 26. Risk

Risk is domain-dependent.

A generic Decision model should not assume a universal definition of risk.

Where a policy uses risk, its metric and semantics must be identifiable.

---

# 27. Utility

Policies may evaluate candidate outcomes using utility.

Conceptually:

```text
candidate outcome
    ↓
utility evaluation
    ↓
selection
```

Utility definitions belong to the relevant domain or application.

---

# 28. Candidate Outcomes

A Policy may evaluate multiple candidates:

```text
Candidate A
Candidate B
Candidate C
    ↓
Policy
    ↓
Selected Outcome
```

The candidate set must be explicit.

---

# 29. Deterministic Policy

A deterministic Policy produces the same decision for equivalent inputs and relevant conditions.

For example:

```text
if risk > threshold
    outcome = NO_ACTION
```

---

# 30. Probabilistic Policy

A policy may use probability or stochastic decision mechanisms.

Randomness must be explicit.

For example:

```text
probability distribution
    ↓
policy
    ↓
sampled outcome
```

The random source must be captured where reproducibility matters.

---

# 31. Optimization-Based Decision

A policy may select the outcome that optimizes an objective:

```text
argmax utility(candidate)
```

Optimization configuration and algorithm version must be explicit.

---

# 32. Rule-Based Decision

A policy may consist of ordered rules:

```text
condition A → outcome A
condition B → outcome B
default     → outcome C
```

Rule priority must be explicit.

---

# 33. Policy Evaluation

Conceptually:

```text
Policy
    +
Inputs
    +
Context
    ↓
Policy Evaluation
    ↓
Decision
```

Policy evaluation is the mechanism.

Decision is the resulting semantic object.

---

# 34. Decision Evaluator

Conceptually:

```text
DecisionEvaluator
{
    policy
    inputs
    configuration
    evaluate()
}
```

The exact C++ API remains deferred.

---

# 35. Evaluator vs Decision

As with Analyzer:

```text
Evaluator:
    mechanism

Decision:
    resulting object
```

One evaluator may produce many Decisions.

---

# 36. Decision Reasoning

A Decision may preserve reasoning explaining why the selected outcome was chosen.

Reasoning may include:

```text
rules satisfied
candidate scores
constraints
objective values
supporting analysis
```

The exact representation is domain-specific.

---

# 37. Decision Evidence

A Decision should identify relevant inputs.

For example:

```text
Decision
    ↓
Analysis A
Analysis B
Pattern C
Context D
```

These references provide decision provenance.

---

# 38. Policy vs Evidence

Policy and evidence answer different questions:

```text
Evidence:
    what is observed / inferred

Policy:
    how evidence should influence choice
```

Changing the policy can change the decision while leaving evidence unchanged.

---

# 39. Decision Provenance

A Decision should identify, where required:

```text
policy
policy version
inputs
configuration
evaluator version
execution context
run identity
```

This allows the decision to be reproduced and audited analytically.

---

# 40. Decision Reproducibility

Conceptually:

```text
Inputs
+
Policy Version
+
Effective Configuration
+
Evaluator Version
+
Relevant Execution Context
+
Required State
    ↓
Decision
```

---

# 41. Decision Determinism

A deterministic policy evaluation should produce equivalent Decisions from equivalent semantic inputs and conditions.

Representation differences must not be mistaken for semantic differences.

---

# 42. Decision and Current Time

If a policy depends on current time, the time source must be explicit.

A replay must not accidentally use the wall clock instead of the original decision time.

---

# 43. Decision and Context

Decision context may include:

```text
environment
available resources
current state
operational constraints
market conditions
session conditions
```

Context must remain distinct from policy configuration.

---

# 44. Decision Lifecycle

A Decision may have a lifecycle:

```text
PROPOSED
    ↓
EVALUATED
    ↓
ACCEPTED / REJECTED
    ↓
EXECUTED
    ↓
OBSERVED
    ↓
EVALUATED
```

Not every application requires every stage.

---

# 45. Proposed Decision

A proposed Decision has been generated but not yet accepted for execution.

---

# 46. Accepted Decision

An accepted Decision has passed whatever approval or policy boundary the application requires.

Acceptance does not imply execution.

---

# 47. Executed Decision

A Decision may result in an Action being executed.

The execution result must remain separate from the Decision itself.

---

# 48. Action

An Action is an operation requested or performed because of a Decision.

Conceptually:

```text
Decision
    ↓
Action Request
    ↓
Action Execution
    ↓
Events
```

The action may succeed, fail, be cancelled, or remain unknown.

---

# 49. Decision vs Action Result

Suppose a Decision says:

```text
execute operation X
```

and execution fails.

The Decision remains:

```text
Decision = operation X
```

while the Action result is:

```text
Action = FAILED
```

The two must not be conflated.

---

# 50. Feedback Loop

Decision systems may form a feedback loop:

```text
Events
    ↓
Analysis
    ↓
Decision
    ↓
Action
    ↓
New Events
    ↓
Analysis
```

This loop is application behavior, not a mandatory Core pipeline.

---

# 51. Outcome Observation

After execution, the resulting events may be analyzed against the original Decision.

For example:

```text
Decision
    ↓
Action
    ↓
Observed Outcome
    ↓
Evaluation
```

This enables policy evaluation and improvement.

---

# 52. Decision Evaluation

Historical Decisions may be evaluated after outcomes are known.

Evaluation may measure:

```text
success
utility
risk
prediction accuracy
constraint violations
policy performance
```

This is separate from the original Decision.

---

# 53. Policy Evaluation vs Decision Evaluation

These terms should remain distinct where needed.

```text
Policy Evaluation:
    did the policy produce an acceptable choice?

Decision Evaluation:
    how did this particular decision perform?
```

The exact terminology may be specialized by the application.

---

# 54. Policy Learning

A policy may be modified based on historical results.

Learning is not an implicit property of Decision.

If supported, learning is an explicit analytical/application capability.

---

# 55. Adaptive Policies

A policy may adapt over time.

Adaptive state must be explicitly represented.

The system must distinguish:

```text
policy definition
policy configuration
policy learned state
decision
```

---

# 56. Policy Versioning with Learning

If learned state changes decision behavior, the effective policy/model state must be identifiable for reproducibility.

---

# 57. Decision History

Decision history may be persisted.

Historical Decisions should not be silently rewritten when policies change.

A new evaluation under a new policy is a new decision result.

---

# 58. Re-evaluation

An historical situation may be re-evaluated using a different policy:

```text
Original evidence
    +
Policy v1
    ↓
Decision A

Same evidence
    +
Policy v2
    ↓
Decision B
```

This is valid and does not imply that Decision A was incorrect.

---

# 59. Counterfactual Decisions

An application may ask:

> What decision would Policy B have made using the same historical evidence?

This is a counterfactual evaluation.

The resulting Decision should identify the policy and run separately from the original decision.

---

# 60. Decision Scope and Identity

Decision identity must remain independent of:

```text
memory address
storage location
policy object address
execution thread
RunId
```

Identity follows the Identity Model.

---

# 61. Decision Inputs and Identity

Input references should preserve logical identities where inputs are persistent.

For example:

```text
AnalysisId
PatternId
MeasurementId
StateId
```

should not be replaced by storage keys.

---

# 62. Missing Inputs

Missing inputs must not silently become defaults unless the policy explicitly defines the default.

For example:

```text
missing risk value
```

must not automatically mean:

```text
risk = 0
```

---

# 63. Unknown Inputs

Unknown inputs are distinct from zero, false, empty, or absent values.

Policies must define how unknown inputs affect feasibility and selection.

---

# 64. Conflicting Evidence

A Decision may receive conflicting analytical evidence.

The Policy should explicitly define how conflicts are handled.

Possible approaches include:

```text
priority
weighted evidence
conflict rejection
request more information
uncertain outcome
```

---

# 65. Evidence Freshness

Some decisions require fresh evidence.

Freshness requirements should be explicit.

For example:

```text
analysis must be less than 5 seconds old
```

is a policy/application requirement, not a generic property of Analysis.

---

# 66. Stale Evidence

Stale evidence may cause:

```text
reject
re-evaluate
wait
degrade
accept with warning
```

according to policy.

---

# 67. Decision Constraints and Resources

A policy may depend on resource availability.

For example:

```text
if available capacity < required capacity
    outcome = WAIT
```

Resource availability belongs to the Resource Model.

The policy determines how it affects the decision.

---

# 68. Security and Authorization

A technically valid Decision may still be unauthorized.

Authorization remains a security/application concern.

For example:

```text
Policy:
    outcome = EXECUTE_X

Authorization:
    user is not allowed to execute X
```

The two must remain separate.

---

# 69. Decision Validation

Decision validation may include:

```text
structural validation
policy validation
input validation
constraint validation
authorization
resource feasibility
```

Validation layers should remain explicit.

---

# 70. Decision Failure

Decision evaluation can fail.

Examples:

```text
invalid policy
invalid input
missing required dependency
unsupported candidate
resource failure
processing failure
```

Failure is distinct from:

```text
NO_DECISION
```

---

# 71. No Decision

A valid policy evaluation may produce no selected outcome.

For example:

```text
insufficient evidence
all candidates violate hard constraints
policy intentionally abstains
```

This is a valid semantic result when defined by the policy.

---

# 72. Cancellation

Decision evaluation may be cancelled.

Cancellation is distinct from:

```text
no decision
decision failure
successful decision
```

---

# 73. Partial Decision

Some decision systems may produce partial candidate evaluations before completion.

Partial evaluation must not automatically become a final Decision.

---

# 74. Decision as Processor Output

Decision evaluation may be implemented through the Processing Model:

```text
Processor
    input: DecisionInput
    state: PolicyState
    output: Decision
```

Processing controls execution.

Policy controls decision semantics.

---

# 75. Decision and Storage

Decision evaluation must not depend directly on a storage technology.

Input retrieval occurs through Query/Read Model abstractions.

---

# 76. Decision and Interface

Interfaces may submit decision requests or expose decisions.

They must not implement policy semantics.

---

# 77. Decision and Application

Applications compose:

```text
policy
inputs
analysis
context
execution
action
```

The application defines the workflow.

---

# 78. Domain Policy

Domain-specific policies belong to the Domain or Application layer.

For example:

```text
PokerPolicy
TradingPolicy
ResourceAllocationPolicy
```

must not become Core concepts.

---

# 79. Generic Policy Mechanisms

Core may eventually provide generic mechanisms for:

```text
candidate evaluation
conditions
constraints
objectives
selection
```

but should not define domain-specific outcomes.

---

# 80. Policy Representation

Policies may eventually be represented as:

```text
rules
expressions
programs
decision tables
optimization models
statistical models
machine-learning models
```

No universal representation is selected by this ADR.

---

# 81. Policy Configuration vs Policy Definition

A Policy Definition describes the decision method.

Configuration supplies parameters.

For example:

```text
Policy Definition:
    risk threshold policy

Configuration:
    threshold = 0.25
```

Changing configuration may produce a different effective policy configuration without necessarily changing the policy implementation.

---

# 82. Policy State

Adaptive policies may have accumulated state.

Policy State is distinct from:

```text
Policy Definition
Policy Configuration
Decision
Execution Context
```

---

# 83. Policy Provenance

A Decision should be able to answer:

```text
Which policy produced this?
Which policy version?
Which configuration?
Which inputs?
Which algorithm?
Which run?
```

---

# 84. Policy Reproducibility

A policy decision is reproducible only when the relevant:

```text
inputs
policy
configuration
algorithm/model versions
execution context
state
```

are available.

---

# 85. Testing

Decision systems should test:

```text
valid inputs
invalid inputs
boundary conditions
hard constraints
soft constraints
multiple candidates
tie-breaking
conflicting evidence
missing inputs
unknown inputs
stale evidence
policy versions
configuration changes
determinism
randomness
resource constraints
authorization boundaries
cancellation
no-decision outcomes
action separation
reproducibility
```

---

# 86. Policy Testing

Policies should have independent tests for:

```text
condition evaluation
constraint handling
objective ordering
candidate selection
tie-breaking
default behavior
abstention
```

---

# 87. Outcome Testing

Decision quality may be evaluated separately using historical outcomes.

This should not modify the original Decision semantics.

---

# 88. Regression Testing

Important historical decision cases should be retained as regression fixtures.

Policy changes should explicitly identify expected changes.

---

# 89. Decision Serialization

Serialization must preserve semantic information required to interpret the Decision.

Where applicable:

```text
identity
policy identity/version
scope
inputs
outcome
reasoning
constraints
confidence
provenance
```

---

# 90. Decision Persistence

Decision persistence depends on application requirements.

A Decision may be:

```text
EPHEMERAL
DERIVED
RECOVERABLE
AUTHORITATIVE
```

according to its role.

---

# 91. Security of Decisions

Decisions may contain sensitive information.

Security controls must prevent accidental disclosure through:

```text
logs
errors
metrics
provenance
debug output
interfaces
```

---

# 92. Deferred Decisions

This ADR does not select:

* exact C++ Policy API
* exact C++ Decision API
* universal condition language
* rule engine
* optimization framework
* machine-learning decision framework
* utility representation
* universal risk model
* policy persistence format
* policy scripting language
* action execution framework
* distributed decision protocol
* approval workflow implementation
* authorization framework

---

# Decision Summary

```text
Policy:
    Defines how decisions are made

Decision:
    Result of applying a policy

Evidence:
    Information considered by the policy

Objective:
    What the policy tries to optimize

Constraint:
    What the decision must satisfy

Candidate:
    Potential outcome considered by the policy

Outcome:
    Selected result

Evaluator:
    Mechanism that applies the policy

Reasoning:
    Why the outcome was selected

Context:
    Conditions surrounding evaluation

Provenance:
    Origin and derivation of the decision

Action:
    Operation resulting from a decision

Decision Evaluation:
    Assessment of a particular decision

Policy Evaluation:
    Assessment of the policy's behavior
```

## Invariant

**A Decision is an explicit policy-driven selection made from defined evidence, context, objectives, and constraints; it remains distinguishable from the analysis that provides evidence, the policy that defines selection semantics, and the action that may subsequently be executed.**

