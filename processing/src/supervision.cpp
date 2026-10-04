#include "evolution/processing/supervision.hpp"

#include <mutex>
#include <unordered_map>
#include <utility>

namespace evolution::processing {
namespace {

struct TargetSupervisionState {
    std::size_t attempts{};
    std::optional<time::MonotonicTimePoint> last_attempt;
    SupervisorState state{SupervisorState::Monitoring};
};

auto automatic(RecoveryAction action) noexcept -> bool {
    return action == RecoveryAction::Retry || action == RecoveryAction::Restart ||
           action == RecoveryAction::Restore || action == RecoveryAction::Replay ||
           action == RecoveryAction::Skip || action == RecoveryAction::Degrade;
}

auto make_recovery(RecoveryAction action, RecoveryScope scope, std::size_t attempt,
                   const Error &error, std::string rationale) -> RecoveryDecision {
    return RecoveryDecision{action,
                            scope,
                            attempt,
                            error,
                            action == RecoveryAction::Retry || action == RecoveryAction::Replay,
                            action == RecoveryAction::Skip || action == RecoveryAction::Degrade,
                            std::move(rationale)};
}

} // namespace

struct SupervisorData {
    SupervisorData(SupervisionPolicy supervision_policy, RecoveryPolicy policy)
        : configuration(std::move(supervision_policy)), recovery_policy(std::move(policy)) {}

    mutable std::mutex mutex;
    SupervisionPolicy configuration;
    RecoveryPolicy recovery_policy;
    std::unordered_map<ComponentId, TargetSupervisionState> targets;
};

Supervisor::Supervisor(std::shared_ptr<SupervisorData> data) : data_(std::move(data)) {}

auto Supervisor::create(SupervisionPolicy policy, RecoveryPolicy recovery_policy)
    -> Result<Supervisor> {
    if (policy.maximum_attempts == 0) {
        return Result<Supervisor>::failure(
            Error(ErrorCode::create("processing.invalid_supervision_limit"),
                  ErrorCategory::InvalidArgument,
                  "supervision requires a bounded positive recovery-attempt limit"));
    }
    if (policy.cooldown < std::chrono::milliseconds::zero()) {
        return Result<Supervisor>::failure(
            Error(ErrorCode::create("processing.invalid_supervision_cooldown"),
                  ErrorCategory::InvalidArgument, "supervision cooldown cannot be negative"));
    }
    if (automatic(policy.exhausted_action) || automatic(policy.required_dependency_action)) {
        return Result<Supervisor>::failure(Error(
            ErrorCode::create("processing.invalid_supervision_escalation"),
            ErrorCategory::InvalidArgument,
            "supervision exhaustion and required-dependency actions must be terminal or manual"));
    }
    return Result<Supervisor>::success(Supervisor(
        std::make_shared<SupervisorData>(std::move(policy), std::move(recovery_policy))));
}

auto Supervisor::observe_failure(const FailureObservation &observation,
                                 time::MonotonicTimePoint now) -> SupervisionDecision {
    const std::scoped_lock lock(data_->mutex);
    auto &target = data_->targets[observation.target];

    std::vector<ComponentId> unavailable;
    bool required_dependency_unavailable = false;
    bool optional_degradation = false;
    for (const auto &dependency : observation.dependencies) {
        if (dependency.available) {
            continue;
        }
        unavailable.push_back(dependency.component_id);
        required_dependency_unavailable = required_dependency_unavailable ||
                                          dependency.criticality == DependencyCriticality::Required;
        optional_degradation =
            optional_degradation || (dependency.criticality == DependencyCriticality::Optional &&
                                     dependency.degradation_supported);
    }

    if (required_dependency_unavailable) {
        target.state = data_->configuration.escalation_scope ? SupervisorState::Escalated
                                                             : SupervisorState::Recovering;
        auto recovery = make_recovery(data_->configuration.required_dependency_action,
                                      data_->configuration.scope, target.attempts,
                                      observation.error, "required dependency is unavailable");
        return {std::move(recovery), target.state,           target.attempts,
                std::nullopt,        std::move(unavailable), data_->configuration.escalation_scope};
    }
    if (optional_degradation && data_->configuration.optional_dependency_degradation &&
        observation.conditions.degradation_supported) {
        target.state = SupervisorState::Degraded;
        auto recovery = make_recovery(RecoveryAction::Degrade, data_->configuration.scope,
                                      target.attempts, observation.error,
                                      "optional dependency permits explicit degraded operation");
        return {std::move(recovery), target.state,           target.attempts,
                std::nullopt,        std::move(unavailable), std::nullopt};
    }

    if (target.attempts >= data_->configuration.maximum_attempts) {
        target.state = data_->configuration.exhausted_action == RecoveryAction::ManualIntervention
                           ? SupervisorState::RequiresIntervention
                           : SupervisorState::Escalated;
        auto recovery = make_recovery(
            data_->configuration.exhausted_action, data_->configuration.scope, target.attempts,
            observation.error, "supervisor recovery-attempt limit is exhausted");
        return {std::move(recovery), target.state, target.attempts,
                std::nullopt,        {},           data_->configuration.escalation_scope};
    }

    if (target.last_attempt) {
        const auto next_attempt = *target.last_attempt + data_->configuration.cooldown;
        if (now < next_attempt) {
            target.state = SupervisorState::Cooldown;
            auto recovery = make_recovery(
                RecoveryAction::SuspendScheduling, data_->configuration.scope, target.attempts,
                observation.error, "supervisor cooldown prevents a recovery storm");
            return {std::move(recovery), target.state, target.attempts,
                    next_attempt,        {},           std::nullopt};
        }
    }

    auto recovery = data_->recovery_policy.decide(observation.error, data_->configuration.scope,
                                                  target.attempts, observation.conditions);
    if (automatic(recovery.action)) {
        ++target.attempts;
        target.last_attempt = now;
    }
    if (recovery.action == RecoveryAction::ManualIntervention) {
        target.state = SupervisorState::RequiresIntervention;
    } else if (recovery.action == RecoveryAction::Degrade) {
        target.state = SupervisorState::Degraded;
    } else if (recovery.action == RecoveryAction::Fail || recovery.action == RecoveryAction::Stop) {
        target.state = data_->configuration.escalation_scope ? SupervisorState::Escalated
                                                             : SupervisorState::Recovering;
    } else {
        target.state = SupervisorState::Recovering;
    }
    return {std::move(recovery),
            target.state,
            target.attempts,
            std::nullopt,
            {},
            target.state == SupervisorState::Escalated ? data_->configuration.escalation_scope
                                                       : std::nullopt};
}

void Supervisor::record_recovery_success(const ComponentId &target) {
    const std::scoped_lock lock(data_->mutex);
    data_->targets[target] = TargetSupervisionState{};
}

auto Supervisor::state(const ComponentId &target) const noexcept -> SupervisorState {
    const std::scoped_lock lock(data_->mutex);
    const auto iterator = data_->targets.find(target);
    return iterator == data_->targets.end() ? SupervisorState::Monitoring : iterator->second.state;
}

} // namespace evolution::processing
