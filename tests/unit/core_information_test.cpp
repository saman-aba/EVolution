#include "evolution/core/event/event.hpp"
#include "evolution/core/event/event_stream.hpp"
#include "evolution/core/measurement/measurement.hpp"
#include "evolution/core/processing/envelope.hpp"
#include "evolution/core/series/time_series.hpp"
#include "evolution/core/state/state.hpp"

#include <chrono>
#include <compare>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

struct Payload {
    int delta;
    std::string label;

    friend auto operator==(const Payload &, const Payload &) -> bool = default;
};

struct CounterState {
    int value;

    friend auto operator==(const CounterState &, const CounterState &) -> bool = default;
};

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

template <typename T> auto require_value(evolution::Result<T> result) -> T {
    if (result.has_error()) {
        throw std::runtime_error(std::string(result.error().message()));
    }
    return std::move(result).value();
}

auto stable_event_id(std::string_view value) -> evolution::EventId {
    return require_value(evolution::EventId::from_stable_name(value));
}

auto make_context() -> evolution::Context {
    return require_value(evolution::Context::create(evolution::ContextScope::Session,
                                                    {{"session", "session-42"}},
                                                    {{"region", std::string("test")}}));
}

auto make_provenance(std::string_view suffix) -> evolution::Provenance {
    const auto source = require_value(
        evolution::SourceId::from_stable_name(std::string("source/") + std::string(suffix)));
    const auto component = require_value(
        evolution::ComponentId::from_stable_name(std::string("component/") + std::string(suffix)));
    return require_value(evolution::Provenance::create(
        evolution::SourceReference{source, "test", std::string(suffix)}, {},
        evolution::TransformationReference{component, "test-component", "1", std::nullopt,
                                           std::nullopt, std::nullopt}));
}

auto make_event(std::string_view identity, int delta, std::uint64_t sequence)
    -> evolution::Event<Payload> {
    const auto source_id = require_value(evolution::SourceId::from_stable_name("source/events"));
    evolution::EventMetadata metadata{
        stable_event_id(identity),
        require_value(evolution::EventType::create("Test.CounterChanged")),
        require_value(evolution::EventSource::create(source_id, "test-source")),
        evolution::time::TemporalInformation::known(
            evolution::time::TimePoint{std::chrono::seconds(sequence)},
            evolution::time::Precision::Second),
        evolution::time::TimePoint{std::chrono::seconds(sequence + 10)},
        evolution::EventSequence{"test-stream", sequence},
        make_context(),
        make_provenance(identity),
        "schema-v1",
        "event-v1",
    };
    return require_value(evolution::Event<Payload>::create(std::move(metadata),
                                                           Payload{delta, std::string(identity)}));
}

auto make_metric() -> evolution::Metric {
    const auto metric_id = require_value(evolution::MetricId::from_stable_name("metric/counter"));
    const auto unit = require_value(evolution::Unit::create("count", "count"));
    return require_value(evolution::Metric::create(metric_id, "counter", "Observed counter value",
                                                   evolution::MetricValueKind::Real, unit,
                                                   "metric-v1"));
}

class CounterProjection final : public evolution::Projection<CounterState, Payload> {
  public:
    CounterProjection()
        : id_(require_value(evolution::ProjectionId::from_stable_name("projection/counter"))),
          state_id_(require_value(evolution::StateId::from_stable_name("state/counter"))) {}

    [[nodiscard]] auto id() const noexcept -> const evolution::ProjectionId & override {
        return id_;
    }

    [[nodiscard]] auto version() const noexcept -> std::string_view override {
        return "projection-v1";
    }

    auto initial_state() const -> evolution::Result<evolution::State<CounterState>> override {
        return evolution::State<CounterState>::create(
            state_id_, "counter-1", evolution::StateVersion{0, std::nullopt}, id_,
            std::string(version()), CounterState{0}, evolution::StateCompleteness::Complete,
            evolution::time::TemporalInformation::unknown(), std::nullopt, make_context(),
            make_provenance("initial-state"));
    }

    auto apply(const evolution::State<CounterState> &current,
               const evolution::Event<Payload> &event) const
        -> evolution::Result<evolution::State<CounterState>> override {
        return evolution::State<CounterState>::create(
            state_id_, std::string(current.entity_identity()),
            evolution::StateVersion{
                current.version().revision + 1,
                event.metadata().id,
            },
            id_, std::string(version()),
            CounterState{current.value().value + event.payload().delta},
            evolution::StateCompleteness::Complete, event.metadata().occurrence_time, std::nullopt,
            event.metadata().context, event.metadata().provenance);
    }

