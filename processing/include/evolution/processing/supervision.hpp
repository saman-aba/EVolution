#pragma once

#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"
#include "evolution/processing/api.hpp"
#include "evolution/processing/recovery.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace evolution::processing {

enum class DependencyCriticality {
    Required,
    Optional,
};

struct DependencyCondition {
    ComponentId component_id;
    DependencyCriticality criticality;
    bool available{};
    bool degradation_supported{};
};

enum class SupervisorState {
    Monitoring,
    Recovering,
    Cooldown,
    Degraded,
    Escalated,
    RequiresIntervention,
};

struct SupervisionPolicy {
    RecoveryScope scope;
    std::size_t maximum_attempts{1};
    std::chrono::milliseconds cooldown{};
    RecoveryAction exhausted_action{RecoveryAction::ManualIntervention};
    RecoveryAction required_dependency_action{RecoveryAction::Stop};
    bool optional_dependency_degradation{};
    std::optional<RecoveryScope> escalation_scope;
};

struct FailureObservation {
    ComponentId target;
    Error error;
    RecoveryConditions conditions;
    std::vector<DependencyCondition> dependencies;
};

struct SupervisionDecision {
    RecoveryDecision recovery;
    SupervisorState state;
    std::size_t attempts_used{};
    std::optional<time::MonotonicTimePoint> next_attempt_at;
    std::vector<ComponentId> unavailable_dependencies;
    std::optional<RecoveryScope> escalation_scope;
};

struct SupervisorData;

class EVOLUTION_PROCESSING_API Supervisor {
  public:
    static auto create(SupervisionPolicy policy, RecoveryPolicy recovery_policy)
        -> Result<Supervisor>;

    auto observe_failure(const FailureObservation &observation, time::MonotonicTimePoint now)
        -> SupervisionDecision;
    void record_recovery_success(const ComponentId &target);
    [[nodiscard]] auto state(const ComponentId &target) const noexcept -> SupervisorState;

  private:
    explicit Supervisor(std::shared_ptr<SupervisorData> data);
    std::shared_ptr<SupervisorData> data_;
};

} // namespace evolution::processing
