#include "evolution/storage/in_memory.hpp"

#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;
using evolution::storage::QueryTemporalDimension;

class TestRunner {
  public:
    void expect(bool condition, std::string message) {
        if (!condition) {
            ++failures_;
            std::cerr << "FAIL: " << message << '\n';
        }
    }

    [[nodiscard]] auto failures() const noexcept -> int {
        return failures_;
    }

  private:
    int failures_{};
};

template <typename Value> auto require_value(evolution::Result<Value> result) -> Value {
    if (!result) {
        throw std::runtime_error(std::string(result.error().message()));
    }
    return std::move(result).value();
}

auto metadata(std::string identity) -> evolution::storage::PersistenceMetadata {
    return {std::move(identity),
            {"test", "1", "native", true},
            evolution::storage::RetentionPolicy::forever(),
            {},
            std::nullopt};
}

auto query_record(std::string identity, std::string tenant, std::string rank,
                  std::string version = "v1", std::optional<evolution::RunId> run_id = std::nullopt,
                  std::optional<evolution::time::TemporalInformation> event_time = std::nullopt,
                  std::size_t bytes = 1) -> evolution::storage::QueryRecord<int> {
    std::map<QueryTemporalDimension, evolution::time::TemporalInformation> times;
    if (event_time) {
        times.emplace(QueryTemporalDimension::EventTime, *event_time);
    }
    return {std::move(identity),
            1,
            {"records", std::move(tenant), std::string("resource/")},
            {{"rank", std::move(rank)}, {"snapshot", "snapshot-a"}},
            std::move(times),
            std::move(version),
            std::move(run_id),
            std::nullopt,
            bytes};
}

auto make_query(evolution::storage::QueryConsistencyRequest consistency = {},
                evolution::storage::QueryTemporalRequest temporal = {}, std::size_t page_limit = 10,
                std::size_t scan_limit = 100, std::size_t result_limit = 100,
                std::size_t byte_limit = 100, bool partial = false,
                std::optional<std::string> permission = "records.read",
                std::optional<evolution::storage::QueryCursor> cursor = std::nullopt)
    -> evolution::storage::QueryDefinition {
    return require_value(evolution::storage::QueryDefinition::create(
        {require_value(evolution::storage::QueryId::from_stable_name("storage-query")),
         {"records", std::string("tenant-a"), std::string("resource/")},
         {},
         temporal,
         {"rank", evolution::storage::QueryOrderDirection::Ascending, "logical_identity"},
         std::move(consistency),
         {page_limit, std::move(cursor)},
         {scan_limit, result_limit, byte_limit, partial},
         std::move(permission)}));
}

auto authorized_context() -> evolution::storage::QueryExecutionContext {
    return {{"principal-a", std::string("tenant-a"), {"records.read"}}, {}, std::nullopt};
}

