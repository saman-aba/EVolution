#include "evolution/processing/reproducibility.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>

namespace evolution::processing {
namespace {

auto invalid(std::string message) -> Result<ReproducibilityRecord> {
    return Result<ReproducibilityRecord>::failure(
        Error(ErrorCode::create("processing.invalid_reproducibility_record"),
              ErrorCategory::InvalidArgument, std::move(message)));
}

auto sensitive_key(std::string key) -> bool {
    std::ranges::transform(key, key.begin(), [](unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return key.find("secret") != std::string::npos || key.find("password") != std::string::npos ||
           key.find("token") != std::string::npos || key.find("credential") != std::string::npos;
}

template <typename T>
void compare_value(const T &left, const T &right, std::string name,
                   ReproducibilityComparison &comparison) {
    if (left != right) {
        comparison.equivalent_conditions = false;
        comparison.differences.push_back(std::move(name));
    }
}

} // namespace

ReproducibilityRecord::ReproducibilityRecord(ReproducibilityRecordData data)
    : data_(std::move(data)) {}

auto ReproducibilityRecord::create(ReproducibilityRecordData data)
    -> Result<ReproducibilityRecord> {
    if (data.level != ReproducibilityLevel::None) {
        if (data.inputs.empty() || data.configuration_version.empty() ||
            data.graph_version.empty() || data.components.empty()) {
            return invalid("reproducibility claims require inputs, effective configuration, graph, "
                           "and component versions");
        }
    }
    for (const auto &input : data.inputs) {
        if (input.kind.empty() || input.identity.empty()) {
            return invalid("reproducibility inputs require kind and logical identity");
        }
    }
    for (const auto &component : data.components) {
        if (component.version.empty()) {
            return invalid("reproducibility component version cannot be empty");
        }
    }
    for (const auto &algorithm : data.algorithms) {
        if (algorithm.version.empty()) {
            return invalid("reproducibility algorithm version cannot be empty");
        }
    }
    for (const auto &randomness : data.randomness) {
        if (randomness.source_id.empty() || (randomness.affects_results && !randomness.seed)) {
            return invalid(
                "result-affecting randomness requires an identified source and captured seed");
        }
    }
    for (const auto &clock : data.clocks) {
        if (clock.source_id.empty() || clock.semantics.empty() ||
            (clock.affects_results && !clock.captured_time) ||
            (clock.affects_results && !clock.historical_time_preserved)) {
            return invalid(
                "result-affecting clocks require captured semantics and preserved historical time");
        }
    }
    if (data.ordering.affects_results && data.ordering.tie_breaker.empty()) {
        return invalid("result-affecting ordering requires a deterministic tie-breaker");
    }
    for (const auto &[key, value] : data.environment) {
        static_cast<void>(value);
        if (key.empty() || sensitive_key(key)) {
            return invalid(
                "reproducibility environment must not contain empty or secret-bearing keys");
        }
    }
    for (const auto &resource : data.resource_conditions) {
        if (resource.semantic_effect.empty()) {
            return invalid("recorded resource conditions require their semantic effect");
        }
    }
    for (const auto &dependency : data.external_dependencies) {
        if (dependency.name.empty() || dependency.version.empty() ||
            (dependency.affects_results && !dependency.captured_response_identity)) {
            return invalid("result-affecting external dependencies require version and captured "
                           "response identity");
        }
    }
    if (data.level == ReproducibilityLevel::SemanticDeterministic &&
        data.result_identities.empty()) {
        return invalid("semantic determinism claims require identifiable results");
    }
    return Result<ReproducibilityRecord>::success(ReproducibilityRecord(std::move(data)));
}

auto ReproducibilityRecord::data() const noexcept -> const ReproducibilityRecordData & {
    return data_;
}

auto ReproducibilityComparator::compare_conditions(const ReproducibilityRecord &left,
                                                   const ReproducibilityRecord &right)
    -> ReproducibilityComparison {
    ReproducibilityComparison comparison{true, {}};
    const auto &left_data = left.data();
    const auto &right_data = right.data();
    compare_value(left_data.level, right_data.level, "level", comparison);
    compare_value(left_data.inputs, right_data.inputs, "inputs", comparison);
    compare_value(left_data.configuration_id, right_data.configuration_id, "configuration_id",
                  comparison);
    compare_value(left_data.configuration_version, right_data.configuration_version,
                  "configuration_version", comparison);
    compare_value(left_data.graph_id, right_data.graph_id, "graph_id", comparison);
    compare_value(left_data.graph_version, right_data.graph_version, "graph_version", comparison);
    compare_value(left_data.components, right_data.components, "components", comparison);
    compare_value(left_data.algorithms, right_data.algorithms, "algorithms", comparison);
    compare_value(left_data.states, right_data.states, "states", comparison);
    compare_value(left_data.randomness, right_data.randomness, "randomness", comparison);
    compare_value(left_data.clocks, right_data.clocks, "clocks", comparison);
    compare_value(left_data.environment, right_data.environment, "environment", comparison);
    compare_value(left_data.ordering, right_data.ordering, "ordering", comparison);
    compare_value(left_data.admission, right_data.admission, "admission", comparison);
    compare_value(left_data.resource_conditions, right_data.resource_conditions,
                  "resource_conditions", comparison);
    compare_value(left_data.external_dependencies, right_data.external_dependencies,
                  "external_dependencies", comparison);
    return comparison;
}

auto ReproducibilityComparator::compare_results(const ReproducibilityRecord &left,
                                                std::string_view left_semantic_digest,
                                                const ReproducibilityRecord &right,
                                                std::string_view right_semantic_digest)
    -> DifferentialOutcome {
    if (!compare_conditions(left, right).equivalent_conditions) {
        return DifferentialOutcome::Incomparable;
    }
    return left_semantic_digest == right_semantic_digest ? DifferentialOutcome::Equivalent
                                                         : DifferentialOutcome::Different;
}

} // namespace evolution::processing
