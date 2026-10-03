# ADR 0034 — Testing Architecture

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution contains multiple architectural layers with different correctness requirements:

```text
Core
Processing
Domains
Analysis
Storage
Ingestion
Applications
Interfaces
```

The system also contains several execution concerns:

```text
Processor lifecycle
Processing graphs
Scheduling
Concurrency
Backpressure
Delivery
Recovery
Persistence
Serialization
Supervision
```

Testing therefore cannot be treated as a single category.

A test that proves a function produces the expected value is fundamentally different from a test that proves a processor recovers correctly after a checkpoint failure.

The testing architecture must provide confidence at multiple boundaries while preserving the architectural separation of production code.

## Decision

EVolution uses a **layered testing architecture** based on explicit contracts.

The primary test categories are:

```text
Unit Tests
Contract Tests
Integration Tests
System Tests
End-to-End Tests
Property Tests
Regression Tests
Performance / Benchmark Tests
Fault / Recovery Tests
```

Not every component requires every category.

Tests should be placed at the lowest level that can meaningfully prove the required behavior.

## Testing Principle

The primary testing principle is:

> **Test semantic contracts independently from implementation details whenever possible.**

Tests should verify what a component promises rather than how it happens to implement that promise.

For example, a processor test should primarily verify:

```text
input
+
configuration
+
execution context
+
state
    ↓
expected output/state/outcome
```

rather than asserting:

```text
specific internal class
specific queue implementation
specific thread
specific private function
```

unless those implementation details are themselves contractually important.

## Test Categories

### Unit Tests

Unit tests verify small, isolated components.

Examples:

```text
Error
Result
Identity
Time
Measurement
Aggregation
Pattern detector
Metric calculation
Processor logic
```

Unit tests should minimize external dependencies.

## Unit Test Isolation

A unit test should normally not require:

```text
database
network
external process
specific filesystem layout
real scheduler
real distributed environment
```

unless the component being tested explicitly owns that dependency.

Dependencies should be replaceable through explicit interfaces where isolation is required.

## Contract Tests

Contract tests verify that an implementation satisfies an architectural interface.

Examples:

```text
Storage Contract
Processor Contract
Serializer Contract
Execution Backend Contract
```

A contract test defines behavior once and can run against multiple implementations.

For example:

```text
Storage Contract Tests
        ↓
 ┌──────┼──────┐
 │      │      │
Memory SQLite  Other
```

The implementation should not receive a different semantic contract merely because its physical mechanism differs.

## Contract Test Requirements

A contract test should verify only guarantees claimed by the interface.

It must not require unsupported capabilities.

For example, a storage backend that does not claim:

```text
transactions
```

must not be considered invalid merely because a generic transaction test exists.

Capabilities must determine which contract tests apply.

## Integration Tests

Integration tests verify interactions between multiple real components.

Examples:

```text
Processor + Queue
Processor + Storage
Graph + Scheduler
Serializer + Storage
Ingestion + Domain
Recovery + Checkpoint Store
```

Integration tests may use real infrastructure where behavior depends on it.

## System Tests

System tests verify a larger EVolution subsystem as a coherent unit.

Examples:

```text
complete processing graph
replay pipeline
persistence/recovery subsystem
poker analytical pipeline
```

System tests should verify architectural behavior rather than individual implementation details.

## End-to-End Tests

End-to-end tests exercise a complete externally observable workflow.

Conceptually:

```text
External Input
    ↓
Ingestion
    ↓
Events
    ↓
Processing
    ↓
Analysis
    ↓
Storage
    ↓
Application Output
```

E2E tests should be relatively few because they are expensive and can be sensitive to infrastructure.

## Property Tests

Property-based testing may be used where behavior can be expressed as general invariants.

Examples:

```text
serialize(deserialize(x)) ≡ x
```

or:

```text
aggregation(partitions(input))
    ≡
aggregation(input)
```

