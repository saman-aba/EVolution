# ADR 0049: Security and Secrets Operational Model

**Status:** Accepted
**Date:** 2026-10-04

## Decision

EVolution separates **security semantics** from the mechanisms used to implement them.

The architecture distinguishes:

```text id="c7r3w1"
Identity
Authentication
Authorization
Trust
Secrets
Confidentiality
Integrity
Audit
Provenance
Observability
```

These concepts must not be silently substituted for one another.

Security-sensitive information must have explicit ownership, lifetime, access, propagation, and exposure rules.

Secrets must not be treated as ordinary configuration values once loaded into runtime components.

---

## 1. Security Boundary

Security is a cross-cutting concern across:

```text id="v1a4ce"
External Interfaces
        ↓
Application
        ↓
Domain / Processing / Analysis
        ↓
Storage / Dependencies
```

Security decisions must be enforced at the layer that possesses the required semantic information.

Core must remain security-mechanism-neutral while providing contracts that allow security-sensitive operations to be represented safely.

---

## 2. Principal Identity

A **Principal** represents an actor requesting or performing an operation.

A principal may represent:

* a human user
* an application
* a service
* a process
* an automated agent
* another trusted system

Principal identity is distinct from:

```text id="4z0c3u"
EventId
OperationId
ProcessorId
ProcessId
TraceId
RunId
```

A principal must never be inferred from an unrelated technical identifier.

---

## 3. Authentication

Authentication establishes who or what a principal is.

Conceptually:

```text id="n7t2hk"
Credentials / Evidence
        ↓
Authentication
        ↓
Principal
```

Authentication does not determine what the principal is allowed to do.

Therefore:

```text id="1sq1l7"
Authenticated
    ≠
Authorized
```

Authentication mechanisms remain implementation-dependent.

---

## 4. Authorization

Authorization determines whether a principal may perform a requested operation or access a resource.

Conceptually:

```text id="m8d4p2"
Principal
    +
Operation
    +
Resource / Scope
    ↓
Authorization Decision
```

Possible outcomes include:

```text id="x1j7k9"
ALLOW
DENY
UNKNOWN / UNAVAILABLE
```

Authorization is separate from input validation.

An authorized request may still contain invalid input.

---

## 5. Authorization Scope

Authorization may apply at multiple levels:

```text id="8v6p1z"
Application
Graph
Operation
Domain Entity
Dataset
Storage Resource
External Dependency
```

The appropriate layer owns the semantic decision.

For example:

```text id="a2d8f0"
Interface:
    Is the caller authenticated?

Application:
    May this principal invoke this operation?

Domain:
    May this principal perform this domain action?

Storage:
    May this principal access this resource?
```

The exact division depends on the security model of the application.

---

## 6. Security Context

Security information may be part of an operation or execution context.

Conceptually:

```text id="5w9j2s"
Security Context
{
    principal
    authentication state
    authorization information?
    trust information?
}
```

Only security information relevant to the receiving component should cross a boundary.

A generic context object must not become an unrestricted security information container.

---

## 7. Security Context Lifetime

Security context must have explicit lifetime semantics.

A principal associated with an external request does not automatically remain valid for the lifetime of an application operation.

Long-running operations must define whether authorization is:

* checked once
* periodically revalidated
* tied to a capability/token
* evaluated at each protected action

The architecture does not select a universal policy.

---

## 8. Secrets

A secret is sensitive information whose disclosure can enable unauthorized access or compromise.

Examples include:

* passwords
* API keys
* access tokens
* private keys
* session credentials
* database credentials
* cryptographic secrets

Secrets must be treated separately from ordinary configuration.

Conceptually:

```text id="s3b7p4"
Configuration
    ↓
Secret Reference
    ↓
Secret Resolution
    ↓
Secret Value
```

The secret value should only exist where it is actually required.

---

## 9. Secret References

Configuration should preferably identify secrets through references rather than embedding secret material.

