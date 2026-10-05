#include "evolution/analysis/analysis.hpp"
#include "evolution/analysis/decision.hpp"
#include "evolution/applications/operations.hpp"
#include "evolution/core/processing/envelope.hpp"
#include "evolution/domains/poker/model.hpp"
#include "evolution/ingestion/ingestion.hpp"
#include "evolution/interfaces/interfaces.hpp"
#include "evolution/processing/processor.hpp"
#include "evolution/security/security.hpp"
#include "evolution/storage/in_memory.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
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
namespace analytical = evolution::analysis;
namespace app = evolution::applications;
namespace ingest = evolution::ingestion;
namespace iface = evolution::interfaces;
namespace poker = evolution::domains::poker;
namespace security = evolution::security;

template <typename Value> auto require_value(evolution::Result<Value> result) -> Value {
    if (!result) {
        throw std::runtime_error(std::string(result.error().message()));
    }
    return std::move(result).value();
}

template <typename Id> auto stable(std::string_view value) -> Id {
    return require_value(Id::from_stable_name(value));
}

auto execution() -> evolution::ExecutionContext {
    return evolution::ExecutionContext::create(evolution::ExecutionMode::Test,
                                               stable<evolution::RunId>("system-run"));
}

struct ParsedAction {
    std::string player;
    std::int64_t amount{};
};

class ActionAcquirer final : public ingest::Acquirer<std::string> {
  public:
    auto acquire(const std::string &request, const evolution::ExecutionContext &) const
        -> evolution::Result<ingest::RawRecord> override {
        std::vector<std::byte> bytes;
        for (const char value : request) {
            bytes.push_back(static_cast<std::byte>(value));
        }
        return ingest::RawRecord::create(
            {stable<evolution::SourceId>("external-poker-feed"),
             stable<ingest::SourceRecordId>(request), "text/csv", std::move(bytes),
             evolution::time::TemporalInformation::known(evolution::time::TimePoint{10s},
                                                         evolution::time::Precision::Second),
             security::TrustLevel::Untrusted, "feed-v1"});
    }
};

class ActionParser final : public ingest::Parser<ParsedAction> {
  public:
    [[nodiscard]] auto identity() const noexcept -> std::string_view override {
        return "poker-csv-parser";
    }
    [[nodiscard]] auto version() const noexcept -> std::string_view override {
        return "1";
    }
    auto parse(const ingest::RawRecord &record) const -> evolution::Result<ParsedAction> override {
        std::string text;
        for (const auto value : record.data().bytes) {
            text.push_back(static_cast<char>(value));
        }
        const auto separator = text.find(',');
        if (separator == std::string::npos || text.substr(separator + 1) != "25") {
            return evolution::Result<ParsedAction>::failure(
                evolution::Error(evolution::ErrorCode::create("system.invalid_action"),
                                 evolution::ErrorCategory::InvalidArgument, "expected player,25"));
        }
        return evolution::Result<ParsedAction>::success({text.substr(0, separator), 25});
    }
};

class ActionValidator final : public ingest::StructuralValidator<ParsedAction>,
                              public ingest::SemanticValidator<ParsedAction> {
  public:
    [[nodiscard]] auto validate(const ParsedAction &value) const
        -> evolution::Result<ingest::ValidationDecision> override {
        return evolution::Result<ingest::ValidationDecision>::success(
            {!value.player.empty() && value.amount > 0, {}});
    }
};

class ActionNormalizer final : public ingest::Normalizer<ParsedAction, ParsedAction> {
  public:
    [[nodiscard]] auto identity() const noexcept -> std::string_view override {
        return "poker-action-normalizer";
    }
    [[nodiscard]] auto version() const noexcept -> std::string_view override {
        return "1";
    }
    auto normalize(const ParsedAction &value) const
        -> evolution::Result<ingest::NormalizedRecord<ParsedAction>> override {
        return evolution::Result<ingest::NormalizedRecord<ParsedAction>>::success(
            {value, evolution::time::TemporalInformation::known(
                        evolution::time::TimePoint{8s}, evolution::time::Precision::Second)});
    }
};

