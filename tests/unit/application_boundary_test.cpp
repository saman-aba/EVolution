#include "evolution/applications/lifecycle.hpp"
#include "evolution/applications/operations.hpp"
#include "evolution/core/configuration/configuration.hpp"
#include "evolution/ingestion/ingestion.hpp"
#include "evolution/interfaces/interfaces.hpp"
#include "evolution/security/security.hpp"

#include <chrono>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;
namespace app = evolution::applications;
namespace ingest = evolution::ingestion;
namespace iface = evolution::interfaces;
namespace security = evolution::security;

static_assert(!std::copy_constructible<security::SecretValue>);
static_assert(std::move_constructible<security::SecretValue>);

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

template <typename Id> auto stable(std::string_view name) -> Id {
    return require_value(Id::from_stable_name(name));
}

auto execution(std::optional<evolution::time::MonotonicTimePoint> deadline = std::nullopt)
    -> evolution::ExecutionContext {
    return evolution::ExecutionContext::create(evolution::ExecutionMode::Test,
                                               stable<evolution::RunId>("phase9-run"), std::nullopt,
                                               nullptr, nullptr, std::nullopt, deadline);
}

auto security_context(std::string tenant = "tenant-a") -> security::SecurityContext {
    return {{stable<security::PrincipalId>("principal-a"), "user", "Alice", std::move(tenant), {}},
            security::TrustLevel::Authenticated,
            {"operate"},
            evolution::time::TemporalInformation::unknown(),
            std::nullopt};
}

class Component final : public app::ApplicationComponent {
  public:
    Component(std::string name, std::vector<evolution::ComponentId> dependencies, bool required,
              std::vector<std::string> &calls, bool fail_start = false)
        : descriptor_{stable<evolution::ComponentId>(name), std::move(name),
                      std::move(dependencies), required},
          calls_(calls), fail_start_(fail_start) {}

    [[nodiscard]] auto descriptor() const noexcept
        -> const app::ApplicationComponentDescriptor & override {
        return descriptor_;
    }

    auto configure(const evolution::Configuration &) -> evolution::Result<void> override {
        calls_.push_back("configure:" + descriptor_.name);
        state_ = app::ComponentOperationalState::Configured;
        return evolution::Result<void>::success();
    }

    auto start(const evolution::ExecutionContext &) -> evolution::Result<void> override {
        calls_.push_back("start:" + descriptor_.name);
        if (fail_start_) {
            state_ = app::ComponentOperationalState::Failed;
            return evolution::Result<void>::failure(
                evolution::Error(evolution::ErrorCode::create("test.start_failed"),
                                 evolution::ErrorCategory::Unavailable, "start failed"));
        }
        state_ = app::ComponentOperationalState::Running;
        return evolution::Result<void>::success();
    }

    auto stop() -> evolution::Result<void> override {
        calls_.push_back("stop:" + descriptor_.name);
        state_ = app::ComponentOperationalState::Stopped;
        return evolution::Result<void>::success();
    }

    [[nodiscard]] auto status() const -> app::ComponentOperationalStatus override {
        return {descriptor_.id, state_, std::nullopt};
    }

  private:
    app::ApplicationComponentDescriptor descriptor_;
    std::vector<std::string> &calls_;
    bool fail_start_{};
    app::ComponentOperationalState state_{app::ComponentOperationalState::Created};
};