For example:

```text id="h6m2q8"
database_password = secret://database/main
```

rather than:

```text id="y4n1s7"
database_password = "actual-password"
```

The exact secret-reference syntax is deferred.

The important distinction is:

```text id="q8p4f1"
Secret Identity / Reference
    ≠
Secret Value
```

---

## 10. Secret Ownership

The component that needs a secret may use it without becoming the system-wide owner of secret management.

For example:

```text id="m3v7k0"
Secret Provider
       ↓
Application
       ↓
Storage Adapter
```

The secret provider controls secret retrieval.

The consuming component controls use according to its contract.

Ownership and access lifetime must be explicit.

---

## 11. Secret Lifetime

Secrets should have the shortest practical lifetime.

A component should not retain a secret beyond the period required by its operation.

If a dependency requires long-lived credentials, the component must follow the dependency's explicit credential lifetime contract.

Secrets must not accidentally survive through:

* cached query results
* errors
* logs
* traces
* metrics
* provenance
* domain objects
* checkpoints
* ordinary application state

---

## 12. Secret Propagation

Secrets must not propagate through generic data paths unless explicitly required.

For example:

```text id="8a4h2c"
Secret
    ↓
Storage Connection
```

does not imply:

```text id="p7x3j1"
Secret
    ↓
Processor Envelope
    ↓
Event
    ↓
Analysis
```

Security-sensitive values must not become domain data merely because they passed through a component.

---

## 13. Secret Redaction

Security-sensitive information must have explicit redaction semantics.

Potential exposure points include:

```text id="s0j9r5"
Logs
Errors
Metrics
Traces
Configuration Dumps
Debugging Output
Crash Reports
Provenance
Serialized State
```

The default policy is:

```text id="m1z8c4"
Do not expose secret values.
```

A diagnostic representation should use:

```text id="e4q2n7"
<redacted>
```

or an equivalent non-sensitive representation.

---

## 14. Errors and Secrets

Errors must not expose secret material.

For example, an external failure such as:

```text id="3c7x9p"
authentication failed for user X
```

must not automatically include:

```text id="q1v5b8"
password
private key
access token
authorization header
```

Error context should be bounded and security-aware.

---

## 15. Logging and Secrets

Structured logging must apply redaction before sensitive information reaches the logging backend.

Components should not rely on the logging implementation to discover every secret automatically.

The producer of sensitive information is responsible for identifying it where practical.

Observability remains separate from security semantics.

---

## 16. Metrics and Secrets

Metrics must not contain raw secrets.

Secret values must not become:

```text id="c4n7w2"
metric labels
metric values
dimension values
metric names
```

Even hashed or encoded secrets should not automatically be considered safe.

If a security-related metric is required, it should expose a deliberately defined non-sensitive representation.

---

## 17. Tracing and Secrets

Trace metadata must not automatically capture request payloads or headers containing credentials.

Distributed tracing must therefore follow the same redaction principles as logging.

Trace identity remains operational metadata and must not be confused with security identity.

---

## 18. Provenance and Secrets

Provenance records origin and derivation.

It must not become a mechanism for storing secret material.

For example:

```text id="k3w8p6"
Configuration used:
    secret reference = database/main
```

may be appropriate.

Storing:

```text id="s8v2m5"
database/main = actual-password
```

is not.

Provenance should identify security-sensitive dependencies without exposing their values.

---

## 19. Serialization and Secrets

Secrets must not be serialized as ordinary object fields unless serialization is explicitly required for a secure purpose.

Checkpointing and persistence are especially important boundaries.

A processor checkpoint must not accidentally persist:

* credentials
* access tokens
* private keys
* temporary authentication material

If a secret is genuinely required for recovery, its representation must use an explicit secure mechanism rather than ordinary state serialization.

---

## 20. Memory Lifetime

Secret handling must respect the ownership model.

When practical, sensitive buffers should have explicit lifetime and cleanup semantics.

