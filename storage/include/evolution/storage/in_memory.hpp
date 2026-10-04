#pragma once

#include "evolution/storage/contracts.hpp"
#include "evolution/storage/query.hpp"
#include "evolution/storage/read_model.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace evolution::storage {

enum class DuplicateWritePolicy {
    Reject,
    Idempotent,
};

namespace detail {

inline auto storage_error(std::string code, ErrorCategory category, std::string message) -> Error {
    return Error(ErrorCode::create(std::move(code)), category, std::move(message));
}

inline auto scope_matches(const QueryScope &candidate, const QueryScope &requested) -> bool {
    if (candidate.name_space != requested.name_space) {
        return false;
    }
    if (requested.tenant && candidate.tenant != requested.tenant) {
        return false;
    }
    if (requested.resource_prefix) {
        if (!candidate.resource_prefix ||
            !candidate.resource_prefix->starts_with(*requested.resource_prefix)) {
            return false;
        }
    }
    return true;
}

inline auto compare_text(std::string_view left, FilterOperator operation, std::string_view right)
    -> bool {
    switch (operation) {
    case FilterOperator::Equal:
        return left == right;
    case FilterOperator::NotEqual:
        return left != right;
    case FilterOperator::Less:
        return left < right;
    case FilterOperator::LessOrEqual:
        return left <= right;
    case FilterOperator::Greater:
        return left > right;
    case FilterOperator::GreaterOrEqual:
        return left >= right;
    case FilterOperator::Prefix:
        return left.starts_with(right);
    }
    return false;
}

template <typename Record>
auto field_value(const QueryRecord<Record> &record, std::string_view field)
    -> std::optional<std::string_view> {
    if (field == "logical_identity") {
        return record.logical_identity;
    }
    if (field == "version") {
        return record.version;
    }
    const auto iterator = record.fields.find(std::string(field));
    if (iterator == record.fields.end()) {
        return std::nullopt;
    }
    return iterator->second;
}

template <typename Record>
auto matches_filters(const QueryRecord<Record> &record, const std::vector<QueryFilter> &filters)
    -> bool {
    return std::all_of(filters.begin(), filters.end(), [&](const QueryFilter &filter) {
        const auto value = field_value(record, filter.field);
        return value && compare_text(*value, filter.operation, filter.value);
    });
}

template <typename Record>
auto matches_temporal(const QueryRecord<Record> &record, const QueryTemporalRequest &temporal)
    -> bool {
    if (temporal.dimension == QueryTemporalDimension::None) {
        return true;
    }
    const auto iterator = record.times.find(temporal.dimension);
    if (iterator == record.times.end() || !iterator->second.value()) {
        return temporal.unknown_time == UnknownTimePolicy::Include;
    }
    return !temporal.range || temporal.range->contains(*iterator->second.value());
}

template <typename Record>
auto matches_consistency(const QueryRecord<Record> &record,
                         const QueryConsistencyRequest &consistency) -> bool {
    switch (consistency.mode) {
    case QueryConsistency::Latest:
    case QueryConsistency::EventTime:
        return true;
    case QueryConsistency::Snapshot: {
        const auto snapshot = record.fields.find("snapshot");
        return snapshot != record.fields.end() && consistency.snapshot &&
               snapshot->second == *consistency.snapshot;
    }
    case QueryConsistency::Versioned:
        return consistency.version && record.version == *consistency.version;
    case QueryConsistency::RunSpecific:
        return consistency.run_id && record.run_id == consistency.run_id;
    }
    return false;
}

inline auto parse_cursor(const std::optional<QueryCursor> &cursor) -> Result<std::size_t> {
    if (!cursor) {
        return Result<std::size_t>::success(0);
    }
    std::size_t offset{};
    const auto token = cursor->token();
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), offset);
    if (error != std::errc{} || end != token.data() + token.size()) {
        return Result<std::size_t>::failure(
            storage_error("storage.invalid_query_cursor", ErrorCategory::InvalidArgument,
                          "in-memory query cursor is not a valid offset"));
    }
    return Result<std::size_t>::success(offset);
}

} // namespace detail