void test_application_lifecycle(TestRunner &test) {
    std::vector<std::string> calls;
    auto first =
        std::make_shared<Component>("first", std::vector<evolution::ComponentId>{}, true, calls);
    auto second = std::make_shared<Component>(
        "second", std::vector<evolution::ComponentId>{first->descriptor().id}, true, calls);
    auto configuration = require_value(evolution::Configuration::create({}, "phase9"));
    auto application = require_value(app::Application::create(
        {stable<app::ApplicationId>("application"), "application", "1", false}, configuration,
        {second, first}));
    test.expect(application.start(execution()).has_value(), "application starts dependencies");
    test.expect(calls == std::vector<std::string>{"configure:first", "start:first",
                                                  "configure:second", "start:second"},
                "application uses dependency startup order");
    require_value(application.shutdown());
    test.expect(calls[calls.size() - 2] == "stop:second" && calls.back() == "stop:first",
                "application shuts down started components in reverse order");

    calls.clear();
    auto required =
        std::make_shared<Component>("required", std::vector<evolution::ComponentId>{}, true, calls);
    auto failing = std::make_shared<Component>(
        "failing", std::vector<evolution::ComponentId>{required->descriptor().id}, true, calls,
        true);
    auto failed = require_value(
        app::Application::create({stable<app::ApplicationId>("failed-app"), "failed", "1", false},
                                 configuration, {required, failing}));
    test.expect(failed.start(execution()).has_error() && calls.back() == "stop:required",
                "required startup failure rolls back started dependencies");

    calls.clear();
    auto optional = std::make_shared<Component>("optional", std::vector<evolution::ComponentId>{},
                                                false, calls, true);
    auto healthy =
        std::make_shared<Component>("healthy", std::vector<evolution::ComponentId>{}, true, calls);
    auto degraded = require_value(app::Application::create(
        {stable<app::ApplicationId>("degraded-app"), "degraded", "1", true}, configuration,
        {optional, healthy}));
    require_value(degraded.start(execution()));
    test.expect(degraded.status().state == app::ApplicationState::Degraded,
                "optional failure produces explicit degraded status");
    require_value(degraded.shutdown());
    test.expect(calls.back() == "stop:healthy",
                "shutdown does not stop an optional component that never started");
}

class Authorizer final : public security::Authorizer {
  public:
    explicit Authorizer(bool allowed = true) : allowed_(allowed) {}
    [[nodiscard]] auto authorize(const security::SecurityContext &context,
                                 const security::AuthorizationRequest &request) const
        -> evolution::Result<security::AuthorizationDecision> override {
        ++calls;
        const bool same_tenant = !request.tenant || request.tenant == context.principal.tenant;
        return evolution::Result<security::AuthorizationDecision>::success(
            {allowed_ && same_tenant ? security::AuthorizationOutcome::Allowed
                                     : security::AuthorizationOutcome::Denied,
             "test-policy",
             allowed_ && same_tenant ? "allowed" : "denied",
             {}});
    }
    mutable int calls{};

  private:
    bool allowed_;
};

class Audit final : public security::AuditSink {
  public:
    auto record(security::AuditEvent event) -> evolution::Result<void> override {
        events.push_back(std::move(event));
        return evolution::Result<void>::success();
    }
    std::vector<security::AuditEvent> events;
};

class CommandHandler final : public app::CommandHandler<int, int> {
  public:
    auto execute(const int &input, const app::OperationContext &) const
        -> evolution::Result<app::CommandExecution<int>> override {
        ++calls;
        return evolution::Result<app::CommandExecution<int>>::success(
            {app::OperationCompletion::Completed,
             input * 2,
             {{"event", "event-1", "at-least-once"}}});
    }
    mutable int calls{};
};

class QueryHandler final : public app::QueryHandler<int, int> {
  public:
    [[nodiscard]] auto execute(const int &input,
                               const evolution::storage::QueryConsistencyRequest &consistency,
                               const app::OperationContext &) const
        -> evolution::Result<app::QueryExecution<int>> override {
        observed = consistency.mode;
        return evolution::Result<app::QueryExecution<int>>::success(
            {app::OperationCompletion::Completed, input + 1, "snapshot-7"});
    }
    mutable evolution::storage::QueryConsistency observed{
        evolution::storage::QueryConsistency::Latest};
};

