#include "evolution/core/configuration/configuration.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/processing/envelope.hpp"
#include "evolution/core/time/time.hpp"
#include "evolution/processing/admission.hpp"
#include "evolution/processing/bounded_queue.hpp"
#include "evolution/processing/concurrency.hpp"
#include "evolution/processing/delivery.hpp"
#include "evolution/processing/lifecycle.hpp"
#include "evolution/processing/processor.hpp"

#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

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

auto make_metadata(std::string_view name, bool replay = false) -> evolution::ProcessingMetadata {
    const auto id = require_value(evolution::EnvelopeId::from_stable_name(name));
    const auto correlation =
        require_value(evolution::CorrelationId::from_stable_name("processing-test"));
    return require_value(evolution::ProcessingMetadata::create(
        id,
        evolution::time::TemporalInformation::known(evolution::time::TimePoint{10s},
                                                    evolution::time::Precision::Second),
        correlation, std::nullopt, evolution::OrderingMetadata{"input", 1},
        std::string("partition-a"), std::nullopt, std::nullopt, 1, replay));
}

auto make_configuration() -> evolution::Configuration {
    return require_value(evolution::Configuration::create(
        {{"processor.multiplier", std::int64_t{2}}}, "processing-test-v1"));
}

auto make_execution_context(bool cancelled = false) -> evolution::ExecutionContext {
    const auto run_id = require_value(evolution::RunId::from_stable_name("processing-run"));
    evolution::CancellationSource cancellation;
    if (cancelled) {
        cancellation.cancel();
    }
    return evolution::ExecutionContext::create(
        evolution::ExecutionMode::Replay, run_id, std::nullopt, nullptr,
        std::make_shared<evolution::DeterministicRandomSource>(42), cancellation.token());
}

class DeterministicProcessor final : public evolution::processing::Processor<int, int, int> {
  public:
    auto process(evolution::Envelope<int> input, std::optional<int> state,
                 const evolution::Configuration &configuration,
                 const evolution::ExecutionContext &execution_context) const
        -> evolution::Result<evolution::processing::ProcessingOutcome<int, int>> override {
        if (execution_context.is_cancelled()) {
            return evolution::Result<evolution::processing::ProcessingOutcome<int, int>>::success(
                evolution::processing::ProcessingOutcome<int, int>::cancelled(state));
        }
        if (input.payload() < 0) {
            return evolution::Result<evolution::processing::ProcessingOutcome<int, int>>::failure(
                evolution::Error(evolution::ErrorCode::create("processor.negative_input"),
                                 evolution::ErrorCategory::InvalidArgument,
                                 "negative input is not supported"));
        }

        const auto multiplier = configuration.get<std::int64_t>("processor.multiplier");
        if (multiplier.has_error()) {
            return evolution::Result<evolution::processing::ProcessingOutcome<int, int>>::failure(
                multiplier.error());
        }
        const auto current = state.value_or(0);
        const auto updated = current + input.payload();
        if (input.payload() == 0) {
            return evolution::Result<evolution::processing::ProcessingOutcome<int, int>>::success(
                evolution::processing::ProcessingOutcome<int, int>::success({}, updated));
        }

        const auto output_id = require_value(evolution::EnvelopeId::from_stable_name(
            "output/" + std::to_string(input.payload()) + '/' + std::to_string(current)));
        auto output_metadata = input.metadata().for_output(
            output_id, input.metadata().processing_time(), std::nullopt);
        std::vector outputs{evolution::Envelope<int>::create(
            input.payload() * static_cast<int>(multiplier.value()), std::move(output_metadata))};
        if (input.payload() == 13) {
            std::vector failures{evolution::Error(
                evolution::ErrorCode::create("processor.partial_branch_failed"),
                evolution::ErrorCategory::Unavailable, "one optional branch failed")};
            return evolution::Result<evolution::processing::ProcessingOutcome<int, int>>::success(
                evolution::processing::ProcessingOutcome<int, int>::partial(
                    std::move(outputs), updated, std::move(failures)));
        }
        return evolution::Result<evolution::processing::ProcessingOutcome<int, int>>::success(
            evolution::processing::ProcessingOutcome<int, int>::success(std::move(outputs),
                                                                        updated));
    }
};

class DurableTestDeduplicationStore final : public evolution::processing::DeduplicationStore {
  public:
    [[nodiscard]] auto is_durable() const noexcept -> bool override {
        return true;
    }

    [[nodiscard]] auto contains(const evolution::processing::IdempotencyKey &key) const
        -> evolution::Result<bool> override {
        const std::scoped_lock lock(mutex_);
        return evolution::Result<bool>::success(completed_.contains(std::string(key.value())));
    }