template <typename Record>
class InMemoryPersistence final : public CapabilityProvider,
                                  public PersistentAppender<Record>,
                                  public PersistentPointReader<Record>,
                                  public PersistentBatchWriter<Record> {
  public:
    static auto create(RetentionPolicy retention = RetentionPolicy::forever(),
                       DuplicateWritePolicy duplicate_policy = DuplicateWritePolicy::Reject,
                       bool atomic_batches = true) -> Result<InMemoryPersistence> {
        std::set<PersistenceCapability> capabilities{
            PersistenceCapability::Append, PersistenceCapability::PointRead,
            PersistenceCapability::BatchWrite, PersistenceCapability::Retention};
        if (atomic_batches) {
            capabilities.insert(PersistenceCapability::AtomicBatch);
        }
        auto profile = PersistenceProfile::create(
            std::move(capabilities), DurabilityGuarantee::ProcessLifetime,
            VisibilityGuarantee::Immediate,
            atomic_batches ? AtomicityGuarantee::Batch : AtomicityGuarantee::SingleRecord);
        if (!profile) {
            return Result<InMemoryPersistence>::failure(profile.error());
        }
        return Result<InMemoryPersistence>::success(InMemoryPersistence(
            std::move(profile).value(), std::move(retention), duplicate_policy, atomic_batches));
    }

    InMemoryPersistence(InMemoryPersistence &&other) noexcept
        : profile_(other.profile_), retention_(other.retention_),
          duplicate_policy_(other.duplicate_policy_), atomic_batches_(other.atomic_batches_) {
        std::lock_guard lock(other.mutex_);
        records_ = std::move(other.records_);
        insertion_order_ = std::move(other.insertion_order_);
        next_location_ = other.next_location_;
    }

    InMemoryPersistence(const InMemoryPersistence &) = delete;
    auto operator=(const InMemoryPersistence &) -> InMemoryPersistence & = delete;
    auto operator=(InMemoryPersistence &&) -> InMemoryPersistence & = delete;

    [[nodiscard]] auto profile() const noexcept -> const PersistenceProfile & override {
        return profile_;
    }

    auto append(Record record, PersistenceMetadata metadata) -> Result<WriteReceipt> override {
        std::lock_guard lock(mutex_);
        return append_locked(std::move(record), std::move(metadata));
    }

    auto read(std::string_view logical_identity) const -> Result<std::optional<Record>> override {
        std::lock_guard lock(mutex_);
        const auto iterator = records_.find(std::string(logical_identity));
        if (iterator == records_.end()) {
            return Result<std::optional<Record>>::success(std::nullopt);
        }
        return Result<std::optional<Record>>::success(iterator->second.record);
    }

    auto append_batch(std::vector<std::pair<Record, PersistenceMetadata>> records)
        -> Result<BatchWriteReceipt> override {
        std::lock_guard lock(mutex_);
        if (atomic_batches_) {
            std::set<std::string> identities;
            for (const auto &[record, metadata] : records) {
                static_cast<void>(record);
                const auto validation = validate_write_locked(metadata, identities);
                if (validation) {
                    return Result<BatchWriteReceipt>::failure(*validation);
                }
                identities.insert(metadata.logical_identity);
            }
        }

        BatchWriteReceipt receipt{BatchCompletion::Complete, {}, {}};
        for (auto &[record, metadata] : records) {
            auto result = append_locked(std::move(record), std::move(metadata));
            if (result) {
                receipt.writes.push_back(std::move(result).value());
                continue;
            }
            if (atomic_batches_) {
                return Result<BatchWriteReceipt>::failure(result.error());
            }
            receipt.completion = BatchCompletion::Partial;
            receipt.failures.push_back(result.error());
        }
        return Result<BatchWriteReceipt>::success(std::move(receipt));
    }

    [[nodiscard]] auto size() const -> std::size_t {
        std::lock_guard lock(mutex_);
        return records_.size();
    }

  private:
    struct StoredRecord {
        Record record;
        PersistenceMetadata metadata;
        StorageLocation location;
        time::TimePoint stored_at;
    };

    InMemoryPersistence(PersistenceProfile profile, RetentionPolicy retention,
                        DuplicateWritePolicy duplicate_policy, bool atomic_batches)
        : profile_(std::move(profile)), retention_(std::move(retention)),
          duplicate_policy_(duplicate_policy), atomic_batches_(atomic_batches) {}

    auto validate_write_locked(const PersistenceMetadata &metadata,
                               const std::set<std::string> &batch_identities = {}) const
        -> std::optional<Error> {
        if (metadata.logical_identity.empty()) {
            return detail::storage_error("storage.invalid_logical_identity",
                                         ErrorCategory::InvalidArgument,
                                         "logical identity cannot be empty");
        }
        const bool duplicate = records_.contains(metadata.logical_identity) ||
                               batch_identities.contains(metadata.logical_identity);
        if (duplicate && duplicate_policy_ == DuplicateWritePolicy::Reject) {
            return detail::storage_error("storage.duplicate_logical_identity",
                                         ErrorCategory::Conflict,
                                         "logical identity already exists");
        }
        return std::nullopt;
    }

    auto append_locked(Record record, PersistenceMetadata metadata) -> Result<WriteReceipt> {
        if (const auto error = validate_write_locked(metadata)) {
            return Result<WriteReceipt>::failure(*error);
        }
        const auto existing = records_.find(metadata.logical_identity);
        if (existing != records_.end()) {
            return Result<WriteReceipt>::success(WriteReceipt{
                metadata.logical_identity, WriteOutcome::IdempotentExisting, profile_.durability(),
                profile_.visibility(), existing->second.location});
        }

        auto location =
            StorageLocation::create("memory://record/" + std::to_string(next_location_++));
        if (!location) {
            return Result<WriteReceipt>::failure(location.error());
        }
        const auto logical_identity = metadata.logical_identity;
        const auto physical_location = location.value();
        records_.emplace(logical_identity, StoredRecord{std::move(record), std::move(metadata),
                                                        location.value(), time::Clock::now()});
        insertion_order_.push_back(logical_identity);
        apply_retention_locked();
        return Result<WriteReceipt>::success(
            WriteReceipt{logical_identity, WriteOutcome::Stored, profile_.durability(),
                         profile_.visibility(), physical_location});
    }

    void apply_retention_locked() {
        if (retention_.kind() == RetentionKind::CountBounded) {
            while (records_.size() > *retention_.maximum_records()) {
                records_.erase(insertion_order_.front());
                insertion_order_.pop_front();
            }
        } else if (retention_.kind() == RetentionKind::TimeBounded) {
            const auto cutoff = time::Clock::now() - *retention_.duration();
            while (!insertion_order_.empty()) {
                const auto iterator = records_.find(insertion_order_.front());
                if (iterator != records_.end() && iterator->second.stored_at >= cutoff) {
                    break;
                }
                if (iterator != records_.end()) {
                    records_.erase(iterator);
                }
                insertion_order_.pop_front();
            }
        }
    }

    PersistenceProfile profile_;
    RetentionPolicy retention_;
    DuplicateWritePolicy duplicate_policy_;
    bool atomic_batches_;
    mutable std::mutex mutex_;
    std::map<std::string, StoredRecord> records_;
    std::deque<std::string> insertion_order_;
    std::size_t next_location_{1};
};