auto operation(app::OperationKind kind, bool idempotent = false) -> app::OperationDescriptor {
    return require_value(app::OperationDescriptor::create(
        {kind == app::OperationKind::Command ? "double" : "lookup", "1", kind, "operate",
         idempotent, kind == app::OperationKind::Command}));
}

auto operation_context(evolution::ExecutionContext context = execution()) -> app::OperationContext {
    return {stable<app::RequestId>("request-a"),
            stable<evolution::CorrelationId>("correlation-a"),
            security_context(),
            std::move(context),
            8,
            1};
}

void test_security_and_operations(TestRunner &test) {
    auto secret = require_value(security::SecretValue::create("secret-value"));
    test.expect(secret.reveal() == "secret-value", "secret values are explicit move-only values");
    const auto redacted = security::Redactor::redact(
        {{"public", "shown", security::SensitiveClassification::Public},
         {"secret", "hidden", security::SensitiveClassification::Secret}},
        security::SensitiveClassification::Internal);
    test.expect(redacted.at("public") == "shown" && redacted.at("secret") == "<redacted>",
                "redaction follows sensitivity classification");

    auto protector = require_value(security::ResourceProtector::create({16, 2, 1}));
    auto lease = protector.admit({8, 1});
    test.expect(lease.has_value() && protector.admit({8, 1}).has_error(),
                "resource protection enforces concurrent operation limits");
    lease = evolution::Result<security::ResourceLease>::failure(evolution::Error(
        evolution::ErrorCode::create("released"), evolution::ErrorCategory::Cancelled, "released"));
    test.expect(protector.admit({17, 1}).has_error(),
                "resource protection enforces request-size limits");

    Authorizer authorizer;
    Audit audit;
    CommandHandler handler;
    app::InMemoryIdempotencyStore<int> idempotency;
    app::CommandExecutor<int, int> executor(authorizer, audit, protector, handler, idempotency);
    auto key = require_value(app::IdempotencyKey::create("same-command"));
    auto first = executor.execute({operation(app::OperationKind::Command, true), "tenant-a/item", 4,
                                   operation_context(), key});
    auto second = executor.execute({operation(app::OperationKind::Command, true), "tenant-a/item",
                                    4, operation_context(), key});
    test.expect(first && second && handler.calls == 1 && second.value()->idempotent_replay,
                "idempotent command replay does not repeat side effects");
    test.expect(audit.events.size() == 1 && audit.events.front().data().safe_details.empty(),
                "successful commands emit structured audit events without secret details");

    Authorizer denied(false);
    app::CommandExecutor<int, int> denied_executor(denied, audit, protector, handler, idempotency);
    test.expect(denied_executor
                        .execute({operation(app::OperationKind::Command), "item", 1,
                                  operation_context(), std::nullopt})
                        .has_error() &&
                    handler.calls == 1,
                "authorization denial occurs before command effects");

    QueryHandler query_handler;
    app::QueryExecutor<int, int> query_executor(authorizer, protector, query_handler);
    evolution::storage::QueryConsistencyRequest consistency{
        evolution::storage::QueryConsistency::Snapshot, "snapshot-7", std::nullopt, std::nullopt};
    auto queried = query_executor.execute(
        {operation(app::OperationKind::Query), "item", 5, operation_context(), consistency});
    test.expect(queried && queried.value().output == 6 &&
                    query_handler.observed == evolution::storage::QueryConsistency::Snapshot,
                "queries propagate explicit consistency without semantic side effects");

    auto expired = execution(evolution::time::MonotonicClock::now() - 1ms);
    test.expect(query_executor
                    .execute({operation(app::OperationKind::Query), "item", 5,
                              operation_context(expired), consistency})
                    .has_error(),
                "operation executors reject expired deadlines");
}

