#pragma once

#include "evolution/analysis/common.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/time/time.hpp"

#include <optional>
#include <string>
#include <vector>

namespace evolution::analysis {

struct PatternTag;
struct PatternDefinitionTag;
struct DetectorTag;

using PatternId = identity::Id<PatternTag>;
using PatternDefinitionId = identity::Id<PatternDefinitionTag>;
using DetectorId = identity::Id<DetectorTag>;

enum class PatternStatus { Candidate, Active, Confirmed, Completed, Invalidated };
enum class PatternRelationshipKind { Precedes, Overlaps, Contains, Supports, Contradicts, PartOf };

struct PatternRelationship {
    PatternRelationshipKind kind;
    PatternId related_pattern;
    std::optional<double> strength;
};

class EVOLUTION_ANALYSIS_API PatternStrength {
  public:
    static auto create(double value, std::string semantics) -> Result<PatternStrength>;
    [[nodiscard]] auto value() const noexcept -> double;
    [[nodiscard]] auto semantics() const noexcept -> const std::string &;

  private:
    PatternStrength(double value, std::string semantics);
    double value_;
    std::string semantics_;
};

struct PatternDefinitionData {
    PatternDefinitionId id;
    std::string type;
    std::string version;
    DetectorId detector_id;
    std::string detector_version;
    AlgorithmId algorithm_id;
    std::string algorithm_version;
    std::optional<ConfigurationId> configuration_id;
    bool incremental{};
    bool deterministic{};
};

class EVOLUTION_ANALYSIS_API PatternDefinition {
  public:
    static auto create(PatternDefinitionData data) -> Result<PatternDefinition>;
    [[nodiscard]] auto data() const noexcept -> const PatternDefinitionData &;

  private:
    explicit PatternDefinition(PatternDefinitionData data);
    PatternDefinitionData data_;
};

struct PatternResultData {
    PatternId id;
    PatternDefinitionId definition_id;
    std::string definition_version;
    std::string type;
    AnalyticalScope scope;
    std::optional<time::TimeRange> time_range;
    time::TemporalInformation detected_at{time::TemporalInformation::unknown()};
    std::vector<EvidenceReference> evidence;
    std::optional<Confidence> confidence;
    std::optional<PatternStrength> strength;
    PatternStatus status{PatternStatus::Candidate};
    std::vector<PatternRelationship> relationships;
    std::vector<NamedValue> values;
    ProvenanceId provenance_id;
    AnalyticalReproducibility reproducibility;
};

class EVOLUTION_ANALYSIS_API PatternResult {
  public:
    static auto create(PatternResultData data) -> Result<PatternResult>;
    [[nodiscard]] auto data() const noexcept -> const PatternResultData &;

  private:
    explicit PatternResult(PatternResultData data);
    PatternResultData data_;
};

} // namespace evolution::analysis
