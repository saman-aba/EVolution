# ADR 0036 — Interface Boundary Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution may be used through several external interaction mechanisms:

```text
Library API
CLI
IPC
HTTP
gRPC
Messaging
File import/export
GUI
Remote services
```

These mechanisms have different protocols, lifetimes, serialization formats, error models, and operational characteristics.

They must not determine the internal architecture.

EVolution therefore needs an explicit boundary between:

```text
External Interaction
        ↓
Interface
        ↓
Application
        ↓
Domain / Analysis / Processing / Core
```

The interface layer should expose application capabilities without embedding domain processing logic or making Core aware of transport protocols.

## Decision

EVolution treats external interfaces as **adapters around application capabilities**.

Conceptually:

```text
External Client
      ↓
Interface Adapter
      ↓
Application API
      ↓
Domain / Processing / Analysis
      ↓
Storage / External Dependencies
```

The interface is responsible for translating between an external interaction model and an internal application contract.

## Interface vs Application

The distinction is:

```text
Interface:
    How an external actor communicates with EVolution

Application:
    What workflow/capability EVolution performs
```

For example:

```text
HTTP request
    ↓
HTTP Interface
    ↓
AnalyzeSession Application Operation
    ↓
Analysis
```

The HTTP layer should not implement the analysis itself.

## Interface Responsibilities

An interface may own:

```text
protocol handling
request parsing
authentication boundary
serialization
input validation at transport level
request/response mapping
connection/session management
transport errors
timeouts
interface-specific cancellation
```

It should not own:

```text
domain semantics
analytical algorithms
processor implementation
storage semantics
business/application policy hidden inside protocol handlers
```

## Application Responsibilities

Application components compose the underlying capabilities into explicit workflows.

Examples:

```text
Import Dataset
Run Analysis
Replay Processing Graph
Query Measurements
Inspect Session
Export Results
```

An application operation may coordinate:

```text
domain
processing
analysis
storage
configuration
execution
```

The application layer is therefore the primary internal boundary for external interfaces.

## Interface Types

Possible interfaces include:

```text
Library
CLI
IPC
HTTP
gRPC
Messaging
File
GUI
```

No specific interface technology is selected by this ADR.

## Library Interface

A C++ library interface may expose application capabilities directly to another C++ program.

This can avoid serialization entirely when both sides share the same process and ABI.

However, the library interface must still respect logical ownership and lifetime contracts.

## C ABI

A C-compatible interface may be provided where stable binary interoperability is required.

The C ABI should use explicit representations for:

```text
buffers
lengths
ownership
errors
handles
lifetime
```

C++ object layout must not become an implicit C ABI.

## CLI Interface

A CLI translates:

```text
arguments
stdin
files
environment
```

into explicit application configuration and operations.

CLI-specific concepts such as:

```text
exit code
stdout
stderr
terminal formatting
```

must remain outside Core semantics.

## CLI Errors

Internal errors should be translated into CLI-specific output.

For example:

```text
Internal:
    ErrorCode::NotFound

CLI:
    exit status + human-readable message
```

The Core error model must not contain CLI exit codes.

## HTTP Interface

An HTTP interface may translate:

```text
HTTP request
    ↓
application request
```

and:

```text
application result
    ↓
HTTP response
```

HTTP status codes are interface semantics.

They must not become Core error identities.

## gRPC Interface

Similarly, gRPC status codes and protobuf messages belong to the gRPC interface boundary.

They must not leak into domain or Core contracts.

## Serialization Boundary

External interfaces that communicate through serialized data must use the Serialization Model.

Conceptually:

```text
External Representation
        ↓
Deserialize
        ↓
Interface Request
        ↓
Application
        ↓
Interface Response
        ↓
Serialize
        ↓
External Representation
```

Transport framing and semantic serialization remain separate concerns.

## Request and Response

An interface may define request/response types specific to that interface.

These should not automatically become domain objects.

For example:

```text
HTTP Request
    ≠
Domain Event
```

The interface may translate one into the other where the application semantics require it.

## Input Validation

Validation occurs at multiple levels.

### Transport Validation

The interface validates things such as:

```text
malformed JSON
invalid HTTP syntax
missing required transport field
invalid protobuf encoding
invalid CLI argument syntax
```

### Application Validation

The application validates workflow requirements.

### Domain Validation

The domain validates domain semantics.

For example:

