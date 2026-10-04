#pragma once

#include "evolution/core/identity/id.hpp"
#include "evolution/processing/api.hpp"
#include "evolution/processing/concurrency.hpp"
#include "evolution/processing/graph.hpp"
#include "evolution/processing/lifecycle.hpp"
#include "evolution/processing/resources.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace evolution::processing {

struct WorkTag;
using WorkId = identity::Id<WorkTag>;

enum class EligibilityDecision {
    Eligible,
    Cancelled,
    AdmissionBlocked,
    LifecycleBlocked,
    DependencyBlocked,
    OrderingBlocked,
    ConcurrencyBlocked,
    ResourceBlocked,
};

struct SchedulingRequest {
    WorkId work_id;
    NodeId node_id;
    std::vector<WorkId> dependencies;
    bool admitted{};
    bool cancellation_requested{};
    LifecycleState lifecycle;
    ConcurrencyContract concurrency;
    std::optional<std::uint64_t> sequence;
    std::optional<std::string> partition;
    ResourceRequirements resources;
};

struct SchedulingSnapshot {
    std::unordered_set<WorkId> completed;
    std::unordered_map<NodeId, std::size_t> running_by_node;
    std::unordered_set<std::string> running_partitions;
    std::unordered_map<NodeId, std::uint64_t> next_sequence_by_node;
    ResourceSnapshot resources;
};

class EVOLUTION_PROCESSING_API Scheduler {
  public:
    virtual ~Scheduler() = default;
    [[nodiscard]] virtual auto evaluate(const SchedulingRequest &request,
                                        const SchedulingSnapshot &snapshot) const noexcept
        -> EligibilityDecision = 0;
    [[nodiscard]] virtual auto rank_eligible(const std::vector<SchedulingRequest> &requests,
                                             const SchedulingSnapshot &snapshot) const
        -> std::vector<WorkId> = 0;
};

class EVOLUTION_PROCESSING_API DeterministicScheduler final : public Scheduler {
  public:
    [[nodiscard]] auto evaluate(const SchedulingRequest &request,
                                const SchedulingSnapshot &snapshot) const noexcept
        -> EligibilityDecision override;
    [[nodiscard]] auto rank_eligible(const std::vector<SchedulingRequest> &requests,
                                     const SchedulingSnapshot &snapshot) const
        -> std::vector<WorkId> override;

    [[nodiscard]] static auto partition_slot(const NodeId &node, std::string_view partition)
        -> std::string;
};

} // namespace evolution::processing
