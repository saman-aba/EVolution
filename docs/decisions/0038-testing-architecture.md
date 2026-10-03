# ADR 0038 — Testing Architecture

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution contains multiple architectural layers with different correctness requirements:

```text
Core
Processing
Domain
Analysis
Storage
Ingestion
Applications
Interfaces
```

The project also contains behavior that depends on:

```text
identity
time
ordering
configuration
ownership
lifecycle
concurrency
backpressure
delivery semantics
recovery
serialization
provenance
```

Testing therefore cannot be treated only as a collection of unit tests around individual C++ functions.

The architecture needs explicit testing boundaries that verify:

1. individual component behavior,
2. contracts between components,
3. processing behavior,
4. application composition,
5. persistence and recovery,
6. deterministic/reproducible execution,
7. external interfaces.

## Decision

EVolution uses a **layered testing architecture**.

The primary test categories are:

```text
Unit Tests
Contract Tests
Integration Tests
System Tests
Property Tests
Replay / Determinism Tests
Performance / Benchmark Tests
```

These categories are complementary.

No single category is considered sufficient for the entire system.

## Testing Principle

Tests should verify **architectural contracts**, not merely implementation details.

A test is most valuable when it would fail if an architectural invariant were violated.

For example:

```text
Bad test:
    private member currently has value 3

Useful contract test:
    processor rejects work after entering STOPPING
```

Tests may inspect implementation details when necessary, but implementation details should not become accidental architectural contracts.

## Unit Tests

Unit tests verify a relatively isolated component.

Examples:

```text
Error
Result
Identity
Time
Measurement
Aggregation
Pattern detector
Serializer
Processor
```

Unit tests should normally avoid external infrastructure.

Typical dependencies should be replaceable with:

```text
in-memory data
test clocks
test processors
fake storage
deterministic execution context
```

## Core Unit Tests

Core should have extensive unit coverage because it provides generic infrastructure used by many components.

Examples:

```text
Result<T>
Error
Identity
Temporal Information
Configuration
Envelope
Measurement
Aggregation
Provenance
```

Core tests should not require:

```text
database
network
GUI
HTTP server
Poker-specific components
```

## Domain Unit Tests

Domain tests verify semantic rules.

For example:

```text
Poker:
    valid action
    invalid action
    hand state transition
    metric definition
```

Domain tests may depend on Core but should not require a particular storage implementation unless persistence itself is being tested.

## Processor Unit Tests

Processor tests should verify:

```text
input contract
output contract
configuration
state transition
error behavior
cancellation
ordering
cardinality
identity
provenance
```

where applicable.

## Stateful Processor Tests

Stateful processors should test state transitions explicitly.

Conceptually:

```text
Initial State
    +
Input
    ↓
Expected State
    +
Expected Output
```

Multiple input sequences should be tested where ordering matters.

## Lifecycle Tests

Processor lifecycle behavior must be tested according to ADRs 0014 and 0015.

Examples:

```text
CREATED → CONFIGURED
CONFIGURED → INITIALIZED
INITIALIZED → ACTIVE
ACTIVE → STOPPING
STOPPING → STOPPED
```

Invalid transitions should return:

```text
Error(InvalidState)
```

Tests should also cover:

```text
STOP
CANCEL
ABORT
```

and escalation:

```text
STOP → CANCEL → ABORT
```

## Operation Outcome Tests

Processor lifecycle and operation outcome must remain distinct.

Tests should distinguish:

```text
SUCCESS
FAILURE
CANCELLED
```

from:

```text
ACTIVE
STOPPING
STOPPED
FAILED
```

A cancelled operation should not automatically imply processor failure.

## Ownership Tests

Tests should verify lifetime contracts where ownership is semantically important.

Examples:

```text
returned result remains valid after operation
queued work owns admitted data
borrowed views do not outlive their source
errors remain valid after originating operation ends
```

Tests should not depend solely on memory-debugging tools; the ownership contract should also be represented behaviorally.

