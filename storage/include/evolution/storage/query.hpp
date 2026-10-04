#pragma once

#include "evolution/core/context/cancellation.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"
#include "evolution/storage/api.hpp"
#include "evolution/storage/contracts.hpp"

#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace evolution::storage {

enum class QueryConsistency {
    Latest,
    Snapshot,
    Versioned,
    RunSpecific,
    EventTime,
};

enum class QueryTemporalDimension {
    None,
    EventTime,
    IngestionTime,
    ProcessingTime,
    ObservationTime,
    StoredAt,
};

enum class UnknownTimePolicy {
    Exclude,
    Include,
};

enum class QueryOrderDirection {
    Ascending,
    Descending,
};

enum class FilterOperator {
    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
    Prefix,
};

struct QueryFilter {
    std::string field;
    FilterOperator operation;
    std::string value;
};

struct QueryOrdering {
    std::string field;
    QueryOrderDirection direction;
    std::string deterministic_tie_breaker;
};

struct QueryScope {
    std::string name_space;
    std::optional<std::string> tenant;
    std::optional<std::string> resource_prefix;
};

struct QueryConsistencyRequest {
    QueryConsistency mode{QueryConsistency::Latest};
    std::optional<std::string> snapshot;
    std::optional<std::string> version;
    std::optional<RunId> run_id;
};

struct QueryTemporalRequest {
    QueryTemporalDimension dimension{QueryTemporalDimension::None};
    std::optional<time::TimeRange> range;
    UnknownTimePolicy unknown_time{UnknownTimePolicy::Exclude};
};

class EVOLUTION_STORAGE_API QueryCursor {
  public:
    static auto create(std::string token) -> Result<QueryCursor>;
    [[nodiscard]] auto token() const noexcept -> const std::string &;

  private:
    explicit QueryCursor(std::string token);
    std::string token_;
};

struct PageRequest {
    std::size_t limit{};
    std::optional<QueryCursor> cursor;
};

struct QueryLimits {
    std::size_t maximum_scan_items{};
    std::size_t maximum_result_items{};
    std::size_t maximum_result_bytes{};
    bool partial_results_allowed{};
};

struct QueryDefinitionData {
    QueryId id;
    QueryScope scope;
    std::vector<QueryFilter> filters;
    QueryTemporalRequest temporal;
    QueryOrdering ordering;
    QueryConsistencyRequest consistency;
    PageRequest page;
    QueryLimits limits;
    std::optional<std::string> required_permission;
};

class EVOLUTION_STORAGE_API QueryDefinition {
  public:
    static auto create(QueryDefinitionData data) -> Result<QueryDefinition>;
    [[nodiscard]] auto data() const noexcept -> const QueryDefinitionData &;
    [[nodiscard]] auto semantic_fingerprint() const -> std::string;

  private:
    explicit QueryDefinition(QueryDefinitionData data);
    QueryDefinitionData data_;
};

struct QueryAuthorization {
    std::string principal;
    std::optional<std::string> tenant;
    std::set<std::string> permissions;

    [[nodiscard]] auto fingerprint() const -> std::string;
};

struct QueryExecutionContext {
    QueryAuthorization authorization;
    context::CancellationToken cancellation;
    std::optional<time::MonotonicTimePoint> deadline;
};

enum class QueryCompleteness {
    Complete,
    Partial,
};

struct QueryResultMetadata {
    QueryConsistency consistency;
    QueryCompleteness completeness;
    std::string dataset_version;
    std::optional<time::TemporalInformation> freshness;
    std::optional<ProvenanceId> provenance_id;
};

template <typename Record> struct QueryRecord {
    std::string logical_identity;
    Record value;
    QueryScope scope;
    std::map<std::string, std::string> fields;
    std::map<QueryTemporalDimension, time::TemporalInformation> times;
    std::string version;
    std::optional<RunId> run_id;
    std::optional<ProvenanceId> provenance_id;
    std::size_t estimated_bytes{};
};

template <typename Record> struct QueryPage {
    std::vector<QueryRecord<Record>> records;
    std::optional<QueryCursor> next_cursor;
    QueryResultMetadata metadata;
};

template <typename Record> class QueryProvider {
  public:
    virtual ~QueryProvider() = default;
    virtual auto execute(const QueryDefinition &query, const QueryExecutionContext &context) const
        -> Result<QueryPage<Record>> = 0;
};

template <typename Record> class QueryStream {
  public:
    virtual ~QueryStream() = default;
    virtual auto next() -> Result<std::optional<QueryRecord<Record>>> = 0;
};

template <typename Record> class StreamingQueryProvider {
  public:
    virtual ~StreamingQueryProvider() = default;
    virtual auto stream(const QueryDefinition &query, QueryExecutionContext context) const
        -> Result<std::unique_ptr<QueryStream<Record>>> = 0;
};

struct QueryCacheKey {
    std::string read_model_identity;
    std::string query_fingerprint;
    std::string authorization_fingerprint;

    friend auto operator==(const QueryCacheKey &, const QueryCacheKey &) -> bool = default;
};

} // namespace evolution::storage
