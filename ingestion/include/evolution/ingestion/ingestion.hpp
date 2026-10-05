#pragma once

#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"
#include "evolution/ingestion/api.hpp"
#include "evolution/security/security.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace evolution::ingestion {

struct SourceRecordTag;
using SourceRecordId = identity::Id<SourceRecordTag>;

struct RawRecordData {
    SourceId source_id;
    SourceRecordId record_id;
    std::string media_type;
    std::vector<std::byte> bytes;
    time::TemporalInformation acquired_at{time::TemporalInformation::unknown()};
    security::TrustLevel trust{security::TrustLevel::Untrusted};
    std::optional<std::string> source_version;
};

class EVOLUTION_INGESTION_API RawRecord {
  public:
    static auto create(RawRecordData data) -> Result<RawRecord>;
    [[nodiscard]] auto data() const noexcept -> const RawRecordData &;

  private:
    explicit RawRecord(RawRecordData data);
    RawRecordData data_;
};

struct ValidationIssue {
    std::string code;
    std::string path;
    std::string message;
};

struct ValidationDecision {
    bool accepted{};
    std::vector<ValidationIssue> issues;
};

struct IngestionProvenance {
    SourceId source_id;
    SourceRecordId source_record_id;
    std::optional<std::string> source_version;
    std::string parser;
    std::string parser_version;
    std::string normalizer;
    std::string normalizer_version;
    std::string mapper;
    std::string mapper_version;
    std::optional<ConfigurationId> configuration_id;
    time::TemporalInformation acquired_at{time::TemporalInformation::unknown()};
    time::TemporalInformation event_time{time::TemporalInformation::unknown()};
};

template <typename Value> struct NormalizedRecord {
    Value value;
    time::TemporalInformation event_time{time::TemporalInformation::unknown()};
};

enum class DuplicateDisposition { New, Duplicate, Correction };
enum class FilterDisposition { Include, Exclude };
enum class IngestionCompletion { Accepted, Duplicate, Corrected, Filtered, Rejected };

struct FilterDecision {
    FilterDisposition disposition{FilterDisposition::Include};
    std::string reason;
};

template <typename Output> struct IngestionOutcome {
    IngestionCompletion completion;
    std::optional<Output> output;
    IngestionProvenance provenance;
    std::vector<ValidationIssue> issues;
};

template <typename Request> class Acquirer {
  public:
    virtual ~Acquirer() = default;
    virtual auto acquire(const Request &request, const ExecutionContext &context) const
        -> Result<RawRecord> = 0;
};

template <typename Parsed> class Parser {
  public:
    virtual ~Parser() = default;
    [[nodiscard]] virtual auto identity() const noexcept -> std::string_view = 0;
    [[nodiscard]] virtual auto version() const noexcept -> std::string_view = 0;
    virtual auto parse(const RawRecord &record) const -> Result<Parsed> = 0;
};

template <typename Value> class StructuralValidator {
  public:
    virtual ~StructuralValidator() = default;
    [[nodiscard]] virtual auto validate(const Value &value) const -> Result<ValidationDecision> = 0;
};

template <typename Parsed, typename Normalized> class Normalizer {
  public:
    virtual ~Normalizer() = default;
    [[nodiscard]] virtual auto identity() const noexcept -> std::string_view = 0;
    [[nodiscard]] virtual auto version() const noexcept -> std::string_view = 0;
    virtual auto normalize(const Parsed &value) const -> Result<NormalizedRecord<Normalized>> = 0;
};

template <typename Value> class SemanticValidator {
  public:
    virtual ~SemanticValidator() = default;
    [[nodiscard]] virtual auto validate(const Value &value) const -> Result<ValidationDecision> = 0;
};

template <typename Input, typename Output> class Mapper {
  public:
    virtual ~Mapper() = default;
    [[nodiscard]] virtual auto identity() const noexcept -> std::string_view = 0;
    [[nodiscard]] virtual auto version() const noexcept -> std::string_view = 0;
    virtual auto map(const Input &value, const IngestionProvenance &provenance) const
        -> Result<Output> = 0;
};

template <typename Output> class Deduplicator {
  public:
    virtual ~Deduplicator() = default;
    virtual auto classify(const Output &output, const IngestionProvenance &provenance) const
        -> Result<DuplicateDisposition> = 0;
};

template <typename Output> class IngestionFilter {
  public:
    virtual ~IngestionFilter() = default;
    virtual auto evaluate(const Output &output, const IngestionProvenance &provenance) const
        -> Result<FilterDecision> = 0;
};

template <typename Request, typename Parsed, typename Normalized, typename Output>
class IngestionPipeline {
  public:
    IngestionPipeline(const Acquirer<Request> &acquirer, const Parser<Parsed> &parser,
                      const StructuralValidator<Parsed> &structural_validator,
                      const Normalizer<Parsed, Normalized> &normalizer,
                      const SemanticValidator<Normalized> &semantic_validator,
                      const Mapper<Normalized, Output> &mapper,
                      const Deduplicator<Output> &deduplicator,
                      const IngestionFilter<Output> &filter,
                      security::ResourceProtector resource_protector,
                      std::optional<ConfigurationId> configuration_id = std::nullopt)
        : acquirer_(acquirer), parser_(parser), structural_validator_(structural_validator),
          normalizer_(normalizer), semantic_validator_(semantic_validator), mapper_(mapper),
          deduplicator_(deduplicator), filter_(filter),
          resource_protector_(std::move(resource_protector)),
          configuration_id_(std::move(configuration_id)) {}