    auto record_completed(evolution::processing::IdempotencyKey key)
        -> evolution::Result<void> override {
        const std::scoped_lock lock(mutex_);
        completed_.insert(std::string(key.value()));
        return evolution::Result<void>::success();
    }

  private:
    mutable std::mutex mutex_;
    std::unordered_set<std::string> completed_;
};

void test_processor_contract(TestRunner &test) {
    const DeterministicProcessor processor;
    const auto configuration = make_configuration();
    const auto execution_context = make_execution_context();
    const auto input = evolution::Envelope<int>::create(5, make_metadata("input/5", true));
    const auto first =
        require_value(processor.process(input, 10, configuration, execution_context));
    const auto replay =
        require_value(processor.process(input, 10, configuration, execution_context));

    test.expect(first.outcome == evolution::processing::OperationOutcome::Success &&
                    first.outputs.front().payload() == 10 && first.updated_state == 15,
                "Processor contract returns explicit output and updated state");
    test.expect(first.outputs == replay.outputs,
                "Equivalent replay produces deterministic outputs");
    test.expect(first.updated_state == replay.updated_state,
                "Replay preserves deterministic state");

    const auto no_output = require_value(
        processor.process(evolution::Envelope<int>::create(0, make_metadata("input/0")), 2,
                          configuration, execution_context));
    test.expect(no_output.outcome == evolution::processing::OperationOutcome::NoOutput &&
                    no_output.updated_state == 2,
                "Zero output remains a successful operation outcome");

    const auto partial = require_value(
        processor.process(evolution::Envelope<int>::create(13, make_metadata("input/13")), 0,
                          configuration, execution_context));
    test.expect(partial.outcome == evolution::processing::OperationOutcome::PartialSuccess &&
                    partial.outputs.size() == 1 && partial.partial_failures.size() == 1,
                "Partial success preserves outputs and branch failures");

    const auto cancelled =
        require_value(processor.process(input, 10, configuration, make_execution_context(true)));
    test.expect(cancelled.outcome == evolution::processing::OperationOutcome::Cancelled,
                "Cancellation is distinct from processor failure");
    test.expect(processor
                    .process(evolution::Envelope<int>::create(-1, make_metadata("input/negative")),
                             0, configuration, execution_context)
                    .has_error(),
                "Operational processor failure uses Result/Error");
}

void activate(evolution::processing::ProcessorLifecycle &lifecycle) {
    if (lifecycle.configure().has_error() || lifecycle.initialize().has_error() ||
        lifecycle.activate().has_error()) {
        throw std::runtime_error("unable to activate lifecycle in test");
    }
}

void test_lifecycle(TestRunner &test) {
    evolution::processing::ProcessorLifecycle lifecycle;
    test.expect(lifecycle.activate().has_error(),
                "Lifecycle rejects activation before initialization");
    activate(lifecycle);
    test.expect(lifecycle.accepts_work(), "Only ACTIVE lifecycle accepts normal work");
    test.expect(lifecycle.request_stop().has_value(), "Orderly stop request is accepted");
    test.expect(lifecycle.state() == evolution::processing::LifecycleState::Stopping &&
                    lifecycle.termination_mode() == evolution::processing::TerminationMode::Stop,
                "Stop request and completion remain distinct");
    test.expect(lifecycle.request_cancel().has_value(), "Cancellation escalates an orderly stop");
    test.expect(lifecycle.termination_mode() == evolution::processing::TerminationMode::Cancel,
                "Cancellation has stronger termination semantics than stop");
    test.expect(lifecycle.complete_termination().has_value(),
                "Termination completion reaches STOPPED");
    test.expect(lifecycle.request_stop().has_value(),
                "Repeated stop after completion is idempotent");
    test.expect(lifecycle.configure().has_error() && lifecycle.initialize().has_error(),
                "Stopped lifecycle is terminal");

    evolution::processing::ProcessorLifecycle aborted;
    activate(aborted);
    test.expect(aborted.abort().has_value(), "Abort transitions active lifecycle to FAILED");
    test.expect(aborted.state() == evolution::processing::LifecycleState::Failed &&
                    aborted.failure(),
                "Abort records terminal failure information");
    test.expect(aborted.configure().has_error() && aborted.request_stop().has_error(),
                "Failed lifecycle is terminal");

    evolution::processing::ProcessorLifecycle failed;
    test.expect(failed.configure().has_value(), "Configured lifecycle is ready to initialize");
    test.expect(
        failed
            .fail(evolution::Error(evolution::ErrorCode::create("processing.initialization_failed"),
                                   evolution::ErrorCategory::Unavailable, "initialization failed"))
            .has_value(),
        "Unrecoverable lifecycle failure transitions to FAILED");
    test.expect(failed.state() == evolution::processing::LifecycleState::Failed &&
                    failed.failure() && failed.fail(*failed.failure()).has_error(),
                "Lifecycle failure is recorded and FAILED remains terminal");

    evolution::processing::ProcessorLifecycle concurrent;
    activate(concurrent);
    std::thread stop([&concurrent] { static_cast<void>(concurrent.request_stop()); });
    std::thread cancel([&concurrent] { static_cast<void>(concurrent.request_cancel()); });
    std::thread abort([&concurrent] { static_cast<void>(concurrent.abort()); });
    stop.join();
    cancel.join();
    abort.join();
    test.expect(concurrent.state() == evolution::processing::LifecycleState::Failed &&
                    concurrent.termination_mode() == evolution::processing::TerminationMode::Abort,
                "Concurrent termination deterministically gives abort highest precedence");
}