class Acquirer final : public ingest::Acquirer<std::string> {
  public:
    auto acquire(const std::string &request, const evolution::ExecutionContext &) const
        -> evolution::Result<ingest::RawRecord> override {
        std::vector<std::byte> bytes;
        for (const char value : request) {
            bytes.push_back(static_cast<std::byte>(value));
        }
        return ingest::RawRecord::create(
            {stable<evolution::SourceId>("source"), stable<ingest::SourceRecordId>(request),
             "text/plain", std::move(bytes), evolution::time::TemporalInformation::unknown(),
             security::TrustLevel::Untrusted, "source-v1"});
    }
};

class Parser final : public ingest::Parser<int> {
  public:
    [[nodiscard]] auto identity() const noexcept -> std::string_view override {
        return "integer";
    }
    [[nodiscard]] auto version() const noexcept -> std::string_view override {
        return "1";
    }
    auto parse(const ingest::RawRecord &record) const -> evolution::Result<int> override {
        const auto value = static_cast<char>(record.data().bytes.front());
        if (value < '0' || value > '9') {
            return evolution::Result<int>::failure(
                evolution::Error(evolution::ErrorCode::create("parse.invalid"),
                                 evolution::ErrorCategory::InvalidArgument, "not a digit"));
        }
        return evolution::Result<int>::success(value - '0');
    }
};

class Validator final : public ingest::StructuralValidator<int>,
                        public ingest::SemanticValidator<int> {
  public:
    [[nodiscard]] auto validate(const int &value) const
        -> evolution::Result<ingest::ValidationDecision> override {
        return evolution::Result<ingest::ValidationDecision>::success(
            {value != 0, value == 0 ? std::vector<ingest::ValidationIssue>{{"zero", {}, "zero"}}
                                    : std::vector<ingest::ValidationIssue>{}});
    }
};

class Normalizer final : public ingest::Normalizer<int, int> {
  public:
    [[nodiscard]] auto identity() const noexcept -> std::string_view override {
        return "scale";
    }
    [[nodiscard]] auto version() const noexcept -> std::string_view override {
        return "2";
    }
    auto normalize(const int &value) const
        -> evolution::Result<ingest::NormalizedRecord<int>> override {
        return evolution::Result<ingest::NormalizedRecord<int>>::success(
            {value * 10, evolution::time::TemporalInformation::known(
                             evolution::time::TimePoint{5s}, evolution::time::Precision::Second)});
    }
};

class Mapper final : public ingest::Mapper<int, int> {
  public:
    [[nodiscard]] auto identity() const noexcept -> std::string_view override {
        return "identity";
    }
    [[nodiscard]] auto version() const noexcept -> std::string_view override {
        return "3";
    }
    auto map(const int &value, const ingest::IngestionProvenance &) const
        -> evolution::Result<int> override {
        return evolution::Result<int>::success(value);
    }
};

class Deduplicator final : public ingest::Deduplicator<int> {
  public:
    auto classify(const int &value, const ingest::IngestionProvenance &) const
        -> evolution::Result<ingest::DuplicateDisposition> override {
        return evolution::Result<ingest::DuplicateDisposition>::success(
            value == 20   ? ingest::DuplicateDisposition::Duplicate
            : value == 30 ? ingest::DuplicateDisposition::Correction
                          : ingest::DuplicateDisposition::New);
    }
};

class Filter final : public ingest::IngestionFilter<int> {
  public:
    auto evaluate(const int &value, const ingest::IngestionProvenance &) const
        -> evolution::Result<ingest::FilterDecision> override {
        return evolution::Result<ingest::FilterDecision>::success(
            {value == 40 ? ingest::FilterDisposition::Exclude : ingest::FilterDisposition::Include,
             value == 40 ? "excluded by test policy" : "included"});
    }
};