class EventMapper final : public ingest::Mapper<ParsedAction, poker::PokerEvent> {
  public:
    [[nodiscard]] auto identity() const noexcept -> std::string_view override {
        return "poker-event-mapper";
    }
    [[nodiscard]] auto version() const noexcept -> std::string_view override {
        return "1";
    }
    auto map(const ParsedAction &value, const ingest::IngestionProvenance &provenance) const
        -> evolution::Result<poker::PokerEvent> override {
        evolution::EventMetadata metadata{
            stable<evolution::EventId>("poker-action-event"),
            require_value(evolution::EventType::create("Poker.PlayerActed")),
            require_value(evolution::EventSource::create(provenance.source_id, "external-feed")),
            provenance.event_time,
            evolution::time::TimePoint{10s},
            evolution::EventSequence{"hand-a", 1},
            std::nullopt,
            std::nullopt,
            "poker-action-v1",
            "1"};
        poker::PlayerActed payload{stable<poker::HandId>("hand-a"),
                                   stable<poker::PlayerId>(value.player), poker::ActionKind::Raise,
                                   value.amount};
        return poker::PokerEvent::create(std::move(metadata), poker::PokerEventPayload{payload});
    }
};

class NewEvent final : public ingest::Deduplicator<poker::PokerEvent> {
  public:
    auto classify(const poker::PokerEvent &, const ingest::IngestionProvenance &) const
        -> evolution::Result<ingest::DuplicateDisposition> override {
        return evolution::Result<ingest::DuplicateDisposition>::success(
            ingest::DuplicateDisposition::New);
    }
};

class IncludeEvent final : public ingest::IngestionFilter<poker::PokerEvent> {
  public:
    auto evaluate(const poker::PokerEvent &, const ingest::IngestionProvenance &) const
        -> evolution::Result<ingest::FilterDecision> override {
        return evolution::Result<ingest::FilterDecision>::success(
            {ingest::FilterDisposition::Include, "accepted"});
    }
};

class ActionProcessor final
    : public evolution::processing::Processor<poker::PokerEvent, std::int64_t, std::int64_t> {
  public:
    auto process(evolution::Envelope<poker::PokerEvent> input, std::optional<std::int64_t> state,
                 const evolution::Configuration &, const evolution::ExecutionContext &) const
        -> evolution::Result<
            evolution::processing::ProcessingOutcome<std::int64_t, std::int64_t>> override {
        const auto *action = std::get_if<poker::PlayerActed>(&input.payload().payload());
        if (action == nullptr) {
            return evolution::Result<
                evolution::processing::ProcessingOutcome<std::int64_t, std::int64_t>>::
                failure(evolution::Error(evolution::ErrorCode::create("system.unexpected_event"),
                                         evolution::ErrorCategory::InvalidArgument,
                                         "expected player action"));
        }
        const auto total = state.value_or(0) + action->amount;
        auto metadata = input.metadata().for_output(
            stable<evolution::EnvelopeId>("processed-action"),
            evolution::time::TemporalInformation::known(evolution::time::TimePoint{11s},
                                                        evolution::time::Precision::Second),
            std::nullopt);
        return evolution::
            Result<evolution::processing::ProcessingOutcome<std::int64_t, std::int64_t>>::success(
                evolution::processing::ProcessingOutcome<std::int64_t, std::int64_t>::success(
                    {evolution::Envelope<std::int64_t>::create(total, std::move(metadata))},
                    total));
    }
};

auto reproducibility() -> analytical::AnalyticalReproducibility {
    return {stable<evolution::RunId>("system-run"),
            stable<evolution::AlgorithmId>("system-analysis"),
            "1",
            std::nullopt,
            {{"domain:poker", "1"}},
            true,
            "system-digest"};
}

auto make_analysis(std::int64_t amount) -> analytical::AnalysisResult {
    const analytical::EvidenceReference evidence{"event", "poker-action-event", "1", "input"};
    return require_value(analytical::AnalysisResult::create(
        {stable<evolution::AnalysisId>("system-analysis-result"),
         stable<analytical::AnalysisDefinitionId>("system-analysis-definition"),
         "1",
         analytical::AnalysisCompletion::Complete,
         {"player", "alice", {{"hand", "hand-a"}}},
         std::nullopt,
         {evidence},
         {{stable<analytical::FindingId>("system-finding"),
           "raise amount is available for policy evaluation",
           analytical::ClaimStrength::Observation,
           {evidence},
           {"read persisted processed amount"},
           std::nullopt,
           std::nullopt}},
         {"processed and queried amount " + std::to_string(amount)},
         {},
         {},
         std::nullopt,
         std::nullopt,
         {},
         {},
         stable<evolution::ProvenanceId>("system-analysis-provenance"),
         reproducibility()}));
}

