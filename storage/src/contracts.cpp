#include "evolution/storage/contracts.hpp"

#include <utility>

namespace evolution::storage {

PersistenceProfile::PersistenceProfile(std::set<PersistenceCapability> capabilities,
                                       DurabilityGuarantee durability,
                                       VisibilityGuarantee visibility, AtomicityGuarantee atomicity)
    : capabilities_(std::move(capabilities)), durability_(durability), visibility_(visibility),
      atomicity_(atomicity) {}

auto PersistenceProfile::create(std::set<PersistenceCapability> capabilities,
                                DurabilityGuarantee durability, VisibilityGuarantee visibility,
                                AtomicityGuarantee atomicity) -> Result<PersistenceProfile> {
    if (capabilities.contains(PersistenceCapability::Transactions) &&
        atomicity != AtomicityGuarantee::Transactional) {
        return Result<PersistenceProfile>::failure(
            Error(ErrorCode::create("storage.invalid_transaction_capability"),
                  ErrorCategory::InvalidArgument,
                  "transaction capability requires transactional atomicity"));
    }
    if (capabilities.contains(PersistenceCapability::AtomicBatch) &&
        atomicity != AtomicityGuarantee::Batch && atomicity != AtomicityGuarantee::Transactional) {
        return Result<PersistenceProfile>::failure(
            Error(ErrorCode::create("storage.invalid_atomic_batch_capability"),
                  ErrorCategory::InvalidArgument,
                  "atomic batch capability requires batch or transactional atomicity"));
    }
    return Result<PersistenceProfile>::success(
        PersistenceProfile(std::move(capabilities), durability, visibility, atomicity));
}

auto PersistenceProfile::supports(PersistenceCapability capability) const noexcept -> bool {
    return capabilities_.contains(capability);
}

auto PersistenceProfile::capabilities() const noexcept -> const std::set<PersistenceCapability> & {
    return capabilities_;
}

auto PersistenceProfile::durability() const noexcept -> DurabilityGuarantee {
    return durability_;
}

auto PersistenceProfile::visibility() const noexcept -> VisibilityGuarantee {
    return visibility_;
}

auto PersistenceProfile::atomicity() const noexcept -> AtomicityGuarantee {
    return atomicity_;
}

RetentionPolicy::RetentionPolicy(RetentionKind kind, std::optional<std::size_t> maximum_records,
                                 std::optional<std::chrono::seconds> duration,
                                 std::optional<std::string> external_policy_id)
    : kind_(kind), maximum_records_(maximum_records), duration_(duration),
      external_policy_id_(std::move(external_policy_id)) {}

auto RetentionPolicy::forever() -> RetentionPolicy {
    return RetentionPolicy(RetentionKind::Forever, std::nullopt, std::nullopt, std::nullopt);
}

auto RetentionPolicy::count_bounded(std::size_t maximum_records) -> Result<RetentionPolicy> {
    if (maximum_records == 0) {
        return Result<RetentionPolicy>::failure(Error(
            ErrorCode::create("storage.invalid_retention_count"), ErrorCategory::InvalidArgument,
            "count-bounded retention requires a positive record limit"));
    }
    return Result<RetentionPolicy>::success(
        RetentionPolicy(RetentionKind::CountBounded, maximum_records, std::nullopt, std::nullopt));
}

auto RetentionPolicy::time_bounded(std::chrono::seconds duration) -> Result<RetentionPolicy> {
    if (duration <= std::chrono::seconds::zero()) {
        return Result<RetentionPolicy>::failure(Error(
            ErrorCode::create("storage.invalid_retention_duration"), ErrorCategory::InvalidArgument,
            "time-bounded retention requires a positive duration"));
    }
    return Result<RetentionPolicy>::success(
        RetentionPolicy(RetentionKind::TimeBounded, std::nullopt, duration, std::nullopt));
}

auto RetentionPolicy::external(std::string policy_id) -> Result<RetentionPolicy> {
    if (policy_id.empty()) {
        return Result<RetentionPolicy>::failure(Error(
            ErrorCode::create("storage.invalid_retention_policy"), ErrorCategory::InvalidArgument,
            "external retention policy identity cannot be empty"));
    }
    return Result<RetentionPolicy>::success(RetentionPolicy(
        RetentionKind::ExternalPolicy, std::nullopt, std::nullopt, std::move(policy_id)));
}

auto RetentionPolicy::kind() const noexcept -> RetentionKind {
    return kind_;
}

auto RetentionPolicy::maximum_records() const noexcept -> std::optional<std::size_t> {
    return maximum_records_;
}

auto RetentionPolicy::duration() const noexcept -> std::optional<std::chrono::seconds> {
    return duration_;
}

auto RetentionPolicy::external_policy_id() const noexcept -> const std::optional<std::string> & {
    return external_policy_id_;
}

StorageLocation::StorageLocation(std::string value) : value_(std::move(value)) {}

auto StorageLocation::create(std::string value) -> Result<StorageLocation> {
    if (value.empty()) {
        return Result<StorageLocation>::failure(Error(ErrorCode::create("storage.invalid_location"),
                                                      ErrorCategory::InvalidArgument,
                                                      "physical storage location cannot be empty"));
    }
    return Result<StorageLocation>::success(StorageLocation(std::move(value)));
}

auto StorageLocation::value() const noexcept -> const std::string & {
    return value_;
}

} // namespace evolution::storage
