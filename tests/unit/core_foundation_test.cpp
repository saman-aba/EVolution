#include "evolution/core/configuration/configuration.hpp"
#include "evolution/core/context/context.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/provenance/provenance.hpp"
#include "evolution/core/time/time.hpp"

#include <chrono>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

static_assert(std::copy_constructible<evolution::Error>);
static_assert(std::move_constructible<evolution::Error>);
static_assert(!std::default_initializable<evolution::EventId>);
static_assert(!std::same_as<evolution::EventId, evolution::MetricId>);
static_assert(std::copy_constructible<evolution::Configuration>);
static_assert(std::copy_constructible<evolution::Context>);
static_assert(std::copy_constructible<evolution::Provenance>);
static_assert(!std::copy_constructible<evolution::Result<std::unique_ptr<int>>>);
static_assert(std::move_constructible<evolution::Result<std::unique_ptr<int>>>);

class FixedTimeSource final : public evolution::time::TimeSource {
  public:
    explicit FixedTimeSource(evolution::time::TimePoint value) : value_(value) {}

    [[nodiscard]] auto now() const -> evolution::time::TimePoint override {
        return value_;
    }

    [[nodiscard]] auto source_id() const noexcept -> std::string_view override {
        return "fixed";
    }

  private:
    evolution::time::TimePoint value_;
};

class TestRunner {
  public:
    void expect(bool condition, std::string message) {
        if (!condition) {
            ++failures_;
            std::cerr << "FAIL: " << message << '\n';
        }
    }

    template <typename Function> void expect_throws(Function &&function, std::string message) {
        try {
            std::forward<Function>(function)();
            expect(false, std::move(message));
        } catch (const std::exception &) {
        }
    }

    [[nodiscard]] auto failures() const noexcept -> int {
        return failures_;
    }

  private:
    int failures_{};
};

template <typename T> auto require_value(evolution::Result<T> result) -> T {
    if (result.has_error()) {
        throw std::runtime_error(std::string(result.error().message()));
    }
    return std::move(result).value();
}

void test_error_and_result(TestRunner &test) {
    std::string external_message = "invalid input";
    const evolution::Error original(evolution::ErrorCode::create("core.invalid_input"),
                                    evolution::ErrorCategory::InvalidArgument, external_message);
    external_message.assign("changed");

    const auto enriched = original.with_context("field", "count");
    const auto translated =
        enriched.translated(evolution::ErrorCode::create("api.request_rejected"),
                            evolution::ErrorCategory::InvalidArgument, "request rejected");

    test.expect(original.message() == "invalid input", "Error owns its message");
    test.expect(original.context().empty(), "Error enrichment does not mutate the source");
    test.expect(enriched.context().size() == 1, "Error enrichment adds owned context");
    test.expect(translated.cause() != nullptr, "Error translation preserves the cause");
    test.expect(translated.cause()->code().value() == "core.invalid_input",
                "Error translation preserves stable cause identity");

    auto success = evolution::Result<int>::success(42);
    auto failure = evolution::Result<int>::failure(original);
    test.expect(success.has_value() && success.value() == 42, "Result stores successful values");
    test.expect(failure.has_error(), "Result stores errors");
    test.expect_throws([&failure] { static_cast<void>(failure.value()); },
                       "Error value access fails");
    test.expect_throws([&success] { static_cast<void>(success.error()); },
                       "Success error access fails");

    auto move_only = evolution::Result<std::unique_ptr<int>>::success(std::make_unique<int>(7));
    auto pointer = std::move(move_only).value();
    test.expect(pointer && *pointer == 7, "Result supports move-only owned values");

    auto void_failure = evolution::Result<void>::failure(original);
    const auto contextual = void_failure.with_error_context("operation", "save");
    test.expect(contextual.error().context().size() == 1, "Result propagation enriches immutably");

    test.expect_throws([] { static_cast<void>(evolution::ErrorCode::create("Invalid Code")); },
                       "Error codes reject unstable formatting");
}

void test_identity(TestRunner &test) {
    const auto event_id = require_value(evolution::EventId::generate());
    const auto parsed = require_value(evolution::EventId::parse(event_id.to_string()));
    const auto stable_a = require_value(evolution::EventId::from_stable_name("event/source/42"));
    const auto stable_b = require_value(evolution::EventId::from_stable_name("event/source/42"));

    test.expect(parsed == event_id, "Identifier formatting and parsing round-trip");
    test.expect(stable_a == stable_b, "Stable identity generation is deterministic");
    test.expect(std::hash<evolution::EventId>{}(stable_a) ==
                    std::hash<evolution::EventId>{}(stable_b),
                "Equal identifiers have equal hashes");
    test.expect(evolution::EventId::parse("not-an-identifier").has_error(),
                "Identifier parsing rejects malformed input");

    evolution::EventId::Bytes zero{};
    test.expect(evolution::EventId::from_bytes(zero).has_error(),
                "The all-zero identifier is explicitly invalid");
}