  private:
    evolution::ProjectionId id_;
    evolution::StateId state_id_;
};

void test_event(TestRunner &test) {
    const auto event = make_event("event-1", 3, 1);
    test.expect(event.payload().delta == 3, "Event owns its typed payload");
    test.expect(event.metadata().occurrence_time.value() != event.metadata().ingestion_time,
                "Event and ingestion time remain distinct");
    test.expect(event.metadata().context && event.metadata().provenance,
                "Event retains Context and Provenance");

    auto invalid_metadata = event.metadata();
    invalid_metadata.schema_version.clear();
    test.expect(evolution::Event<Payload>::create(invalid_metadata, event.payload()).has_error(),
                "Event construction rejects missing schema version");

    auto invalid_sequence = event.metadata();
    invalid_sequence.sequence = evolution::EventSequence{"", 1};
    test.expect(evolution::Event<Payload>::create(invalid_sequence, event.payload()).has_error(),
                "Event sequence requires explicit scope");

    struct NeutralEventRecord {
        evolution::EventMetadata metadata;
        Payload payload;
    };
    const NeutralEventRecord record{event.metadata(), event.payload()};
    const auto round_trip =
        require_value(evolution::Event<Payload>::create(record.metadata, record.payload));
    test.expect(round_trip == event, "Event round-trips through a format-neutral record");
}

void test_state_and_projection(TestRunner &test) {
    const std::vector events{
        make_event("state-event-1", 2, 1),
        make_event("state-event-2", 5, 2),
    };
    const CounterProjection projection;
    const auto state =
        require_value(evolution::reconstruct<CounterState, Payload>(projection, events));
    test.expect(state.value().value == 7, "Projection reconstructs State from ordered Events");
    test.expect(state.version().revision == 2, "State version identifies reconstruction point");
    test.expect(state.version().through_event == events.back().metadata().id,
                "State records the Event boundary it represents");

    const auto stream_id = require_value(evolution::StreamId::from_stable_name("stream/state"));
    const evolution::StateSnapshot snapshot{
        state,
        evolution::SnapshotBoundary{stream_id, 2},
        "snapshot-v1",
        make_provenance("snapshot"),
    };
    const auto snapshot_copy = snapshot;
    test.expect(snapshot_copy == snapshot, "State snapshots have ordinary value semantics");

    const auto round_trip = require_value(evolution::State<CounterState>::create(
        state.id(), std::string(state.entity_identity()), state.version(), state.projection_id(),
        std::string(state.projection_version()), state.value(), state.completeness(),
        state.effective_time(), state.validity(), state.context(), state.provenance()));
    test.expect(round_trip == state, "State round-trips through its semantic fields");
}

auto make_measurement(double value, std::string_view identity) -> evolution::Measurement<double> {
    const auto id = require_value(evolution::MeasurementId::from_stable_name(identity));
    return require_value(evolution::Measurement<double>::observed(
        id, make_metric(), value, "counter-1", {{"region", "test"}},
        evolution::MeasurementPeriod::point(evolution::time::TemporalInformation::known(
            evolution::time::TimePoint{10s}, evolution::time::Precision::Second)),
        evolution::MeasurementQuality{evolution::QualityLevel::Good, 0.95, 0.1, {}}, make_context(),
        make_provenance(identity)));
}

