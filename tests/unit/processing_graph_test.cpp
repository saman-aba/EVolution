#include "evolution/processing/execution.hpp"
#include "evolution/processing/graph.hpp"
#include "evolution/processing/observability.hpp"
#include "evolution/processing/resources.hpp"
#include "evolution/processing/scheduler.hpp"
#include "evolution/processing/validation.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {

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

auto requirements(std::uint64_t slots = 1, int priority = 0,
                  std::optional<std::string> fairness_group = std::nullopt)
    -> evolution::processing::ResourceRequirements {
    using evolution::ResourceKind;
    using evolution::processing::ResourceRange;
    using evolution::processing::ResourceRequirements;
    return require_value(ResourceRequirements::create(
        {{ResourceKind::ExecutionSlots, ResourceRange{slots, slots, slots}}}, priority,
        std::move(fairness_group)));
}

auto resource_snapshot(std::uint64_t slots) -> evolution::processing::ResourceSnapshot {
    using evolution::ResourceKind;
    return {{{ResourceKind::ExecutionSlots, slots}},
            {{ResourceKind::ExecutionSlots, 0}},
            {{ResourceKind::ExecutionSlots, slots}}};
}

struct GraphOptions {
    bool duplicate_node{};
    bool incompatible_type{};
    bool cycle{};
    bool allow_cycle{};
    bool ordering_mismatch{};
    bool temporal_mismatch{};
    bool missing_provenance{};
    bool unbounded{};
    bool missing_configuration{};
};

auto make_graph(const GraphOptions &options = {}) -> evolution::processing::GraphDefinition {
    using namespace evolution;
    using namespace evolution::processing;

    const auto source_node_id = stable_id<NodeId>("node/source");
    const auto destination_node_id = stable_id<NodeId>("node/destination");
    const auto source_input_id = stable_id<PortId>("port/source/input");
    const auto source_output_id = stable_id<PortId>("port/source/output");
    const auto destination_input_id = stable_id<PortId>("port/destination/input");
    const auto destination_output_id = stable_id<PortId>("port/destination/output");

    const auto source_input = PortDefinition{
        source_input_id,
        "input",
        PortDirection::Input,
        PortCardinality::Multiple,
        "evolution.test.record",
        "1",
        OrderingRequirement::Arrival,
        TemporalSemantics::EventTime,
        true,
        true,
        true,
    };
    auto source_output = PortDefinition{
        source_output_id,
        "output",
        PortDirection::Output,
        PortCardinality::Multiple,
        "evolution.test.record",
        "1",
        OrderingRequirement::Arrival,
        TemporalSemantics::EventTime,
        false,
        true,
        !options.missing_provenance,
    };
    auto destination_input = PortDefinition{
        destination_input_id,
        "input",
        PortDirection::Input,
        PortCardinality::Single,
        options.incompatible_type ? "evolution.test.other" : "evolution.test.record",
        "1",
        OrderingRequirement::Arrival,
        options.temporal_mismatch ? TemporalSemantics::ProcessingTime
                                  : TemporalSemantics::EventTime,
        true,
        true,
        true,
    };
    const auto destination_output = PortDefinition{
        destination_output_id,
        "output",
        PortDirection::Output,
        PortCardinality::Multiple,
        "evolution.test.record",
        "1",
        OrderingRequirement::Arrival,
        TemporalSemantics::EventTime,
        false,
        true,
        true,
    };

    const auto configuration =
        ConfigurationBinding{stable_id<ConfigurationId>("configuration/phase4"), "processors"};
    std::vector<NodeDefinition> nodes{
        NodeDefinition{source_node_id,
                       stable_id<ProcessorId>("processor/source"),
                       "1.0.0",
                       configuration,
                       true,
                       ConcurrencyContract::serial(),
                       requirements(),
                       {source_input, source_output}},
        NodeDefinition{destination_node_id,
                       stable_id<ProcessorId>("processor/destination"),
                       "1.0.0",
                       options.missing_configuration
                           ? std::optional<ConfigurationBinding>{}
                           : std::optional<ConfigurationBinding>{configuration},
                       true,
                       ConcurrencyContract::serial(),
                       requirements(),
                       {destination_input, destination_output}},
    };
    if (options.duplicate_node) {
        nodes.push_back(nodes.front());
    }

    const auto capacity = options.unbounded ? QueueCapacity{0, 0, 0} : QueueCapacity{8, 4096, 8};
    std::vector<ConnectionDefinition> connections{
        ConnectionDefinition{stable_id<ConnectionId>("connection/source-destination"),
                             source_node_id, source_output_id, destination_node_id,
                             destination_input_id, capacity, AdmissionPolicyKind::Reject,
                             options.ordering_mismatch ? OrderingRequirement::Unordered
                                                       : OrderingRequirement::Arrival,
                             FanOutDelivery::Independent},
    };
    if (options.cycle) {
        connections.push_back(ConnectionDefinition{
            stable_id<ConnectionId>("connection/destination-source"),
            destination_node_id,
            destination_output_id,
            source_node_id,
            source_input_id,
            {8, 4096, 8},
            AdmissionPolicyKind::Reject,
            OrderingRequirement::Arrival,
            FanOutDelivery::Independent,
        });
    }

    return require_value(GraphDefinition::create(
        stable_id<GraphId>("graph/phase4"), "1.0.0", std::move(nodes), std::move(connections),
        {{"input", source_node_id, source_input_id, true}},
        {{"output", destination_node_id, destination_output_id, true}},
        CycleContract{options.allow_cycle, options.allow_cycle
                                               ? std::optional<std::string>{"bounded iterations"}
                                               : std::nullopt}));
}

