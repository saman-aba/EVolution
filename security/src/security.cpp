#include "evolution/security/security.hpp"

#include <algorithm>
#include <atomic>
#include <utility>

namespace evolution::security {

struct ResourceAdmissionState {
    std::atomic_size_t active{};
};

namespace {

template <typename Value> auto invalid(std::string code, std::string message) -> Result<Value> {
    return Result<Value>::failure(Error(ErrorCode::create(std::move(code)),
                                        ErrorCategory::InvalidArgument, std::move(message)));
}

} // namespace

SecretValue::SecretValue(std::string value) : value_(std::move(value)) {}

SecretValue::SecretValue(SecretValue &&other) noexcept : value_(std::move(other.value_)) {
    other.clear();
}

auto SecretValue::operator=(SecretValue &&other) noexcept -> SecretValue & {
    if (this != &other) {
        clear();
        value_ = std::move(other.value_);
        other.clear();
    }
    return *this;
}

SecretValue::~SecretValue() {
    clear();
}

auto SecretValue::create(std::string value) -> Result<SecretValue> {
    if (value.empty()) {
        return invalid<SecretValue>("security.empty_secret", "secret value cannot be empty");
    }
    return Result<SecretValue>::success(SecretValue(std::move(value)));
}

auto SecretValue::reveal() const noexcept -> std::string_view {
    return value_;
}

void SecretValue::clear() noexcept {
    std::fill(value_.begin(), value_.end(), '\0');
    value_.clear();
}

auto Redactor::redact(std::vector<SensitiveField> fields, SensitiveClassification maximum_visible)
    -> std::map<std::string, std::string> {
    std::map<std::string, std::string> result;
    for (auto &field : fields) {
        result.insert_or_assign(std::move(field.name), static_cast<int>(field.classification) <=
                                                               static_cast<int>(maximum_visible)
                                                           ? std::move(field.value)
                                                           : std::string("<redacted>"));
    }
    return result;
}

AuditEvent::AuditEvent(AuditEventData data) : data_(std::move(data)) {}

auto AuditEvent::create(AuditEventData data) -> Result<AuditEvent> {
    if (data.operation.empty() || data.resource.empty() || data.reason_code.empty()) {
        return invalid<AuditEvent>("security.invalid_audit_event",
                                   "audit event requires operation, resource, and reason code");
    }
    for (const auto &[name, value] : data.safe_details) {
        if (name.empty() || value.empty()) {
            return invalid<AuditEvent>("security.invalid_audit_detail",
                                       "audit details cannot contain empty names or values");
        }
    }
    return Result<AuditEvent>::success(AuditEvent(std::move(data)));
}

auto AuditEvent::data() const noexcept -> const AuditEventData & {
    return data_;
}

ResourceLease::ResourceLease(std::shared_ptr<ResourceAdmissionState> state)
    : state_(std::move(state)) {}

ResourceLease::ResourceLease(ResourceLease &&other) noexcept : state_(std::move(other.state_)) {}

auto ResourceLease::operator=(ResourceLease &&other) noexcept -> ResourceLease & {
    if (this != &other) {
        release();
        state_ = std::move(other.state_);
    }
    return *this;
}

ResourceLease::~ResourceLease() {
    release();
}

void ResourceLease::release() noexcept {
    if (state_) {
        state_->active.fetch_sub(1, std::memory_order_release);
        state_.reset();
    }
}

ResourceProtector::ResourceProtector(ResourceProtection protection,
                                     std::shared_ptr<ResourceAdmissionState> state)
    : protection_(protection), state_(std::move(state)) {}

auto ResourceProtector::create(ResourceProtection protection) -> Result<ResourceProtector> {
    if (protection.maximum_request_bytes == 0 || protection.maximum_items == 0 ||
        protection.maximum_concurrent_operations == 0) {
        return invalid<ResourceProtector>("security.invalid_resource_protection",
                                          "resource protection limits must be positive");
    }
    return Result<ResourceProtector>::success(
        ResourceProtector(protection, std::make_shared<ResourceAdmissionState>()));
}

auto ResourceProtector::admit(const ResourceRequest &request) const -> Result<ResourceLease> {
    if (request.request_bytes > protection_.maximum_request_bytes ||
        request.items > protection_.maximum_items) {
        return Result<ResourceLease>::failure(
            Error(ErrorCode::create("security.resource_rejected"), ErrorCategory::ResourceExhausted,
                  "request exceeds the configured security resource boundary"));
    }
    auto active = state_->active.load(std::memory_order_relaxed);
    while (active < protection_.maximum_concurrent_operations) {
        if (state_->active.compare_exchange_weak(active, active + 1, std::memory_order_acquire,
                                                 std::memory_order_relaxed)) {
            return Result<ResourceLease>::success(ResourceLease(state_));
        }
    }
    return Result<ResourceLease>::failure(
        Error(ErrorCode::create("security.concurrent_operation_limit"),
              ErrorCategory::ResourceExhausted, "concurrent operation limit has been reached"));
}

auto ResourceProtector::protection() const noexcept -> const ResourceProtection & {
    return protection_;
}

auto validate_security_context(const SecurityContext &context, time::TimePoint now)
    -> Result<void> {
    if (context.principal.kind.empty() || context.principal.display_name.empty()) {
        return invalid<void>("security.invalid_principal",
                             "security context principal is incomplete");
    }
    if (context.expires_at && now >= *context.expires_at) {
        return Result<void>::failure(Error(ErrorCode::create("security.context_expired"),
                                           ErrorCategory::Unauthorized,
                                           "security context has expired"));
    }
    return Result<void>::success();
}

} // namespace evolution::security