void test_persistence(TestRunner &test) {
    using evolution::storage::DuplicateWritePolicy;
    using evolution::storage::InMemoryPersistence;
    using evolution::storage::PersistenceCapability;

    auto store = require_value(InMemoryPersistence<int>::create());
    test.expect(store.profile().supports(PersistenceCapability::AtomicBatch),
                "atomic in-memory persistence advertises atomic batches");
    test.expect(!store.profile().supports(PersistenceCapability::Transactions),
                "in-memory persistence does not advertise unsupported transactions");
    test.expect(store.profile().durability() ==
                    evolution::storage::DurabilityGuarantee::ProcessLifetime,
                "in-memory persistence does not claim durable storage");

    const auto missing = store.read("missing");
    test.expect(missing && !missing.value(), "point-read absence is a successful empty result");

    const auto first = store.append(7, metadata("logical-a"));
    test.expect(first && first.value().physical_location &&
                    first.value().physical_location->value() != first.value().logical_identity,
                "logical identity remains independent from physical location");
    const auto duplicate = store.append(8, metadata("logical-a"));
    test.expect(!duplicate && duplicate.error().category() == evolution::ErrorCategory::Conflict,
                "reject policy reports duplicate logical identities");

    auto idempotent = require_value(InMemoryPersistence<int>::create(
        evolution::storage::RetentionPolicy::forever(), DuplicateWritePolicy::Idempotent));
    require_value(idempotent.append(1, metadata("same")));
    const auto repeated = idempotent.append(2, metadata("same"));
    test.expect(repeated &&
                    repeated.value().outcome ==
                        evolution::storage::WriteOutcome::IdempotentExisting &&
                    require_value(idempotent.read("same")) == std::optional<int>{1},
                "idempotent writes preserve the first stored value");

    std::vector<std::pair<int, evolution::storage::PersistenceMetadata>> atomic_batch;
    atomic_batch.emplace_back(2, metadata("new"));
    atomic_batch.emplace_back(3, metadata("same"));
    const auto rolled_back = idempotent.append_batch(std::move(atomic_batch));
    test.expect(rolled_back && idempotent.size() == 2,
                "idempotent atomic batch completes without partial failure");

    std::vector<std::pair<int, evolution::storage::PersistenceMetadata>> rejected_batch;
    rejected_batch.emplace_back(2, metadata("batch-new"));
    rejected_batch.emplace_back(3, metadata("logical-a"));
    const auto rejected = store.append_batch(std::move(rejected_batch));
    test.expect(!rejected && !require_value(store.read("batch-new")),
                "atomic batch validation prevents partial commit");

    auto non_atomic = require_value(InMemoryPersistence<int>::create(
        evolution::storage::RetentionPolicy::forever(), DuplicateWritePolicy::Reject, false));
    require_value(non_atomic.append(1, metadata("existing")));
    std::vector<std::pair<int, evolution::storage::PersistenceMetadata>> partial_batch;
    partial_batch.emplace_back(2, metadata("accepted"));
    partial_batch.emplace_back(3, metadata("existing"));
    const auto partial = non_atomic.append_batch(std::move(partial_batch));
    test.expect(partial &&
                    partial.value().completion == evolution::storage::BatchCompletion::Partial &&
                    partial.value().failures.size() == 1 && non_atomic.size() == 2,
                "non-atomic batch reports explicit partial completion");

    auto retained = require_value(InMemoryPersistence<int>::create(
        require_value(evolution::storage::RetentionPolicy::count_bounded(2))));
    require_value(retained.append(1, metadata("one")));
    require_value(retained.append(2, metadata("two")));
    require_value(retained.append(3, metadata("three")));
    test.expect(!require_value(retained.read("one")) && retained.size() == 2,
                "count-bounded retention removes the oldest record");
}