void test_graph_validation(TestRunner &test) {
    using namespace evolution::processing;
    const auto graph = make_graph();
    test.expect(GraphValidator::validate_static(graph).has_value(),
                "Valid graph satisfies structural and semantic contracts");
    test.expect(GraphValidator::validate_initialization(
                    graph, InitializationValidationContext{resource_snapshot(2)})
                    .has_value(),
                "Initialization validation checks configuration and resources");

    const auto destination_node = stable_id<NodeId>("node/destination");
    const auto destination_input = stable_id<PortId>("port/destination/input");
    test.expect(GraphValidator::validate_runtime(
                    graph, RuntimeValidationContext{destination_node, destination_input,
                                                    LifecycleState::Active, true, true,
                                                    std::nullopt, resource_snapshot(1)})
                    .has_value(),
                "Runtime validation accepts active work with required metadata and resources");
    test.expect(GraphValidator::validate_runtime(
                    graph, RuntimeValidationContext{destination_node, destination_input,
                                                    LifecycleState::Stopping, true, true,
                                                    std::nullopt, resource_snapshot(1)})
                    .has_error(),
                "Runtime validation rejects work after lifecycle admission closes");
    test.expect(GraphValidator::validate_runtime(
                    graph, RuntimeValidationContext{destination_node, destination_input,
                                                    LifecycleState::Active, true, false,
                                                    std::nullopt, resource_snapshot(1)})
                    .has_error(),
                "Runtime validation enforces provenance requirements");

    test.expect(GraphValidator::validate_static(make_graph({.duplicate_node = true})).has_error(),
                "Static validation rejects duplicate node identity");
    test.expect(
        GraphValidator::validate_static(make_graph({.incompatible_type = true})).has_error(),
        "Static validation rejects incompatible semantic types");
    test.expect(
        GraphValidator::validate_static(make_graph({.ordering_mismatch = true})).has_error(),
        "Static validation rejects ordering and concurrency mismatches");
    test.expect(
        GraphValidator::validate_static(make_graph({.temporal_mismatch = true})).has_error(),
        "Static validation rejects temporal semantic changes");
    test.expect(
        GraphValidator::validate_static(make_graph({.missing_provenance = true})).has_error(),
        "Static validation rejects lost provenance");
    test.expect(GraphValidator::validate_static(make_graph({.unbounded = true})).has_error(),
                "Static validation rejects implicit unbounded admission");
    test.expect(GraphValidator::validate_static(make_graph({.cycle = true})).has_error(),
                "Static validation enforces the acyclic default");
    test.expect(GraphValidator::validate_static(make_graph({.cycle = true, .allow_cycle = true}))
                    .has_value(),
                "Explicit cycle termination semantics permit a cycle");
    test.expect(GraphValidator::validate_initialization(
                    make_graph({.missing_configuration = true}),
                    InitializationValidationContext{resource_snapshot(2)})
                    .has_error(),
                "Initialization validation rejects missing effective configuration");
    test.expect(GraphValidator::validate_initialization(
                    graph, InitializationValidationContext{resource_snapshot(0)})
                    .has_error(),
                "Initialization validation reports resource exhaustion");
}

void test_resources(TestRunner &test) {
    using namespace evolution;
    using namespace evolution::processing;
    auto pool = require_value(ResourcePool::create({{ResourceKind::ExecutionSlots, 1}}));
    {
        auto reservation = require_value(pool.reserve(requirements()));
        test.expect(reservation.active() &&
                        pool.snapshot().available.at(ResourceKind::ExecutionSlots) == 0,
                    "Resource reservation accounts for active capacity");
        test.expect(pool.reserve(requirements()).has_error(),
                    "Resource exhaustion is an explicit outcome");
    }
    test.expect(pool.reserve(requirements()).has_value(),
                "RAII reservation release restores resource availability");
    test.expect(
        ResourceRequirements::create({{ResourceKind::ExecutionSlots, ResourceRange{2, 1, 3}}})
            .has_error(),
        "Invalid resource ranges are rejected");
}