auto make_decision(const analytical::AnalysisResult &analysis) -> analytical::Decision {
    const auto candidate_id = stable<analytical::DecisionCandidateId>("review-candidate");
    const analytical::EvidenceReference evidence{"analysis", analysis.data().id.to_string(), "1",
                                                 "basis"};
    return require_value(analytical::Decision::create(
        {stable<analytical::DecisionId>("system-decision"),
         stable<analytical::PolicyId>("review-policy"),
         "1",
         analytical::DecisionCompletion::Selected,
         analytical::DecisionStatus::Proposed,
         analysis.data().scope,
         {evidence},
         {{"review", "review raises at or above threshold", 1, std::nullopt}},
         {},
         {{candidate_id,
           "review",
           std::string("review"),
           analytical::Feasibility::Feasible,
           {{"amount", std::int64_t{25}}},
           {}}},
         analytical::Feasibility::Feasible,
         analytical::DecisionOutcome{candidate_id, std::string("review")},
         {"persisted raise meets threshold"},
         std::nullopt,
         std::nullopt,
         std::nullopt,
         stable<evolution::ProvenanceId>("system-decision-provenance"),
         reproducibility()}));
}

class EndToEndHandler final : public app::CommandHandler<std::string, std::string> {
  public:
    EndToEndHandler()
        : protector_(require_value(security::ResourceProtector::create({1024, 1, 1}))),
          pipeline_(acquirer_, parser_, validator_, normalizer_, validator_, mapper_, deduplicator_,
                    filter_, protector_),
          store_(require_value(evolution::storage::InMemoryPersistence<std::int64_t>::create())) {}

    auto execute(const std::string &input, const app::OperationContext &context) const
        -> evolution::Result<app::CommandExecution<std::string>> override {
        auto ingested = pipeline_.ingest(input, context.execution);
        if (!ingested || !ingested.value().output) {
            return evolution::Result<app::CommandExecution<std::string>>::failure(
                ingested ? evolution::Error(evolution::ErrorCode::create("system.no_event"),
                                            evolution::ErrorCategory::InvalidArgument,
                                            "ingestion produced no event")
                         : ingested.error());
        }
        auto metadata = require_value(evolution::ProcessingMetadata::create(
            stable<evolution::EnvelopeId>("incoming-action"),
            evolution::time::TemporalInformation::known(evolution::time::TimePoint{11s},
                                                        evolution::time::Precision::Second),
            context.correlation_id, std::nullopt, evolution::OrderingMetadata{"hand-a", 1},
            "hand-a"));
        auto configuration = require_value(evolution::Configuration::create({}, "system"));
        auto processed = processor_.process(evolution::Envelope<poker::PokerEvent>::create(
                                                *ingested.value().output, std::move(metadata)),
                                            std::nullopt, configuration, context.execution);
        if (!processed || processed.value().outputs.empty()) {
            return evolution::Result<app::CommandExecution<std::string>>::failure(
                processed ? evolution::Error(evolution::ErrorCode::create("system.no_output"),
                                             evolution::ErrorCategory::Internal,
                                             "processor produced no output")
                          : processed.error());
        }
        const auto amount = processed.value().outputs.front().payload();
        auto stored = store_.append(amount, {"processed/hand-a/1",
                                             {"int64", "1", "native", true},
                                             evolution::storage::RetentionPolicy::forever(),
                                             {},
                                             std::nullopt});
        if (!stored) {
            return evolution::Result<app::CommandExecution<std::string>>::failure(stored.error());
        }
        auto queried = store_.read("processed/hand-a/1");
        if (!queried || !queried.value()) {
            return evolution::Result<app::CommandExecution<std::string>>::failure(
                queried ? evolution::Error(evolution::ErrorCode::create("system.not_found"),
                                           evolution::ErrorCategory::NotFound,
                                           "stored result was not queryable")
                        : queried.error());
        }
        auto analysis = make_analysis(*queried.value());
        auto decision = make_decision(analysis);
        return evolution::Result<app::CommandExecution<std::string>>::success(
            {app::OperationCompletion::Completed,
             std::get<std::string>(decision.data().outcome->value),
             {{"event", ingested.value().output->metadata().id.to_string(), "replayable"},
              {"decision", decision.data().id.to_string(), "stored"}}});
    }