when the aggregation contract declares the required associativity/mergeability properties.

Property tests are especially useful for:

```text
serialization
aggregation
identity
time handling
state reconstruction
event replay
```

## Deterministic Test Inputs

Tests should prefer deterministic inputs.

A test should explicitly control:

```text
clock
randomness
configuration
input ordering
execution mode
external dependencies
```

where those factors affect behavior.

Tests should not depend on the host machine's current wall clock unless the behavior being tested explicitly concerns wall-clock behavior.

## Time in Tests

Time-dependent components should preferably receive an explicit time source.

Tests should be able to provide:

```text
fixed time
controlled time
advancing time
monotonic test clock
```

without changing production semantics.

A test must not rely on:

```text
sleep(100ms)
```

when deterministic synchronization can express the same condition.

## Randomness in Tests

Randomness that affects semantic output should be explicitly injectable or represented in execution context.

Tests should use deterministic seeds when reproducibility is required.

A failed random test should be reproducible from its relevant random state/seed.

## Test Identity

Tests themselves may use generated identifiers, but production logical identity must not depend on test execution order.

Test ordering must not determine:

```text
EventId
AnalysisId
sequence
```

unless the contract explicitly defines such behavior.

## Test Isolation

Tests should not accidentally depend on state created by previous tests.

Each test should explicitly establish its required:

```text
configuration
state
input
storage contents
execution context
```

Shared fixtures must not introduce hidden mutable global state.

## Global State

Production global mutable state should not be required merely to make tests convenient.

Tests that require global state should explicitly identify why the dependency exists.

## Test Fixtures

Fixtures should represent semantic setup rather than implementation internals.

For example:

```text
create_hand_with_actions(...)
```

is preferable to:

```text
manually construct internal parser buffers
```

when the behavior under test is domain semantics.

Low-level tests may of course construct low-level structures directly.

## Test Data

Test data should be:

```text
small
deterministic
purposeful
readable
```

Large datasets should be used only when the behavior requires scale.

## Golden Data

Golden/reference datasets may be used for:

```text
protocol decoding
serialization
analytical output
regression cases
```

A golden result should identify:

```text
input version
configuration
algorithm/version
expected representation
```

when reproducibility depends on them.

## Golden Data Maintenance

A changed golden result must not automatically be accepted merely because implementation output changed.

The semantic reason for the change must be understood and recorded.

Golden files are test artifacts, not unquestionable truth.

## Regression Tests

Every confirmed defect that can be expressed as a stable reproducible behavior should normally receive a regression test.

The test should capture:

```text
previously failing condition
expected corrected behavior
```

without unnecessarily reproducing unrelated implementation details.

## Error Testing

Tests must verify the established `Result<T>` contract.

For expected failures, tests should check:

```text
failure occurs
Error category/code
relevant context
propagation behavior
```

where applicable.

Tests should not rely on human-readable error messages as the primary contract.

## Error Message Testing

Exact diagnostic messages should generally not be asserted unless the message itself is an external interface contract.

Preferred:

```text
ErrorCode == InvalidInput
```

rather than:

```text
message == "invalid event timestamp"
```

## Error Propagation

Tests should verify that lower-level errors remain identifiable after propagation.

For example:

```text
Storage failure
    ↓
Processor
    ↓
Graph
    ↓
Application
```

must not silently become an unrelated generic failure if the abstraction boundary can preserve the underlying cause.

## Cancellation Testing

Cancellation should be tested separately from failure.

Tests should distinguish:

```text
SUCCESS
FAILURE
CANCELLED
```

and verify that cancellation does not accidentally become a processor failure.

## Lifecycle Testing

Processor lifecycle tests should verify valid and invalid transitions.

Examples:

```text
CREATED → CONFIGURED
CONFIGURED → INITIALIZED
INITIALIZED → ACTIVE
ACTIVE → STOPPING
STOPPING → STOPPED
```

