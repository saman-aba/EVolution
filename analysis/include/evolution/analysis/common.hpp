#pragma once

#include "evolution/analysis/api.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/provenance/provenance.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace evolution::analysis {

using AnalyticalValue = std::variant<bool, std::int64_t, double, std::string>;

struct NamedValue {
    std::string name;
    AnalyticalValue value;

    friend auto operator==(const NamedValue &, const NamedValue &) -> bool = default;
};

struct EvidenceReference {
    std::string kind;
    std::string identity;
    std::optional<std::string> version;
    std::string role;

    friend auto operator==(const EvidenceReference &, const EvidenceReference &) -> bool = default;
};

class EVOLUTION_ANALYSIS_API Confidence {
  public:
    static auto create(double value, std::string semantics) -> Result<Confidence>;

    [[nodiscard]] auto value() const noexcept -> double;
    [[nodiscard]] auto semantics() const noexcept -> const std::string &;

  private:
    Confidence(double value, std::string semantics);
    double value_;
    std::string semantics_;
};

struct Uncertainty {
    std::string kind;
    std::optional<double> lower;
    std::optional<double> upper;
    std::string description;

    friend auto operator==(const Uncertainty &, const Uncertainty &) -> bool = default;
};

struct AnalyticalScope {
    std::string kind;
    std::string identity;
    std::map<std::string, std::string> dimensions;

    friend auto operator==(const AnalyticalScope &, const AnalyticalScope &) -> bool = default;
};

struct AnalyticalReproducibility {
    RunId run_id;
    AlgorithmId algorithm_id;
    std::string algorithm_version;
    std::optional<ConfigurationId> configuration_id;
    std::vector<VersionReference> semantic_versions;
    bool deterministic{};
    std::optional<std::string> semantic_digest;

    friend auto operator==(const AnalyticalReproducibility &, const AnalyticalReproducibility &)
        -> bool = default;
};

EVOLUTION_ANALYSIS_API auto validate_evidence(const std::vector<EvidenceReference> &evidence,
                                              bool require_non_empty = true) -> Result<void>;
EVOLUTION_ANALYSIS_API auto validate_scope(const AnalyticalScope &scope) -> Result<void>;
EVOLUTION_ANALYSIS_API auto validate_uncertainty(const Uncertainty &uncertainty) -> Result<void>;
EVOLUTION_ANALYSIS_API auto
validate_reproducibility(const AnalyticalReproducibility &reproducibility) -> Result<void>;

} // namespace evolution::analysis
