#include "evolution/processing/scheduler.hpp"

#include <algorithm>
#include <limits>
#include <string_view>

namespace evolution::processing {

auto DeterministicScheduler::partition_slot(const NodeId &node, std::string_view partition)
    -> std::string {
    return node.to_string() + '/' + std::string(partition);
}

auto DeterministicScheduler::evaluate(const SchedulingRequest &request,
                                      const SchedulingSnapshot &snapshot) const noexcept
    -> EligibilityDecision {
    if (request.cancellation_requested) {
        return EligibilityDecision::Cancelled;
    }
    if (!request.admitted) {
        return EligibilityDecision::AdmissionBlocked;
    }
    if (request.lifecycle != LifecycleState::Active) {
        return EligibilityDecision::LifecycleBlocked;
    }
    for (const auto &dependency : request.dependencies) {
        if (!snapshot.completed.contains(dependency)) {
            return EligibilityDecision::DependencyBlocked;
        }
    }

    const auto running_iterator = snapshot.running_by_node.find(request.node_id);
    const auto running =
        running_iterator == snapshot.running_by_node.end() ? 0 : running_iterator->second;
    if (running >= request.concurrency.maximum_parallelism()) {
        return EligibilityDecision::ConcurrencyBlocked;
    }
    if (request.concurrency.mode() == ConcurrencyMode::Serial && running != 0) {
        return EligibilityDecision::ConcurrencyBlocked;
    }
    if (request.concurrency.mode() == ConcurrencyMode::Partitioned) {
        if (!request.partition || request.partition->empty()) {
            return EligibilityDecision::OrderingBlocked;
        }
        if (snapshot.running_partitions.contains(
                partition_slot(request.node_id, *request.partition))) {
            return EligibilityDecision::ConcurrencyBlocked;
        }
    }

    if (request.concurrency.ordering() != OrderingRequirement::Unordered) {
        if (!request.sequence) {
            return EligibilityDecision::OrderingBlocked;
        }
        const auto next = snapshot.next_sequence_by_node.find(request.node_id);
        if (next != snapshot.next_sequence_by_node.end() && *request.sequence != next->second) {
            return EligibilityDecision::OrderingBlocked;
        }
    }
    if (!snapshot.resources.can_satisfy(request.resources)) {
        return EligibilityDecision::ResourceBlocked;
    }
    return EligibilityDecision::Eligible;
}

auto DeterministicScheduler::rank_eligible(const std::vector<SchedulingRequest> &requests,
                                           const SchedulingSnapshot &snapshot) const
    -> std::vector<WorkId> {
    std::vector<const SchedulingRequest *> eligible;
    eligible.reserve(requests.size());
    for (const auto &request : requests) {
        if (evaluate(request, snapshot) == EligibilityDecision::Eligible) {
            eligible.push_back(&request);
        }
    }
    std::ranges::sort(eligible, [](const SchedulingRequest *left, const SchedulingRequest *right) {
        if (left->resources.priority() != right->resources.priority()) {
            return left->resources.priority() > right->resources.priority();
        }
        const auto left_group = left->resources.fairness_group().value_or("");
        const auto right_group = right->resources.fairness_group().value_or("");
        if (left_group != right_group) {
            return left_group < right_group;
        }
        const auto left_sequence =
            left->sequence.value_or(std::numeric_limits<std::uint64_t>::max());
        const auto right_sequence =
            right->sequence.value_or(std::numeric_limits<std::uint64_t>::max());
        if (left_sequence != right_sequence) {
            return left_sequence < right_sequence;
        }
        return left->work_id.to_string() < right->work_id.to_string();
    });

    std::vector<WorkId> ranked;
    ranked.reserve(eligible.size());
    for (const auto *request : eligible) {
        ranked.push_back(request->work_id);
    }
    return ranked;
}

} // namespace evolution::processing