template <typename Record>
class InMemoryQueryProvider final : public QueryProvider<Record>,
                                    public StreamingQueryProvider<Record> {
  public:
    explicit InMemoryQueryProvider(std::vector<QueryRecord<Record>> records,
                                   std::string dataset_version = "in-memory")
        : records_(std::move(records)), dataset_version_(std::move(dataset_version)) {}

    auto execute(const QueryDefinition &query, const QueryExecutionContext &context) const
        -> Result<QueryPage<Record>> override {
        if (context.cancellation.is_cancelled()) {
            return failure<QueryPage<Record>>("storage.query_cancelled", ErrorCategory::Cancelled,
                                              "query was cancelled");
        }
        if (context.deadline && time::MonotonicClock::now() >= *context.deadline) {
            return failure<QueryPage<Record>>("storage.query_deadline_exceeded",
                                              ErrorCategory::DeadlineExceeded,
                                              "query deadline has been exceeded");
        }
        const auto &data = query.data();
        if (data.required_permission &&
            !context.authorization.permissions.contains(*data.required_permission)) {
            return failure<QueryPage<Record>>("storage.query_unauthorized",
                                              ErrorCategory::Unauthorized,
                                              "query permission is not granted");
        }
        if (context.authorization.tenant &&
            (!data.scope.tenant || data.scope.tenant != context.authorization.tenant)) {
            return failure<QueryPage<Record>>("storage.query_tenant_mismatch",
                                              ErrorCategory::Unauthorized,
                                              "query tenant is outside the authorization scope");
        }
        if (data.consistency.mode == QueryConsistency::EventTime &&
            data.temporal.dimension != QueryTemporalDimension::EventTime) {
            return failure<QueryPage<Record>>(
                "storage.invalid_event_time_query", ErrorCategory::InvalidArgument,
                "event-time consistency requires event-time semantics");
        }

        const auto cursor = detail::parse_cursor(data.page.cursor);
        if (!cursor) {
            return Result<QueryPage<Record>>::failure(cursor.error());
        }

        std::vector<QueryRecord<Record>> matches;
        std::size_t scanned{};
        bool partial{};
        std::string dataset_version;
        {
            std::lock_guard lock(mutex_);
            dataset_version = dataset_version_;
            for (const auto &record : records_) {
                if (context.cancellation.is_cancelled()) {
                    return failure<QueryPage<Record>>(
                        "storage.query_cancelled", ErrorCategory::Cancelled, "query was cancelled");
                }
                if (context.deadline && time::MonotonicClock::now() >= *context.deadline) {
                    return failure<QueryPage<Record>>("storage.query_deadline_exceeded",
                                                      ErrorCategory::DeadlineExceeded,
                                                      "query deadline has been exceeded");
                }
                if (++scanned > data.limits.maximum_scan_items) {
                    if (!data.limits.partial_results_allowed) {
                        return exhausted<QueryPage<Record>>("query scan limit exceeded");
                    }
                    partial = true;
                    break;
                }
                if (detail::scope_matches(record.scope, data.scope) &&
                    detail::matches_consistency(record, data.consistency) &&
                    detail::matches_temporal(record, data.temporal) &&
                    detail::matches_filters(record, data.filters)) {
                    matches.push_back(record);
                }
            }
        }

        const auto order = data.ordering;
        std::sort(matches.begin(), matches.end(), [&](const auto &left, const auto &right) {
            const auto left_value = detail::field_value(left, order.field).value_or("");
            const auto right_value = detail::field_value(right, order.field).value_or("");
            if (left_value == right_value) {
                return order.direction == QueryOrderDirection::Ascending
                           ? left.logical_identity < right.logical_identity
                           : left.logical_identity > right.logical_identity;
            }
            return order.direction == QueryOrderDirection::Ascending ? left_value < right_value
                                                                     : left_value > right_value;
        });

        if (cursor.value() > matches.size()) {
            return failure<QueryPage<Record>>("storage.query_cursor_out_of_range",
                                              ErrorCategory::InvalidArgument,
                                              "query cursor exceeds the result set");
        }
        QueryPage<Record> page{
            {},
            std::nullopt,
            QueryResultMetadata{data.consistency.mode,
                                partial ? QueryCompleteness::Partial : QueryCompleteness::Complete,
                                data.consistency.version.value_or(dataset_version), std::nullopt,
                                std::nullopt}};
        std::size_t bytes{};
        for (std::size_t index = cursor.value(); index < matches.size(); ++index) {
            if (page.records.size() == data.page.limit) {
                page.next_cursor = QueryCursor::create(std::to_string(index)).value();
                break;
            }
            const auto next_bytes = bytes + matches[index].estimated_bytes;
            if (page.records.size() >= data.limits.maximum_result_items ||
                next_bytes > data.limits.maximum_result_bytes) {
                if (!data.limits.partial_results_allowed) {
                    return exhausted<QueryPage<Record>>("query result limit exceeded");
                }
                page.metadata.completeness = QueryCompleteness::Partial;
                break;
            }
            bytes = next_bytes;
            page.records.push_back(matches[index]);
        }
        return Result<QueryPage<Record>>::success(std::move(page));
    }

    auto stream(const QueryDefinition &query, QueryExecutionContext context) const
        -> Result<std::unique_ptr<QueryStream<Record>>> override {
        auto page = execute(query, context);
        if (!page) {
            return Result<std::unique_ptr<QueryStream<Record>>>::failure(page.error());
        }
        return Result<std::unique_ptr<QueryStream<Record>>>::success(
            std::make_unique<VectorQueryStream>(std::move(page).value().records,
                                                std::move(context)));
    }

    void replace(std::vector<QueryRecord<Record>> records, std::string dataset_version) {
        std::lock_guard lock(mutex_);
        records_ = std::move(records);
        dataset_version_ = std::move(dataset_version);
    }

  private:
    class VectorQueryStream final : public QueryStream<Record> {
      public:
        VectorQueryStream(std::vector<QueryRecord<Record>> records, QueryExecutionContext context)
            : records_(std::move(records)), context_(std::move(context)) {}

        auto next() -> Result<std::optional<QueryRecord<Record>>> override {
            if (context_.cancellation.is_cancelled()) {
                return failure<std::optional<QueryRecord<Record>>>("storage.query_cancelled",
                                                                   ErrorCategory::Cancelled,
                                                                   "query stream was cancelled");
            }
            if (context_.deadline && time::MonotonicClock::now() >= *context_.deadline) {
                return failure<std::optional<QueryRecord<Record>>>(
                    "storage.query_deadline_exceeded", ErrorCategory::DeadlineExceeded,
                    "query stream deadline has been exceeded");
            }
            if (position_ == records_.size()) {
                return Result<std::optional<QueryRecord<Record>>>::success(std::nullopt);
            }
            return Result<std::optional<QueryRecord<Record>>>::success(records_[position_++]);
        }

      private:
        std::vector<QueryRecord<Record>> records_;
        QueryExecutionContext context_;
        std::size_t position_{};
    };

    template <typename Value>
    static auto failure(std::string code, ErrorCategory category, std::string message)
        -> Result<Value> {
        return Result<Value>::failure(
            detail::storage_error(std::move(code), category, std::move(message)));
    }

    template <typename Value> static auto exhausted(std::string message) -> Result<Value> {
        return failure<Value>("storage.query_resource_exhausted", ErrorCategory::ResourceExhausted,
                              std::move(message));
    }

    mutable std::mutex mutex_;
    std::vector<QueryRecord<Record>> records_;
    std::string dataset_version_;
};

