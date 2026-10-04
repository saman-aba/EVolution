#include "evolution/processing/recovery.hpp"

#include <utility>

namespace evolution::processing {
namespace {

auto matches(const RecoveryRule &rule, const Error &error, RecoveryScope scope) -> bool {
    return (!rule.category || *rule.category == error.category()) &&
           (!rule.error_code || *rule.error_code == error.code().value()) &&
           (!rule.scope || *rule.scope == scope);
}

auto automatic_action(RecoveryAction action) noexcept -> bool {
    return action != RecoveryAction::Fail && action != RecoveryAction::Stop &&
           action != RecoveryAction::ManualIntervention;
}

auto feasible(RecoveryAction action, const RecoveryConditions &conditions,
              bool require_idempotency) noexcept -> bool {
    switch (action) {
    case RecoveryAction::Retry:
        if (conditions.delivery == DeliveryGuarantee::AtMostOnce) {
            return false;
        }
        if (conditions.completion == CompletionKnowledge::Unknown && !conditions.idempotent) {
            return false;
        }
        return !require_idempotency || conditions.idempotent;
    case RecoveryAction::Restore:
        return conditions.checkpoint_available && conditions.checkpoint_compatible;
    case RecoveryAction::Replay:
        return conditions.retained_input_available;
    case RecoveryAction::Skip:
        return conditions.skip_allowed;
    case RecoveryAction::Degrade:
        return conditions.degradation_supported;
    case RecoveryAction::Restart:
    case RecoveryAction::SuspendScheduling:
    case RecoveryAction::Stop:
    case RecoveryAction::Fail:
    case RecoveryAction::ManualIntervention:
        return true;
    }
    return false;
}

auto decision(RecoveryAction action, RecoveryScope scope, std::size_t attempt, const Error &error,
              std::string rationale) -> RecoveryDecision {
    return RecoveryDecision{
        action,
        scope,
        attempt,
        error,
        action == RecoveryAction::Retry || action == RecoveryAction::Replay,
        action == RecoveryAction::Skip || action == RecoveryAction::Degrade,
        std::move(rationale),
    };
}

} // namespace

RecoveryPolicy::RecoveryPolicy(std::vector<RecoveryRule> rules, RecoveryAction fallback)
    : rules_(std::move(rules)), fallback_(fallback) {}

auto RecoveryPolicy::create(std::vector<RecoveryRule> rules, RecoveryAction fallback)
    -> Result<RecoveryPolicy> {
    if (automatic_action(fallback)) {
        return Result<RecoveryPolicy>::failure(
            Error(ErrorCode::create("processing.invalid_recovery_fallback"),
                  ErrorCategory::InvalidArgument,
                  "automatic recovery requires an explicit bounded rule, not a fallback"));
    }
    for (const auto &rule : rules) {
        if (rule.error_code && rule.error_code->empty()) {
            return Result<RecoveryPolicy>::failure(
                Error(ErrorCode::create("processing.invalid_recovery_rule"),
                      ErrorCategory::InvalidArgument, "recovery rule error code cannot be empty"));
        }
        if (automatic_action(rule.action) && rule.maximum_attempts == 0) {
            return Result<RecoveryPolicy>::failure(
                Error(ErrorCode::create("processing.invalid_recovery_limit"),
                      ErrorCategory::InvalidArgument,
                      "automatic recovery actions require a bounded positive attempt limit"));
        }
        if (automatic_action(rule.exhausted_action)) {
            return Result<RecoveryPolicy>::failure(
                Error(ErrorCode::create("processing.invalid_recovery_escalation"),
                      ErrorCategory::InvalidArgument,
                      "exhausted recovery action must stop, fail, or require intervention"));
        }
    }
    return Result<RecoveryPolicy>::success(RecoveryPolicy(std::move(rules), fallback));
}

auto RecoveryPolicy::decide(const Error &error, RecoveryScope scope, std::size_t attempt,
                            const RecoveryConditions &conditions) const -> RecoveryDecision {
    for (const auto &rule : rules_) {
        if (!matches(rule, error, scope)) {
            continue;
        }
        if (automatic_action(rule.action) && attempt >= rule.maximum_attempts) {
            return decision(rule.exhausted_action, scope, attempt, error,
                            "bounded recovery attempts are exhausted");
        }
        if (!feasible(rule.action, conditions, rule.require_idempotency)) {
            return decision(RecoveryAction::ManualIntervention, scope, attempt, error,
                            "selected recovery action cannot satisfy current delivery, state, or "
                            "capability contracts");
        }
        return decision(rule.action, scope, attempt, error,
                        "explicit recovery rule selected this action");
    }
    return decision(fallback_, scope, attempt, error,
                    "no recovery rule matched the original error");
}

} // namespace evolution::processing