```text
Transport:
    valid JSON

Application:
    required session identifier exists

Domain:
    action is valid for current poker state
```

These responsibilities must remain distinct.

## Validation Translation

An interface may translate internal validation errors into its own protocol representation.

It must preserve the semantic distinction where possible.

For example:

```text
ErrorCode::InvalidInput
```

may become:

```text
HTTP 400
```

but the HTTP status is not the internal error identity.

## Error Translation

Errors may be translated at interface boundaries.

Conceptually:

```text
Core Error
    ↓
Application Error
    ↓
Interface Error
```

Translation should preserve the underlying cause where meaningful.

The interface must not rely on human-readable error messages to determine the error category.

## Interface-Specific Errors

An interface may introduce errors that do not exist in Core.

Examples:

```text
connection closed
malformed request
authentication failure
protocol timeout
message too large
```

These belong to the interface boundary unless they represent a deeper application failure.

## Authentication

Authentication is an interface/security concern.

Core should not depend directly on:

```text
HTTP Authorization header
JWT
TLS client certificate
CLI login
```

An interface may establish an authenticated identity and pass an appropriate application-level identity/context inward.

## Authorization

Authorization determines whether an external actor may perform an operation.

The exact policy belongs to the appropriate application/security layer.

Core should not know about HTTP roles or CLI users.

## Identity Translation

External identity and EVolution logical identity are distinct.

For example:

```text
HTTP user identity
    ≠
EventId
    ≠
Processor instance identity
```

An interface may associate an external actor with an application operation without changing the logical identity of domain objects.

## Correlation

Interfaces may provide correlation information such as:

```text
request ID
trace ID
client request ID
```

These are not automatically domain object identities.

Correlation should remain distinct from logical identity.

## Interface Context

An interface may create an execution context containing relevant information such as:

```text
request identity
authenticated principal
deadline
cancellation
trace/correlation identity
interface metadata
```

Only information that materially affects application behavior should cross the application boundary.

## Cancellation

External cancellation should be translated into the application's cancellation mechanism.

For example:

```text
HTTP client disconnect
    ↓
request cancellation
    ↓
application operation cancellation
```

Cancellation must remain distinct from operation failure.

## Timeouts

Interface timeouts are transport/application constraints.

They must not silently become domain time semantics.

For example:

```text
HTTP timeout
```

is not:

```text
Event time
```

or:

```text
Measurement time
```

## Deadlines

Where supported, an interface may propagate a deadline through execution context.

Processors may respect that deadline only if their contract supports cancellation/deadline behavior.

## Streaming Interfaces

Interfaces may expose streaming operations:

```text
stream events
stream measurements
stream analysis results
stream processing status
```

Streaming semantics must define:

```text
ordering
completion
cancellation
failure
backpressure
delivery semantics
```

The interface must not silently claim stronger guarantees than the underlying application operation provides.

## Interface Backpressure

A network or IPC interface may itself have bounded capacity.

Examples:

```text
request queue full
connection buffer full
client too slow
message size limit
```

These conditions should be represented explicitly.

Interface backpressure must not silently cause analytical data loss unless the application contract explicitly permits it.

## Interface Delivery Semantics

A message-based interface may provide:

```text
AT_MOST_ONCE
AT_LEAST_ONCE
EXACTLY_ONCE
```

delivery semantics.

These are transport/application guarantees and must be reconciled with the processing delivery semantics.

The interface must not imply exactly-once analytical effects merely because a transport reports successful delivery.

## Interface Sessions

Some interfaces maintain sessions:

```text
CLI session
WebSocket session
IPC connection
authenticated API session
```

Session state is interface/application state.

It must not automatically become domain state.

## Connection Lifetime

An interface connection may disappear while an application operation continues.

The application contract must explicitly determine whether:

```text
disconnect
```

means:

```text
cancel operation
continue operation
detach client
```

The interface must not assume one universal behavior.

## Long-Running Operations

Long-running operations should not require a client connection to remain open unless explicitly designed that way.

Possible application semantics include:

```text
submit job
    ↓
execution
    ↓
query status
    ↓
retrieve result
```

This is an application-level workflow, not merely an HTTP implementation detail.

## Interface State vs Processor State

These must remain separate.

```text
Interface:
    client connection state

Processor:
    processing lifecycle/state

Domain:
    domain state
```

Closing a connection must not silently corrupt or mutate processor/domain state.

