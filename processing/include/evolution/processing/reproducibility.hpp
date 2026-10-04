#pragma once

#include "evolution/core/context/resource_view.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"
#include "evolution/processing/admission.hpp"
#include "evolution/processing/api.hpp"
#include "evolution/processing/concurrency.hpp"
#include "evolution/processing/graph.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace evolution::processing {

enum class ReproducibilityLevel {
    None,
    IdentifiableInputs,
    ReconstructableContext,
    SemanticDeterministic,
    BitwiseCanonical,
};

struct ReproducibilityInput {
    std::string kind;
    std::string identity;
    std::optional<std::string> version;
    friend auto operator==(const ReproducibilityInput &, const ReproducibilityInput &)
        -> bool = default;
};

struct ReproducibilityComponent {
    ComponentId id;
    std::string version;
    friend auto operator==(const ReproducibilityComponent &, const ReproducibilityComponent &)
        -> bool = default;
};

struct ReproducibilityAlgorithm {
    AlgorithmId id;
    std::string version;
    friend auto operator==(const ReproducibilityAlgorithm &, const ReproducibilityAlgorithm &)
        -> bool = default;
};

struct ReproducibilityState {
    StateId id;
    std::string version;
    friend auto operator==(const ReproducibilityState &, const ReproducibilityState &)
        -> bool = default;
};

struct RandomnessRecord {
    std::string source_id;
    std::optional<std::uint64_t> seed;
    bool affects_results{};
    friend auto operator==(const RandomnessRecord &, const RandomnessRecord &) -> bool = default;
};

struct ClockRecord {
    std::string source_id;
    std::string semantics;
    std::optional<time::TimePoint> captured_time;
    bool affects_results{};
    bool historical_time_preserved{};
    friend auto operator==(const ClockRecord &, const ClockRecord &) -> bool = default;
};

struct OrderingRecord {
    OrderingRequirement requirement{OrderingRequirement::Unordered};
    std::optional<std::string> partition_contract;
    std::string tie_breaker;
    bool affects_results{};
    friend auto operator==(const OrderingRecord &, const OrderingRecord &) -> bool = default;
};

struct AdmissionRecord {
    AdmissionPolicyKind policy{AdmissionPolicyKind::Reject};
    std::uint64_t rejected{};
    std::uint64_t dropped{};
    std::uint64_t sampled_out{};
    bool affects_semantic_input{};
    friend auto operator==(const AdmissionRecord &, const AdmissionRecord &) -> bool = default;
};

struct ResourceConditionRecord {
    context::ResourceKind kind;
    std::uint64_t available{};
    std::optional<std::uint64_t> limit;
    std::string semantic_effect;
    friend auto operator==(const ResourceConditionRecord &, const ResourceConditionRecord &)
        -> bool = default;
};

struct ExternalDependencyRecord {
    std::string name;
    std::string version;
    std::optional<std::string> captured_response_identity;
    bool affects_results{};
    friend auto operator==(const ExternalDependencyRecord &, const ExternalDependencyRecord &)
        -> bool = default;
};

struct ReproducibilityRecordData {
    RunId run_id;
    ReproducibilityLevel level;
    std::vector<ReproducibilityInput> inputs;
    ConfigurationId configuration_id;
    std::string configuration_version;
    GraphId graph_id;
    std::string graph_version;
    std::vector<ReproducibilityComponent> components;
    std::vector<ReproducibilityAlgorithm> algorithms;
    std::vector<ReproducibilityState> states;
    std::vector<RandomnessRecord> randomness;
    std::vector<ClockRecord> clocks;
    std::map<std::string, std::string> environment;
    OrderingRecord ordering;
    AdmissionRecord admission;
    std::vector<ResourceConditionRecord> resource_conditions;
    std::vector<ExternalDependencyRecord> external_dependencies;
    std::vector<std::string> result_identities;
};

class EVOLUTION_PROCESSING_API ReproducibilityRecord {
  public:
    static auto create(ReproducibilityRecordData data) -> Result<ReproducibilityRecord>;
    [[nodiscard]] auto data() const noexcept -> const ReproducibilityRecordData &;

  private:
    explicit ReproducibilityRecord(ReproducibilityRecordData data);
    ReproducibilityRecordData data_;
};

struct ReproducibilityComparison {
    bool equivalent_conditions{};
    std::vector<std::string> differences;
};

enum class DifferentialOutcome {
    Equivalent,
    Different,
    Incomparable,
};

class EVOLUTION_PROCESSING_API ReproducibilityComparator {
  public:
    [[nodiscard]] static auto compare_conditions(const ReproducibilityRecord &left,
                                                 const ReproducibilityRecord &right)
        -> ReproducibilityComparison;
    [[nodiscard]] static auto
    compare_results(const ReproducibilityRecord &left, std::string_view left_semantic_digest,
                    const ReproducibilityRecord &right, std::string_view right_semantic_digest)
        -> DifferentialOutcome;
};

} // namespace evolution::processing