void test_measurement(TestRunner &test) {
    const auto lower = make_measurement(10.0, "measurement/lower");
    const auto upper = make_measurement(20.0, "measurement/upper");
    const auto comparison = require_value(lower.compare(upper));
    test.expect(comparison == std::partial_ordering::less, "Compatible measurements compare");

    const auto missing = require_value(evolution::Measurement<double>::missing(
        require_value(evolution::MeasurementId::from_stable_name("measurement/missing")),
        make_metric(), "counter-1", {}, lower.period(), make_provenance("measurement/missing")));
    const auto unknown = require_value(evolution::Measurement<double>::unknown(
        require_value(evolution::MeasurementId::from_stable_name("measurement/unknown")),
        make_metric(), "counter-1", {}, lower.period(), make_provenance("measurement/unknown")));
    test.expect(missing.status() != unknown.status() && !missing.value() && !unknown.value(),
                "Missing and unknown measurements remain distinct from zero");
    test.expect(evolution::Measurement<double>::observed(
                    require_value(evolution::MeasurementId::from_stable_name("measurement/nan")),
                    make_metric(), std::numeric_limits<double>::quiet_NaN(), "counter-1", {},
                    lower.period(), {}, std::nullopt, make_provenance("measurement/nan"))
                    .has_error(),
                "Non-finite measurements are rejected");
    const auto integer_metric = require_value(evolution::Metric::create(
        require_value(evolution::MetricId::from_stable_name("metric/integer")), "integer-counter",
        "Integer counter", evolution::MetricValueKind::Integer, std::nullopt, "metric-v1"));
    test.expect(
        evolution::Measurement<double>::observed(
            require_value(evolution::MeasurementId::from_stable_name("measurement/mismatch")),
            integer_metric, 1.0, "counter-1", {}, lower.period(), {}, std::nullopt,
            make_provenance("measurement/mismatch"))
            .has_error(),
        "Measurement value type must match its Metric definition");

    const auto round_trip = require_value(evolution::Measurement<double>::observed(
        lower.id(), lower.metric(), *lower.value(), std::string(lower.scope()), lower.dimensions(),
        lower.period(), lower.quality(), lower.context(), lower.provenance()));
    test.expect(round_trip == lower, "Measurement round-trips through semantic fields");
}

void test_envelope(TestRunner &test) {
    const auto envelope_id = require_value(evolution::EnvelopeId::from_stable_name("envelope/1"));
    const auto output_id = require_value(evolution::EnvelopeId::from_stable_name("envelope/2"));
    const auto correlation =
        require_value(evolution::CorrelationId::from_stable_name("correlation/1"));
    const auto metadata = require_value(evolution::ProcessingMetadata::create(
        envelope_id,
        evolution::time::TemporalInformation::known(evolution::time::TimePoint{20s},
                                                    evolution::time::Precision::Second),
        correlation, std::nullopt, evolution::OrderingMetadata{"partition-a", 10},
        std::string("partition-a"), make_context(), make_provenance("envelope"), 1, true));
    const auto envelope = evolution::Envelope<Payload>::create(Payload{1, "payload"}, metadata);
    const auto retry = metadata.for_retry();
    const auto output = metadata.for_output(
        output_id,
        evolution::time::TemporalInformation::known(evolution::time::TimePoint{21s},
                                                    evolution::time::Precision::Second),
        make_provenance("envelope/output"));

    test.expect(envelope.payload().label == "payload", "Envelope preserves typed payload");
    test.expect(retry.delivery_attempt() == 2, "Retry increments explicit delivery attempt");
    test.expect(output.causation_id() == envelope_id, "Output records its causal Envelope");
    test.expect(output.correlation_id() == correlation, "Output preserves correlation identity");
    test.expect(!output.ordering(), "Output does not silently inherit input ordering");
    test.expect(output.partition_key() == metadata.partition_key(), "Partition key propagates");
    test.expect(output.is_replay(), "Replay metadata propagates explicitly");
    test.expect(evolution::ProcessingMetadata::create(envelope_id, metadata.processing_time(),
                                                      std::nullopt, std::nullopt, std::nullopt,
                                                      std::nullopt, std::nullopt, std::nullopt, 0)
                    .has_error(),
                "Delivery attempt zero is rejected");

    const auto round_trip =
        evolution::Envelope<Payload>::create(envelope.payload(), envelope.metadata());
    test.expect(round_trip == envelope, "Envelope round-trips through neutral fields");
}

