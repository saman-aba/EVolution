#include "evolution/processing/resources.hpp"

#include <mutex>
#include <utility>

namespace evolution::processing {

struct ResourcePoolState {
    explicit ResourcePoolState(ResourceSnapshot::Values capacity_values)
        : capacity(std::move(capacity_values)) {}

    std::mutex mutex;
    ResourceSnapshot::Values capacity;
    ResourceSnapshot::Values reserved;
};

namespace {

auto value_or_zero(const ResourceSnapshot::Values &values, context::ResourceKind kind) noexcept
    -> std::uint64_t {
    const auto iterator = values.find(kind);
    return iterator == values.end() ? 0 : iterator->second;
}

void release_reservation(const std::shared_ptr<ResourcePoolState> &state,
                         const ResourceSnapshot::Values &amounts) noexcept {
    if (!state) {
        return;
    }
    const std::scoped_lock lock(state->mutex);
    for (const auto &[kind, amount] : amounts) {
        auto &reserved = state->reserved[kind];
        reserved = amount > reserved ? 0 : reserved - amount;
    }
}

} // namespace

ResourceRequirements::ResourceRequirements(Values values, int priority,
                                           std::optional<std::string> fairness_group,
                                           bool degradation_allowed)
    : values_(std::move(values)), priority_(priority), fairness_group_(std::move(fairness_group)),
      degradation_allowed_(degradation_allowed) {}

auto ResourceRequirements::create(Values values, int priority,
                                  std::optional<std::string> fairness_group,
                                  bool degradation_allowed) -> Result<ResourceRequirements> {
    if (fairness_group && fairness_group->empty()) {
        return Result<ResourceRequirements>::failure(
            Error(ErrorCode::create("processing.invalid_fairness_group"),
                  ErrorCategory::InvalidArgument, "resource fairness group cannot be empty"));
    }
    for (const auto &[kind, range] : values) {
        static_cast<void>(kind);
        if (range.preferred < range.minimum ||
            (range.maximum &&
             (*range.maximum < range.minimum || *range.maximum < range.preferred))) {
            return Result<ResourceRequirements>::failure(
                Error(ErrorCode::create("processing.invalid_resource_range"),
                      ErrorCategory::InvalidArgument,
                      "resource range must satisfy minimum <= preferred <= maximum"));
        }
    }
    return Result<ResourceRequirements>::success(ResourceRequirements(
        std::move(values), priority, std::move(fairness_group), degradation_allowed));
}

auto ResourceRequirements::none() -> ResourceRequirements {
    return ResourceRequirements({}, 0, std::nullopt, false);
}

auto ResourceRequirements::values() const noexcept -> const Values & {
    return values_;
}

auto ResourceRequirements::priority() const noexcept -> int {
    return priority_;
}

auto ResourceRequirements::fairness_group() const noexcept -> const std::optional<std::string> & {
    return fairness_group_;
}

auto ResourceRequirements::degradation_allowed() const noexcept -> bool {
    return degradation_allowed_;
}

auto ResourceSnapshot::can_satisfy(const ResourceRequirements &requirements) const noexcept
    -> bool {
    for (const auto &[kind, range] : requirements.values()) {
        if (value_or_zero(available, kind) < range.minimum) {
            return false;
        }
    }
    return true;
}

ResourceReservation::ResourceReservation(ResourceReservationId id, ResourceSnapshot::Values amounts,
                                         std::shared_ptr<ResourcePoolState> state)
    : id_(std::move(id)), amounts_(std::move(amounts)), state_(std::move(state)) {}

ResourceReservation::ResourceReservation(ResourceReservation &&other) noexcept
    : id_(std::move(other.id_)), amounts_(std::move(other.amounts_)),
      state_(std::move(other.state_)) {}

auto ResourceReservation::operator=(ResourceReservation &&other) noexcept -> ResourceReservation & {
    if (this != &other) {
        release();
        id_ = std::move(other.id_);
        amounts_ = std::move(other.amounts_);
        state_ = std::move(other.state_);
    }
    return *this;
}

ResourceReservation::~ResourceReservation() {
    release();
}

auto ResourceReservation::id() const noexcept -> const ResourceReservationId & {
    return id_;
}

auto ResourceReservation::amounts() const noexcept -> const ResourceSnapshot::Values & {
    return amounts_;
}

auto ResourceReservation::active() const noexcept -> bool {
    return static_cast<bool>(state_);
}

void ResourceReservation::release() noexcept {
    release_reservation(state_, amounts_);
    state_.reset();
}

ResourcePool::ResourcePool(std::shared_ptr<ResourcePoolState> state) : state_(std::move(state)) {}

auto ResourcePool::create(ResourceSnapshot::Values capacity) -> Result<ResourcePool> {
    for (const auto &[kind, amount] : capacity) {
        static_cast<void>(kind);
        if (amount == 0) {
            return Result<ResourcePool>::failure(
                Error(ErrorCode::create("processing.invalid_resource_capacity"),
                      ErrorCategory::InvalidArgument,
                      "declared resource capacities must be greater than zero"));
        }
    }
    return Result<ResourcePool>::success(
        ResourcePool(std::make_shared<ResourcePoolState>(std::move(capacity))));
}

auto ResourcePool::snapshot() const -> ResourceSnapshot {
    const std::scoped_lock lock(state_->mutex);
    ResourceSnapshot snapshot{state_->capacity, state_->reserved, {}};
    for (const auto &[kind, capacity] : state_->capacity) {
        const auto reserved = value_or_zero(state_->reserved, kind);
        snapshot.available[kind] = reserved > capacity ? 0 : capacity - reserved;
    }
    return snapshot;
}

auto ResourcePool::reserve(const ResourceRequirements &requirements)
    -> Result<ResourceReservation> {
    const std::scoped_lock lock(state_->mutex);
    ResourceSnapshot::Values amounts;
    for (const auto &[kind, range] : requirements.values()) {
        const auto capacity = value_or_zero(state_->capacity, kind);
        const auto reserved = value_or_zero(state_->reserved, kind);
        const auto available = reserved > capacity ? 0 : capacity - reserved;
        if (available < range.minimum) {
            return Result<ResourceReservation>::failure(
                Error(ErrorCode::create("processing.resource_exhausted"),
                      ErrorCategory::ResourceExhausted,
                      "required execution resources are not currently available"));
        }
        amounts[kind] = range.minimum;
    }
    for (const auto &[kind, amount] : amounts) {
        state_->reserved[kind] += amount;
    }
    auto id = ResourceReservationId::generate();
    if (id.has_error()) {
        for (const auto &[kind, amount] : amounts) {
            state_->reserved[kind] -= amount;
        }
        return Result<ResourceReservation>::failure(id.error());
    }
    return Result<ResourceReservation>::success(
        ResourceReservation(std::move(id).value(), std::move(amounts), state_));
}

} // namespace evolution::processing
