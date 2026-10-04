#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/processing/api.hpp"
#include "evolution/processing/delivery.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace evolution::processing {

enum class RecoveryScope {
    Operation,
    Processor,
    Connection,
    Graph,
    Application,
};

enum class RecoveryAction {
    Retry,
    Restart,
    Restore,
    Replay,
    Skip,
    Degrade,
    SuspendScheduling,
    Stop,
    Fail,
    ManualIntervention,
};

enum class CompletionKnowledge {
    KnownNotCompleted,
    KnownCompleted,
    Unknown,
};

struct RecoveryConditions {
    DeliveryGuarantee delivery{DeliveryGuarantee::AtLeastOnce};
    CompletionKnowledge completion{CompletionKnowledge::KnownNotCompleted};
    bool idempotent{};
    bool checkpoint_available{};
    bool checkpoint_compatible{};
    bool retained_input_available{};
    bool skip_allowed{};
    bool degradation_supported{};
};

struct RecoveryRule {
    std::optional<ErrorCategory> category;
    std::optional<std::string> error_code;
    std::optional<RecoveryScope> scope;
    RecoveryAction action;
    std::size_t maximum_attempts{1};
    RecoveryAction exhausted_action{RecoveryAction::ManualIntervention};
    bool require_idempotency{};
};

struct RecoveryDecision {
    RecoveryAction action;
    RecoveryScope scope;
    std::size_t attempt{};
    Error original_error;
    bool duplicate_execution_possible{};
    bool lossy{};
    std::string rationale;
};

class EVOLUTION_PROCESSING_API RecoveryPolicy {
  public:
    static auto create(std::vector<RecoveryRule> rules,
                       RecoveryAction fallback = RecoveryAction::ManualIntervention)
        -> Result<RecoveryPolicy>;

    [[nodiscard]] auto decide(const Error &error, RecoveryScope scope, std::size_t attempt,
                              const RecoveryConditions &conditions) const -> RecoveryDecision;

  private:
    RecoveryPolicy(std::vector<RecoveryRule> rules, RecoveryAction fallback);

    std::vector<RecoveryRule> rules_;
    RecoveryAction fallback_;
};

} // namespace evolution::processing
