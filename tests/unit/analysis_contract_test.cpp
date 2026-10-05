#include "evolution/analysis/aggregation.hpp"
#include "evolution/analysis/analysis.hpp"
#include "evolution/analysis/decision.hpp"
#include "evolution/analysis/pattern.hpp"
#include "evolution/analysis/processors.hpp"

#include <chrono>
#include <concepts>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;
namespace analytical = evolution::analysis;

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

auto evidence(std::string identity = "measurement-a")
    -> std::vector<analytical::EvidenceReference> {
    return {{"measurement", std::move(identity), std::string("1"), "input"}};
}

auto scope() -> analytical::AnalyticalScope {
    return {"player", "player-a", {{"session", "session-a"}}};
}

auto reproducibility(std::string digest = "digest-a") -> analytical::AnalyticalReproducibility {
    return {stable<evolution::RunId>("analysis-run"),
            stable<evolution::AlgorithmId>("analysis-algorithm"),
            "1",
            std::nullopt,
            {{"domain:poker", "1"}},
            true,
            std::move(digest)};
}

auto provenance_id() -> evolution::ProvenanceId {
    return stable<evolution::ProvenanceId>("analysis-provenance");
}

auto sum_definition() -> analytical::AggregationDefinition {
    return require_value(analytical::AggregationDefinition::create(
        {stable<analytical::AggregationId>("aggregation.sum"),
         "sum",
         "1",
         stable<evolution::AlgorithmId>("aggregation.sum.algorithm"),
         "1",
         {analytical::AggregationWindowKind::ObservationCount,
          analytical::AggregationWindowMode::Rolling, std::nullopt, 100, std::nullopt,
          std::nullopt},
         {"player"},
         {},
         analytical::MissingValuePolicy::Exclude,
         analytical::UnknownValuePolicy::Propagate,
         analytical::EmptyInputPolicy::Identity,
         {true, true, true},
         {true, true, true, false}}));
}

class SumAggregator final : public analytical::IncrementalAggregator<int, int, int> {
  public:
    SumAggregator() : definition_(sum_definition()) {}

    [[nodiscard]] auto definition() const noexcept
        -> const analytical::AggregationDefinition & override {
        return definition_;
    }

    auto initial_state() const -> evolution::Result<int> override {
        return evolution::Result<int>::success(0);
    }

    auto add(int state, const int &input) const -> evolution::Result<int> override {
        return evolution::Result<int>::success(state + input);
    }

    auto merge(int left, const int &right) const -> evolution::Result<int> override {
        return evolution::Result<int>::success(left + right);
    }

    auto retract(int state, const int &input) const -> evolution::Result<int> override {
        return evolution::Result<int>::success(state - input);
    }

    auto finish(const int &state) const -> evolution::Result<int> override {
        return evolution::Result<int>::success(state);
    }

  private:
    analytical::AggregationDefinition definition_;
};

