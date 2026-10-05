#pragma once

#include "evolution/analysis/aggregation.hpp"
#include "evolution/analysis/analysis.hpp"
#include "evolution/analysis/decision.hpp"
#include "evolution/analysis/pattern.hpp"
#include "evolution/processing/processor.hpp"

#include <vector>

namespace evolution::analysis {

template <typename Input> struct AggregationRequest {
    AggregationDefinition definition;
    std::vector<Input> inputs;
};

template <typename Input> struct PatternDetectionRequest {
    PatternDefinition definition;
    std::vector<Input> inputs;
};

struct AnalysisRequest {
    AnalysisDefinition definition;
    std::vector<EvidenceReference> evidence;
    AnalyticalScope scope;
};

struct DecisionEvaluationRequest {
    PolicyDefinition policy;
    std::vector<EvidenceReference> evidence;
    std::vector<DecisionCandidate> candidates;
    AnalyticalScope scope;
};

template <typename Input, typename State, typename Output>
using AggregatorProcessor =
    processing::Processor<AggregationRequest<Input>, State, AggregationResult<Output>>;

template <typename Input, typename State>
using PatternDetectorProcessor =
    processing::Processor<PatternDetectionRequest<Input>, State, PatternResult>;

template <typename State>
using AnalyzerProcessor = processing::Processor<AnalysisRequest, State, AnalysisResult>;

template <typename State>
using DecisionEvaluatorProcessor =
    processing::Processor<DecisionEvaluationRequest, State, Decision>;

} // namespace evolution::analysis
