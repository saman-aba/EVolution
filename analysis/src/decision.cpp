#include "evolution/analysis/decision.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace evolution::analysis {
namespace {

template <typename Value> auto invalid(std::string message) -> Result<Value> {
    return Result<Value>::failure(Error(ErrorCode::create("analysis.invalid_decision"),
                                        ErrorCategory::InvalidArgument, std::move(message)));
}

auto validate_objectives(const std::vector<PolicyObjective> &objectives,
                         ObjectiveRelationship relationship) -> Result<void> {
    if (objectives.empty()) {
        return Result<void>::failure(Error(ErrorCode::create("analysis.missing_objective"),
                                           ErrorCategory::InvalidArgument,
                                           "policy requires at least one objective"));
    }
    std::set<std::string> identities;
    for (const auto &objective : objectives) {
        if (objective.identity.empty() || objective.description.empty() ||
            !identities.insert(objective.identity).second ||
            (objective.weight && !std::isfinite(*objective.weight))) {
            return Result<void>::failure(Error(
                ErrorCode::create("analysis.invalid_objective"), ErrorCategory::InvalidArgument,
                "objectives require unique identities, descriptions, and finite weights"));
        }
        if (relationship == ObjectiveRelationship::Weighted && !objective.weight) {
            return Result<void>::failure(Error(
                ErrorCode::create("analysis.missing_objective_weight"),
                ErrorCategory::InvalidArgument, "weighted objectives require explicit weights"));
        }
    }
    return Result<void>::success();
}

auto validate_constraints(const std::vector<PolicyConstraint> &constraints) -> Result<void> {
    std::set<std::string> identities;
    for (const auto &constraint : constraints) {
        if (constraint.identity.empty() || constraint.description.empty() ||
            !identities.insert(constraint.identity).second ||
            (constraint.penalty && !std::isfinite(*constraint.penalty))) {
            return Result<void>::failure(Error(
                ErrorCode::create("analysis.invalid_constraint"), ErrorCategory::InvalidArgument,
                "constraints require unique identities, descriptions, and finite penalties"));
        }
        if (constraint.kind == ConstraintKind::Hard && constraint.penalty) {
            return Result<void>::failure(
                Error(ErrorCode::create("analysis.hard_constraint_penalty"),
                      ErrorCategory::InvalidArgument,
                      "hard constraints cannot use soft-constraint penalties"));
        }
    }
    return Result<void>::success();
}

auto valid_transition(DecisionStatus current, DecisionStatus next) -> bool {
    switch (current) {
    case DecisionStatus::Proposed:
        return next == DecisionStatus::Accepted || next == DecisionStatus::Rejected ||
               next == DecisionStatus::Expired || next == DecisionStatus::Superseded;
    case DecisionStatus::Accepted:
        return next == DecisionStatus::Executed || next == DecisionStatus::Expired ||
               next == DecisionStatus::Superseded;
    case DecisionStatus::Rejected:
    case DecisionStatus::Executed:
    case DecisionStatus::Expired:
    case DecisionStatus::Superseded:
        return false;
    }
    return false;
}

} // namespace

PolicyDefinition::PolicyDefinition(PolicyDefinitionData data) : data_(std::move(data)) {}

auto PolicyDefinition::create(PolicyDefinitionData data) -> Result<PolicyDefinition> {
    if (data.name.empty() || data.version.empty() || data.evaluator_version.empty()) {
        return invalid<PolicyDefinition>("policy name and versions cannot be empty");
    }
    auto scope = validate_scope(data.scope);
    if (!scope) {
        return Result<PolicyDefinition>::failure(scope.error());
    }
    auto objectives = validate_objectives(data.objectives, data.objective_relationship);
    if (!objectives) {
        return Result<PolicyDefinition>::failure(objectives.error());
    }
    auto constraints = validate_constraints(data.constraints);
    if (!constraints) {
        return Result<PolicyDefinition>::failure(constraints.error());
    }
    return Result<PolicyDefinition>::success(PolicyDefinition(std::move(data)));
}

auto PolicyDefinition::data() const noexcept -> const PolicyDefinitionData & {
    return data_;
}

Decision::Decision(DecisionData data) : data_(std::move(data)) {}