void test_concurrency(TestRunner &test) {
    const auto serial = evolution::processing::ConcurrencyContract::serial();
    test.expect(!serial.may_run_concurrently(std::nullopt, std::nullopt),
                "Serial is the safe default concurrency mode");
    test.expect(evolution::processing::ConcurrencyContract::create(
                    evolution::processing::ConcurrencyMode::Concurrent, 4,
                    evolution::processing::OrderingRequirement::Arrival,
                    evolution::processing::StateOwnership::Stateless)
                    .has_error(),
                "Concurrent processors cannot silently promise arrival ordering");
    const auto partitioned = require_value(evolution::processing::ConcurrencyContract::create(
        evolution::processing::ConcurrencyMode::Partitioned, 4,
        evolution::processing::OrderingRequirement::Partition,
        evolution::processing::StateOwnership::PartitionOwned, "partition_key"));
    test.expect(partitioned.may_run_concurrently("a", "b") &&
                    !partitioned.may_run_concurrently("a", "a"),
                "Partitioned concurrency isolates same-partition work");
}

void test_queue(TestRunner &test) {
    using evolution::processing::AdmissionDecision;
    using evolution::processing::AdmissionPolicy;
    using evolution::processing::QueueItem;

    auto queue = require_value(evolution::processing::BoundedQueue<std::string>::create({2, 8, 4}));
    test.expect(
        queue->admit(QueueItem<std::string>{"one", 3, 1}, AdmissionPolicy::reject()).decision ==
            AdmissionDecision::Accepted,
        "Queue accepts work within item/byte/weight capacity");
    test.expect(
        queue->admit(QueueItem<std::string>{"two", 3, 1}, AdmissionPolicy::reject()).decision ==
            AdmissionDecision::Accepted,
        "Queue accepts until bounded capacity");
    auto rejected = queue->admit(QueueItem<std::string>{"three", 3, 1}, AdmissionPolicy::reject());
    test.expect(rejected.decision == AdmissionDecision::Rejected && rejected.returned_item &&
                    rejected.returned_item->value == "three",
                "Rejected admission returns ownership to the producer");
    const auto pressure = queue->pressure();
    test.expect(pressure.items == 2 && pressure.item_ratio == 1.0,
                "Queue pressure exposes bounded occupancy");
    const auto first = queue->try_pop();
    test.expect(first && first->value == "one", "FIFO queue preserves declared queue ordering");

    auto drop_queue = require_value(evolution::processing::BoundedQueue<int>::create({1, 8, 2}));
    static_cast<void>(drop_queue->admit({1, 4, 1}, AdmissionPolicy::reject()));
    test.expect(drop_queue->admit({2, 4, 1}, AdmissionPolicy::drop()).decision ==
                    AdmissionDecision::Dropped,
                "Drop is an explicit lossy outcome, not success");

    auto sample_queue = require_value(evolution::processing::BoundedQueue<int>::create({2, 8, 2}));
    const auto sample_policy = require_value(AdmissionPolicy::sample(2));
    test.expect(
        sample_queue->admit({1, 1, 1}, sample_policy).decision == AdmissionDecision::SampledOut &&
            sample_queue->admit({2, 1, 1}, sample_policy).decision == AdmissionDecision::Accepted,
        "Sampling is explicit and deterministic");

    std::optional<std::string> spilled_value;
    auto spill_queue = require_value(evolution::processing::BoundedQueue<std::string>::create(
        {1, 4, 1}, evolution::processing::QueueOrdering::Fifo,
        [&spilled_value](const QueueItem<std::string> &item) {
            spilled_value = item.value;
            return evolution::Result<void>::success();
        }));
    static_cast<void>(spill_queue->admit({"full", 4, 1}, AdmissionPolicy::reject()));
    test.expect(spill_queue->admit({"spill", 4, 1}, AdmissionPolicy::spill()).decision ==
                        AdmissionDecision::Spilled &&
                    spilled_value == "spill",
                "Spill is explicit and delegated to a persistence hook");

    auto wait_queue = require_value(evolution::processing::BoundedQueue<int>::create({1, 4, 1}));
    static_cast<void>(wait_queue->admit({1, 4, 1}, AdmissionPolicy::reject()));
    evolution::CancellationSource cancellation;
    auto waiting = std::async(std::launch::async, [&] {
        return wait_queue->admit({2, 4, 1}, require_value(AdmissionPolicy::wait(500ms)),
                                 cancellation.token());
    });
    std::this_thread::sleep_for(15ms);
    cancellation.cancel();
    const auto cancelled = waiting.get();
    test.expect(cancelled.decision == AdmissionDecision::Cancelled && cancelled.returned_item,
                "Blocking admission is cancellable and returns ownership");

    const auto retained = queue->close(evolution::processing::QueueCloseMode::Drain);
    test.expect(retained.empty(), "Drain close retains already-admitted work");
    const auto drained = queue->drain();
    test.expect(drained.size() == 1 && drained.front().value == "two",
                "Orderly drain returns work");

    auto discard_queue = require_value(evolution::processing::BoundedQueue<int>::create({2, 8, 2}));
    static_cast<void>(discard_queue->admit({1, 1, 1}, AdmissionPolicy::reject()));
    static_cast<void>(discard_queue->admit({2, 1, 1}, AdmissionPolicy::reject()));
    const auto discarded = discard_queue->close(evolution::processing::QueueCloseMode::Discard);
    test.expect(discarded.size() == 2, "Cancellation-style close explicitly discards queued work");
}