## Error Tests

Every component with expected failures should test:

```text
success
expected failure
error category
error code
context
cause where applicable
propagation
translation
```

Human-readable messages should not normally be the primary assertion.

## Error Immutability Tests

The `Error` contract should verify:

```text
copy
move
copy assignment
move assignment
enrichment
translation
cause preservation
context preservation
```

Tests should ensure that enriching an error does not mutate the original error.

## Identity Tests

Identity tests should verify:

```text
type separation
stable identity
serialization preservation
copy semantics
version separation
storage independence
```

For example, an `EventId` must not be implicitly interchangeable with an `AnalysisId`.

## Time Tests

Time-model tests should verify:

```text
event time
ingestion time
processing time
measurement time
duration
UTC representation
known time
estimated time
unknown time
```

Particular attention should be given to preventing:

```text
unknown event time
    →
current time
```

from occurring implicitly.

## Estimated/Unknown Time Tests

Tests should verify that:

```text
KNOWN
ESTIMATED
UNKNOWN
```

remain distinguishable through:

```text
processing
serialization
storage
replay
analysis
```

An estimated timestamp must not silently become known.

## Configuration Tests

Configuration tests should verify:

```text
resolution
defaults
validation
effective configuration
invalid configuration
configuration identity/version where applicable
```

Tests should also verify that meaningful component behavior does not depend on hidden mutable global configuration.

## Serialization Tests

Serialization tests should cover:

```text
round trip
invalid input
truncated input
missing fields
unknown fields
schema versions
identity preservation
temporal status preservation
numeric semantics
optional values
```

Round-trip testing should distinguish:

```text
semantic equality
```

from:

```text
byte-for-byte equality
```

unless canonical serialization is explicitly required.

## Contract Tests

Contract tests verify that an implementation satisfies an architectural interface.

For example:

```text
Processor Contract
        ↑
    implementation
```

The same contract suite can potentially be executed against:

```text
Processor A
Processor B
Processor C
```

This prevents implementations from subtly acquiring incompatible semantics.

## Storage Contract Tests

Every storage implementation should be tested against the storage contract it claims to implement.

Tests may cover:

```text
append
read
query
identity
version
ordering
persistence
durability
conflict behavior
deletion
retention
errors
```

Only capabilities actually claimed by the implementation should be tested.

## Capability-Aware Testing

A backend that does not support a capability must not be considered broken merely because the test exists.

For example:

```text
Storage A:
    supports transactions

Storage B:
    does not support transactions
```

The contract test suite should distinguish:

```text
required capability
optional capability
unsupported capability
```

## Processing Graph Tests

Graph tests should verify:

```text
node identity
connection validity
contract compatibility
configuration
reachability
fan-in
fan-out
cycle rules
graph inputs
graph outputs
validation
lifecycle
```

Invalid graphs must not activate.

## Graph Validation Tests

Each validation category should have explicit tests:

```text
structural validation
contract validation
configuration validation
execution validation
```

A graph that fails required validation must produce an explicit error.

## Scheduler Tests

Scheduler tests should verify semantic eligibility rather than a particular thread implementation.

Examples:

```text
dependency readiness
multi-input readiness
ordering
partition ordering
admission constraints
lifecycle restrictions
cancellation
failure handling
```

The scheduler should not be tested through assumptions about a particular executor unless the executor itself is being tested.

## Execution Backend Tests

Execution backend tests should verify:

```text
accepted work executes
completion is reported
failure is reported
cancellation is represented
resource constraints are respected
processor concurrency contract is respected
shutdown behavior is respected
```

The backend must not invent scheduling semantics.

## Queue and Buffer Tests

Queue/buffer tests should verify:

```text
capacity
ordering
admission
blocking
rejection
drop/sample behavior
cancellation
shutdown
ownership
partitioning
```

Tests should explicitly verify that bounded capacity does not silently become unbounded memory growth.

## Backpressure Tests

Backpressure tests should distinguish:

```text
submitted
admitted
processed
completed
rejected
dropped
cancelled
```

For example:

```text
REJECT
```

must not appear as successful processing.

## Delivery Semantics Tests

Tests should verify the declared delivery behavior:

```text
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

where applicable.

At-least-once tests should explicitly account for duplicate execution.

Exactly-once tests, if ever implemented, must test the actual required semantics rather than merely counting function invocations.

## Idempotency Tests

Side-effecting components should have tests demonstrating their declared idempotency behavior.

For example:

```text
execute(input)
execute(input again)
```

should produce the documented effect semantics.

Duplicate execution must not be confused with duplicate input identity.

## Recovery Tests

Recovery tests should cover:

```text
checkpoint creation
checkpoint loading
checkpoint validation
state restoration
input-position restoration
replay
duplicate execution
corrupt checkpoint
missing checkpoint
incompatible checkpoint
recovery failure
```

Recovery must preserve the required identity, temporal, ordering, and provenance semantics.

## Checkpoint Compatibility Tests

A checkpoint created with one processor/configuration/version must be tested against:

```text
same compatible version
incompatible version
changed configuration
changed graph
```

Incompatible checkpoints must not be silently interpreted as valid state.

## Supervision Tests

Supervision tests should verify:

```text
failure detection
scope
retry limits
recovery action
escalation
shutdown behavior
optional dependency handling
```

A supervisor must not silently modify processor configuration or topology.

## Determinism Tests

Deterministic processors should be tested repeatedly with equivalent inputs and execution conditions.

For example:

```text
Input
+
Configuration
+
Version
+
Relevant Context
    ↓
Run A

same inputs/conditions
    ↓
Run B
```

The semantic results should be equivalent.

## Reproducibility Tests

Reproducibility tests verify that sufficient information has been retained to reconstruct a result.

Conceptually:

```text
Recorded Execution Information
        ↓
Reconstruction
        ↓
Equivalent Result
```

This may include:

```text
input
configuration
component version
graph
state
execution context
extension versions
```

## Replay Tests

Replay tests should preserve required historical semantics.

In particular:

```text
event time
event identity
event sequence
input ordering
configuration
processor version
```

must not be silently replaced with current runtime values.

## Controlled Time

Tests involving time should use an explicit controllable time source where practical.

This prevents tests from depending on:

```text
wall-clock timing
machine timezone
test execution speed
current date
```

For elapsed-time behavior, a monotonic test clock should be available where appropriate.

## Randomness

Tests involving randomness should use an explicit deterministic randomness source when reproducibility is required.

A fixed seed alone should not be assumed sufficient if the random implementation or consumption order is part of the result.

## Property-Based Testing

Property-based testing may be used where behavior has useful general invariants.

Examples:

```text
aggregation properties
serialization round trips
identity properties
state transition invariants
ordering properties
parser robustness
```

Property testing is complementary to example-based tests.

## Algebraic Properties

Aggregations are especially suitable for property tests.

Where an aggregation claims associativity or mergeability, tests may verify:

```text
aggregate(A + B)
```

against:

```text
merge(aggregate(A), aggregate(B))
```

subject to the aggregation's declared semantics.

Floating-point operations may require tolerance or explicit numerical semantics.

## Fuzz Testing

Fuzzing should be considered for boundaries exposed to uncontrolled or malformed input.

Potential targets include:

```text
serializers
parsers
ingestion
configuration
external interfaces
protocol decoders
```

Fuzzing should respect explicit resource limits.

## Security-Oriented Testing

Security-sensitive components should include tests for:

```text
malformed input
resource exhaustion
oversized input
invalid configuration
unexpected serialization values
authentication boundary
authorization boundary
secret leakage
```

Security testing remains broader than ordinary functional testing.

## Integration Tests

Integration tests verify multiple components working together.

Examples:

```text
Processor + Queue
Processor + Storage
Graph + Scheduler
Graph + Execution Backend
Ingestion + Domain
Application + Storage
Application + Interface
```

Integration tests may use real implementations where interaction semantics are the subject of the test.

## System Tests

System tests exercise a complete application or major subsystem.

For example:

```text
external input
    ↓