void test_time(TestRunner &test) {
    const evolution::time::TimePoint timestamp{123s};
    const auto known =
        evolution::time::TemporalInformation::known(timestamp, evolution::time::Precision::Second);
    const auto estimated = require_value(evolution::time::TemporalInformation::estimated(
        timestamp, evolution::time::Precision::Second, 2s, "source interpolation"));
    const auto unknown = evolution::time::TemporalInformation::unknown();

    test.expect(known.value() == timestamp, "Known temporal information retains its timestamp");
    test.expect(estimated.status() == evolution::time::TemporalStatus::Estimated &&
                    estimated.uncertainty() == 2s,
                "Estimated time retains estimation semantics");
    test.expect(!unknown.value(), "Unknown time does not silently acquire current time");
    test.expect(evolution::time::TemporalInformation::estimated(
                    timestamp, evolution::time::Precision::Second, -1s, "invalid")
                    .has_error(),
                "Negative temporal uncertainty is rejected");

    const auto range =
        require_value(evolution::time::TimeRange::create(timestamp, timestamp + 10s));
    test.expect(range.contains(timestamp), "Time ranges include their start");
    test.expect(!range.contains(timestamp + 10s), "Time ranges exclude their end");
    test.expect(evolution::time::TimeRange::create(timestamp + 1s, timestamp).has_error(),
                "Invalid time ranges are rejected");
}

auto make_configuration() -> evolution::Configuration {
    auto secret = require_value(evolution::SecretReference::create("vault", "database/password"));
    evolution::Configuration::Entries entries{
        {"processor.enabled", true},
        {"processor.limit", std::int64_t{32}},
        {"processor.name", std::string("baseline")},
        {"storage.password", std::move(secret)},
    };
    return require_value(evolution::Configuration::create(std::move(entries), "v1"));
}

void test_configuration(TestRunner &test) {
    const auto configuration = make_configuration();
    const auto equivalent = make_configuration();

    test.expect(configuration.id() == equivalent.id(),
                "Effective configuration identity is stable");
    test.expect(require_value(configuration.get<std::int64_t>("processor.limit")) == 32,
                "Configuration provides typed lookup");
    test.expect(configuration.get<std::string>("processor.limit").has_error(),
                "Configuration rejects type confusion");
    test.expect(configuration.get<std::string>("storage.password").has_error(),
                "Secret references cannot be read as ordinary strings");

    const auto child = require_value(configuration.child("processor"));
    test.expect(child.contains("limit") && !child.contains("processor.limit"),
                "Child projection is hierarchical");

    evolution::Configuration::Entries override_entries{{"processor.limit", std::int64_t{64}}};
    const auto override =
        require_value(evolution::Configuration::create(std::move(override_entries), "v2"));
    const auto merged = configuration.overlay(override);
    test.expect(require_value(merged.get<std::int64_t>("processor.limit")) == 64,
                "Higher-precedence configuration overrides lower-precedence values");
    test.expect(merged.version() == "v2", "Overlay adopts the effective version");
    test.expect(evolution::Configuration::create({{"bad..path", true}}).has_error(),
                "Invalid hierarchical paths are rejected");
}

void test_execution_context(TestRunner &test) {
    const auto run_id = require_value(evolution::RunId::from_stable_name("run/1"));
    const auto trace_id = require_value(evolution::TraceId::from_stable_name("trace/1"));
    const evolution::time::TimePoint timestamp{321s};
    auto clock = std::make_shared<FixedTimeSource>(timestamp);
    auto random = std::make_shared<evolution::DeterministicRandomSource>(1234);
    evolution::CancellationSource cancellation_source;
    evolution::ResourceView resources({{evolution::ResourceKind::ExecutionSlots, 4}});
    const auto deadline = evolution::time::MonotonicClock::now() + 10s;

    const auto context = evolution::ExecutionContext::create(
        evolution::ExecutionMode::Replay, run_id, trace_id, clock, random,
        cancellation_source.token(), deadline, resources);

    test.expect(require_value(context.current_time()) == timestamp, "Execution time is explicit");
    test.expect(context.trace_id() == trace_id, "Execution trace identity is explicit");
    test.expect(context.resources()->available(evolution::ResourceKind::ExecutionSlots) == 4,
                "Resource availability is explicit");
    test.expect(!context.is_cancelled(), "Cancellation is initially inactive");
    cancellation_source.cancel();
    test.expect(context.is_cancelled(), "Cancellation propagates through an owned token");

    const auto projected = context.project(evolution::ExecutionCapability::Cancellation |
                                           evolution::ExecutionCapability::Deadline);
    test.expect(projected.current_time().has_error(), "Projection removes unrequested time access");
    test.expect(!projected.trace_id(), "Projection removes unrequested trace identity");
    test.expect(projected.is_cancelled(), "Projection preserves requested cancellation");

    auto first_random = std::make_shared<evolution::DeterministicRandomSource>(9);
    auto second_random = std::make_shared<evolution::DeterministicRandomSource>(9);
    const auto first = evolution::ExecutionContext::create(evolution::ExecutionMode::Test, run_id,
                                                           std::nullopt, nullptr, first_random);
    const auto second = evolution::ExecutionContext::create(evolution::ExecutionMode::Test, run_id,
                                                            std::nullopt, nullptr, second_random);
    test.expect(require_value(first.next_random_u64()) == require_value(second.next_random_u64()),
                "Injected randomness is reproducible");
}

