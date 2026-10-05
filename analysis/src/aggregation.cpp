#include "evolution/analysis/aggregation.hpp"

#include <set>
#include <utility>

namespace evolution::analysis {
namespace {

auto invalid(std::string message) -> Result<AggregationDefinition> {
    return Result<AggregationDefinition>::failure(
        Error(ErrorCode::create("analysis.invalid_aggregation_definition"),
              ErrorCategory::InvalidArgument, std::move(message)));
}

} // namespace

AggregationDefinition::AggregationDefinition(AggregationDefinitionData data)
    : data_(std::move(data)) {}

auto AggregationDefinition::create(AggregationDefinitionData data)
    -> Result<AggregationDefinition> {
    if (data.name.empty() || data.version.empty() || data.function_version.empty()) {
        return invalid("aggregation name and versions cannot be empty");
    }
    std::set<std::string> grouping;
    for (const auto &dimension : data.grouping_dimensions) {
        if (dimension.empty() || !grouping.insert(dimension).second) {
            return invalid("aggregation grouping dimensions must be non-empty and unique");
        }
    }
    if (data.ordering.required &&
        (!data.ordering.field || data.ordering.field->empty() || !data.ordering.tie_breaker ||
         data.ordering.tie_breaker->empty())) {
        return invalid("ordered aggregation requires a field and deterministic tie-breaker");
    }
    if (!data.ordering.required && (data.ordering.field || data.ordering.tie_breaker)) {
        return invalid("unordered aggregation cannot declare ordering fields");
    }
    if (data.algebra.commutative && data.algebra.order_sensitive) {
        return invalid("commutative aggregation cannot be order-sensitive");
    }
    if (data.capabilities.mergeable && !data.algebra.associative) {
        return invalid("mergeable aggregation must declare associativity");
    }
    if (data.capabilities.retractable && !data.capabilities.incremental) {
        return invalid("retractable aggregation must support incremental updates");
    }
    switch (data.window.kind) {
    case AggregationWindowKind::None:
        if (data.window.duration || data.window.count || data.window.boundary_key ||
            data.window.step) {
            return invalid("unwindowed aggregation cannot declare window bounds");
        }
        break;
    case AggregationWindowKind::Time:
        if (!data.window.duration || *data.window.duration <= std::chrono::nanoseconds::zero()) {
            return invalid("time window requires a positive duration");
        }
        break;
    case AggregationWindowKind::ObservationCount:
        if (!data.window.count || *data.window.count == 0) {
            return invalid("observation window requires a positive count");
        }
        break;
    case AggregationWindowKind::Event:
    case AggregationWindowKind::Session:
    case AggregationWindowKind::Domain:
        if (!data.window.boundary_key || data.window.boundary_key->empty()) {
            return invalid("semantic window requires a boundary key");
        }
        break;
    }
    if (data.window.step && *data.window.step <= std::chrono::nanoseconds::zero()) {
        return invalid("window step must be positive");
    }
    return Result<AggregationDefinition>::success(AggregationDefinition(std::move(data)));
}

auto AggregationDefinition::data() const noexcept -> const AggregationDefinitionData & {
    return data_;
}

} // namespace evolution::analysis