ingestion
    ↓
events
    ↓
processing graph
    ↓
measurements
    ↓
analysis
    ↓
storage
    ↓
external interface
```

System tests should verify externally observable behavior rather than internal implementation details.

## End-to-End Tests

End-to-end tests should be relatively few and focused on critical workflows.

Examples:

```text
import dataset
run analysis
persist result
restart application
recover state
retrieve result
```

They should not replace unit and contract tests.

## Test Doubles

Possible test doubles include:

```text
fake
stub
mock
spy
in-memory implementation
deterministic implementation
```

No universal mocking strategy is selected.

A test double should be used when replacing a dependency improves isolation or determinism.

## Fake vs Mock

Tests should generally prefer behavior-oriented fakes or simple test implementations when possible.

Mocks should not force internal call structure to become an accidental contract.

For example:

```text
Prefer:
    "result is persisted"

over:
    "Storage::write() called exactly three times"
```

unless call count is itself a semantic requirement.

## Test Data

Test data should be:

```text
small
deterministic
explicit
reproducible
representative
```

Large datasets should be reserved for integration/system/performance tests.

## Golden Data

Golden/reference datasets may be used for:

```text
serialization
parsing
analysis
replay
regression
```

Golden outputs must document:

```text
input version
configuration
component version
expected semantics
```

otherwise they can become unexplained fixtures.

## Regression Tests

Every significant discovered defect should result in a regression test when practical.

The test should capture the violated contract rather than merely reproduce the implementation symptom.

## Negative Testing

Negative tests are first-class.

They should cover:

```text
invalid input
invalid configuration
invalid lifecycle operation
resource exhaustion
missing dependency
unsupported capability
serialization failure
storage failure
cancellation
corrupt state
```

## Failure Injection

Components involving recovery should support controlled failure injection in tests.

Examples:

```text
storage write failure
queue exhaustion
processor failure
checkpoint failure
external dependency failure
cancellation
```

Failure injection should not require production code to contain test-only semantic behavior.

The exact mechanism is deferred.

## Concurrency Testing

Concurrency tests should verify declared semantic guarantees.

Examples:

```text
serial processor rejects concurrent execution
partitioned processor preserves per-partition ordering
concurrent processor safely handles permitted simultaneous work
```

Tests must not infer correctness merely because no data race was observed.

## Race Detection

Thread-safety tooling may be used when concurrency is introduced.

Potential tooling includes:

```text
Thread Sanitizer
Address Sanitizer
Undefined Behavior Sanitizer
```

The exact CI/toolchain configuration remains a build decision.

## Performance Tests

Performance tests measure implementation characteristics rather than semantic correctness.

Examples:

```text
throughput
latency
memory usage
allocation rate
serialization cost
queue overhead
graph scheduling overhead
storage throughput
```

Performance tests should not replace correctness tests.

## Benchmark Stability

Benchmarks should record relevant conditions such as:

```text
build type
compiler
hardware
input size
configuration
algorithm version
```

Performance results are measurements of a particular execution environment.

## Performance Regression

Performance thresholds may be used where justified.

A performance regression test should distinguish:

```text
semantic failure
```

from:

```text
performance degradation
```

and should avoid fragile thresholds where environmental variance dominates.

## Test Classification

Tests should have explicit categories.

Conceptually:

```text
UNIT
CONTRACT
INTEGRATION
SYSTEM
PROPERTY
REPLAY
PERFORMANCE
FUZZ
```

A test may belong to more than one conceptual category.

## Test Dependencies

Tests should declare meaningful dependencies rather than relying on execution order.

A test should not depend on:

```text
another test having run
global mutable state
previous test data
machine-local accidental configuration
```

## Test Isolation

Tests should be independently repeatable where practical.

Shared infrastructure may be used for performance or system tests when the shared lifetime is itself part of what is being tested.

## Test Determinism

Tests should avoid uncontrolled:

```text
current time
randomness
thread scheduling
filesystem state
network state
environment variables
```

when those conditions are not the subject of the test.

## Flaky Tests

A test whose result depends unpredictably on execution timing or external conditions should be treated as a defect in the test or test environment unless nondeterminism is explicitly the subject of the test.

Flaky tests must not become silently ignored.

## Test Environment

Tests may require environment capabilities such as:

```text
filesystem
network
database
external service
specific CPU features
```

Such requirements should be explicit.

## Unit Test Environment

Unit tests should minimize external requirements.

Core tests should ideally execute in a minimal build environment.

## Integration Test Environment

Integration tests may use controlled infrastructure such as:

```text
temporary directories
in-memory services
test databases
local processes
```

External production services should not be required for ordinary integration testing unless the integration itself is the subject.

## System Test Environment

System tests may use realistic deployment environments.

Their environment requirements must be documented.

## CMake and CTest

Tests are first-class CMake targets and are executed through CTest according to ADR 0002.

Conceptually:

```text
CMake
    ↓