However, the architecture does not mandate a particular memory-clearing mechanism because compiler optimization, allocator behavior, copies, and platform mechanisms affect its correctness.

Secret handling must therefore be treated as an implementation concern governed by an explicit security contract.

---

## 21. External Dependencies

External dependencies may require authentication or authorization.

The Dependency Model therefore represents security requirements explicitly:

```text id="6u8r4q"
Dependency
    ├── capability
    ├── configuration
    ├── authentication requirement
    └── authorization requirement
```

A dependency being reachable does not mean it is trusted.

Likewise:

```text id="j2m6v9"
TLS / encrypted transport
    ≠
Application authorization
```

---

## 22. Trust Boundaries

A trust boundary separates components or systems with different security assumptions.

Examples:

```text id="7w4n0k"
External Client
      |
      | Trust Boundary
      ↓
EVolution Interface
```

or:

```text id="e3p6c8"
Application
      |
      | Trust Boundary
      ↓
External Service
```

Data crossing a trust boundary must be treated according to the security contract of that boundary.

Physical process separation may provide an additional security boundary, but physical separation alone does not establish trust.

---

## 23. Untrusted Input

External input is untrusted by default.

This includes:

* network requests
* imported files
* external datasets
* extension metadata
* plugin input
* serialized objects
* external service responses

Input must pass through appropriate structural and semantic validation.

Security validation and domain validation are related but distinct.

A value can be:

```text id="q8f5y2"
Structurally valid
Semantically valid
Security-invalid
```

or vice versa.

---

## 24. Extensions and Trust

Registration does not establish trust.

An extension may be:

```text id="h2v7r4"
Registered
Discoverable
Selectable
Constructable
Trusted
```

These are different properties.

An extension's ability to execute code may require stronger isolation or security policy.

The Registration and Discovery Model does not itself establish permission to perform sensitive operations.

---

## 25. Application Operations

Protected commands must perform authorization before executing protected effects.

Conceptually:

```text id="a6n9w3"
Request
  ↓
Authenticate
  ↓
Validate
  ↓
Authorize
  ↓
Execute
```

The exact order of validation and authorization may vary where security policy requires it, but unauthorized requests must not accidentally produce protected side effects.

---

## 26. Queries and Security

Queries may also require authorization.

Authorization applies to:

* requested resource
* requested scope
* principal
* requested operation
* sensitivity level

A query returning data is therefore not inherently safe merely because it causes no state transition.

---

## 27. Multi-Tenancy

If EVolution is used in a multi-tenant environment, tenant identity and isolation must be explicit.

Conceptually:

```text id="m5j1c8"
Principal
    +
Tenant
    +
Resource
    ↓
Authorization
```

Tenant identity must not be inferred from unrelated object identifiers.

Cross-tenant access must require explicit authorization.

A future multi-tenant implementation must define:

* tenant identity
* tenant scope
* resource ownership
* isolation
* query filtering
* storage isolation
* cache isolation
* provenance semantics

---

## 28. Security and Caching

Caches must respect security boundaries.

A cached result must not be returned to a principal for whom the original result was not authorized.

Cache identity may therefore need to incorporate:

```text id="v7k2n1"
Principal
Tenant
Authorization Scope
Resource Identity
Query
Version
Consistency
```

when those values affect result visibility.

A cache must never bypass authorization simply because a result already exists.

---

## 29. Security and Reproducibility

Security state should participate in reproducibility only when it materially changes processing semantics.

For example:

```text id="f3r6w8"
Authorization policy version
```

may matter if it determines which data an analysis was allowed to access.

The actual secret used to authenticate to a service generally should not become part of analytical provenance.

Instead, provenance may record a safe identity/version/reference for the relevant security dependency.

---

## 30. Audit vs Provenance

Audit and provenance are distinct.

```text id="k4x7m0"
Provenance:
    Where did this information come from?

Audit:
    Who performed or attempted this protected action?
```

