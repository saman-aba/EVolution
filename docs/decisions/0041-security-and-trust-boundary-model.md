# ADR 0041 — Security and Trust Boundary Model

**Status:** Accepted
**Date:** 2026-10-03

## Context

EVolution accepts information from external sources and exposes capabilities through external interfaces.

These boundaries may involve:

```text
files
network connections
APIs
IPC
CLI input
databases
external services
plugins
user-provided data
```

External information cannot automatically be treated as trusted.

At the same time, security concerns must not leak into generic analytical semantics.

The architecture therefore needs to distinguish:

```text
trust
identity
authentication
authorization
input validation
data provenance
data quality
```

These concepts are related but not interchangeable.

## Decision

EVolution treats security as a **cross-cutting boundary concern**.

Security mechanisms may protect:

```text
interfaces
applications
ingestion
storage
extensions
external dependencies
execution resources
```

Core analytical concepts remain security-mechanism-independent.

The architecture distinguishes:

```text
Authentication
Authorization
Trust
Input Validation
Data Provenance
Data Quality
```

and does not treat any one of them as a substitute for another.

## Trust Boundary

A trust boundary separates components or information with different security assumptions.

Conceptually:

```text
Untrusted External Source
        ↓
   Trust Boundary
        ↓
Interface / Ingestion
        ↓
Application
        ↓
Domain / Processing / Analysis
```

The exact boundary depends on deployment and integration architecture.

## Untrusted Input

External input should be treated as untrusted unless the relevant security contract explicitly establishes stronger assumptions.

Examples include:

```text
file contents
network payloads
HTTP requests
IPC messages
plugin metadata
database responses
user parameters
external API responses
```

Trust does not mean that the data is semantically correct.

## Trusted Internal Data

Data produced inside a trusted processing boundary may have stronger assumptions regarding:

```text
representation
memory safety
schema
ownership
identity
```

However, trusted data may still be:

```text
incorrect
stale
incomplete
malformed semantically
```

Security trust and analytical correctness remain separate.

## Authentication

Authentication answers:

> Who or what is making this request?

Possible principals include:

```text
user
service
application
device
process
extension
automated agent
```

Authentication is an interface/security concern.

Core analytical objects should not depend on a particular authentication protocol.

## Principal

A principal represents an authenticated actor or execution identity.

Conceptually:

```text
Principal
{
    identity
    authentication_context?
}
```

The exact representation is deferred.

## Principal Identity vs Object Identity

A principal identity is not an EVolution object identity.

For example:

```text
PrincipalId
EventId
AnalysisId
RunId
```

must remain distinct.

A user who creates an analysis does not thereby become the analysis's logical identity.

## Authorization

Authorization answers:

> Is this principal allowed to perform this operation on this resource?

Authorization is distinct from authentication.

A request may be:

```text
authenticated
but unauthorized
```

or:

```text
unauthenticated
and rejected
```

according to the interface contract.

## Authorization Boundary

Authorization should occur at a layer that has enough information to make the relevant decision.

Conceptually:

```text
Interface
    ↓
Authentication
    ↓
Application
    ↓
Authorization
    ↓
Operation
```

Some domain-specific authorization may require domain knowledge.

## Authentication vs Authorization

The distinction is:

```text
Authentication:
    who are you?

Authorization:
    what are you allowed to do?
```

Neither determines whether supplied domain data is correct.

## Input Validation

Input validation determines whether supplied input satisfies structural and semantic requirements.

For example:

```text
valid graph configuration
valid time range
valid metric identifier
valid event representation
```

Authorization does not replace validation.

A user may be authorized to submit invalid input.

## Data Provenance

Provenance answers:

> Where did this information come from and how was it derived?

It may identify:

```text
source
transformation
configuration
component
version
input identities
```

Provenance is not authentication.

A source can be authenticated while producing incorrect data.

## Data Quality

Data quality describes properties such as:

```text
completeness
accuracy
precision
consistency
temporal quality
validation status
```

Quality is not a security property.

## Trust and Data Quality

A trusted source can produce low-quality data.

An untrusted source can provide data that happens to be correct.

Therefore:

```text
Trust ≠ Quality
```

## Security and Domain Semantics

Security information should not silently redefine domain meaning.

For example:

```text
authenticated user
```

does not automatically mean:

```text
trusted event
```

unless the domain explicitly defines such semantics.

## Security Metadata

Security-related metadata may be associated with processing context or provenance where necessary.

Examples:

```text
principal
authentication method
authorization decision
security policy version
trust boundary
```

Only materially relevant metadata should cross into internal processing.

## Security Context

Conceptually:

```text
Security Context
{
    principal?
    authentication_state
    authorization_context?
    trust_context?
}
```

This is distinct from generic Execution Context.

A processor should receive security context only when its semantic or access-control contract requires it.

## Security Context and Provenance

Security context may be recorded in provenance when legally, operationally, or reproducibly required.

However, sensitive authentication material must not be copied into analytical provenance merely because it was available.

## Secrets

Secrets may include:

```text
passwords
private keys
access tokens
API keys
session credentials
cryptographic secrets
```

Secrets must not be placed into:

```text
logs
errors
metrics
analysis results
general provenance
```

unless explicitly protected and required by a dedicated security contract.

## Error Handling and Security

Security failures use the established `Result<T>` / `Error` model.

Possible conceptual errors include:

```text
Unauthorized
AuthenticationFailed
Forbidden
InvalidCredential
SecurityPolicyViolation
```

The exact error-code namespace remains deferred.

## Information Disclosure

Error messages should not reveal sensitive information unnecessarily.

For example, an external interface may intentionally expose:

```text
Unauthorized
```

instead of revealing internal authorization details.

Internal diagnostic context may contain additional information under controlled access.

## Authentication Failure

Authentication failure is distinct from:

```text
invalid domain input
resource exhaustion
processor failure
storage failure
```

The interface/application boundary should translate authentication failures appropriately.

## Authorization Failure

Authorization failure means the operation is not permitted for the relevant principal.

It does not imply that:

```text
input is invalid
resource does not exist
operation failed internally
```

These conditions remain distinct.

## Resource Existence Disclosure

Applications may need to avoid revealing whether unauthorized resources exist.

For example:

```text
GetAnalysis(AnalysisId)
```

may produce different external behavior depending on the security model.

The exact information-disclosure policy is application/interface-specific.

## Object-Level Authorization

Authorization may depend on the requested object.

For example:

```text
Read Analysis A
```

may be permitted while:

```text
Read Analysis B
```

is not.

This requires the application layer to evaluate resource context.

## Operation-Level Authorization

Authorization may also depend on operation type.

For example:

```text
QueryAnalysis
RunAnalysis
DeleteAnalysis
ModifyGraph
```

may require different permissions.

The authorization model remains application-specific.

## Domain Authorization

Some authorization decisions require domain semantics.

For example:

```text
only owner may modify object
```

or:

```text
only authorized role may execute operation
```

The application/domain boundary must explicitly define where such decisions belong.

Core must not embed application-specific permission concepts.

## Authorization vs Policy

Security authorization is a policy decision.

However, security policy is distinct from:

```text
analytical policy
domain policy
decision policy
processing policy
```

A security authorization decision determines whether an operation is permitted.

It does not determine what the domain should conclude.

## Security Policy Version

If authorization behavior materially affects reproducibility or auditability, the relevant policy/version may be recorded.

This does not make the security policy part of analytical configuration automatically.

## Interface Security

Interfaces may perform:

```text
authentication
request validation
authorization checks
rate limiting
resource limits
transport security
```

before invoking application operations.

## Application Security

Applications may enforce:

```text
operation authorization
resource authorization
domain authorization
security policy
```

where interface-level checks are insufficient.

## Core Security Independence

Core concepts should not depend on:

```text
HTTP authentication
TLS
OAuth
JWT
Kerberos
Unix permissions
API gateways
```

as architectural requirements.

These mechanisms belong to external/security boundaries.

## Ingestion Security

Ingestion is an untrusted-input boundary when source data is externally controlled.

It should enforce appropriate:

```text
size limits
parser limits
resource limits
encoding validation
schema validation
```

Security validation must not replace semantic validation.

## File Ingestion

File ingestion may need to protect against:

```text
path traversal
unexpected file types
oversized files
malformed content
resource exhaustion
symbolic-link attacks
```

The exact filesystem security model is deployment-specific.

## Network Ingestion

Network ingestion may need:

```text
authentication
authorization
transport security
connection limits
message size limits
rate limits
resource limits
```

These are boundary concerns.

## Database Sources

Data obtained from a database should not automatically be trusted merely because the database connection was authenticated.

The returned data remains subject to:

```text
schema validation
semantic validation
provenance
quality checks
```

## External API Sources

Likewise, authentication of an external API does not guarantee semantic correctness of its responses.