and invalid operations such as:

```text
CREATED → ACTIVE
FAILED → ACTIVE
```

must produce the appropriate error.

## Stop vs Cancel

Tests should explicitly verify:

```text
STOP:
    admitted work drains according to contract

CANCEL:
    admitted work may terminate early

ABORT:
    completion is not guaranteed
```

The exact behavior depends on processor contract, but the distinction must remain observable.

## Lifecycle Request vs Completion

A test must not assume that accepting:

```text
stop()
```

means the processor is already:

```text
STOPPED
```

Where lifecycle completion is asynchronous, tests must explicitly wait for or observe completion using the eventual lifecycle API.

## Concurrency Tests

Concurrency tests verify declared concurrency semantics.

For a `SERIAL` processor:

```text
maximum concurrent processing operations == 1
```

For `CONCURRENT`:

```text
declared simultaneous operations
```

For `PARTITIONED`:

```text
operations in the same partition
    obey declared ordering/concurrency rules
```

Tests must not infer concurrency guarantees merely from the underlying executor.

## Race Detection

Where supported by the implementation/toolchain, concurrency tests should be run with appropriate race-detection tooling.

The exact tooling is deferred.

A race detector is a diagnostic mechanism, not a semantic substitute for a concurrency contract.

## Ordering Tests

Tests should distinguish:

```text
input order
domain sequence
event time
processing order
execution order
completion order
output order
```

A test should assert only the ordering guarantees declared by the component.

## Out-of-Order Tests

Processors that explicitly support out-of-order data should have tests covering:

```text
ordered input
slightly out-of-order input
strongly out-of-order input
late input
unknown temporal information
estimated temporal information
```

The expected handling must follow the processor contract.

## Backpressure Tests

Backpressure behavior must be testable independently from normal processing.

Examples:

```text
capacity available
capacity exhausted
BLOCK
REJECT
DROP
SAMPLE
DEGRADE
SPILL
```

where the processor/connection supports those policies.

Tests must verify that:

```text
REJECT
```

does not appear as successful processing.

Likewise:

```text
DROP
```

must not silently appear as successful completion.

## Queue Tests

Queue/buffer tests should verify:

```text
capacity
ordering
ownership
shutdown
cancellation
admission behavior
partition behavior
```

where applicable.

Unbounded growth should not be an accidental property of a test implementation.

## Delivery Semantics Tests

Tests should distinguish:

```text
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

where a component claims such semantics.

For at-least-once processing, tests should explicitly tolerate or detect duplicate execution according to the contract.

For exactly-once claims, tests must cover recovery and commit boundaries rather than only normal execution.

## Idempotency Tests

Side-effecting components that claim idempotency should be tested with repeated execution of the same logical input.

The expected semantic result should remain unchanged according to the contract.

## Recovery Tests

Recovery tests should simulate:

```text
processor failure
connection failure
storage failure
process restart
checkpoint restore
replay
duplicate execution
missing checkpoint
corrupt checkpoint
incompatible checkpoint
```

where applicable.

## Checkpoint Tests

Checkpoint tests should verify that:

```text
state
+
input position
```

represent the same recovery point when atomicity is required.

Tests should also verify incompatible checkpoint versions are rejected rather than silently interpreted.

## Replay Tests

Replay tests should verify deterministic behavior when the processor claims determinism.

Conceptually:

```text
Input
 + Configuration
 + Version
 + Relevant Execution Context
       ↓
    Result A

same inputs/context
       ↓
    Result B

A ≡ B
```

where semantic determinism is declared.

## Recovery and Side Effects

Processors with external side effects require tests for:

```text
retry
duplicate execution
unknown completion
recovery
deduplication
```

A processor must not claim stronger effect semantics than its tests demonstrate.

## Storage Contract Tests

Every storage implementation should run the applicable common storage contract tests.

Tests should cover capabilities actually advertised by that implementation.

For example:

```text
InMemoryStorage
    supports append/read
    does not claim durability

