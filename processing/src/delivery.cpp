#include "evolution/processing/delivery.hpp"

#include <utility>

namespace evolution::processing {

DeliveryContract::DeliveryContract(DeliveryGuarantee guarantee,
                                   AcknowledgementPoint acknowledgement,
                                   IdempotencyMode idempotency, std::uint32_t maximum_attempts,
                                   ExactlyOnceMechanism exactly_once_mechanism)
    : guarantee_(guarantee), acknowledgement_(acknowledgement), idempotency_(idempotency),
      maximum_attempts_(maximum_attempts), exactly_once_mechanism_(exactly_once_mechanism) {}

auto DeliveryContract::create(DeliveryGuarantee guarantee, AcknowledgementPoint acknowledgement,
                              IdempotencyMode idempotency, std::uint32_t maximum_attempts,
                              ExactlyOnceMechanism exactly_once_mechanism)
    -> Result<DeliveryContract> {
    if (maximum_attempts == 0) {
        return Result<DeliveryContract>::failure(Error(
            ErrorCode::create("processing.invalid_delivery_attempts"),
            ErrorCategory::InvalidArgument, "delivery contract must allow at least one attempt"));
    }
    if (guarantee == DeliveryGuarantee::AtMostOnce && maximum_attempts != 1) {
        return Result<DeliveryContract>::failure(
            Error(ErrorCode::create("processing.invalid_at_most_once_contract"),
                  ErrorCategory::InvalidArgument, "at-most-once delivery cannot permit retries"));
    }
    if (guarantee == DeliveryGuarantee::ExactlyOnce) {
        if (acknowledgement != AcknowledgementPoint::DurableCompletion ||
            idempotency == IdempotencyMode::None ||
            exactly_once_mechanism == ExactlyOnceMechanism::None) {
            return Result<DeliveryContract>::failure(
                Error(ErrorCode::create("processing.incomplete_exactly_once_contract"),
                      ErrorCategory::InvalidArgument,
                      "exactly-once requires durable completion, idempotency, and an explicit "
                      "mechanism"));
        }
    } else if (exactly_once_mechanism != ExactlyOnceMechanism::None) {
        return Result<DeliveryContract>::failure(
            Error(ErrorCode::create("processing.unused_exactly_once_mechanism"),
                  ErrorCategory::InvalidArgument,
                  "exactly-once mechanism may only be declared for exactly-once delivery"));
    }
    return Result<DeliveryContract>::success(DeliveryContract(
        guarantee, acknowledgement, idempotency, maximum_attempts, exactly_once_mechanism));
}

auto DeliveryContract::guarantee() const noexcept -> DeliveryGuarantee {
    return guarantee_;
}

auto DeliveryContract::acknowledgement() const noexcept -> AcknowledgementPoint {
    return acknowledgement_;
}

auto DeliveryContract::idempotency() const noexcept -> IdempotencyMode {
    return idempotency_;
}

auto DeliveryContract::maximum_attempts() const noexcept -> std::uint32_t {
    return maximum_attempts_;
}

auto DeliveryContract::exactly_once_mechanism() const noexcept -> ExactlyOnceMechanism {
    return exactly_once_mechanism_;
}

IdempotencyKey::IdempotencyKey(std::string value) : value_(std::move(value)) {}

auto IdempotencyKey::create(std::string value) -> Result<IdempotencyKey> {
    if (value.empty()) {
        return Result<IdempotencyKey>::failure(
            Error(ErrorCode::create("processing.invalid_idempotency_key"),
                  ErrorCategory::InvalidArgument, "idempotency key cannot be empty"));
    }
    return Result<IdempotencyKey>::success(IdempotencyKey(std::move(value)));
}

auto IdempotencyKey::value() const noexcept -> std::string_view {
    return value_;
}

auto InMemoryDeduplicationStore::contains(const IdempotencyKey &key) const -> Result<bool> {
    const std::scoped_lock lock(mutex_);
    return Result<bool>::success(completed_.contains(std::string(key.value())));
}

auto InMemoryDeduplicationStore::is_durable() const noexcept -> bool {
    return false;
}

auto InMemoryDeduplicationStore::record_completed(IdempotencyKey key) -> Result<void> {
    const std::scoped_lock lock(mutex_);
    completed_.insert(std::string(key.value()));
    return Result<void>::success();
}

DeliveryGuard::DeliveryGuard(DeliveryContract contract,
                             std::shared_ptr<DeduplicationStore> deduplication_store)
    : contract_(std::move(contract)), deduplication_store_(std::move(deduplication_store)) {}

auto DeliveryGuard::create(DeliveryContract contract,
                           std::shared_ptr<DeduplicationStore> deduplication_store)
    -> Result<DeliveryGuard> {
    if (contract.idempotency() == IdempotencyMode::Keyed && !deduplication_store) {
        return Result<DeliveryGuard>::failure(Error(
            ErrorCode::create("processing.missing_deduplication_store"),
            ErrorCategory::InvalidArgument, "keyed idempotency requires a deduplication store"));
    }
    if (contract.guarantee() == DeliveryGuarantee::ExactlyOnce &&
        contract.exactly_once_mechanism() ==
            ExactlyOnceMechanism::IdempotentEffectWithDurableDeduplication &&
        (!deduplication_store || !deduplication_store->is_durable())) {
        return Result<DeliveryGuard>::failure(
            Error(ErrorCode::create("processing.non_durable_deduplication_store"),
                  ErrorCategory::InvalidArgument,
                  "exactly-once durable deduplication requires a durable store"));
    }
    return Result<DeliveryGuard>::success(
        DeliveryGuard(std::move(contract), std::move(deduplication_store)));
}

auto DeliveryGuard::begin(const std::optional<IdempotencyKey> &key) const
    -> Result<DeliveryDecision> {
    if (contract_.idempotency() == IdempotencyMode::None ||
        contract_.idempotency() == IdempotencyMode::Natural) {
        return Result<DeliveryDecision>::success(DeliveryDecision::Execute);
    }
    if (!key) {
        return Result<DeliveryDecision>::failure(Error(
            ErrorCode::create("processing.missing_idempotency_key"), ErrorCategory::InvalidArgument,
            "keyed idempotency requires a key for each operation"));
    }
    auto existing = deduplication_store_->contains(*key);
    if (existing.has_error()) {
        return Result<DeliveryDecision>::failure(existing.error());
    }
    return Result<DeliveryDecision>::success(existing.value() ? DeliveryDecision::AlreadyCompleted
                                                              : DeliveryDecision::Execute);
}

auto DeliveryGuard::complete(const std::optional<IdempotencyKey> &key) const -> Result<void> {
    if (contract_.idempotency() != IdempotencyMode::Keyed) {
        return Result<void>::success();
    }
    if (!key) {
        return Result<void>::failure(Error(ErrorCode::create("processing.missing_idempotency_key"),
                                           ErrorCategory::InvalidArgument,
                                           "keyed idempotency requires a completion key"));
    }
    return deduplication_store_->record_completed(*key);
}

} // namespace evolution::processing