    [[nodiscard]] auto stored_records() const -> std::size_t {
        return store_.size();
    }

  private:
    ActionAcquirer acquirer_;
    ActionParser parser_;
    ActionValidator validator_;
    ActionNormalizer normalizer_;
    EventMapper mapper_;
    NewEvent deduplicator_;
    IncludeEvent filter_;
    security::ResourceProtector protector_;
    ingest::IngestionPipeline<std::string, ParsedAction, ParsedAction, poker::PokerEvent> pipeline_;
    ActionProcessor processor_;
    mutable evolution::storage::InMemoryPersistence<std::int64_t> store_;
};

class Authenticator final : public security::Authenticator {
  public:
    [[nodiscard]] auto authenticate(const security::AuthenticationRequest &) const
        -> evolution::Result<security::SecurityContext> override {
        return evolution::Result<security::SecurityContext>::success(
            {{stable<security::PrincipalId>("system-principal"),
              "service",
              "system-test",
              "tenant-a",
              {}},
             security::TrustLevel::Authenticated,
             {"poker.write"},
             evolution::time::TemporalInformation::unknown(),
             std::nullopt});
    }
};

class Authorizer final : public security::Authorizer {
  public:
    [[nodiscard]] auto authorize(const security::SecurityContext &,
                                 const security::AuthorizationRequest &) const
        -> evolution::Result<security::AuthorizationDecision> override {
        return evolution::Result<security::AuthorizationDecision>::success(
            {security::AuthorizationOutcome::Allowed, "system-policy", "allowed", {}});
    }
};

class Audit final : public security::AuditSink {
  public:
    auto record(security::AuditEvent event) -> evolution::Result<void> override {
        events.push_back(std::move(event));
        return evolution::Result<void>::success();
    }
    std::vector<security::AuditEvent> events;
};

class RequestMapper final : public iface::DtoMapper<std::string, std::string> {
  public:
    [[nodiscard]] auto to_model(const std::string &dto) const
        -> evolution::Result<std::string> override {
        return evolution::Result<std::string>::success(dto);
    }
};

class ResponseMapper final : public iface::ResponseMapper<std::string, std::string> {
  public:
    [[nodiscard]] auto to_dto(const std::string &model) const
        -> evolution::Result<std::string> override {
        return evolution::Result<std::string>::success(model);
    }
};

} // namespace

int main() {
    try {
        Authenticator authenticator;
        Authorizer authorizer;
        Audit audit;
        RequestMapper request_mapper;
        ResponseMapper response_mapper;
        EndToEndHandler handler;
        app::InMemoryIdempotencyStore<std::string> idempotency;
        auto protector = require_value(security::ResourceProtector::create({1024, 1, 1}));
        app::CommandExecutor<std::string, std::string> executor(authorizer, audit, protector,
                                                                handler, idempotency);
        auto operation = require_value(app::OperationDescriptor::create(
            {"ingest-poker-action", "1", app::OperationKind::Command, "poker.write", true, true}));
        iface::CommandAdapter<std::string, std::string, std::string, std::string> adapter(
            authenticator, request_mapper, response_mapper, executor, operation, "poker/hand-a");
        auto metadata = require_value(iface::InterfaceRequestMetadata::create(
            {"v1", "system-request", "system-correlation", 8, 1}));
        const iface::CommandDto<std::string> request{
            metadata, {"test", "credential-reference", "tenant-a"}, "alice,25", "system-key"};
        const auto first = adapter.handle(request, execution());
        const auto replay = adapter.handle(request, execution());
        if (first.error || first.payload != std::optional<std::string>{"review"} ||
            !replay.idempotent_replay || handler.stored_records() != 1 ||
            audit.events.size() != 1) {
            std::cerr << "FAIL: end-to-end application path did not preserve expected semantics\n";
            return 1;
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
        return 1;
    }
}