void test_ingestion(TestRunner &test) {
    Acquirer acquirer;
    Parser parser;
    Validator validator;
    Normalizer normalizer;
    Mapper mapper;
    Deduplicator deduplicator;
    Filter filter;
    auto protector = require_value(security::ResourceProtector::create({64, 1, 1}));
    ingest::IngestionPipeline<std::string, int, int, int> pipeline(acquirer, parser, validator,
                                                                   normalizer, validator, mapper,
                                                                   deduplicator, filter, protector);
    const auto accepted = pipeline.ingest("1", execution());
    const auto duplicate = pipeline.ingest("2", execution());
    const auto correction = pipeline.ingest("3", execution());
    const auto filtered = pipeline.ingest("4", execution());
    const auto rejected = pipeline.ingest("0", execution());
    test.expect(accepted && accepted.value().output == 10 &&
                    accepted.value().provenance.parser == "integer" &&
                    accepted.value().provenance.normalizer_version == "2" &&
                    accepted.value().provenance.mapper_version == "3",
                "ingestion executes all stages and records source/tool provenance");
    test.expect(duplicate.value().completion == ingest::IngestionCompletion::Duplicate &&
                    correction.value().completion == ingest::IngestionCompletion::Corrected &&
                    filtered.value().completion == ingest::IngestionCompletion::Filtered,
                "ingestion distinguishes duplicate, correction, and filtering outcomes");
    test.expect(rejected.value().completion == ingest::IngestionCompletion::Rejected &&
                    pipeline.ingest("x", execution()).has_error(),
                "validation rejection remains distinct from parser failure");
}

class Authenticator final : public security::Authenticator {
  public:
    [[nodiscard]] auto authenticate(const security::AuthenticationRequest &request) const
        -> evolution::Result<security::SecurityContext> override {
        if (request.credential_reference != "credential-ref") {
            return evolution::Result<security::SecurityContext>::failure(
                evolution::Error(evolution::ErrorCode::create("auth.failed"),
                                 evolution::ErrorCategory::Unauthorized, "authentication failed"));
        }
        return evolution::Result<security::SecurityContext>::success(security_context());
    }
};

class IntMapper final : public iface::DtoMapper<std::string, int> {
  public:
    [[nodiscard]] auto to_model(const std::string &dto) const -> evolution::Result<int> override {
        return evolution::Result<int>::success(dto == "four" ? 4 : 0);
    }
};

class StringMapper final : public iface::ResponseMapper<int, std::string> {
  public:
    [[nodiscard]] auto to_dto(const int &model) const -> evolution::Result<std::string> override {
        return evolution::Result<std::string>::success(std::to_string(model));
    }
};

void test_interface_adapter(TestRunner &test) {
    Authenticator authenticator;
    IntMapper input;
    StringMapper output;
    Authorizer authorizer;
    Audit audit;
    CommandHandler handler;
    app::InMemoryIdempotencyStore<int> store;
    auto protector = require_value(security::ResourceProtector::create({64, 1, 1}));
    app::CommandExecutor<int, int> executor(authorizer, audit, protector, handler, store);
    iface::CommandAdapter<std::string, int, int, std::string> adapter(
        authenticator, input, output, executor, operation(app::OperationKind::Command, true),
        "item");
    auto metadata = require_value(iface::InterfaceRequestMetadata::create(
        {"v1", "external-request", "external-correlation", 4, 1}));
    auto response = adapter.handle(
        {metadata, {"test", "credential-ref", "tenant-a"}, "four", "idempotency-a"}, execution());
    test.expect(response.payload == std::optional<std::string>{"8"} && !response.error,
                "transport-neutral adapter authenticates and maps DTOs to application operations");
    auto failed = adapter.handle(
        {metadata, {"test", "bad-reference", "tenant-a"}, "four", "idempotency-b"}, execution());
    test.expect(
        failed.error && failed.error->category == evolution::ErrorCategory::Unauthorized,
        "interface error translation preserves structured semantics without transport codes");
}

} // namespace

int main() {
    TestRunner test;
    try {
        test_application_lifecycle(test);
        test_security_and_operations(test);
        test_ingestion(test);
        test_interface_adapter(test);
    } catch (const std::exception &error) {
        std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
        return 1;
    }
    return test.failures() == 0 ? 0 : 1;
}