PersistentStorage
    supports append/read
    claims durability
```

The test suites differ accordingly.

## Serialization Tests

Serialization implementations should test:

```text
round-trip
identity preservation
temporal status preservation
version compatibility
invalid data
truncated data
unknown fields
boundary values
resource limits
```

where applicable.

## Serialization Property Tests

Where suitable:

```text
deserialize(serialize(x)) ≡ x
```

should hold for the declared semantic equivalence relation.

If canonical representation is required:

```text
serialize(x) == serialize(y)
```

should hold whenever `x` and `y` are contractually equivalent.

## Graph Validation Tests

Processing graph validation should be tested independently from graph execution.

Tests should cover:

```text
valid topology
duplicate node identity
invalid connection
type mismatch
semantic mismatch
missing input
missing output
cycle
unsupported capability
invalid configuration
resource incompatibility
```

where relevant.

## Graph Scheduling Tests

Scheduling tests should verify eligibility independently from the execution backend.

For example:

```text
input not ready
    → not eligible

all required inputs ready
    → eligible
```

The test should not require a particular thread or executor.

## Execution Backend Tests

Execution backend contract tests should verify:

```text
accepted work executes
completion is reported
failure is propagated
cancellation is respected
resource constraints are respected
shutdown is respected
```

The backend should not be tested as though it were responsible for graph scheduling semantics.

## Supervision Tests

Supervision tests should verify:

```text
failure detection
recovery selection
retry limits
escalation
shutdown behavior
dependency handling
```

according to the configured supervision policy.

A supervisor test must not accidentally encode domain-specific recovery policy into Core.

## Resource Tests

Resource-sensitive components should test:

```text
available resources
exact capacity
capacity exhaustion
reservation failure
release
cancellation during reservation
```

where applicable.

Tests should verify that resource exhaustion does not silently become successful processing.

## Performance Tests

Performance tests are separate from correctness tests.

Examples:

```text
latency
throughput
allocation rate
memory usage
queue overhead
serialization cost
storage throughput
```

Performance thresholds should be used only where the project has an explicit requirement.

A benchmark result is not automatically a correctness criterion.

## Benchmark Reproducibility

Benchmarks should record relevant execution context such as:

```text
build configuration
compiler
processor version
dataset
configuration
hardware
```

where those factors materially affect results.

## Performance Regression Tests

Performance regression testing may compare measurements against established baselines.

A performance regression should be interpreted together with:

```text
hardware
compiler
build mode
dataset
configuration
```

to avoid false conclusions.

## Fault Injection

Fault injection may be used to test recovery behavior.

Examples:

```text
storage unavailable
queue full
allocation failure
processor crash
corrupt checkpoint
network failure
timeout
cancellation
```

Fault injection must be explicit and deterministic where possible.

## Test Failure Classification

A failed test may indicate:

```text
production defect
test defect
invalid expectation
environment problem
unsupported capability
nondeterministic behavior
```

The test framework should not automatically classify every failure as a production defect.

## Flaky Tests

Tests should not be made "stable" by arbitrary retries that hide nondeterministic failures.

If a test is nondeterministic, the source of nondeterminism should be identified where practical.

Retries may be appropriate for genuinely external/transient infrastructure failures, but must not hide correctness failures.

## Test Timeouts

Timeouts should protect the test environment from hangs.

They should not be used as the primary synchronization mechanism when deterministic completion signaling is available.

## Test Parallelism

Tests may execute concurrently where isolation permits.

Tests must not rely on execution order unless ordering is explicitly part of the behavior under test.

## Test Resources

Tests requiring:

```text
ports
files
temporary directories
database instances
processes
threads
```

must manage ownership and cleanup explicitly.

## Temporary Data

Temporary test data should be isolated from production data.

Tests must not accidentally write to the user's real application storage.

## Test Configuration

Tests should use explicit configuration.

Configuration should not silently inherit arbitrary developer-machine settings.

Environment-specific integration tests may intentionally use environment configuration, but this must be explicit.

## Test Execution Modes

The architecture should support at least conceptually:

```text
UNIT
INTEGRATION
SYSTEM
END_TO_END
BENCHMARK
FAULT
```

The exact CMake/CTest mechanism is deferred.

## Test Dependency Direction

Tests may depend on production components:

```text
tests
   ↓