auto scheduling_request(const evolution::processing::WorkId &work_id,
                        const evolution::processing::NodeId &node_id,
                        evolution::processing::ResourceRequirements work_resources,
                        std::vector<evolution::processing::WorkId> dependencies = {})
    -> evolution::processing::SchedulingRequest {
    using namespace evolution::processing;
    return SchedulingRequest{work_id,
                             node_id,
                             std::move(dependencies),
                             true,
                             false,
                             LifecycleState::Active,
                             ConcurrencyContract::serial(),
                             1,
                             std::nullopt,
                             std::move(work_resources)};
}

void test_scheduler(TestRunner &test) {
    using namespace evolution::processing;
    DeterministicScheduler scheduler;
    const auto node = stable_id<NodeId>("node/scheduler");
    const auto dependency = stable_id<WorkId>("work/dependency");
    const auto work = stable_id<WorkId>("work/main");
    auto request = scheduling_request(work, node, requirements(), {dependency});
    auto snapshot = SchedulingSnapshot{{}, {}, {}, {{node, 1}}, resource_snapshot(1)};

    test.expect(scheduler.evaluate(request, snapshot) == EligibilityDecision::DependencyBlocked,
                "Scheduler waits for graph dependencies");
    snapshot.completed.insert(dependency);
    test.expect(scheduler.evaluate(request, snapshot) == EligibilityDecision::Eligible,
                "Scheduler makes admitted active work eligible independently of execution");
    snapshot.running_by_node[node] = 1;
    test.expect(scheduler.evaluate(request, snapshot) == EligibilityDecision::ConcurrencyBlocked,
                "Scheduler respects processor concurrency limits");
    snapshot.running_by_node.clear();
    snapshot.resources = resource_snapshot(0);
    test.expect(scheduler.evaluate(request, snapshot) == EligibilityDecision::ResourceBlocked,
                "Scheduler accounts for current resource availability");
    snapshot.resources = resource_snapshot(1);
    request.sequence = 2;
    test.expect(scheduler.evaluate(request, snapshot) == EligibilityDecision::OrderingBlocked,
                "Scheduler preserves declared ordering");

    const auto lower =
        scheduling_request(stable_id<WorkId>("work/lower"), node, requirements(1, 1, "fair"));
    const auto higher =
        scheduling_request(stable_id<WorkId>("work/higher"), node, requirements(1, 10, "fair"));
    const auto ranked = scheduler.rank_eligible({lower, higher}, snapshot);
    test.expect(ranked.size() == 2 && ranked.front() == higher.work_id,
                "Deterministic scheduling exposes priority and fairness hooks");
}

auto execution_context() -> evolution::ExecutionContext {
    return evolution::ExecutionContext::create(evolution::ExecutionMode::Test,
                                               stable_id<evolution::RunId>("run/phase4"));
}

auto executable(const std::string &name, evolution::processing::ResourceRequirements resources,
                evolution::processing::ExecutionOperation operation)
    -> evolution::processing::ExecutableWork {
    using namespace evolution::processing;
    return ExecutableWork{stable_id<WorkId>("work/" + name), stable_id<NodeId>("node/backend"),
                          execution_context(), std::move(resources), std::move(operation)};
}

void test_execution_backend(TestRunner &test) {
    using namespace evolution;
    using namespace evolution::processing;
    auto pool = require_value(ResourcePool::create({{ResourceKind::ExecutionSlots, 1}}));
    auto backend = require_value(DeterministicSingleThreadBackend::create(1, pool));

    auto success_work = executable("success", requirements(),
                                   [](const ExecutionContext &, const CancellationToken &) {
                                       return Result<void>::success();
                                   });
    const auto cancelled_id = require_value(backend.submit(success_work));
    test.expect(backend.submit(success_work).has_error(),
                "Backend submission is bounded independently from scheduling");
    test.expect(backend.request_cancel(cancelled_id).has_value(),
                "Backend supports cooperative cancellation requests");
    const auto cancelled = require_value(backend.run_next());
    test.expect(cancelled && cancelled->outcome == ExecutionOutcome::Cancelled,
                "Cancellation completion is distinct from failure");

    static_cast<void>(backend.submit(executable(
        "success-run", requirements(), [](const ExecutionContext &, const CancellationToken &) {
            return Result<void>::success();
        })));
    const auto completed = require_value(backend.run_next());
    test.expect(completed && completed->outcome == ExecutionOutcome::Success,
                "Deterministic backend executes one scheduler-approved operation at a time");

    static_cast<void>(backend.submit(executable(
        "operation-failure", requirements(),
        [](const ExecutionContext &, const CancellationToken &) {
            return Result<void>::failure(Error(ErrorCode::create("test.operation_failure"),
                                               ErrorCategory::Unavailable, "operation failed"));
        })));
    const auto operation_failure = require_value(backend.run_next());
    test.expect(operation_failure &&
                    operation_failure->outcome == ExecutionOutcome::OperationFailure,
                "Processor operation failure remains distinct from backend failure");

    static_cast<void>(backend.submit(
        executable("backend-failure", requirements(),
                   [](const ExecutionContext &, const CancellationToken &) -> Result<void> {
                       throw std::runtime_error("executor boundary failed");
                   })));
    const auto backend_failure = require_value(backend.run_next());
    test.expect(backend_failure && backend_failure->outcome == ExecutionOutcome::BackendFailure,
                "Execution boundary exceptions become explicit backend failures");

    static_cast<void>(backend.submit(executable(
        "resource-race", requirements(2), [](const ExecutionContext &, const CancellationToken &) {
            return Result<void>::success();
        })));
    const auto exhausted = require_value(backend.run_next());
    test.expect(exhausted && exhausted->outcome == ExecutionOutcome::ResourceExhausted,
                "Backend reports resource exhaustion if availability changes after scheduling");

    test.expect(backend.shutdown().has_value() && backend.submit(success_work).has_error(),
                "Backend shutdown prevents new submission without implying successful execution");
}

