#include "evolution/processing/checkpoint.hpp"
#include "evolution/processing/delivery.hpp"
#include "evolution/processing/recovery.hpp"
#include "evolution/processing/reproducibility.hpp"
#include "evolution/processing/supervision.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

class TestRunner {
  public:
    void expect(bool condition, const std::string &message) {
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

template <typename Id> auto stable_id(const std::string &name) -> Id {
    return require_value(Id::from_stable_name(name));
}

auto historical_time(std::int64_t seconds) -> evolution::time::TemporalInformation {
    return evolution::time::TemporalInformation::known(
        evolution::time::TimePoint{std::chrono::seconds(seconds)},
        evolution::time::Precision::Second);
}

auto make_manifest() -> evolution::processing::CheckpointManifest {
    using namespace evolution;
    using namespace evolution::processing;
    return require_value(CheckpointManifest::create(CheckpointManifestData{
        stable_id<CheckpointId>("checkpoint/phase5"),
        CheckpointScope::Processor,
        {stable_id<GraphId>("graph/phase5"), "1.0.0"},
        {stable_id<ComponentId>("component/phase5"), stable_id<ProcessorId>("processor/phase5"),
         "2.0.0"},
        {stable_id<ConfigurationId>("configuration/phase5"), "3"},
        {{"event", "1"}},
        CheckpointAlgorithmVersion{stable_id<AlgorithmId>("algorithm/phase5"), "4"},
        {stable_id<StateId>("state/phase5"), "5"},
        {{"events", "partition-1", 10, stable_id<EventId>("event/10"), historical_time(10)}},
        {stable_id<ProvenanceId>("provenance/phase5"), "1"},
        historical_time(100),
        {"test-digest", "state-digest"},
    }));
}

auto compatibility() -> evolution::processing::CheckpointCompatibilityRequirements {
    using namespace evolution;
    using namespace evolution::processing;
    return {stable_id<GraphId>("graph/phase5"),
            "1.0.0",
            stable_id<ComponentId>("component/phase5"),
            stable_id<ProcessorId>("processor/phase5"),
            "2.0.0",
            stable_id<ConfigurationId>("configuration/phase5"),
            "3",
            CheckpointAlgorithmVersion{stable_id<AlgorithmId>("algorithm/phase5"), "4"},
            "5",
            "1",
            {{"event", "1"}}};
}

class IntegerState final : public evolution::processing::SnapshotRestorer<int> {
  public:
    explicit IntegerState(int value) : value_(value) {}

    auto snapshot() const -> evolution::Result<evolution::processing::StateSnapshot<int>> override {
        return evolution::Result<evolution::processing::StateSnapshot<int>>::success(
            {value_, "5", "state-digest", true});
    }

    auto restore(const evolution::processing::StateSnapshot<int> &snapshot,
                 const evolution::CancellationToken &cancellation)
        -> evolution::Result<void> override {
        if (cancellation.is_cancelled()) {
            return evolution::Result<void>::failure(
                evolution::Error(evolution::ErrorCode::create("test.restore_cancelled"),
                                 evolution::ErrorCategory::Cancelled, "restore cancelled"));
        }
        value_ = snapshot.value;
        ++restore_count_;
        return evolution::Result<void>::success();
    }

    [[nodiscard]] auto value() const noexcept -> int {
        return value_;
    }
    [[nodiscard]] auto restore_count() const noexcept -> int {
        return restore_count_;
    }

  private:
    int value_{};
    int restore_count_{};
};

class StringReplaySource final : public evolution::processing::RetainedInputSource<std::string> {
  public:
    explicit StringReplaySource(std::vector<evolution::processing::ReplayItem<std::string>> values)
        : values_(std::move(values)) {}

    auto read_after(const std::vector<evolution::processing::InputPosition> &positions,
                    std::size_t maximum_items)
        -> evolution::Result<std::vector<evolution::processing::ReplayItem<std::string>>> override {
        static_cast<void>(positions);
        auto result = values_;
        if (result.size() > maximum_items) {
            result.resize(maximum_items);
        }
        return evolution::Result<std::vector<evolution::processing::ReplayItem<std::string>>>::
            success(std::move(result));
    }

  private:
    std::vector<evolution::processing::ReplayItem<std::string>> values_;
};

class StringReplayTarget final : public evolution::processing::ReplayTarget<std::string> {
  public:
    auto apply(const evolution::processing::ReplayItem<std::string> &item,
               const evolution::ExecutionContext &context) -> evolution::Result<void> override {
        if (context.mode() != evolution::ExecutionMode::Replay) {
            return evolution::Result<void>::failure(
                evolution::Error(evolution::ErrorCode::create("test.invalid_replay_mode"),
                                 evolution::ErrorCategory::InvalidArgument, "not replay mode"));
        }
        values.push_back(item.input);
        times.push_back(item.position.historical_time);
        return evolution::Result<void>::success();
    }

    std::vector<std::string> values;
    std::vector<evolution::time::TemporalInformation> times;
};

void test_checkpoint_and_replay(TestRunner &test) {
    using namespace evolution;
    using namespace evolution::processing;
    const auto manifest = make_manifest();
    IntegerState state(7);
    const auto checkpoint = Checkpoint<int>{manifest, {42, "5", "state-digest", true}};
    test.expect(restore_checkpoint(checkpoint, compatibility(), state).has_value() &&
                    state.value() == 42 && state.restore_count() == 1,
                "Compatible checkpoint restores state only after full validation");

    auto incompatible = compatibility();
    incompatible.configuration_version = "different";
    IntegerState unchanged(9);
    test.expect(restore_checkpoint(checkpoint, incompatible, unchanged).has_error() &&
                    unchanged.value() == 9 && unchanged.restore_count() == 0,
                "Incompatible checkpoint is rejected before mutating state");

    const auto corrupt = Checkpoint<int>{manifest, {99, "5", "wrong-digest", true}};
    test.expect(restore_checkpoint(corrupt, compatibility(), unchanged).has_error() &&
                    unchanged.value() == 9,
                "Corrupt checkpoint is never treated as valid recovered state");
    CancellationSource cancellation;
    cancellation.cancel();
    test.expect(restore_checkpoint(checkpoint, compatibility(), unchanged, cancellation.token())
                        .has_error() &&
                    unchanged.value() == 9,
                "Cancelled restore does not expose partially restored state");

    const auto replay_time_11 = historical_time(11);
    const auto replay_time_12 = historical_time(12);
    StringReplaySource source({
        {"eleven", {"events", "partition-1", 11, stable_id<EventId>("event/11"), replay_time_11}},
        {"twelve", {"events", "partition-1", 12, stable_id<EventId>("event/12"), replay_time_12}},
    });
    StringReplayTarget target;
    const auto context =
        ExecutionContext::create(ExecutionMode::Replay, stable_id<RunId>("run/replay-phase5"));
    const auto replayed = require_value(
        replay_retained_input(source, target, manifest.data().input_positions, 2, context));
    test.expect(replayed.replayed == 2 && target.values.size() == 2 &&
                    target.times.front() == replay_time_11 && target.times.back() == replay_time_12,
                "Retained-input replay preserves ordering and historical event time");

    StringReplaySource unordered_source({
        {"twelve", {"events", "partition-1", 12, std::nullopt, replay_time_12}},
        {"eleven", {"events", "partition-1", 11, std::nullopt, replay_time_11}},
    });
    StringReplayTarget unordered_target;
    test.expect(replay_retained_input(unordered_source, unordered_target,
                                      manifest.data().input_positions, 2, context)
                    .has_error(),
                "Replay rejects retained input that violates recovery ordering");
    StringReplaySource duplicate_source({
        {"ten", {"events", "partition-1", 10, std::nullopt, historical_time(10)}},
    });
    StringReplayTarget duplicate_target;
    test.expect(replay_retained_input(duplicate_source, duplicate_target,
                                      manifest.data().input_positions, 1, context)
                    .has_error(),
                "Replay begins strictly after the checkpoint input position");
}

auto recovery_policy() -> evolution::processing::RecoveryPolicy {
    using namespace evolution;
    using namespace evolution::processing;
    return require_value(RecoveryPolicy::create({
        {ErrorCategory::Unavailable, std::nullopt, RecoveryScope::Operation, RecoveryAction::Retry,
         2, RecoveryAction::ManualIntervention, true},
        {ErrorCategory::Persistence, std::nullopt, RecoveryScope::Processor,
         RecoveryAction::Restore, 2, RecoveryAction::ManualIntervention, false},
    }));
}

class DurableStore final : public evolution::processing::DeduplicationStore {
  public:
    [[nodiscard]] auto is_durable() const noexcept -> bool override {
        return true;
    }

    [[nodiscard]] auto contains(const evolution::processing::IdempotencyKey &key) const
        -> evolution::Result<bool> override {
        const std::scoped_lock lock(mutex_);
        return evolution::Result<bool>::success(values_.contains(std::string(key.value())));
    }

    auto record_completed(evolution::processing::IdempotencyKey key)
        -> evolution::Result<void> override {
        const std::scoped_lock lock(mutex_);
        values_.insert(std::string(key.value()));
        return evolution::Result<void>::success();
    }

  private:
    mutable std::mutex mutex_;
    std::unordered_set<std::string> values_;
};

void test_recovery_and_duplicate_effect(TestRunner &test) {
    using namespace evolution;
    using namespace evolution::processing;
    const auto error = Error(ErrorCode::create("test.unavailable"), ErrorCategory::Unavailable,
                             "dependency unavailable");
    const auto policy = recovery_policy();
    const auto unsafe = policy.decide(
        error, RecoveryScope::Operation, 0,
        RecoveryConditions{DeliveryGuarantee::AtLeastOnce, CompletionKnowledge::Unknown, false});
    test.expect(unsafe.action == RecoveryAction::ManualIntervention &&
                    unsafe.original_error.code() == error.code(),
                "Unknown completion blocks unsafe retry without changing error identity");

    const auto safe = policy.decide(
        error, RecoveryScope::Operation, 0,
        RecoveryConditions{DeliveryGuarantee::AtLeastOnce, CompletionKnowledge::Unknown, true});
    test.expect(safe.action == RecoveryAction::Retry && safe.duplicate_execution_possible,
                "Explicit idempotency permits retry while acknowledging duplicate execution");
    test.expect(policy.decide(error, RecoveryScope::Operation, 2,
                              RecoveryConditions{DeliveryGuarantee::AtLeastOnce,
                                                 CompletionKnowledge::KnownNotCompleted, true})
                        .action == RecoveryAction::ManualIntervention,
                "Recovery policy enforces bounded attempts");
    test.expect(RecoveryPolicy::create({}, RecoveryAction::Retry).has_error(),
                "Automatic recovery cannot be configured as an unbounded fallback");

    const auto delivery = require_value(DeliveryContract::create(
        DeliveryGuarantee::ExactlyOnce, AcknowledgementPoint::DurableCompletion,
        IdempotencyMode::Keyed, 3, ExactlyOnceMechanism::IdempotentEffectWithDurableDeduplication));
    auto store = std::make_shared<DurableStore>();
    const auto guard = require_value(DeliveryGuard::create(delivery, store));
    const auto key = require_value(IdempotencyKey::create("effect/recovery"));
    int externally_visible_effects = 0;
    if (require_value(guard.begin(key)) == DeliveryDecision::Execute) {
        ++externally_visible_effects;
        static_cast<void>(guard.complete(key));
    }
    const auto after_crash = require_value(guard.begin(key));
    test.expect(after_crash == DeliveryDecision::AlreadyCompleted &&
                    externally_visible_effects == 1,
                "Crash-boundary retry uses durable deduplication to prevent duplicate effects");
}

void test_supervision(TestRunner &test) {
    using namespace evolution;
    using namespace evolution::processing;
    auto supervisor = require_value(Supervisor::create(
        SupervisionPolicy{RecoveryScope::Processor, 2, 100ms, RecoveryAction::ManualIntervention,
                          RecoveryAction::Stop, true, RecoveryScope::Graph},
        require_value(RecoveryPolicy::create({
            {ErrorCategory::Unavailable, std::nullopt, RecoveryScope::Processor,
             RecoveryAction::Restart, 5, RecoveryAction::ManualIntervention, false},
        }))));
    const auto target = stable_id<ComponentId>("component/supervised");
    const auto failure = Error(ErrorCode::create("test.processor_unavailable"),
                               ErrorCategory::Unavailable, "processor unavailable");
    const auto observation = FailureObservation{
        target,
        failure,
        RecoveryConditions{DeliveryGuarantee::AtLeastOnce, CompletionKnowledge::KnownNotCompleted,
                           true},
        {},
    };
    const auto start = time::MonotonicTimePoint{};
    const auto first = supervisor.observe_failure(observation, start);
    const auto cooling = supervisor.observe_failure(observation, start + 10ms);
    const auto second = supervisor.observe_failure(observation, start + 100ms);
    const auto exhausted = supervisor.observe_failure(observation, start + 200ms);
    test.expect(first.recovery.action == RecoveryAction::Restart && first.attempts_used == 1 &&
                    cooling.state == SupervisorState::Cooldown && cooling.next_attempt_at &&
                    second.attempts_used == 2 &&
                    exhausted.state == SupervisorState::RequiresIntervention,
                "Supervisor prevents recovery storms with cooldown and bounded attempts");

    const auto optional_dependency = stable_id<ComponentId>("component/optional");
    auto degraded_observation = observation;
    degraded_observation.conditions.degradation_supported = true;
    degraded_observation.dependencies = {
        {optional_dependency, DependencyCriticality::Optional, false, true}};
    const auto degraded = supervisor.observe_failure(degraded_observation, start + 300ms);
    test.expect(degraded.state == SupervisorState::Degraded &&
                    degraded.recovery.action == RecoveryAction::Degrade &&
                    degraded.unavailable_dependencies.size() == 1,
                "Optional dependency failure permits only explicit supported degradation");

    supervisor.record_recovery_success(target);
    test.expect(supervisor.state(target) == SupervisorState::Monitoring,
                "Successful recovery explicitly restores supervisor eligibility state");
    auto required_observation = observation;
    required_observation.dependencies = {{stable_id<ComponentId>("component/required"),
                                          DependencyCriticality::Required, false, false}};
    const auto escalated = supervisor.observe_failure(required_observation, start + 400ms);
    test.expect(escalated.state == SupervisorState::Escalated &&
                    escalated.recovery.action == RecoveryAction::Stop &&
                    escalated.escalation_scope == RecoveryScope::Graph,
                "Required dependency failure follows explicit graph escalation policy");
}

auto reproducibility_record(const std::string &run, const std::string &configuration_version)
    -> evolution::processing::ReproducibilityRecord {
    using namespace evolution;
    using namespace evolution::processing;
    return require_value(ReproducibilityRecord::create(ReproducibilityRecordData{
        stable_id<RunId>(run),
        ReproducibilityLevel::SemanticDeterministic,
        {{"event", stable_id<EventId>("event/reproducible").to_string(), "1"}},
        stable_id<ConfigurationId>("configuration/reproducible"),
        configuration_version,
        stable_id<GraphId>("graph/reproducible"),
        "1",
        {{stable_id<ComponentId>("component/reproducible"), "1"}},
        {{stable_id<AlgorithmId>("algorithm/reproducible"), "1"}},
        {{stable_id<StateId>("state/reproducible"), "1"}},
        {{"mt19937_64", 42, true}},
        {{"fixed", "historical-event-time", time::TimePoint{std::chrono::seconds(10)}, true, true}},
        {{"compiler", "gcc-11"}, {"architecture", "x86_64"}},
        {OrderingRequirement::Sequence, "partition-key", "event-id", true},
        {AdmissionPolicyKind::Reject, 0, 0, 0, false},
        {{ResourceKind::ExecutionSlots, 1, 1, "fixed parallelism"}},
        {{"reference-data", "1", "snapshot-42", true}},
        {"result/42"},
    }));
}

void test_reproducibility(TestRunner &test) {
    using namespace evolution;
    using namespace evolution::processing;
    const auto first = reproducibility_record("run/first", "1");
    const auto second = reproducibility_record("run/second", "1");
    const auto comparison = ReproducibilityComparator::compare_conditions(first, second);
    test.expect(
        comparison.equivalent_conditions &&
            ReproducibilityComparator::compare_results(first, "semantic-result", second,
                                                       "semantic-result") ==
                DifferentialOutcome::Equivalent,
        "Different run identities remain reproducible under equivalent semantic conditions");
    test.expect(ReproducibilityComparator::compare_results(first, "left", second, "right") ==
                    DifferentialOutcome::Different,
                "Differential testing detects output divergence under equivalent conditions");

    const auto changed_configuration = reproducibility_record("run/third", "2");
    test.expect(
        ReproducibilityComparator::compare_results(first, "left", changed_configuration, "left") ==
            DifferentialOutcome::Incomparable,
        "Material configuration changes make differential results incomparable");

    auto invalid_data = first.data();
    invalid_data.randomness.front().seed.reset();
    test.expect(ReproducibilityRecord::create(std::move(invalid_data)).has_error(),
                "Determinism claims reject uncaptured result-affecting randomness");
    auto secret_data = first.data();
    secret_data.environment["api_token"] = "secret";
    test.expect(ReproducibilityRecord::create(std::move(secret_data)).has_error(),
                "Reproducibility records reject secret-bearing environment fields");
}

} // namespace

int main() {
    TestRunner test;
    test_checkpoint_and_replay(test);
    test_recovery_and_duplicate_effect(test);
    test_supervision(test);
    test_reproducibility(test);

    if (test.failures() != 0) {
        std::cerr << test.failures() << " processing recovery test(s) failed\n";
        return 1;
    }
    return 0;
}