## Interface and Processing Graphs

An interface may create or control processing graphs through an application API.

Conceptually:

```text
Interface
    ↓
Application
    ↓
Graph Definition
    ↓
Graph Validation
    ↓
Graph Lifecycle
```

The interface must not directly mutate graph internals.

## Graph Control

Operations such as:

```text
start
stop
cancel
inspect
query
```

should be exposed through application-level operations that use the graph lifecycle contracts.

## Interface and Configuration

External configuration sources may include:

```text
CLI
environment
HTTP request
configuration file
API call
```

These are configuration sources.

They must resolve into explicit effective configuration according to ADR 0012.

The interface must not create hidden mutable global configuration.

## Interface and Secrets

Interfaces may receive secrets or credentials.

Secrets should not be:

```text
logged
included in ordinary provenance
returned in diagnostics
stored in analytical objects
```

unless explicitly required and protected.

## Interface Resource Limits

External inputs must have explicit resource limits where appropriate.

Examples:

```text
maximum request size
maximum message size
maximum upload size
maximum concurrent requests
maximum stream duration
```

Limits prevent external interfaces from becoming uncontrolled resource consumers.

## Interface Security Limits

Security-sensitive interfaces should define limits for:

```text
authentication attempts
request rate
payload size
connection count
resource consumption
```

Exact policies remain outside this architectural decision.

## Interface Observability

Interfaces may produce operational telemetry:

```text
request count
request latency
connection count
transport errors
authentication failures
bytes received
bytes sent
```

These are observability signals unless explicitly promoted into the analytical model.

## Interface Logging

Interface logs should not automatically duplicate application/processor errors.

For example:

```text
Processor:
    records actual processing failure

HTTP layer:
    may record request-level failure
```

The logging strategy should avoid multiple identical error records.

## Interface Provenance

An interface may provide provenance/context such as:

```text
request identity
source system
import source
client-provided correlation
```

Only information relevant to the semantic result should become part of persistent provenance.

## External Data Ingestion

Interfaces that receive external domain data may act as ingestion boundaries.

For example:

```text
File Interface
    ↓
Parser
    ↓
Normalization
    ↓
Domain Event
```

The interface/parser should not bypass the Event model merely because the source format differs.

## Import vs Command

An interface request may represent either:

```text
data ingestion
```

or:

```text
command/control operation
```

These should remain conceptually distinct.

For example:

```text
Import Hand History
```

is different from:

```text
Stop Processing Graph
```

The first supplies data.

The second requests application behavior.

## Commands and Events

A command is an instruction/request to perform an action.

An event records something that happened.

Therefore:

```text
Command
    ≠
Event
```

An interface command may cause an event later, but it should not be treated as historical fact merely because a client requested it.

## Interface and Decisions

Interfaces may expose application decisions or recommendations, but the interface itself must not invent decision policy.

For example:

```text
Analysis Result
    ↓
Application Policy
    ↓
Decision
    ↓
Interface Response
```

The interface only transports/exposes the result.

## Interface Testing

Each interface should have tests for:

```text
request parsing
validation
serialization
error translation
cancellation
timeouts
resource limits
authentication
authorization
connection handling
streaming
delivery semantics
```

where applicable.

The underlying application behavior should be tested independently.

## Interface Contract Tests

Where an interface protocol is stable, protocol contract tests should verify:

```text
request format
response format
error representation
version compatibility
required fields
optional fields
```

The tests should not require knowledge of internal processor implementation.

## API Versioning

External interfaces may outlive internal implementations.

Therefore API/schema versioning should be explicit where compatibility is required.

An interface version is distinct from:

```text
application version
processor version
domain version
serialization schema version
```

## Backward Compatibility

Changes to external interfaces should identify whether they are:

```text
backward compatible
forward compatible
breaking
```

The exact compatibility policy is deferred.

## Interface Evolution

Internal refactoring should not require external interface changes when the external contract remains semantically valid.

Conversely, changing an external contract should not require changing Core semantics merely because the transport format changed.

## Interface Adapters

Adapters may translate between versions:

```text
Client v1
    ↓
Adapter
    ↓
Application API
```

Explicit adapters are preferred over hidden semantic changes.

## Interface Ownership

The interface layer owns:

```text
transport protocol
external representation
protocol lifecycle
interface-specific errors
transport security
```

The application layer owns:

```text
workflow
capability composition
application policy
```