API source identity and authentication status are separate from data quality.

## Plugin and Extension Security

ADR 0035 allows static or future dynamic extensions.

Dynamic extensions introduce additional trust concerns.

A dynamically loaded extension may have access to:

```text
process memory
filesystem
network
credentials
resources
```

depending on deployment.

Therefore dynamic extension loading must have an explicit security/trust model.

## No Implicit Trust for Extensions

Registration does not imply trust.

An extension being discoverable does not automatically mean:

```text
safe
authorized
trusted
compatible
```

These are separate properties.

## Extension Identity

An extension should have explicit:

```text
identity
version
capabilities
```

and, where security requires:

```text
trust/authenticity information
```

## Extension Verification

Future dynamic extension mechanisms may require:

```text
signature verification
trusted source
allowlist
sandboxing
permission model
integrity verification
```

No specific mechanism is selected by this ADR.

## Capability vs Permission

Extension capabilities describe what an extension can technically provide.

Permissions describe what it is allowed to access.

These are distinct.

For example:

```text
Capability:
    "provides PokerEventParser"

Permission:
    "may read directory X"
```

## Least Privilege

Where execution environments support permissions, components should receive only the resources required by their contract.

This is a security principle rather than a specific implementation mechanism.

## Process Isolation

A component may be isolated by:

```text
thread
process
container
VM
remote service
```

Isolation level is an execution/deployment decision.

Security requirements may influence that decision.

## Security vs Execution Model

Security requirements must not silently determine the processor concurrency or execution model.

For example:

```text
processor is thread-safe
```

does not imply:

```text
processor is safe to execute in a separate process
```

and vice versa.

## Resource Security

Security includes protection against resource exhaustion.

Examples:

```text
memory exhaustion
CPU exhaustion
queue exhaustion
file descriptor exhaustion
storage exhaustion
connection exhaustion
```

The Resource Model defines the execution semantics; security policy may impose limits.

## Denial of Service

An external actor may intentionally or unintentionally cause resource exhaustion.

Defenses may include:

```text
bounded queues
input limits
rate limits
admission control
timeouts
quotas
isolation
```

These mechanisms remain implementation/application concerns.

## Rate Limiting

Rate limiting may be applied to:

```text
principal
interface
operation
source
resource
```

Rate limiting is not automatically part of processor semantics.

## Quotas

Applications may impose quotas such as:

```text
maximum stored data
maximum analyses
maximum concurrent operations
maximum query size
```

Quota semantics are application policy.

## Timeouts and Deadlines

Security boundaries may impose deadlines.

A deadline is distinct from:

```text
event time
measurement time
processing time
```

A timeout does not mean that the underlying domain operation necessarily failed unless its operation contract defines that outcome.

## Cancellation

Security or interface layers may request cancellation.

Cancellation follows ADR 0014 and must remain distinct from failure.

## Audit

Security audit records are distinct from analytical provenance and ordinary observability.

An audit record may answer:

```text
who performed an operation?
when?
on what resource?
with what authorization?
what was the outcome?
```

Analytical provenance answers:

```text
where did this analytical result come from?
```

The two may reference related identities without becoming the same mechanism.

## Audit and Observability

Logs and traces may support operational diagnosis but should not automatically be treated as authoritative audit records.

If audit guarantees are required, they need an explicit contract.

## Audit and Domain Events

A security audit record is not automatically a domain event.

For example:

```text
User deleted analysis
```

may produce:

```text
Audit Record
```

without becoming:

```text
Domain Event
```

unless the domain explicitly defines it as such.

## Security Events

Security subsystems may generate security events.

These remain distinct from analytical/domain events unless intentionally promoted.

## Confidentiality

Confidential information should be protected according to its relevant security contract.

Examples include:

```text
credentials
private user data
proprietary datasets
security configuration
```

The analytical model should not expose sensitive data merely because it is technically accessible.

## Integrity

Integrity protection may be required for:

```text
stored events
configuration
extensions
checkpoints
external messages
```

The exact mechanism is deferred.

## Availability

Security also includes availability requirements.

For example:

```text
bounded resources
failure isolation
rate limits
recovery
```

Availability requirements must remain compatible with the Processing and Resource Models.

## Cryptography

Cryptographic operations may be required by applications or interfaces.

Core should not hard-code a particular cryptographic library merely because cryptography is used somewhere in the system.

Cryptographic implementation choices are deferred.

## Encryption at Rest

Some deployments may require encryption for:

```text
raw data
events
analysis results
credentials
checkpoints
```