production modules
```

Production modules must never depend on tests.

Test utilities should not leak into production targets.

## Test Utilities

Reusable test utilities may include:

```text
fake clock
deterministic random source
in-memory storage
test processor
test execution backend
fault injector
event generator
assertion helpers
```

These belong to test infrastructure.

## Test Doubles

Test doubles may include:

```text
stub
fake
mock
spy
simulator
```

The choice should depend on what behavior needs to be isolated.

Mocks should not be used merely to reproduce implementation structure.

## Mocking Principle

A mock should verify externally meaningful interaction contracts.

Tests that assert every internal method call are fragile and tightly coupled to implementation.

## Testability and Production Architecture

Production interfaces should not be distorted solely to make tests convenient.

However, if a dependency genuinely requires explicit boundaries for:

```text
configuration
time
randomness
storage
execution
external services
```

testability is evidence that the boundary may also be architecturally valuable.

## Observability in Tests

Tests may inspect:

```text
logs
metrics
traces
```

when observability itself is the behavior being tested.

Ordinary correctness tests should not depend on telemetry output unless telemetry is part of the explicit contract.

## Telemetry Isolation

Telemetry failures should not normally cause analytical test failures.

A processor that produces correct output must remain correct even if its optional telemetry exporter is unavailable.

## Test Provenance

Test-generated analytical objects may retain provenance indicating:

```text
test input
test configuration
algorithm version
```

when required by the production contract.

Test metadata should not accidentally become production semantics.

## Test Naming

Test names should describe behavior.

Preferred:

```text
rejects_event_with_invalid_timestamp
```

rather than:

```text
test_event_parser_function_7
```

Behavior-oriented naming makes failures easier to interpret.

## Test Organization

The source tree already reserves:

```text
tests/
```

for testing infrastructure.

A possible conceptual organization is:

```text
tests/
├── unit/
├── contract/
├── integration/
├── system/
├── e2e/
├── property/
├── fault/
├── benchmarks/
└── fixtures/
```

The exact physical structure remains an implementation/build decision.

## CMake Integration

Testing remains a first-class CMake concern.

CTest is already selected as the project test execution mechanism.

Individual test frameworks remain deferred.

Tests should be represented as explicit CMake targets.

## Test Framework

No specific unit-test framework is selected by this ADR.

Possible choices include:

```text
GoogleTest
Catch2
doctest
```

The framework must not determine the architecture of production interfaces.

## Static Analysis

Static analysis is complementary to runtime testing.

Potential tools include:

```text
compiler warnings
sanitizers
static analyzers
formatters
linters
```

Exact tooling remains deferred.

## Sanitizers

Where supported, the project should be capable of dedicated builds using appropriate sanitizers.

Potential categories include:

```text
AddressSanitizer
UndefinedBehaviorSanitizer
LeakSanitizer
ThreadSanitizer
```

The exact CI matrix is deferred.

## Coverage

Code coverage may be used as diagnostic information.

Coverage percentage is not a correctness metric by itself.

A high line coverage value does not prove that semantic contracts are adequately tested.

## Test Quality

A useful test should establish at least one meaningful property.

Tests that merely execute code without verifying behavior should not be treated as sufficient correctness evidence.

## Architectural Boundary Testing

Important architectural rules should be testable where practical.

Examples:

```text
Core does not depend on Poker
Domain does not depend on GUI
Processor does not silently use global configuration
Storage implementation does not redefine logical identity
```

Some of these may be enforced through build/dependency checks rather than runtime tests.

## Consequences

### Positive

* Testing is aligned with architectural boundaries.
* Interfaces can be validated independently from implementations.
* Multiple storage/execution implementations can share contract tests.
* Recovery, backpressure, lifecycle, and concurrency become testable semantics.
* Deterministic testing reduces timing-related flakiness.
* Performance and correctness remain separate concerns.

### Negative

* Multiple test layers increase project complexity.
* Contract tests require careful capability modeling.
* Fault and recovery testing can be expensive.
* Integration and E2E tests require infrastructure management.
* Maintaining deterministic test infrastructure requires deliberate design.

## Deferred Decisions

This ADR does not select:

```text
unit test framework
mocking framework
property-testing framework
benchmark framework
coverage tool
static analyzer
CI provider
test parallelization strategy
test container technology
exact CTest organization
```

These can be selected after the production interfaces become concrete.

## Decision Summary

```text
Testing model:
    Layered and contract-oriented