The domain owns:

```text
domain semantics
```

Core owns:

```text
generic analytical infrastructure
```

## Dependency Direction

The intended direction is:

```text
External System
      ↓
Interface
      ↓
Application
      ↓
Domain / Analysis / Processing
      ↓
Core
```

Storage and external service dependencies remain behind their explicit interfaces.

## Interface Must Not Become Core

Transport-specific abstractions must not be placed in Core merely because multiple interfaces use them.

Only genuinely generic concepts belong in Core.

For example:

```text
HTTP Request
```

does not belong in Core.

A generic:

```text
Cancellation
```

may belong in Core/processing infrastructure because it has transport-independent semantics.

## Interface Independence

Multiple interfaces should be able to expose the same application operation:

```text
CLI ─────┐
HTTP ────┼──→ Application Operation
gRPC ────┤
IPC ─────┘
```

The application operation should not need to know which interface invoked it.

## Consequences

### Positive

* Core remains independent of external protocols.
* Multiple interfaces can expose the same application capabilities.
* Transport-specific errors and serialization remain localized.
* Authentication and authorization remain outside domain semantics.
* Long-running operations can be represented independently of connection lifetime.
* Interface versioning becomes explicit.
* Application workflows become reusable across CLI, HTTP, IPC, and other interfaces.

### Negative

* Interface adapters introduce translation layers.
* Error and type mapping must be maintained.
* External API compatibility can require versioning and adapters.
* Streaming interfaces require explicit delivery/backpressure semantics.

## Deferred Decisions

This ADR does not select:

```text
HTTP framework
gRPC implementation
CLI framework
IPC mechanism
message broker
WebSocket
GUI framework
authentication protocol
authorization framework
API schema format
API versioning policy
rate-limiting implementation
service discovery
TLS implementation
```

These require concrete interface requirements and separate decisions.

## Decision Summary

```text
Interface:
    External interaction adapter

Application:
    Internal workflow/capability boundary

Core:
    Transport-independent

Transport:
    Interface concern

Serialization:
    Explicit boundary

Validation:
    Transport → Application → Domain

Errors:
    Translated at interface boundary

Authentication:
    Interface/security concern

Authorization:
    Application/security concern

Cancellation:
    Explicitly propagated

Timeout:
    Transport/application constraint

Session:
    Interface/application state

Domain Event:
    Not equivalent to external command

Delivery:
    Explicit

Backpressure:
    Explicit

Resource limits:
    Explicit

API version:
    Separate from internal component version

Interface implementation:
    Replaceable
```

## Invariants

1. Interfaces adapt external interaction models to application capabilities.
2. Interfaces must not contain core analytical algorithms.
3. Transport-specific concepts must not leak into Core semantics.
4. Application workflows must remain usable independently of a particular interface.
5. Transport validation, application validation, and domain validation remain distinct responsibilities.
6. Interface-specific errors must not become Core error identities.
7. External identity, correlation identity, and EVolution logical identity remain distinct.
8. External cancellation must not automatically be interpreted as processing failure.
9. Interface timeouts must not be confused with domain temporal information.
10. Connection lifetime must not implicitly determine processor or domain lifetime unless explicitly specified.
11. External commands are not historical events merely because they were received.
12. Serialized interface representations must follow the Serialization Model.
13. Interface delivery guarantees must not silently imply stronger processing or effect guarantees.
14. Interface resource limits must produce explicit behavior rather than uncontrolled resource consumption.
15. Secrets must not be unintentionally exposed through logs, diagnostics, provenance, or analytical objects.
16. Interface observability remains separate from analytical data.
17. API versioning is distinct from processor, application, and serialization versions.
18. External compatibility changes must be explicit.
19. Interface adapters must not silently alter domain semantics.
20. Interfaces must interact with processing graphs through application-level contracts rather than directly manipulating graph internals.
21. Interface implementation technology remains replaceable.
22. Core remains independent of specific transport protocols.
23. Multiple interfaces may expose the same application operation without changing its semantic behavior.
24. Interface failures must not silently become valid domain data.
25. The interface boundary must preserve the semantic contracts of the application capabilities it exposes.

## Invariant

> **EVolution treats external interfaces as replaceable adapters: they translate transport-specific interaction, representation, security, and lifecycle concerns into explicit application operations without allowing a particular protocol, connection model, or external representation to redefine Core, domain, processing, or analytical semantics.**