auto make_context() -> evolution::Context {
    evolution::Context::Dimensions dimensions{
        {"table.stakes", std::string("high")},
        {"table.seats", std::int64_t{6}},
    };
    return require_value(evolution::Context::create(
        evolution::ContextScope::Session, {{"table", "table-42"}}, std::move(dimensions)));
}

void test_context(TestRunner &test) {
    const auto context = make_context();
    const auto equivalent = make_context();
    const std::vector<std::string_view> projection_names{"table.stakes"};
    const auto projected = require_value(context.project(projection_names));

    test.expect(context.id() == equivalent.id(), "Equivalent Context values have stable identity");
    test.expect(require_value(context.dimension<std::int64_t>("table.seats")) == 6,
                "Context dimensions are typed");
    test.expect(projected.dimensions().size() == 1 && projected.parents().front() == context.id(),
                "Context projection records its parent and selected dimensions");
    test.expect(context.project(std::vector<std::string_view>{"missing"}).has_error(),
                "Context projection rejects unavailable dimensions");
    test.expect(evolution::Context::create(evolution::ContextScope::Object, {{"", "identity"}}, {})
                    .has_error(),
                "Invalid Context entities are rejected");
}

auto make_provenance() -> evolution::Provenance {
    const auto source_id = require_value(evolution::SourceId::from_stable_name("source/feed"));
    const auto component_id =
        require_value(evolution::ComponentId::from_stable_name("component/normalizer"));
    const auto algorithm_id =
        require_value(evolution::AlgorithmId::from_stable_name("algorithm/normalize"));
    const auto configuration_id = make_configuration().id();
    const auto run_id = require_value(evolution::RunId::from_stable_name("run/provenance"));

    return require_value(evolution::Provenance::create(
        evolution::SourceReference{source_id, "feed", "capture-2026-10-04"},
        {{"record", "record-7", "schema-v1"}},
        evolution::TransformationReference{component_id, "normalizer", "1.2.0", algorithm_id, "3",
                                           std::nullopt},
        configuration_id, run_id, evolution::time::TemporalInformation::unknown(),
        {{"reference-data", "2026-10-04"}}));
}

void test_provenance(TestRunner &test) {
    const auto provenance = make_provenance();
    const auto equivalent = make_provenance();
    const auto enriched = require_value(provenance.with_environment({"ruleset", "2026-10-04"}));
    const auto child = require_value(enriched.with_parent(provenance.id()));

    test.expect(provenance.id() == equivalent.id(), "Equivalent Provenance is reproducible");
    test.expect(provenance.environment().size() == 1, "Provenance owns environment metadata");
    test.expect(enriched.environment().size() == 2, "Provenance enrichment returns a new value");
    test.expect(provenance.environment().size() == 1, "Provenance enrichment is immutable");
    test.expect(child.parents().front() == provenance.id(), "Provenance parentage is explicit");
    test.expect(evolution::Provenance::create(
                    evolution::SourceReference{
                        require_value(evolution::SourceId::from_stable_name("bad-source")), "", ""},
                    {},
                    evolution::TransformationReference{
                        require_value(evolution::ComponentId::from_stable_name("component")),
                        "component", "1", std::nullopt, std::nullopt, std::nullopt})
                    .has_error(),
                "Invalid Provenance is rejected");
}

} // namespace

int main() {
    TestRunner test;
    test_error_and_result(test);
    test_identity(test);
    test_time(test);
    test_configuration(test);
    test_execution_context(test);
    test_context(test);
    test_provenance(test);

    if (test.failures() != 0) {
        std::cerr << test.failures() << " Core foundation test(s) failed\n";
        return 1;
    }
    return 0;
}
