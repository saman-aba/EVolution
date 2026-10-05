#include "evolution/analysis/pattern.hpp"

#include <cmath>
#include <set>
#include <utility>

namespace evolution::analysis {
namespace {

template <typename Value> auto invalid(std::string code, std::string message) -> Result<Value> {
    return Result<Value>::failure(Error(ErrorCode::create(std::move(code)),
                                        ErrorCategory::InvalidArgument, std::move(message)));
}

} // namespace

PatternStrength::PatternStrength(double value, std::string semantics)
    : value_(value), semantics_(std::move(semantics)) {}

auto PatternStrength::create(double value, std::string semantics) -> Result<PatternStrength> {
    if (!std::isfinite(value) || semantics.empty()) {
        return invalid<PatternStrength>("analysis.invalid_pattern_strength",
                                        "pattern strength must be finite and have semantics");
    }
    return Result<PatternStrength>::success(PatternStrength(value, std::move(semantics)));
}

auto PatternStrength::value() const noexcept -> double {
    return value_;
}

auto PatternStrength::semantics() const noexcept -> const std::string & {
    return semantics_;
}

PatternDefinition::PatternDefinition(PatternDefinitionData data) : data_(std::move(data)) {}

auto PatternDefinition::create(PatternDefinitionData data) -> Result<PatternDefinition> {
    if (data.type.empty() || data.version.empty() || data.detector_version.empty() ||
        data.algorithm_version.empty()) {
        return invalid<PatternDefinition>("analysis.invalid_pattern_definition",
                                          "pattern type and versions cannot be empty");
    }
    return Result<PatternDefinition>::success(PatternDefinition(std::move(data)));
}

auto PatternDefinition::data() const noexcept -> const PatternDefinitionData & {
    return data_;
}

PatternResult::PatternResult(PatternResultData data) : data_(std::move(data)) {}

auto PatternResult::create(PatternResultData data) -> Result<PatternResult> {
    if (data.definition_version.empty() || data.type.empty()) {
        return invalid<PatternResult>("analysis.invalid_pattern_result",
                                      "pattern result type and definition version cannot be empty");
    }
    auto scope = validate_scope(data.scope);
    if (!scope) {
        return Result<PatternResult>::failure(scope.error());
    }
    auto evidence = validate_evidence(data.evidence);
    if (!evidence) {
        return Result<PatternResult>::failure(evidence.error());
    }
    auto reproducibility = validate_reproducibility(data.reproducibility);
    if (!reproducibility) {
        return Result<PatternResult>::failure(reproducibility.error());
    }
    std::set<std::string> related;
    for (const auto &relationship : data.relationships) {
        if (relationship.related_pattern == data.id) {
            return invalid<PatternResult>("analysis.self_pattern_relationship",
                                          "pattern cannot relate to itself");
        }
        if (relationship.strength && !std::isfinite(*relationship.strength)) {
            return invalid<PatternResult>("analysis.invalid_pattern_relationship",
                                          "pattern relationship strength must be finite");
        }
        if (!related.insert(relationship.related_pattern.to_string()).second) {
            return invalid<PatternResult>("analysis.duplicate_pattern_relationship",
                                          "related pattern identities must be unique");
        }
    }
    std::set<std::string> value_names;
    for (const auto &value : data.values) {
        if (value.name.empty() || !value_names.insert(value.name).second) {
            return invalid<PatternResult>("analysis.invalid_pattern_value",
                                          "pattern value names must be non-empty and unique");
        }
    }
    return Result<PatternResult>::success(PatternResult(std::move(data)));
}

auto PatternResult::data() const noexcept -> const PatternResultData & {
    return data_;
}

} // namespace evolution::analysis