This is a storage/deployment security decision.

It does not change the semantic storage model.

## Encryption in Transit

External interfaces may require secure transport.

Transport encryption remains outside Core semantics.

## Key Management

Key generation, storage, rotation, revocation, and access are security infrastructure concerns.

They are not defined by the analytical architecture.

## Security Metadata and Serialization

If security-related metadata crosses a serialization boundary, its representation and confidentiality requirements must be explicit.

Secrets must never be serialized merely because they exist in an execution context.

## Security Metadata and Provenance

Provenance should contain references or non-sensitive security metadata where required.

It should not become a secret storage mechanism.

## Multi-Tenant Data

If EVolution is deployed for multiple independent users or organizations, tenant identity may become part of application/domain context.

Conceptually:

```text
Tenant
    ↓
Authorization Scope
    ↓
Resource
```

Tenant isolation is an application/deployment concern.

## Tenant Identity

Tenant identity is distinct from:

```text
Principal identity
Object identity
Run identity
Source identity
```

A principal may operate within a tenant scope.

## Cross-Tenant Access

Cross-tenant access must be explicitly authorized.

No Core component should assume that identifiers alone establish authorization.

## Data Isolation

Storage implementations may enforce physical isolation.

The semantic architecture still requires explicit authorization and scope.

## Security and Caching

Caches must respect authorization boundaries.

A cached object must not be returned to a principal merely because the object is present in a shared cache.

Cache keys may therefore need security scope where relevant.

## Security and Query Results

Queries must not expose data outside the authorized scope.

Authorization should be applied before returning information.

Filtering unauthorized records must not accidentally alter semantics without being explicit to the application contract.

## Security and Commands

Commands must verify authorization before performing protected actions.

Authorization failure must not partially execute the command.

Where partial execution is possible, the command contract must define compensation/recovery behavior.

## Security and Transactions

Security checks that are part of a command's correctness may need to occur within an appropriate consistency boundary.

The exact transaction mechanism is deferred.

## Security and Reproducibility

Security context may affect whether an operation can be performed, but authorization state does not necessarily affect the semantic result after execution.

Only materially result-affecting security conditions should become part of reproducibility.

## Security and Determinism

Authorization decisions may depend on external security state.

If that state changes whether an operation executes, it is part of execution context rather than deterministic analytical input.

The system must not claim successful reproduction of a result if the required authorization-dependent operation could not be reproduced.

## Security and Errors

Security errors should preserve the normal error ownership/immutability/value semantics.

They must not expose secrets through:

```text
message
context
cause
stack traces
logs
```

unless explicitly permitted.

## Security Testing

Security testing should cover:

```text
authentication
authorization
input validation
resource limits
information disclosure
secret handling
tenant isolation
extension trust
serialization boundaries
storage access
interface boundaries
```

## Negative Testing

Security tests should deliberately exercise:

```text
invalid credentials
missing credentials
expired credentials
unauthorized resources
malformed input
oversized input
resource exhaustion
unexpected extension
invalid serialization
cross-scope access
```

## Security Regression Tests

Confirmed security defects should produce regression tests where practical.

The tests should verify the violated architectural contract rather than merely the exact implementation.

## Threat Model

A deployment may define explicit threats.

Examples:

```text
malicious input
credential theft
unauthorized access
data disclosure
tampering
resource exhaustion
malicious extension
compromised external dependency
```

Threat modeling is deployment/application specific.

## Security Boundary vs Trust Boundary

These terms are related but not identical.

A security boundary defines where security controls apply.

A trust boundary defines where assumptions about trust change.

A single interface may represent both.

## Security Boundary vs Module Boundary

Architectural module boundaries and security boundaries need not coincide.

For example:

```text
Application
    ├── Processor A
    └── Processor B
```

may be one architectural module boundary while process isolation creates an additional security boundary.

## Security Boundary vs Data Boundary

A raw-data boundary may be a trust boundary.

However, not every data transformation creates a security boundary.

## Security and Core

Core should provide mechanisms necessary for safe representation and contract enforcement where generic.

Core should not embed deployment-specific authentication or authorization.

## Security and Domain

Domain modules may define domain-specific authorization requirements when necessary.

They should not require a specific external authentication technology.

## Security and Processing

Processing components may require security context if their contract depends on it.

They must not silently access global credentials or authorization state.

## Security and Storage

Storage implementations may enforce:

```text
access control
encryption
isolation
integrity
```

while exposing semantic persistence through the storage interface.