void test_aggregation(TestRunner &test) {
    const auto definition = sum_definition();
    test.expect(definition.data().capabilities.incremental &&
                    definition.data().capabilities.mergeable &&
                    definition.data().algebra.associative && definition.data().algebra.commutative,
                "aggregation definition exposes incremental and algebraic properties");

    auto invalid_data = definition.data();
    invalid_data.algebra.associative = false;
    test.expect(!analytical::AggregationDefinition::create(std::move(invalid_data)),
                "mergeable aggregations must declare associativity");

    SumAggregator aggregator;
    auto incremental = require_value(aggregator.initial_state());
    for (const auto input : {1, 2, 3, 4}) {
        incremental = require_value(aggregator.add(incremental, input));
    }
    auto left = require_value(aggregator.add(require_value(aggregator.initial_state()), 1));
    left = require_value(aggregator.add(left, 2));
    auto right = require_value(aggregator.add(require_value(aggregator.initial_state()), 3));
    right = require_value(aggregator.add(right, 4));
    const auto merged = require_value(aggregator.merge(left, right));
    test.expect(require_value(aggregator.finish(incremental)) == 10 && merged == incremental,
                "incremental and merged aggregation are equivalent to the golden sum");
    test.expect(require_value(aggregator.retract(incremental, 4)) == 6,
                "retraction capability reverses a prior update");

    for (int first = -3; first <= 3; ++first) {
        for (int second = -3; second <= 3; ++second) {
            for (int third = -3; third <= 3; ++third) {
                const auto left_associated = require_value(
                    aggregator.merge(require_value(aggregator.merge(first, second)), third));
                const auto right_associated = require_value(
                    aggregator.merge(first, require_value(aggregator.merge(second, third))));
                test.expect(left_associated == right_associated,
                            "declared sum associativity holds over the property sample");
            }
        }
    }

    auto result = require_value(analytical::AggregationResult<int>::create(
        {definition.data().id,
         "1",
         analytical::AggregationCompletion::Complete,
         10,
         {{"player", "player-a"}},
         definition.data().window,
         4,
         4,
         0,
         0,
         evolution::time::TemporalInformation::known(evolution::time::TimePoint{10s},
                                                     evolution::time::Precision::Second),
         std::nullopt,
         provenance_id(),
         reproducibility("sum-10")}));
    test.expect(result.data().value == std::optional<int>{10} && result.data().included_count == 4,
                "aggregation golden result retains grouping, counts, time, and provenance");
}

auto pattern_definition() -> analytical::PatternDefinition {
    return require_value(analytical::PatternDefinition::create(
        {stable<analytical::PatternDefinitionId>("pattern.definition"), "trend", "1",
         stable<analytical::DetectorId>("pattern.detector"), "1",
         stable<evolution::AlgorithmId>("pattern.algorithm"), "1", std::nullopt, true, true}));
}

void test_pattern(TestRunner &test) {
    const auto confidence = require_value(analytical::Confidence::create(0.9, "posterior"));
    const auto strength = require_value(analytical::PatternStrength::create(2.5, "slope"));
    const auto pattern_id = stable<analytical::PatternId>("pattern-a");
    const auto related_id = stable<analytical::PatternId>("pattern-b");
    auto time_range = require_value(evolution::time::TimeRange::create(
        evolution::time::TimePoint{0s}, evolution::time::TimePoint{10s}));
    const auto result = require_value(analytical::PatternResult::create(
        {pattern_id,
         pattern_definition().data().id,
         "1",
         "trend",
         scope(),
         time_range,
         evolution::time::TemporalInformation::known(evolution::time::TimePoint{10s},
                                                     evolution::time::Precision::Second),
         evidence(),
         confidence,
         strength,
         analytical::PatternStatus::Confirmed,
         {{analytical::PatternRelationshipKind::Supports, related_id, 0.8}},
         {{"slope", 2.5}},
         provenance_id(),
         reproducibility("pattern-golden")}));
    test.expect(
        result.data().status == analytical::PatternStatus::Confirmed &&
            result.data().evidence.front().identity == "measurement-a" &&
            result.data().relationships.front().related_pattern == related_id,
        "pattern regression fixture preserves evidence, confidence, status, and relationships");

    auto invalid = result.data();
    invalid.relationships = {
        {analytical::PatternRelationshipKind::Contains, pattern_id, std::nullopt}};
    test.expect(!analytical::PatternResult::create(std::move(invalid)),
                "patterns cannot contain self-referential relationships");
    test.expect(!analytical::Confidence::create(1.1, "posterior"),
                "confidence is bounded and semantically identified");
}

auto analysis_definition() -> analytical::AnalysisDefinition {
    return require_value(analytical::AnalysisDefinition::create(
        {stable<analytical::AnalysisDefinitionId>("analysis.definition"), "player-trend", "1",
         analytical::AnalysisKind::Predictive, stable<evolution::ComponentId>("analyzer"), "1",
         stable<evolution::AlgorithmId>("analysis.algorithm"), "1", std::nullopt, true, true}));
}