void test_queries(TestRunner &test) {
    const auto run = require_value(evolution::RunId::from_stable_name("query-run"));
    const auto known_time = evolution::time::TemporalInformation::known(
        evolution::time::TimePoint{10s}, evolution::time::Precision::Second);
    evolution::storage::InMemoryQueryProvider<int> provider(
        {query_record("b", "tenant-a", "1", "v1", run, known_time, 3),
         query_record("a", "tenant-a", "1", "v1", run, known_time, 3),
         query_record("c", "tenant-a", "2", "v2", std::nullopt, std::nullopt, 3),
         query_record("foreign", "tenant-b", "0")},
        "latest-v2");

    auto context = authorized_context();
    const auto ordered = require_value(provider.execute(make_query(), context));
    test.expect(ordered.records.size() == 3 && ordered.records[0].logical_identity == "a" &&
                    ordered.records[1].logical_identity == "b",
                "queries enforce scope and deterministic identity tie-breaking");
    test.expect(ordered.metadata.dataset_version == "latest-v2",
                "latest queries report the provider dataset version");
    const auto base_fingerprint = make_query().semantic_fingerprint();
    auto alternate_range = require_value(evolution::time::TimeRange::create(
        evolution::time::TimePoint{0s}, evolution::time::TimePoint{20s}));
    const auto temporal_fingerprint =
        make_query({}, {QueryTemporalDimension::EventTime, alternate_range,
                        evolution::storage::UnknownTimePolicy::Include})
            .semantic_fingerprint();
    test.expect(base_fingerprint != temporal_fingerprint,
                "query fingerprints include temporal and unknown-time semantics");

    auto first_page = require_value(provider.execute(make_query({}, {}, 1), context));
    auto second_page = require_value(
        provider.execute(make_query({}, {}, 2, 100, 100, 100, false, std::string("records.read"),
                                    first_page.next_cursor),
                         context));
    test.expect(first_page.next_cursor && first_page.records.size() == 1 &&
                    second_page.records.front().logical_identity == "b",
                "opaque cursors continue deterministic pagination");

    evolution::storage::QueryConsistencyRequest versioned{
        evolution::storage::QueryConsistency::Versioned, std::nullopt, std::string("v2"),
        std::nullopt};
    const auto version_page = require_value(provider.execute(make_query(versioned), context));
    test.expect(version_page.records.size() == 1 &&
                    version_page.records.front().logical_identity == "c",
                "versioned consistency selects the requested version");

    evolution::storage::QueryConsistencyRequest run_specific{
        evolution::storage::QueryConsistency::RunSpecific, std::nullopt, std::nullopt, run};
    test.expect(require_value(provider.execute(make_query(run_specific), context)).records.size() ==
                    2,
                "run-specific consistency isolates a run");

    auto range = require_value(evolution::time::TimeRange::create(evolution::time::TimePoint{5s},
                                                                  evolution::time::TimePoint{15s}));
    evolution::storage::QueryTemporalRequest exclude_unknown{
        QueryTemporalDimension::EventTime, range, evolution::storage::UnknownTimePolicy::Exclude};
    test.expect(
        require_value(provider.execute(make_query({}, exclude_unknown), context)).records.size() ==
            2,
        "temporal queries exclude unknown times when requested");
    exclude_unknown.unknown_time = evolution::storage::UnknownTimePolicy::Include;
    test.expect(
        require_value(provider.execute(make_query({}, exclude_unknown), context)).records.size() ==
            3,
        "temporal queries can explicitly include unknown times");

    auto unauthorized = context;
    unauthorized.authorization.permissions.clear();
    const auto denied = provider.execute(make_query(), unauthorized);
    test.expect(!denied && denied.error().category() == evolution::ErrorCategory::Unauthorized,
                "missing query permission is rejected");
    auto wrong_tenant = context;
    wrong_tenant.authorization.tenant = "tenant-b";
    const auto tenant_denied = provider.execute(make_query(), wrong_tenant);
    test.expect(!tenant_denied &&
                    tenant_denied.error().category() == evolution::ErrorCategory::Unauthorized,
                "cross-tenant query is rejected before reading records");

    evolution::context::CancellationSource cancellation;
    cancellation.cancel();
    auto cancelled = context;
    cancelled.cancellation = cancellation.token();
    const auto cancelled_result = provider.execute(make_query(), cancelled);
    test.expect(!cancelled_result &&
                    cancelled_result.error().category() == evolution::ErrorCategory::Cancelled,
                "query cancellation is explicit");
    auto expired = context;
    expired.deadline = evolution::time::MonotonicClock::now() - 1ms;
    const auto deadline_result = provider.execute(make_query(), expired);
    test.expect(!deadline_result && deadline_result.error().category() ==
                                        evolution::ErrorCategory::DeadlineExceeded,
                "query deadline expiration is explicit");

    const auto scan_exhausted = provider.execute(make_query({}, {}, 10, 1), context);
    test.expect(!scan_exhausted && scan_exhausted.error().category() ==
                                       evolution::ErrorCategory::ResourceExhausted,
                "scan exhaustion never silently truncates a complete result");
    const auto partial =
        require_value(provider.execute(make_query({}, {}, 10, 1, 100, 100, true), context));
    test.expect(partial.metadata.completeness == evolution::storage::QueryCompleteness::Partial,
                "allowed scan exhaustion is marked partial");
    const auto bytes_exhausted = provider.execute(make_query({}, {}, 10, 100, 100, 2), context);
    test.expect(!bytes_exhausted && bytes_exhausted.error().category() ==
                                        evolution::ErrorCategory::ResourceExhausted,
                "byte exhaustion is reported as resource exhaustion");

    auto stream = require_value(provider.stream(make_query(), context));
    test.expect(require_value(stream->next())->logical_identity == "a",
                "streaming query preserves deterministic query order");
}