Test Targets
    ↓
CTest
```

The exact test framework remains separate from this ADR.

## Test Naming

Test names should communicate the behavior or contract being verified.

Prefer:

```text
processor_rejects_submission_after_stop
```

over:

```text
test_processor_17
```

## Test Organization

Tests should broadly mirror architectural ownership.

Conceptually:

```text
tests/
├── core/
├── processing/
├── domains/
├── analysis/
├── storage/
├── ingestion/
├── applications/
├── interfaces/
├── integration/
├── system/
├── performance/
└── fuzz/
```

The exact physical structure may evolve.

## Contract Test Reuse

Where multiple implementations share a contract, the contract test suite should be reusable.

For example:

```text
StorageContractTests
    ├── MemoryStorage
    ├── FileStorage
    └── DatabaseStorage
```

Only supported capabilities should be enabled for each implementation.

## Testing Provenance

Tests involving reproducibility should retain enough metadata to identify:

```text
test input
configuration
component version
test implementation
relevant execution context
```

This does not mean every ordinary unit test needs a persistent provenance record.

## Test Output

Test diagnostics should provide enough context to identify:

```text
input
configuration
expected behavior
actual behavior
relevant identities
```

Sensitive information must be redacted.

## Test and Observability

Tests may consume logs, metrics, traces, and health information.

However, tests should not require observability output to determine basic semantic correctness unless observability itself is what is being tested.

## Test and Error Handling

Tests should verify errors through stable machine-readable identity:

```text
ErrorCode
ErrorCategory
```

rather than relying primarily on diagnostic strings.

## Test and Serialization

Tests should verify that serialization preserves the semantic distinctions required by the relevant object contract.

For example:

```text
UNKNOWN time
```

must not become:

```text
1970-01-01
```

or:

```text
current time
```

after serialization/deserialization.

## Test and Storage

Storage tests should distinguish:

```text
logical persistence semantics
```

from:

```text
backend implementation details
```

Backend-specific tests may verify implementation details separately.

## Test and Application Boundaries

Application tests should verify composition and workflow.

They should not duplicate every processor unit test.

## Test Pyramid

EVolution should generally favor:

```text
many
    ↓
unit / contract / property tests
    ↓
fewer
integration tests
    ↓
fewer
system / end-to-end tests
    ↓
