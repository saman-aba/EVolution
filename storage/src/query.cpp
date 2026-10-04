#include "evolution/storage/query.hpp"

#include <chrono>
#include <sstream>
#include <utility>

namespace evolution::storage {
namespace {

auto invalid(std::string message) -> Result<QueryDefinition> {
    return Result<QueryDefinition>::failure(Error(ErrorCode::create("storage.invalid_query"),
                                                  ErrorCategory::InvalidArgument,
                                                  std::move(message)));
}

} // namespace

QueryCursor::QueryCursor(std::string token) : token_(std::move(token)) {}

auto QueryCursor::create(std::string token) -> Result<QueryCursor> {
    if (token.empty()) {
        return Result<QueryCursor>::failure(Error(ErrorCode::create("storage.invalid_query_cursor"),
                                                  ErrorCategory::InvalidArgument,
                                                  "query cursor token cannot be empty"));
    }
    return Result<QueryCursor>::success(QueryCursor(std::move(token)));
}

auto QueryCursor::token() const noexcept -> const std::string & {
    return token_;
}

QueryDefinition::QueryDefinition(QueryDefinitionData data) : data_(std::move(data)) {}

auto QueryDefinition::create(QueryDefinitionData data) -> Result<QueryDefinition> {
    if (data.scope.name_space.empty()) {
        return invalid("query namespace cannot be empty");
    }
    if (data.scope.tenant && data.scope.tenant->empty()) {
        return invalid("query tenant cannot be empty");
    }
    if (data.ordering.field.empty() || data.ordering.deterministic_tie_breaker.empty()) {
        return invalid("query ordering requires a field and deterministic tie-breaker");
    }
    if (data.page.limit == 0 || data.limits.maximum_result_items == 0 ||
        data.page.limit > data.limits.maximum_result_items || data.limits.maximum_scan_items == 0 ||
        data.limits.maximum_result_bytes == 0) {
        return invalid("query page and resource limits must be positive and bounded");
    }
    for (const auto &filter : data.filters) {
        if (filter.field.empty()) {
            return invalid("query filter field cannot be empty");
        }
    }
    if (data.temporal.range && data.temporal.dimension == QueryTemporalDimension::None) {
        return invalid("temporal range requires an explicit temporal dimension");
    }
    if (data.consistency.mode == QueryConsistency::Snapshot && !data.consistency.snapshot) {
        return invalid("snapshot consistency requires a snapshot identity");
    }
    if (data.consistency.snapshot && data.consistency.snapshot->empty()) {
        return invalid("snapshot identity cannot be empty");
    }
    if (data.consistency.mode == QueryConsistency::Versioned && !data.consistency.version) {
        return invalid("versioned consistency requires a dataset version");
    }
    if (data.consistency.version && data.consistency.version->empty()) {
        return invalid("dataset version cannot be empty");
    }
    if (data.consistency.mode == QueryConsistency::RunSpecific && !data.consistency.run_id) {
        return invalid("run-specific consistency requires a run identity");
    }
    if (data.required_permission && data.required_permission->empty()) {
        return invalid("required query permission cannot be empty");
    }
    return Result<QueryDefinition>::success(QueryDefinition(std::move(data)));
}

auto QueryDefinition::data() const noexcept -> const QueryDefinitionData & {
    return data_;
}

auto QueryDefinition::semantic_fingerprint() const -> std::string {
    std::ostringstream stream;
    stream << data_.id.to_string() << '|' << data_.scope.name_space << '|'
           << data_.scope.tenant.value_or("") << '|' << data_.scope.resource_prefix.value_or("")
           << '|' << static_cast<int>(data_.temporal.dimension) << '|'
           << static_cast<int>(data_.temporal.unknown_time) << '|';
    if (data_.temporal.range) {
        const auto start = data_.temporal.range->start();
        const auto end = data_.temporal.range->end();
        if (start) {
            stream << 'v'
                   << std::chrono::duration_cast<std::chrono::nanoseconds>(
                          start->time_since_epoch())
                          .count();
        } else {
            stream << 'n';
        }
        stream << ':';
        if (end) {
            stream << 'v'
                   << std::chrono::duration_cast<std::chrono::nanoseconds>(end->time_since_epoch())
                          .count();
        } else {
            stream << 'n';
        }
    }
    stream << '|' << static_cast<int>(data_.ordering.direction) << '|' << data_.ordering.field
           << '|' << data_.ordering.deterministic_tie_breaker << '|'
           << static_cast<int>(data_.consistency.mode) << '|'
           << data_.consistency.snapshot.value_or("") << '|'
           << data_.consistency.version.value_or("") << '|'
           << (data_.consistency.run_id ? data_.consistency.run_id->to_string() : "") << '|'
           << data_.page.limit << '|' << (data_.page.cursor ? data_.page.cursor->token() : "")
           << '|' << data_.limits.maximum_scan_items << '|' << data_.limits.maximum_result_items
           << '|' << data_.limits.maximum_result_bytes << '|'
           << data_.limits.partial_results_allowed << '|' << data_.required_permission.value_or("");
    for (const auto &filter : data_.filters) {
        stream << '|' << filter.field << ':' << static_cast<int>(filter.operation) << ':'
               << filter.value;
    }
    return stream.str();
}

auto QueryAuthorization::fingerprint() const -> std::string {
    std::ostringstream stream;
    stream << principal << '|' << tenant.value_or("");
    for (const auto &permission : permissions) {
        stream << '|' << permission;
    }
    return stream.str();
}

} // namespace evolution::storage