template <typename Record> class InMemoryQueryCache final : public QueryCache<Record> {
  public:
    auto get(const QueryCacheKey &key) const -> Result<std::optional<QueryPage<Record>>> override {
        std::lock_guard lock(mutex_);
        const auto iterator = pages_.find(cache_key(key));
        return Result<std::optional<QueryPage<Record>>>::success(
            iterator == pages_.end() ? std::nullopt
                                     : std::optional<QueryPage<Record>>(iterator->second));
    }

    auto put(QueryCacheKey key, QueryPage<Record> page) -> Result<void> override {
        if (key.read_model_identity.empty() || key.query_fingerprint.empty() ||
            key.authorization_fingerprint.empty()) {
            return Result<void>::failure(detail::storage_error(
                "storage.invalid_cache_key", ErrorCategory::InvalidArgument,
                "cache keys require read-model, query, and authorization identities"));
        }
        std::lock_guard lock(mutex_);
        pages_.insert_or_assign(cache_key(key), std::move(page));
        return Result<void>::success();
    }

    void invalidate_read_model(const ReadModelId &read_model_id) override {
        const auto prefix = read_model_id.to_string() + "\n";
        std::lock_guard lock(mutex_);
        std::erase_if(pages_, [&](const auto &entry) { return entry.first.starts_with(prefix); });
    }

  private:
    static auto cache_key(const QueryCacheKey &key) -> std::string {
        return key.read_model_identity + "\n" + key.query_fingerprint + "\n" +
               key.authorization_fingerprint;
    }

    mutable std::mutex mutex_;
    std::map<std::string, QueryPage<Record>> pages_;
};