class CollectingSink final : public evolution::processing::ObservabilitySink {
  public:
    explicit CollectingSink(bool fail = false) : fail_(fail) {}

    auto emit(const evolution::processing::TelemetryRecord &record)
        -> evolution::Result<void> override {
        if (fail_) {
            return evolution::Result<void>::failure(evolution::Error(
                evolution::ErrorCode::create("test.telemetry_failure"),
                evolution::ErrorCategory::Unavailable, "telemetry sink unavailable"));
        }
        records.push_back(record);
        return evolution::Result<void>::success();
    }

    std::vector<evolution::processing::TelemetryRecord> records;

  private:
    bool fail_{};
};

void test_observability(TestRunner &test) {
    using namespace evolution;
    using namespace evolution::processing;
    const auto component = stable_id<ComponentId>("component/phase4");
    auto sink = std::make_shared<CollectingSink>();
    auto channel = require_value(ObservabilityChannel::create(5, sink));
    const auto now = std::chrono::system_clock::now();

    test.expect(channel.publish(LogRecord{now,
                                          LogSeverity::Info,
                                          component,
                                          "configured",
                                          {{"token", "secret", FieldSensitivity::Sensitive}},
                                          stable_id<CorrelationId>("correlation/phase4")}) ==
                    TelemetryDisposition::Accepted,
                "Structured logs enter a bounded operational channel");
    static_cast<void>(
        channel.publish(OperationalMetric{now, component, "queue_depth", 1.0, "items", {}}));
    static_cast<void>(channel.publish(TraceSpan{stable_id<TraceId>("trace/phase4"),
                                                stable_id<SpanId>("span/phase4"),
                                                std::nullopt,
                                                component,
                                                "execute",
                                                std::chrono::nanoseconds(5),
                                                {}}));
    static_cast<void>(channel.publish(HealthRecord{component, HealthState::Healthy, "ready"}));
    static_cast<void>(channel.publish(StatusRecord{component, "ACTIVE", {}}));
    const auto flushed = channel.flush();
    test.expect(flushed.emitted == 5 && sink->records.size() == 5,
                "Logs, metrics, traces, health, and status use replaceable sinks");
    const auto &log = std::get<LogRecord>(sink->records.front());
    test.expect(log.fields.front().value == "[redacted]",
                "Sensitive observability fields are redacted before reaching a sink");

    auto bounded = require_value(ObservabilityChannel::create(1, sink));
    static_cast<void>(bounded.publish(HealthRecord{component, HealthState::Healthy, "ready"}));
    test.expect(bounded.publish(StatusRecord{component, "ACTIVE", {}}) ==
                        TelemetryDisposition::Dropped &&
                    bounded.stats().dropped == 1,
                "Telemetry pressure is bounded and loss is explicit");

    auto failing_sink = std::make_shared<CollectingSink>(true);
    auto failing = require_value(ObservabilityChannel::create(1, failing_sink));
    static_cast<void>(failing.publish(HealthRecord{component, HealthState::Unknown, "unknown"}));
    test.expect(failing.flush().failed == 1,
                "Telemetry sink failure is observable but does not become processing failure");
}

} // namespace

int main() {
    TestRunner test;
    test_graph_validation(test);
    test_resources(test);
    test_scheduler(test);
    test_execution_backend(test);
    test_observability(test);

    if (test.failures() != 0) {
        std::cerr << test.failures() << " processing graph test(s) failed\n";
        return 1;
    }
    return 0;
}
