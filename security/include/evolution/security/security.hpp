#pragma once

#include "evolution/core/configuration/secret_reference.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"
#include "evolution/security/api.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace evolution::security {

struct PrincipalTag;
struct AuditEventTag;
using PrincipalId = identity::Id<PrincipalTag>;
using AuditEventId = identity::Id<AuditEventTag>;

enum class TrustLevel { Untrusted, Authenticated, Trusted, System };
enum class AuthorizationOutcome { Allowed, Denied };
enum class AuditOutcome { Succeeded, Denied, Rejected, Failed, Cancelled };

struct Principal {
    PrincipalId id;
    std::string kind;
    std::string display_name;
    std::optional<std::string> tenant;
    std::map<std::string, std::string> attributes;
};

struct AuthenticationRequest {
    std::string mechanism;
    std::string credential_reference;
    std::optional<std::string> claimed_tenant;
};

struct SecurityContext {
    Principal principal;
    TrustLevel trust{TrustLevel::Untrusted};
    std::set<std::string> permissions;
    time::TemporalInformation authenticated_at{time::TemporalInformation::unknown()};
    std::optional<time::TimePoint> expires_at;
};

struct AuthorizationRequest {
    std::string operation;
    std::string resource;
    std::string action;
    std::optional<std::string> tenant;
};

struct AuthorizationDecision {
    AuthorizationOutcome outcome{AuthorizationOutcome::Denied};
    std::string policy;
    std::string reason_code;
    std::vector<std::string> obligations;
};

class Authenticator {
  public:
    virtual ~Authenticator() = default;
    [[nodiscard]] virtual auto authenticate(const AuthenticationRequest &request) const
        -> Result<SecurityContext> = 0;
};

class Authorizer {
  public:
    virtual ~Authorizer() = default;
    [[nodiscard]] virtual auto authorize(const SecurityContext &context,
                                         const AuthorizationRequest &request) const
        -> Result<AuthorizationDecision> = 0;
};

class EVOLUTION_SECURITY_API SecretValue {
  public:
    SecretValue(const SecretValue &) = delete;
    auto operator=(const SecretValue &) -> SecretValue & = delete;
    SecretValue(SecretValue &&other) noexcept;
    auto operator=(SecretValue &&other) noexcept -> SecretValue &;
    ~SecretValue();

    static auto create(std::string value) -> Result<SecretValue>;
    [[nodiscard]] auto reveal() const noexcept -> std::string_view;

  private:
    explicit SecretValue(std::string value);
    void clear() noexcept;
    std::string value_;
};

class SecretProvider {
  public:
    virtual ~SecretProvider() = default;
    virtual auto resolve(const configuration::SecretReference &reference) const
        -> Result<SecretValue> = 0;
};

enum class SensitiveClassification { Public, Internal, Confidential, Secret };

struct SensitiveField {
    std::string name;
    std::string value;
    SensitiveClassification classification{SensitiveClassification::Public};
};

class EVOLUTION_SECURITY_API Redactor {
  public:
    [[nodiscard]] static auto redact(std::vector<SensitiveField> fields,
                                     SensitiveClassification maximum_visible)
        -> std::map<std::string, std::string>;
};

struct AuditEventData {
    AuditEventId id;
    PrincipalId principal_id;
    std::string operation;
    std::string resource;
    AuditOutcome outcome;
    std::string reason_code;
    time::TemporalInformation occurred_at{time::TemporalInformation::unknown()};
    std::optional<CorrelationId> correlation_id;
    std::map<std::string, std::string> safe_details;
};

class EVOLUTION_SECURITY_API AuditEvent {
  public:
    static auto create(AuditEventData data) -> Result<AuditEvent>;
    [[nodiscard]] auto data() const noexcept -> const AuditEventData &;

  private:
    explicit AuditEvent(AuditEventData data);
    AuditEventData data_;
};

class AuditSink {
  public:
    virtual ~AuditSink() = default;
    virtual auto record(AuditEvent event) -> Result<void> = 0;
};

struct ResourceProtection {
    std::size_t maximum_request_bytes{};
    std::size_t maximum_items{};
    std::size_t maximum_concurrent_operations{};
};

struct ResourceRequest {
    std::size_t request_bytes{};
    std::size_t items{};
};

struct ResourceAdmissionState;

class EVOLUTION_SECURITY_API ResourceLease {
  public:
    ResourceLease(const ResourceLease &) = delete;
    auto operator=(const ResourceLease &) -> ResourceLease & = delete;
    ResourceLease(ResourceLease &&other) noexcept;
    auto operator=(ResourceLease &&other) noexcept -> ResourceLease &;
    ~ResourceLease();

  private:
    friend class ResourceProtector;
    explicit ResourceLease(std::shared_ptr<ResourceAdmissionState> state);
    void release() noexcept;
    std::shared_ptr<ResourceAdmissionState> state_;
};

class EVOLUTION_SECURITY_API ResourceProtector {
  public:
    static auto create(ResourceProtection protection) -> Result<ResourceProtector>;
    [[nodiscard]] auto admit(const ResourceRequest &request) const -> Result<ResourceLease>;
    [[nodiscard]] auto protection() const noexcept -> const ResourceProtection &;

  private:
    ResourceProtector(ResourceProtection protection, std::shared_ptr<ResourceAdmissionState> state);
    ResourceProtection protection_;
    std::shared_ptr<ResourceAdmissionState> state_;
};

EVOLUTION_SECURITY_API auto validate_security_context(const SecurityContext &context,
                                                      time::TimePoint now) -> Result<void>;

} // namespace evolution::security