template <typename Record> class InMemoryReadModel final : public QueryProvider<Record> {
  public:
    InMemoryReadModel(ReadModelDescriptor descriptor, std::vector<QueryRecord<Record>> records)
        : descriptor_(std::move(descriptor)),
          provider_(std::move(records), descriptor_.data().version) {}

    [[nodiscard]] auto descriptor() const noexcept -> const ReadModelDescriptor & {
        return descriptor_;
    }

    auto execute(const QueryDefinition &query, const QueryExecutionContext &context) const
        -> Result<QueryPage<Record>> override {
        return provider_.execute(query, context);
    }

    auto rebuild(ReadModelRebuilder<Record> &rebuilder, const ReadModelRebuildRequest &request,
                 const QueryExecutionContext &context) -> Result<void> {
        if (!descriptor_.data().rebuildable) {
            return Result<void>::failure(detail::storage_error(
                "storage.read_model_not_rebuildable", ErrorCategory::InvalidArgument,
                "read model does not advertise rebuild capability"));
        }
        auto rebuilt = rebuilder.rebuild(request, context);
        if (!rebuilt) {
            return Result<void>::failure(rebuilt.error());
        }
        provider_.replace(std::move(rebuilt).value(), descriptor_.data().version);
        return Result<void>::success();
    }

  private:
    ReadModelDescriptor descriptor_;
    InMemoryQueryProvider<Record> provider_;
};

} // namespace evolution::storage