auto analysis_result(std::string digest = "analysis-golden") -> analytical::AnalysisResult {
    const auto confidence = require_value(analytical::Confidence::create(0.8, "calibrated"));
    analytical::AnalysisFinding finding{
        stable<analytical::FindingId>("finding-a"),
        "activity is increasing",
        analytical::ClaimStrength::Association,
        evidence(),
        {"positive fitted slope"},
        confidence,
        analytical::Uncertainty{"interval", 0.1, 0.3, "slope interval"}};
    analytical::AnalysisHypothesis hypothesis{
        stable<analytical::HypothesisId>("hypothesis-a"), "activity exceeds baseline",
        analytical::HypothesisStatus::Supported, evidence(), confidence};
    analytical::AnalysisPrediction prediction{
        stable<analytical::PredictionId>("prediction-a"),
        "next-session-activity",
        12.0,
        evolution::time::TemporalInformation::known(evolution::time::TimePoint{20s},
                                                    evolution::time::Precision::Second),
        confidence,
        analytical::Uncertainty{"interval", 10.0, 14.0, "prediction interval"}};
    return require_value(analytical::AnalysisResult::create(
        {stable<evolution::AnalysisId>("analysis-result"),
         analysis_definition().data().id,
         "1",
         analytical::AnalysisCompletion::Complete,
         scope(),
         std::nullopt,
         evidence(),
         {finding},
         {"fit deterministic trend model"},
         {"stable sampling"},
         {"small sample"},
         analytical::Uncertainty{"model", std::nullopt, std::nullopt, "model uncertainty"},
         analytical::AnalysisBaseline{"baseline-a", "previous session", evidence("baseline")},
         {hypothesis},
         {prediction},
         provenance_id(),
         reproducibility(std::move(digest))}));
}

void test_analysis(TestRunner &test) {
    const auto result = analysis_result();
    test.expect(result.data().findings.front().statement == "activity is increasing" &&
                    result.data().predictions.front().target == "next-session-activity" &&
                    result.data().baseline->identity == "baseline-a",
                "analysis golden fixture retains findings, reasoning, baseline, hypotheses, and "
                "predictions");

    auto causal = result.data();
    causal.findings.front().claim_strength = analytical::ClaimStrength::CausalClaim;
    causal.findings.front().reasoning.clear();
    test.expect(!analytical::AnalysisResult::create(std::move(causal)),
                "causal findings require explicit methodology");

    auto insufficient = result.data();
    insufficient.completion = analytical::AnalysisCompletion::InsufficientEvidence;
    insufficient.evidence.clear();
    insufficient.findings.clear();
    insufficient.reasoning.clear();
    test.expect(analytical::AnalysisResult::create(std::move(insufficient)).has_value(),
                "insufficient evidence remains a valid non-failure analytical outcome");
}

auto policy() -> analytical::PolicyDefinition {
    return require_value(analytical::PolicyDefinition::create(
        {stable<analytical::PolicyId>("policy-a"),
         "recommendation-policy",
         "1",
         scope(),
         {{"maximize-value", "maximize expected value", 1, 1.0}},
         analytical::ObjectiveRelationship::Weighted,
         {{"risk-limit", "risk must remain bounded", analytical::ConstraintKind::Hard,
           std::nullopt},
          {"latency", "prefer lower latency", analytical::ConstraintKind::Soft, 0.1}},
         std::nullopt,
         stable<evolution::AlgorithmId>("policy-evaluator"),
         "1",
         true}));
}

auto evaluate_policy(std::string digest = "decision-golden") -> analytical::Decision {
    const auto candidate_id = stable<analytical::DecisionCandidateId>("candidate-a");
    analytical::DecisionCandidate candidate{
        candidate_id,
        "recommend-a",
        std::string("A"),
        analytical::Feasibility::Feasible,
        {{"utility", 10.0}},
        {{"risk-limit", true, std::nullopt, "within risk limit"},
         {"latency", false, 0.1, "accepted soft latency tradeoff"}}};
    return require_value(analytical::Decision::create(
        {stable<analytical::DecisionId>("decision-a"),
         policy().data().id,
         "1",
         analytical::DecisionCompletion::Selected,
         analytical::DecisionStatus::Proposed,
         scope(),
         evidence(),
         policy().data().objectives,
         policy().data().constraints,
         {candidate},
         analytical::Feasibility::Feasible,
         analytical::DecisionOutcome{candidate_id, std::string("A")},
         {"candidate A has the highest feasible utility"},
         require_value(analytical::Confidence::create(0.85, "policy confidence")),
         analytical::Uncertainty{"evidence", 0.1, 0.2, "input uncertainty"},
         analytical::DecisionRisk{"expected-loss", 0.05, "probability-weighted loss"},
         provenance_id(),
         reproducibility(std::move(digest))}));
}