targeted
performance / fuzz / environment-specific tests
```

This is a guideline, not a rigid numerical requirement.

## Test Coverage

Code coverage may be used as a diagnostic measurement.

Coverage percentage must not be treated as proof of correctness.

High coverage can still miss:

```text
incorrect contracts
bad state transitions
ordering errors
concurrency bugs
recovery bugs
semantic errors
```

## Architectural Coverage

In addition to code coverage, important architectural invariants should have explicit tests.

Examples:

```text
Core cannot depend on Poker
errors remain immutable
unknown time is not fabricated
invalid graphs cannot activate
STOP prevents new admission
at-least-once permits duplicates
checkpoint compatibility is validated
```

## Test Compatibility with Deferred Implementation Choices

Testing architecture must remain independent of:

```text
thread pool implementation
executor
database
serialization library
HTTP framework
plugin loader
```

where the underlying contract can be tested through an abstraction.

## Consequences

### Positive

* Architectural contracts become executable specifications.
* Core behavior can be tested without external infrastructure.
* Multiple implementations can share contract tests.
* Recovery, lifecycle, ownership, and delivery semantics become testable.
* Determinism and reproducibility can be tested explicitly.
* Integration and system tests remain focused on composition rather than duplicating unit tests.

### Negative

* Maintaining multiple test categories increases project complexity.
* Contract tests require carefully defined contracts.
* Determinism and concurrency testing can require specialized infrastructure.
* System and performance tests may require more expensive environments.

## Deferred Decisions

This ADR does not select:

```text
test framework
mocking framework
property-testing framework
fuzzing framework
benchmark framework
CI provider
coverage tool
test reporting format
test data framework
distributed test infrastructure
```

CTest remains the project-level test execution mechanism according to ADR 0002.

## Decision Summary

```text
Testing model:
    Layered

Primary categories:
    Unit
    Contract
    Integration
    System
    Property
    Replay/Determinism
    Performance
    Fuzz

Primary objective:
    Verify contracts and invariants

Unit tests:
    Isolated component behavior

Contract tests:
    Interface/semantic guarantees

Integration tests:
    Component interaction

System tests:
    Complete workflow

Property tests:
    General invariants

Replay tests:
    Determinism/reproducibility

Performance tests:
    Implementation characteristics

CTest:
    Project-level test execution

Coverage:
    Diagnostic, not proof

Test doubles:
    Explicit and behavior-oriented

External infrastructure:
    Avoided for ordinary unit tests

Implementation details:
    Not accidental contracts
```

## Invariants

1. Tests must verify architectural contracts, not merely implementation details.
2. Core tests must not require domain-specific or external application infrastructure.
3. Expected failures must be tested through stable error identity rather than diagnostic text alone.
4. Lifecycle tests must distinguish lifecycle state from operation outcome.
5. Cancellation must remain distinguishable from processing failure.
6. Ownership and lifetime guarantees must be testable where they are semantically relevant.
7. Known, estimated, and unknown temporal information must remain distinguishable through processing and serialization.
8. Invalid processing graphs must not become active.
9. Admission behavior must distinguish accepted, rejected, dropped, delayed, and cancelled work.
10. Delivery semantics must be explicitly testable where claimed.
11. Recovery tests must verify checkpoint compatibility and processing-position semantics.
12. Deterministic components must produce equivalent semantic results under equivalent declared conditions.
13. Reproducibility tests must reconstruct all materially relevant inputs and execution conditions.
14. Contract tests must be reusable across implementations that claim the same contract.
15. Unsupported optional capabilities must not be treated as implementation failures.
16. Tests must not depend on uncontrolled global mutable state.
17. Tests must not rely on execution order unless ordering is explicitly part of the behavior being tested.
18. Concurrency tests must verify declared semantic guarantees rather than merely absence of observed races.
19. Performance tests must remain distinct from semantic correctness tests.
20. System tests must not replace lower-level contract and unit tests.
21. Code coverage is evidence about exercised code, not proof of correctness.
22. Test failures must not be silently ignored because they are considered flaky.
23. Test environments and external dependencies must be explicit.
24. Test architecture must remain independent of replaceable implementation technologies wherever the underlying contract permits.
25. Every important architectural invariant should have a corresponding executable test when practical.

## Invariant

> **EVolution treats testing as an executable expression of architectural contracts: component behavior, inter-component semantics, lifecycle, ownership, temporal meaning, processing guarantees, recovery, determinism, and application workflows must be testable independently of replaceable implementation technologies wherever practical.**