auto Decision::create(DecisionData data) -> Result<Decision> {
    if (data.policy_version.empty()) {
        return invalid<Decision>("decision requires a policy version");
    }
    auto scope = validate_scope(data.scope);
    if (!scope) {
        return Result<Decision>::failure(scope.error());
    }
    auto evidence =
        validate_evidence(data.evidence, data.completion != DecisionCompletion::NoDecision);
    if (!evidence) {
        return Result<Decision>::failure(evidence.error());
    }
    auto objectives = validate_objectives(data.objectives, ObjectiveRelationship::Priority);
    if (!objectives) {
        return Result<Decision>::failure(objectives.error());
    }
    auto constraints = validate_constraints(data.constraints);
    if (!constraints) {
        return Result<Decision>::failure(constraints.error());
    }
    const bool selected = data.completion == DecisionCompletion::Selected;
    if (selected != data.outcome.has_value()) {
        return invalid<Decision>("decision completion and selected outcome are inconsistent");
    }
    if (selected && data.feasibility != Feasibility::Feasible) {
        return invalid<Decision>("selected decision must be feasible");
    }
    std::set<std::string> candidate_ids;
    std::set<std::string> declared_constraints;
    for (const auto &constraint : data.constraints) {
        declared_constraints.insert(constraint.identity);
    }
    for (const auto &candidate : data.candidates) {
        if (candidate.label.empty() || !candidate_ids.insert(candidate.id.to_string()).second) {
            return invalid<Decision>("decision candidates require unique identities and labels");
        }
        std::set<std::string> scores;
        for (const auto &score : candidate.scores) {
            if (score.name.empty() || !scores.insert(score.name).second) {
                return invalid<Decision>("candidate score names must be non-empty and unique");
            }
        }
        std::set<std::string> assessments;
        for (const auto &assessment : candidate.constraints) {
            if (assessment.constraint_identity.empty() || assessment.explanation.empty() ||
                !assessments.insert(assessment.constraint_identity).second ||
                !declared_constraints.contains(assessment.constraint_identity) ||
                (assessment.penalty && !std::isfinite(*assessment.penalty))) {
                return invalid<Decision>("candidate constraint assessments are invalid");
            }
        }
    }
    if (data.outcome && !candidate_ids.contains(data.outcome->candidate_id.to_string())) {
        return invalid<Decision>("decision outcome must reference an evaluated candidate");
    }
    if (data.outcome) {
        const auto is_hard_constraint = [&](std::string_view identity) {
            const auto constraint =
                std::find_if(data.constraints.begin(), data.constraints.end(),
                             [&](const auto &item) { return item.identity == identity; });
            return constraint != data.constraints.end() && constraint->kind == ConstraintKind::Hard;
        };
        const auto chosen = std::find_if(
            data.candidates.begin(), data.candidates.end(),
            [&](const auto &candidate) { return candidate.id == data.outcome->candidate_id; });
        if (chosen == data.candidates.end() || chosen->feasibility != Feasibility::Feasible ||
            std::any_of(
                data.constraints.begin(), data.constraints.end(), [&](const auto &constraint) {
                    if (constraint.kind != ConstraintKind::Hard) {
                        return false;
                    }
                    const auto assessment =
                        std::find_if(chosen->constraints.begin(), chosen->constraints.end(),
                                     [&](const auto &item) {
                                         return item.constraint_identity == constraint.identity;
                                     });
                    return assessment == chosen->constraints.end() || !assessment->satisfied ||
                           !is_hard_constraint(assessment->constraint_identity);
                })) {
            return invalid<Decision>("selected candidate violates feasibility or constraints");
        }
    }
    if (std::any_of(data.reasoning.begin(), data.reasoning.end(),
                    [](const auto &reason) { return reason.empty(); })) {
        return invalid<Decision>("decision reasoning entries cannot be empty");
    }
    if (data.uncertainty) {
        auto uncertainty = validate_uncertainty(*data.uncertainty);
        if (!uncertainty) {
            return Result<Decision>::failure(uncertainty.error());
        }
    }
    if (data.risk && (data.risk->identity.empty() || data.risk->semantics.empty() ||
                      !std::isfinite(data.risk->value))) {
        return invalid<Decision>("decision risk requires identity, semantics, and finite value");
    }
    auto reproducibility = validate_reproducibility(data.reproducibility);
    if (!reproducibility) {
        return Result<Decision>::failure(reproducibility.error());
    }
    return Result<Decision>::success(Decision(std::move(data)));
}

auto Decision::data() const noexcept -> const DecisionData & {
    return data_;
}

auto Decision::transition(DecisionStatus next) const -> Result<Decision> {
    if (!valid_transition(data_.status, next)) {
        return invalid<Decision>("invalid decision lifecycle transition");
    }
    auto next_data = data_;
    next_data.status = next;
    return Decision::create(std::move(next_data));
}

} // namespace evolution::analysis