## Security and Interfaces

Interfaces are primary security boundaries for external callers.

They may translate external authentication and authorization mechanisms into application-level security context.

## Security and Applications

Applications enforce operation-level authorization and coordinate security-sensitive workflows.

## Consequences

### Positive

* Trust assumptions become explicit.
* Authentication and authorization remain separate from analytical semantics.
* External input receives appropriate validation and resource protection.
* Secrets are prevented from leaking into ordinary diagnostic and analytical structures.
* Future security technologies can change without redefining Core.
* Dynamic extensions can receive an explicit trust model later.
* Security, provenance, observability, and data quality remain distinct.

### Negative

* Security context introduces additional metadata and policy considerations.
* Multi-tenant deployments may require substantial application-level authorization.
* Security requirements can affect deployment architecture and resource allocation.
* A complete security model cannot be defined independently of the deployment environment.

## Deferred Decisions

This ADR does not select:

```text
authentication protocol
authorization framework
TLS implementation
cryptographic library
secret manager
identity provider
RBAC
ABAC
capability-based security
ACL representation
security token format
audit backend
sandboxing mechanism
container isolation
process isolation
extension signing mechanism
key management system
encryption format
```

## Decision Summary

```text
Security:
    Cross-cutting concern

Trust:
    Explicit boundary assumption

Authentication:
    Who is the actor?

Authorization:
    What may the actor do?

Input Validation:
    Is the supplied input valid?

Provenance:
    Where did information come from?

Data Quality:
    How good/complete/precise is the information?

Principal:
    Authenticated actor identity

Principal identity:
    Distinct from object identity

Secrets:
    Never ordinary analytical/provenance data

Extensions:
    Registration ≠ trust

External input:
    Untrusted by default

Resource limits:
    Security-relevant where appropriate

Audit:
    Distinct from provenance and observability

Core:
    Security-mechanism independent

Interfaces:
    Primary external security boundary

Applications:
    Operation/resource authorization

Domain:
    Domain-specific authorization semantics where required

Physical isolation:
    Deferred

Cryptography:
    Deferred
```

## Invariants

1. Security is a cross-cutting architectural concern rather than a domain semantic.
2. External input is untrusted unless an explicit contract establishes stronger assumptions.
3. Authentication and authorization are distinct.
4. Authentication does not imply authorization.
5. Authorization does not imply input validity.
6. Authentication does not imply data quality.
7. Trust does not imply analytical correctness.
8. Provenance does not imply trust.
9. Data quality does not imply trust.
10. Principal identity is distinct from EVolution object identity.
11. Security context must not silently redefine domain meaning.
12. Secrets must not be placed into ordinary logs, errors, metrics, analytical objects, or provenance.
13. Security failures must use the established error model.
14. Security error diagnostics must not unnecessarily disclose sensitive information.
15. Extensions being registered or discoverable does not automatically make them trusted.
16. Extension capabilities and permissions are distinct concepts.
17. External interfaces may authenticate and authorize requests, but Core must remain independent of a specific authentication mechanism.
18. Application authorization may depend on resource and operation context.
19. Domain-specific authorization may require domain knowledge and must remain explicit.
20. Security resource limits must integrate with the Resource Model rather than bypassing it.
21. Security rate limits and quotas are policy concerns, not implicit processor semantics.
22. Security audit records are distinct from analytical provenance.
23. Security audit records are distinct from ordinary observability.
24. Security events are not automatically domain events.
25. Encryption and cryptographic mechanisms are implementation/deployment concerns unless explicitly promoted into a domain contract.
26. Security metadata must not be propagated merely because it is available; only relevant information should cross boundaries.
27. Authorization failures must not silently execute protected commands.
28. Security checks must respect command transaction and partial-failure semantics where they affect correctness.
29. Caches must preserve authorization boundaries.
30. Security boundaries and architectural module boundaries are independent concepts.
31. Physical isolation is a deployment decision that may be influenced by security requirements but is not required by the Core architecture.
32. Security policy must not silently become analytical policy.
33. Security context may participate in provenance or execution context when materially relevant, but must not become an implicit global dependency.
34. EVolution must preserve the distinction between protecting information and interpreting information.

## Invariant

> **EVolution treats security as an explicit boundary concern: authentication establishes actor identity, authorization determines permitted operations, validation determines input correctness, provenance establishes information origin, and data quality describes information characteristics; none of these concepts silently substitutes for another or becomes a hidden dependency of Core analytical semantics.**