Unit tests:
    Isolated component behavior

Contract tests:
    Interface/implementation guarantees

Integration tests:
    Component interactions

System tests:
    Subsystem behavior

E2E tests:
    Complete externally observable workflows

Property tests:
    General invariants

Regression tests:
    Confirmed defect behavior

Fault tests:
    Failure/recovery behavior

Benchmarks:
    Performance measurement

Time:
    Explicit/deterministic where relevant

Randomness:
    Explicit/deterministic where relevant

Concurrency:
    Tested according to declared contract

Storage:
    Shared capability-aware contract tests

Serialization:
    Round-trip and compatibility testing

Lifecycle:
    Explicit transition testing

Recovery:
    Checkpoint/replay/failure testing

Test framework:
    Deferred

CTest:
    Adopted through CMake
```

## Invariants

1. Tests verify explicit semantic contracts rather than accidental implementation details wherever possible.
2. Production code must not depend on test code.
3. Unit tests should isolate external dependencies when those dependencies are not part of the behavior under test.
4. Contract tests must test only capabilities and guarantees actually claimed by an implementation.
5. Storage implementations should share behavioral contract tests where practical.
6. Tests must distinguish success, failure, and cancellation.
7. Tests must distinguish lifecycle state from individual operation outcome.
8. Tests involving time should control relevant clocks rather than depend on uncontrolled wall-clock timing.
9. Tests involving randomness should control relevant random sources when reproducibility matters.
10. Test execution order must not affect semantic correctness unless ordering is explicitly part of the contract.
11. Tests must not rely on arbitrary sleeps when deterministic synchronization is available.
12. Backpressure tests must distinguish rejection, dropping, sampling, degradation, and successful processing.
13. Delivery-semantics tests must account for duplicate execution where the contract permits it.
14. Recovery tests must validate checkpoint compatibility and processing-position semantics.
15. Performance measurements must remain distinct from correctness claims.
16. Golden test data must be versioned and reviewed when expected results change.
17. Human-readable error messages must not normally be the primary assertion for expected failures.
18. Fault injection must not silently alter the semantic contract being tested.
19. Test infrastructure must explicitly manage temporary resources and external dependencies.
20. Test doubles must represent meaningful architectural boundaries rather than implementation structure.
21. Coverage percentage must not be treated as proof of correctness.
22. Telemetry must not normally be required for correctness unless it is itself part of the tested contract.
23. Tests should provide reproducible evidence when deterministic behavior is claimed.
24. Architectural dependency restrictions should be testable or enforceable where practical.
25. Testing mechanisms must remain independent of the physical production implementation where the contract permits multiple implementations.

## Invariant

> **EVolution testing is contract-oriented and layered: each architectural boundary is tested at the lowest level that can meaningfully establish its semantics, while integration, recovery, concurrency, persistence, and end-to-end behavior are verified separately without coupling production architecture to a particular testing mechanism.**