An audit record may contain:

* principal
* operation
* resource
* decision
* timestamp
* outcome

but must follow security/privacy policies.

Audit records are not automatically domain events.

---

## 31. Security Events

Security-relevant events may include:

```text id="x8n2c5"
Authentication failure
Authorization denial
Credential rotation
Security policy change
Suspicious activity
```

These should not automatically enter the domain event stream.

If a security event becomes analytical data, that transition must be explicit.

---

## 32. Credential Rotation

Credentials may change while a component is running.

The architecture must distinguish:

```text id="r5j8v2"
Credential Identity
Credential Version
Credential Value
Credential Validity
```

A future runtime credential rotation mechanism must define:

* update authority
* propagation
* atomicity
* active operations
* new operations
* failure behavior
* rollback
* observability

Dynamic rotation is not selected by this ADR.

---

## 33. Security Failure

Security failures use the standard Error/Result model.

Examples include:

```text id="k1c4s7"
AuthenticationFailure
AuthorizationDenied
CredentialUnavailable
TrustFailure
SecurityPolicyViolation
```

A security failure must not be silently converted into a successful result.

The exact generic error-code taxonomy remains governed by the Error Model.

---

## 34. Security and Recovery

Security failures do not automatically imply retry.

For example:

```text id="h6r3p9"
Authorization denied
    ≠
retry immediately
```

Likewise, credential unavailability may be recoverable while an authorization denial may not be.

Recovery decisions remain governed by the Recovery and Supervision Models.

---

## 35. Security and Resource Limits

Security controls may impose resource constraints.

Examples:

```text id="v0m5q8"
Maximum request size
Authentication attempt limits
Query limits
Connection limits
Credential lookup limits
```

These interact with the Resource Model.

Security-induced rejection must remain distinguishable from ordinary resource exhaustion where the distinction matters.

---

## 36. Security and Physical Isolation

Process and deployment isolation may strengthen security boundaries.

Examples include:

* restricted filesystem access
* restricted network access
* separate credentials
* reduced privileges
* isolated extensions

However:

```text id="z3n7x4"
Physical isolation
    ≠
complete security
```

The security contract remains explicit regardless of deployment.

---

## 37. Deferred Decisions

This ADR does not select:

* authentication protocol
* authorization framework
* identity provider
* TLS implementation
* encryption library
* secret manager
* credential storage
* key management system
* OS sandbox
* container security model
* access-control model
* RBAC
* ABAC
* OAuth
* JWT
* certificate infrastructure
* audit backend
* security monitoring platform
* memory-clearing implementation
* secure IPC mechanism

These remain implementation, deployment, or operational decisions.

---

## Decision Summary

```text id="c5n8w2"
Security:                       Cross-cutting concern
Principal identity:             Explicit
Authentication:                 Separate from authorization
Authorization:                  Explicit
Security context:               Explicit and bounded
Secrets:                        Separate from ordinary config
Secret references:              Preferred
Secret values:                  Minimized lifetime/exposure
Logs:                           Redacted
Errors:                         Redacted
Metrics:                        No raw secrets
Traces:                         No automatic secret capture
Provenance:                     No secret values
Serialization:                 No implicit secret persistence
External input:                Untrusted by default
Extensions:                     Registration ≠ trust
Caching:                        Must preserve authorization
Audit:                          Separate from provenance
Security events:                Not automatically domain events
Recovery:                       Security failure does not imply retry
Physical isolation:             Additional boundary, not complete security
Authentication mechanism:       Deferred
Authorization mechanism:        Deferred
Secret manager:                 Deferred
```

## Invariant

**EVolution treats security as an explicit operational boundary: identity, authentication, authorization, trust, secrets, audit, provenance, and observability remain distinct concerns, while sensitive information must have controlled ownership, lifetime, propagation, and exposure and must never become an accidental part of ordinary analytical or diagnostic data.**