void test_delivery(TestRunner &test) {
    using namespace evolution::processing;
    test.expect(DeliveryContract::create(DeliveryGuarantee::ExactlyOnce,
                                         AcknowledgementPoint::Execution, IdempotencyMode::None, 2)
                    .has_error(),
                "Exactly-once cannot be declared without an explicit mechanism");

    const auto at_least_once = require_value(DeliveryContract::create(
        DeliveryGuarantee::AtLeastOnce, AcknowledgementPoint::Execution, IdempotencyMode::None, 3));
    const auto unguarded = require_value(DeliveryGuard::create(at_least_once));
    test.expect(require_value(unguarded.begin(std::nullopt)) == DeliveryDecision::Execute &&
                    require_value(unguarded.begin(std::nullopt)) == DeliveryDecision::Execute,
                "At-least-once permits duplicate execution when no idempotency contract exists");

    const auto exactly_once = require_value(DeliveryContract::create(
        DeliveryGuarantee::ExactlyOnce, AcknowledgementPoint::DurableCompletion,
        IdempotencyMode::Keyed, 3, ExactlyOnceMechanism::IdempotentEffectWithDurableDeduplication));
    const auto naturally_idempotent_exactly_once = require_value(DeliveryContract::create(
        DeliveryGuarantee::ExactlyOnce, AcknowledgementPoint::DurableCompletion,
        IdempotencyMode::Natural, 3,
        ExactlyOnceMechanism::IdempotentEffectWithDurableDeduplication));
    test.expect(DeliveryGuard::create(naturally_idempotent_exactly_once).has_error(),
                "Durable deduplication exactly-once requires a store for natural idempotency");
    test.expect(DeliveryGuard::create(exactly_once, std::make_shared<InMemoryDeduplicationStore>())
                    .has_error(),
                "Volatile deduplication cannot satisfy durable exactly-once completion");
    auto store = std::make_shared<DurableTestDeduplicationStore>();
    const auto guarded = require_value(DeliveryGuard::create(exactly_once, store));
    const auto key = require_value(IdempotencyKey::create("effect/42"));
    test.expect(require_value(guarded.begin(key)) == DeliveryDecision::Execute,
                "New idempotency key is executable");
    test.expect(guarded.complete(key).has_value(), "Durable completion records deduplication key");
    test.expect(require_value(guarded.begin(key)) == DeliveryDecision::AlreadyCompleted,
                "Completed keyed effect is deduplicated");

    const auto at_most_once = require_value(DeliveryContract::create(
        DeliveryGuarantee::AtMostOnce, AcknowledgementPoint::Admission, IdempotencyMode::None, 1));
    test.expect(at_most_once.maximum_attempts() == 1, "At-most-once forbids retries");
}

} // namespace

int main() {
    TestRunner test;
    test_processor_contract(test);
    test_lifecycle(test);
    test_concurrency(test);
    test_queue(test);
    test_delivery(test);

    if (test.failures() != 0) {
        std::cerr << test.failures() << " processing contract test(s) failed\n";
        return 1;
    }
    return 0;
}