template <typename Value>
concept HasActionMember = requires(Value value) { value.action; };

void test_decision(TestRunner &test) {
    static_assert(!HasActionMember<analytical::DecisionData>);
    const auto decision = evaluate_policy();
    test.expect(std::get<std::string>(decision.data().outcome->value) == "A" &&
                    decision.data().candidates.front().constraints.back().satisfied == false,
                "policy evaluation may select a feasible candidate with an explicit soft tradeoff");

    const auto repeated = evaluate_policy();
    test.expect(repeated.data().outcome->candidate_id == decision.data().outcome->candidate_id &&
                    repeated.data().reasoning == decision.data().reasoning &&
                    repeated.data().reproducibility.semantic_digest ==
                        decision.data().reproducibility.semantic_digest,
                "deterministic policy evaluation reproduces the same semantic decision");

    auto hard_violation = decision.data();
    hard_violation.candidates.front().constraints.front().satisfied = false;
    test.expect(!analytical::Decision::create(std::move(hard_violation)),
                "selected outcomes cannot violate hard constraints");

    const auto accepted = require_value(decision.transition(analytical::DecisionStatus::Accepted));
    const auto executed = require_value(accepted.transition(analytical::DecisionStatus::Executed));
    test.expect(executed.data().status == analytical::DecisionStatus::Executed &&
                    !executed.transition(analytical::DecisionStatus::Accepted),
                "decision lifecycle is explicit and terminal execution cannot transition backward");

    auto no_decision = decision.data();
    no_decision.id = stable<analytical::DecisionId>("decision-none");
    no_decision.completion = analytical::DecisionCompletion::NoDecision;
    no_decision.feasibility = analytical::Feasibility::Infeasible;
    no_decision.outcome.reset();
    no_decision.evidence.clear();
    test.expect(analytical::Decision::create(std::move(no_decision)).has_value(),
                "no-decision is a valid policy outcome distinct from evaluation failure");
}

void test_processor_contracts(TestRunner &test) {
    using Aggregator = analytical::AggregatorProcessor<int, int, int>;
    using Detector = analytical::PatternDetectorProcessor<int, int>;
    using Analyzer = analytical::AnalyzerProcessor<int>;
    using Evaluator = analytical::DecisionEvaluatorProcessor<int>;
    static_assert(
        std::same_as<Aggregator,
                     evolution::processing::Processor<analytical::AggregationRequest<int>, int,
                                                      analytical::AggregationResult<int>>>);
    static_assert(
        std::same_as<Detector,
                     evolution::processing::Processor<analytical::PatternDetectionRequest<int>, int,
                                                      analytical::PatternResult>>);
    static_assert(
        std::same_as<Analyzer, evolution::processing::Processor<analytical::AnalysisRequest, int,
                                                                analytical::AnalysisResult>>);
    static_assert(
        std::same_as<Evaluator,
                     evolution::processing::Processor<analytical::DecisionEvaluationRequest, int,
                                                      analytical::Decision>>);
    test.expect(true, "analytical mechanisms compose through distinct processor contracts");
}

} // namespace

int main() {
    TestRunner test;
    try {
        test_aggregation(test);
        test_pattern(test);
        test_analysis(test);
        test_decision(test);
        test_processor_contracts(test);
    } catch (const std::exception &exception) {
        std::cerr << "UNEXPECTED: " << exception.what() << '\n';
        return 1;
    }
    return test.failures() == 0 ? 0 : 1;
}
