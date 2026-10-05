#pragma once

#include "evolution/analysis/common.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/time/time.hpp"

#include <optional>
#include <string>
#include <vector>

namespace evolution::analysis {

struct AnalysisDefinitionTag;
struct FindingTag;
struct HypothesisTag;
struct PredictionTag;

using AnalysisDefinitionId = identity::Id<AnalysisDefinitionTag>;
using FindingId = identity::Id<FindingTag>;
using HypothesisId = identity::Id<HypothesisTag>;
using PredictionId = identity::Id<PredictionTag>;

enum class AnalysisKind {
    Descriptive,
    Comparative,
    Diagnostic,
    Statistical,
    Temporal,
    Causal,
    Predictive,
    Behavioral
};
enum class ClaimStrength { Observation, Association, Explanation, CausalClaim };
enum class HypothesisStatus { Untested, Supported, Rejected, Inconclusive };
enum class AnalysisCompletion { Complete, Partial, InsufficientEvidence };

struct AnalysisDefinitionData {
    AnalysisDefinitionId id;
    std::string name;
    std::string version;
    AnalysisKind kind;
    ComponentId analyzer_id;
    std::string analyzer_version;
    AlgorithmId algorithm_id;
    std::string algorithm_version;
    std::optional<ConfigurationId> configuration_id;
    bool incremental{};
    bool deterministic{};
};

class EVOLUTION_ANALYSIS_API AnalysisDefinition {
  public:
    static auto create(AnalysisDefinitionData data) -> Result<AnalysisDefinition>;
    [[nodiscard]] auto data() const noexcept -> const AnalysisDefinitionData &;

  private:
    explicit AnalysisDefinition(AnalysisDefinitionData data);
    AnalysisDefinitionData data_;
};

struct AnalysisFinding {
    FindingId id;
    std::string statement;
    ClaimStrength claim_strength{ClaimStrength::Observation};
    std::vector<EvidenceReference> evidence;
    std::vector<std::string> reasoning;
    std::optional<Confidence> confidence;
    std::optional<Uncertainty> uncertainty;
};

struct AnalysisBaseline {
    std::string identity;
    std::string description;
    std::vector<EvidenceReference> evidence;
};

struct AnalysisHypothesis {
    HypothesisId id;
    std::string statement;
    HypothesisStatus status{HypothesisStatus::Untested};
    std::vector<EvidenceReference> evidence;
    std::optional<Confidence> confidence;
};

struct AnalysisPrediction {
    PredictionId id;
    std::string target;
    AnalyticalValue value;
    time::TemporalInformation predicted_for{time::TemporalInformation::unknown()};
    std::optional<Confidence> confidence;
    std::optional<Uncertainty> uncertainty;
};

struct AnalysisResultData {
    AnalysisId id;
    AnalysisDefinitionId definition_id;
    std::string definition_version;
    AnalysisCompletion completion{AnalysisCompletion::Complete};
    AnalyticalScope scope;
    std::optional<time::TimeRange> time_range;
    std::vector<EvidenceReference> evidence;
    std::vector<AnalysisFinding> findings;
    std::vector<std::string> reasoning;
    std::vector<std::string> assumptions;
    std::vector<std::string> limitations;
    std::optional<Uncertainty> uncertainty;
    std::optional<AnalysisBaseline> baseline;
    std::vector<AnalysisHypothesis> hypotheses;
    std::vector<AnalysisPrediction> predictions;
    ProvenanceId provenance_id;
    AnalyticalReproducibility reproducibility;
};

class EVOLUTION_ANALYSIS_API AnalysisResult {
  public:
    static auto create(AnalysisResultData data) -> Result<AnalysisResult>;
    [[nodiscard]] auto data() const noexcept -> const AnalysisResultData &;

  private:
    explicit AnalysisResult(AnalysisResultData data);
    AnalysisResultData data_;
};

} // namespace evolution::analysis