    [[nodiscard]] auto ingest(const Request &request, const ExecutionContext &context) const
        -> Result<IngestionOutcome<Output>> {
        if (context.is_cancelled()) {
            return failure("ingestion.cancelled", ErrorCategory::Cancelled,
                           "ingestion was cancelled");
        }
        if (context.deadline_exceeded(time::MonotonicClock::now())) {
            return failure("ingestion.deadline_exceeded", ErrorCategory::DeadlineExceeded,
                           "ingestion deadline was exceeded");
        }
        auto raw = acquirer_.acquire(request, context);
        if (!raw) {
            return Result<IngestionOutcome<Output>>::failure(raw.error());
        }
        auto resource = resource_protector_.admit({raw.value().data().bytes.size(), 1});
        if (!resource) {
            return Result<IngestionOutcome<Output>>::failure(resource.error());
        }
        auto parsed = parser_.parse(raw.value());
        if (!parsed) {
            return Result<IngestionOutcome<Output>>::failure(parsed.error());
        }
        auto structural = structural_validator_.validate(parsed.value());
        if (!structural) {
            return Result<IngestionOutcome<Output>>::failure(structural.error());
        }
        IngestionProvenance provenance{
            raw.value().data().source_id,       raw.value().data().record_id,
            raw.value().data().source_version,  std::string(parser_.identity()),
            std::string(parser_.version()),     std::string(normalizer_.identity()),
            std::string(normalizer_.version()), std::string(mapper_.identity()),
            std::string(mapper_.version()),     configuration_id_,
            raw.value().data().acquired_at,     time::TemporalInformation::unknown()};
        if (!structural.value().accepted) {
            return Result<IngestionOutcome<Output>>::success({IngestionCompletion::Rejected,
                                                              std::nullopt, std::move(provenance),
                                                              structural.value().issues});
        }
        auto normalized = normalizer_.normalize(parsed.value());
        if (!normalized) {
            return Result<IngestionOutcome<Output>>::failure(normalized.error());
        }
        provenance.event_time = normalized.value().event_time;
        auto semantic = semantic_validator_.validate(normalized.value().value);
        if (!semantic) {
            return Result<IngestionOutcome<Output>>::failure(semantic.error());
        }
        if (!semantic.value().accepted) {
            return Result<IngestionOutcome<Output>>::success({IngestionCompletion::Rejected,
                                                              std::nullopt, std::move(provenance),
                                                              semantic.value().issues});
        }
        auto output = mapper_.map(normalized.value().value, provenance);
        if (!output) {
            return Result<IngestionOutcome<Output>>::failure(output.error());
        }
        auto duplicate = deduplicator_.classify(output.value(), provenance);
        if (!duplicate) {
            return Result<IngestionOutcome<Output>>::failure(duplicate.error());
        }
        if (duplicate.value() == DuplicateDisposition::Duplicate) {
            return Result<IngestionOutcome<Output>>::success(
                {IngestionCompletion::Duplicate, std::nullopt, std::move(provenance), {}});
        }
        auto filtering = filter_.evaluate(output.value(), provenance);
        if (!filtering) {
            return Result<IngestionOutcome<Output>>::failure(filtering.error());
        }
        if (filtering.value().disposition == FilterDisposition::Exclude) {
            return Result<IngestionOutcome<Output>>::success(
                {IngestionCompletion::Filtered,
                 std::nullopt,
                 std::move(provenance),
                 {{"ingestion.filtered", {}, filtering.value().reason}}});
        }
        const auto completion = duplicate.value() == DuplicateDisposition::Correction
                                    ? IngestionCompletion::Corrected
                                    : IngestionCompletion::Accepted;
        return Result<IngestionOutcome<Output>>::success(
            {completion, std::move(output).value(), std::move(provenance), {}});
    }

  private:
    static auto failure(std::string code, ErrorCategory category, std::string message)
        -> Result<IngestionOutcome<Output>> {
        return Result<IngestionOutcome<Output>>::failure(
            Error(ErrorCode::create(std::move(code)), category, std::move(message)));
    }

    const Acquirer<Request> &acquirer_;
    const Parser<Parsed> &parser_;
    const StructuralValidator<Parsed> &structural_validator_;
    const Normalizer<Parsed, Normalized> &normalizer_;
    const SemanticValidator<Normalized> &semantic_validator_;
    const Mapper<Normalized, Output> &mapper_;
    const Deduplicator<Output> &deduplicator_;
    const IngestionFilter<Output> &filter_;
    security::ResourceProtector resource_protector_;
    std::optional<ConfigurationId> configuration_id_;
};

} // namespace evolution::ingestion
