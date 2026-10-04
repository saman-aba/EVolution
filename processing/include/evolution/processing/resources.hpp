#pragma once

#include "evolution/core/context/resource_view.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/processing/api.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace evolution::processing {

struct ResourceReservationTag;
using ResourceReservationId = identity::Id<ResourceReservationTag>;

struct ResourceRange {
    std::uint64_t minimum{};
    std::uint64_t preferred{};
    std::optional<std::uint64_t> maximum;

    friend auto operator==(const ResourceRange &, const ResourceRange &) -> bool = default;
};

class EVOLUTION_PROCESSING_API ResourceRequirements {
  public:
    using Values = std::map<context::ResourceKind, ResourceRange>;

    static auto create(Values values, int priority = 0,
                       std::optional<std::string> fairness_group = std::nullopt,
                       bool degradation_allowed = false) -> Result<ResourceRequirements>;
    static auto none() -> ResourceRequirements;

    [[nodiscard]] auto values() const noexcept -> const Values &;
    [[nodiscard]] auto priority() const noexcept -> int;
    [[nodiscard]] auto fairness_group() const noexcept -> const std::optional<std::string> &;
    [[nodiscard]] auto degradation_allowed() const noexcept -> bool;

  private:
    ResourceRequirements(Values values, int priority, std::optional<std::string> fairness_group,
                         bool degradation_allowed);

    Values values_;
    int priority_{};
    std::optional<std::string> fairness_group_;
    bool degradation_allowed_{};
};

struct ResourceSnapshot {
    using Values = std::map<context::ResourceKind, std::uint64_t>;

    Values capacity;
    Values reserved;
    Values available;

    [[nodiscard]] auto can_satisfy(const ResourceRequirements &requirements) const noexcept -> bool;
};

struct ResourcePoolState;

class EVOLUTION_PROCESSING_API ResourceReservation {
  public:
    ResourceReservation(const ResourceReservation &) = delete;
    auto operator=(const ResourceReservation &) -> ResourceReservation & = delete;
    ResourceReservation(ResourceReservation &&other) noexcept;
    auto operator=(ResourceReservation &&other) noexcept -> ResourceReservation &;
    ~ResourceReservation();

    [[nodiscard]] auto id() const noexcept -> const ResourceReservationId &;
    [[nodiscard]] auto amounts() const noexcept -> const ResourceSnapshot::Values &;
    [[nodiscard]] auto active() const noexcept -> bool;
    void release() noexcept;

  private:
    friend class ResourcePool;
    ResourceReservation(ResourceReservationId id, ResourceSnapshot::Values amounts,
                        std::shared_ptr<ResourcePoolState> state);

    ResourceReservationId id_;
    ResourceSnapshot::Values amounts_;
    std::shared_ptr<ResourcePoolState> state_;
};

class EVOLUTION_PROCESSING_API ResourcePool {
  public:
    static auto create(ResourceSnapshot::Values capacity) -> Result<ResourcePool>;

    [[nodiscard]] auto snapshot() const -> ResourceSnapshot;
    auto reserve(const ResourceRequirements &requirements) -> Result<ResourceReservation>;

  private:
    explicit ResourcePool(std::shared_ptr<ResourcePoolState> state);
    std::shared_ptr<ResourcePoolState> state_;
};

} // namespace evolution::processing
