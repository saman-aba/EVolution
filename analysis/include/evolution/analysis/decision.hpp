#pragma once

#include "evolution/analysis/common.hpp"
#include "evolution/core/identity/id.hpp"

#include <optional>
#include <string>
#include <vector>

namespace evolution::analysis {

struct PolicyTag;
struct DecisionTag;
struct DecisionCandidateTag;

using PolicyId = identity::Id<PolicyTag>;
using DecisionId = identity::Id<DecisionTag>;
using DecisionCandidateId = identity::Id<DecisionCandidateTag>;

enum class ObjectiveRelationship { Priority, Weighted, Lexicographic, ConstraintFirst, Pareto };
enum class ConstraintKind { Hard, Soft };
enum class Feasibility { Feasible, Infeasible, Undetermined };
enum class DecisionCompletion { Selected, NoDecision, Partial };
enum class DecisionStatus { Proposed, Accepted, Rejected, Executed, Expired, Superseded };

struct PolicyObjective {
    std::string identity;
    std::string description;
    int priority{};
    std::optional<double> weight;
};

struct PolicyConstraint {
    std::string identity;
    std::string description;
    ConstraintKind kind{ConstraintKind::Hard};
    std::optional<double> penalty;
};

struct PolicyDefinitionData {
    PolicyId id;
    std::string name;
    std::string version;
    AnalyticalScope scope;
    std::vector<PolicyObjective> objectives;
    ObjectiveRelationship objective_relationship{ObjectiveRelationship::Priority};
    std::vector<PolicyConstraint> constraints;
    std::optional<ConfigurationId> configuration_id;
    AlgorithmId evaluator_id;
    std::string evaluator_version;
    bool deterministic{};
};

class EVOLUTION_ANALYSIS_API PolicyDefinition {
  public:
    static auto create(PolicyDefinitionData data) -> Result<PolicyDefinition>;
    [[nodiscard]] auto data() const noexcept -> const PolicyDefinitionData &;

  private:
    explicit PolicyDefinition(PolicyDefinitionData data);
    PolicyDefinitionData data_;
};

struct ConstraintAssessment {
    std::string constraint_identity;
    bool satisfied{};
    std::optional<double> penalty;
    std::string explanation;
};

struct DecisionCandidate {
    DecisionCandidateId id;
    std::string label;
    AnalyticalValue outcome;
    Feasibility feasibility{Feasibility::Undetermined};
    std::vector<NamedValue> scores;
    std::vector<ConstraintAssessment> constraints;
};

struct DecisionRisk {
    std::string identity;
    double value{};
    std::string semantics;
};

struct DecisionOutcome {
    DecisionCandidateId candidate_id;
    AnalyticalValue value;
};

struct DecisionData {
    DecisionId id;
    PolicyId policy_id;
    std::string policy_version;
    DecisionCompletion completion{DecisionCompletion::Selected};
    DecisionStatus status{DecisionStatus::Proposed};
    AnalyticalScope scope;
    std::vector<EvidenceReference> evidence;
    std::vector<PolicyObjective> objectives;
    std::vector<PolicyConstraint> constraints;
    std::vector<DecisionCandidate> candidates;
    Feasibility feasibility{Feasibility::Undetermined};
    std::optional<DecisionOutcome> outcome;
    std::vector<std::string> reasoning;
    std::optional<Confidence> confidence;
    std::optional<Uncertainty> uncertainty;
    std::optional<DecisionRisk> risk;
    ProvenanceId provenance_id;
    AnalyticalReproducibility reproducibility;
};

class EVOLUTION_ANALYSIS_API Decision {
  public:
    static auto create(DecisionData data) -> Result<Decision>;
    [[nodiscard]] auto data() const noexcept -> const DecisionData &;
    [[nodiscard]] auto transition(DecisionStatus next) const -> Result<Decision>;

  private:
    explicit Decision(DecisionData data);
    DecisionData data_;
};

} // namespace evolution::analysis