void test_event_history(TestRunner &test) {
    const auto stream_id = require_value(evolution::StreamId::from_stable_name("stream/events"));
    auto history = require_value(evolution::EventHistory<Payload>::create(
        stream_id, evolution::DuplicateEventPolicy::Idempotent, 2));
    const auto first_event = make_event("history-event-1", 1, 1);
    const auto second_event = make_event("history-event-2", 2, 2);
    const auto first = require_value(history.append(first_event));
    const auto duplicate = require_value(history.append(first_event));
    const auto second = require_value(history.append(second_event));

    test.expect(first.position.value() == 0, "First Event receives stable position zero");
    test.expect(duplicate.outcome == evolution::AppendOutcome::IdempotentExisting &&
                    duplicate.position == first.position,
                "Idempotent duplicate append preserves the existing position");
    test.expect(second.position.value() == 1, "Second Event receives the next stream position");

    const evolution::EventReader<Payload> &reader = history;
    const evolution::EventAppender<Payload> &appender = history;
    static_cast<void>(appender);
    const auto batch = require_value(
        reader.read(evolution::StreamCursor{stream_id, evolution::StreamPosition(0)}, 2));
    test.expect(batch.records.size() == 2 && batch.end_of_stream, "Bounded read preserves order");
    test.expect(batch.records[0].event.metadata().id == first_event.metadata().id,
                "Stream order is independent and explicit");

    const auto replay_run = require_value(evolution::RunId::from_stable_name("run/replay"));
    const auto replay = require_value(reader.replay(
        replay_run, evolution::StreamCursor{stream_id, evolution::StreamPosition(0)}, 1));
    test.expect(replay.replay_run == replay_run, "Replay identifies the execution run");
    test.expect(replay.records.front().event == first_event,
                "Replay reads the original Event without creating a new identity");
    test.expect(reader.read(evolution::StreamCursor{stream_id, evolution::StreamPosition(0)}, 3)
                    .has_error(),
                "Reads cannot exceed configured bounded capacity");
}

auto make_series(evolution::ObservationInsertionPolicy insertion_policy)
    -> evolution::TimeSeries<double> {
    const auto series_id =
        require_value(evolution::TimeSeriesId::from_stable_name("series/counter"));
    return require_value(evolution::TimeSeries<double>::create(
        series_id, make_metric(), "counter-1", {{"region", "test"}}, "series-v1",
        evolution::DuplicateTimestampPolicy::Reject, insertion_policy, 1s,
        make_provenance("series")));
}

auto observed_at(std::int64_t seconds, double value) -> evolution::Observation<double> {
    return require_value(evolution::Observation<double>::observed(
        evolution::time::TemporalInformation::known(
            evolution::time::TimePoint{std::chrono::seconds(seconds)},
            evolution::time::Precision::Second),
        value, make_provenance("observation/" + std::to_string(seconds))));
}

void test_time_series(TestRunner &test) {
    auto series = make_series(evolution::ObservationInsertionPolicy::OrderedAppend);
    series = require_value(series.with_observation(observed_at(1, 10.0)));
    series = require_value(series.with_observation(observed_at(3, 30.0)));

    test.expect(series.with_observation(observed_at(2, 20.0)).has_error(),
                "Ordered append rejects out-of-order observations");
    test.expect(series.with_observation(observed_at(3, 31.0)).has_error(),
                "Duplicate timestamp policy is explicit");
    const auto gaps = require_value(series.gaps(1s));
    test.expect(gaps.size() == 1, "Series detects temporal gaps using an explicit interval");

    const auto range = require_value(evolution::time::TimeRange::create(
        evolution::time::TimePoint{1s}, evolution::time::TimePoint{3s}));
    const auto window = series.window(range);
    test.expect(window.observations().size() == 1, "Series windows use half-open ranges");

    const std::vector timestamps{
        evolution::time::TimePoint{1s},
        evolution::time::TimePoint{2s},
        evolution::time::TimePoint{3s},
    };
    const auto aligned_id =
        require_value(evolution::TimeSeriesId::from_stable_name("series/aligned"));
    const auto aligned =
        require_value(series.align(aligned_id, timestamps, evolution::InterpolationPolicy::Linear,
                                   make_provenance("series/alignment")));
    test.expect(aligned.observations().size() == 3 && *aligned.observations()[1].value() == 20.0,
                "Linear alignment produces explicitly derived observations");

    const auto neutral_round_trip = require_value(evolution::TimeSeries<double>::create(
        series.id(), series.metric(), std::string(series.scope()), series.dimensions(),
        std::string(series.version()), series.duplicate_policy(), series.insertion_policy(),
        series.expected_interval(), series.provenance(), series.observations()));
    test.expect(neutral_round_trip == series, "TimeSeries round-trips through semantic fields");

    const auto unknown_time =
        evolution::Observation<double>::unknown(evolution::time::TemporalInformation::unknown(),
                                                make_provenance("observation/unknown-time"));
    test.expect(unknown_time.has_error(),
                "Ordered series reject observations without a temporal position");
}

} // namespace

int main() {
    TestRunner test;
    test_event(test);
    test_state_and_projection(test);
    test_measurement(test);
    test_envelope(test);
    test_event_history(test);
    test_time_series(test);

    if (test.failures() != 0) {
        std::cerr << test.failures() << " Core information test(s) failed\n";
        return 1;
    }
    return 0;
}