class SuccessfulRebuilder final : public evolution::storage::ReadModelRebuilder<int> {
  public:
    auto rebuild(const evolution::storage::ReadModelRebuildRequest &,
                 const evolution::storage::QueryExecutionContext &)
        -> evolution::Result<std::vector<evolution::storage::QueryRecord<int>>> override {
        return evolution::Result<std::vector<evolution::storage::QueryRecord<int>>>::success(
            {query_record("rebuilt", "tenant-a", "1")});
    }
};

class FailedRebuilder final : public evolution::storage::ReadModelRebuilder<int> {
  public:
    auto rebuild(const evolution::storage::ReadModelRebuildRequest &,
                 const evolution::storage::QueryExecutionContext &)
        -> evolution::Result<std::vector<evolution::storage::QueryRecord<int>>> override {
        return evolution::Result<std::vector<evolution::storage::QueryRecord<int>>>::failure(
            evolution::Error(evolution::ErrorCode::create("test.rebuild_failed"),
                             evolution::ErrorCategory::Persistence, "rebuild failed"));
    }
};

void test_read_models(TestRunner &test) {
    const auto read_model_id =
        require_value(evolution::storage::ReadModelId::from_stable_name("read-model"));
    const auto configuration_id =
        require_value(evolution::ConfigurationId::from_stable_name("configuration"));
    const auto provenance_id =
        require_value(evolution::ProvenanceId::from_stable_name("provenance"));
    const auto component_id = require_value(evolution::ComponentId::from_stable_name("projection"));
    const auto descriptor = require_value(evolution::storage::ReadModelDescriptor::create(
        {read_model_id,
         "test-model",
         "v1",
         evolution::storage::ReadModelAuthority::Derived,
         component_id,
         std::string("projection-v1"),
         configuration_id,
         provenance_id,
         {"events"},
         {evolution::storage::FreshnessStatus::Current,
          evolution::time::TemporalInformation::known(evolution::time::TimePoint{10s},
                                                      evolution::time::Precision::Second),
          std::string("position-10"), 0s},
         true,
         false}));
    test.expect(descriptor.data().authority == evolution::storage::ReadModelAuthority::Derived &&
                    descriptor.data().provenance_id == provenance_id,
                "derived read model retains explicit provenance and freshness");

    evolution::storage::InMemoryReadModel<int> model(descriptor,
                                                     {query_record("original", "tenant-a", "1")});
    auto context = authorized_context();
    FailedRebuilder failed;
    evolution::storage::ReadModelRebuildRequest request{
        {"events-v1"}, "projection-v1", configuration_id, std::nullopt};
    test.expect(
        !model.rebuild(failed, request, context) &&
            require_value(model.execute(make_query(), context)).records.front().logical_identity ==
                "original",
        "failed rebuild leaves the previous model atomically intact");
    SuccessfulRebuilder successful;
    test.expect(
        model.rebuild(successful, request, context) &&
            require_value(model.execute(make_query(), context)).records.front().logical_identity ==
                "rebuilt",
        "successful rebuild atomically replaces read-model records");

    evolution::storage::InMemoryQueryCache<int> cache;
    const auto page = require_value(model.execute(make_query(), context));
    const evolution::storage::QueryCacheKey principal_a{read_model_id.to_string(), "query",
                                                        "principal-a"};
    const evolution::storage::QueryCacheKey principal_b{read_model_id.to_string(), "query",
                                                        "principal-b"};
    require_value(cache.put(principal_a, page));
    test.expect(require_value(cache.get(principal_a)).has_value() &&
                    !require_value(cache.get(principal_b)).has_value(),
                "cache entries are isolated by authorization fingerprint");
    cache.invalidate_read_model(read_model_id);
    test.expect(!require_value(cache.get(principal_a)).has_value(),
                "read-model invalidation removes all associated cache entries");
}

} // namespace

int main() {
    TestRunner test;
    try {
        test_persistence(test);
        test_queries(test);
        test_read_models(test);
    } catch (const std::exception &exception) {
        std::cerr << "UNEXPECTED: " << exception.what() << '\n';
        return 1;
    }
    return test.failures() == 0 ? 0 : 1;
}
