#include "evolution/analysis/analysis.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace evolution::analysis {
namespace {

template <typename Value> auto invalid(std::string message) -> Result<Value> {
    return Result<Value>::failure(Error(ErrorCode::create("analysis.invalid_analysis"),
                                        ErrorCategory::InvalidArgument, std::move(message)));
}

auto validate_texts(const std::vector<std::string> &values, std::string_view label)
    -> Result<void> {
    if (std::any_of(values.begin(), values.end(),
                    [](const auto &value) { return value.empty(); })) {
        return Result<void>::failure(Error(ErrorCode::create("analysis.invalid_analysis_text"),
                                           ErrorCategory::InvalidArgument,
                                           std::string(label) + " entries cannot be empty"));
    }
    return Result<void>::success();
}

} // namespace

AnalysisDefinition::AnalysisDefinition(AnalysisDefinitionData data) : data_(std::move(data)) {}

auto AnalysisDefinition::create(AnalysisDefinitionData data) -> Result<AnalysisDefinition> {
    if (data.name.empty() || data.version.empty() || data.analyzer_version.empty() ||
        data.algorithm_version.empty()) {
        return invalid<AnalysisDefinition>(
            "analysis definition names and versions cannot be empty");
    }
    return Result<AnalysisDefinition>::success(AnalysisDefinition(std::move(data)));
}

auto AnalysisDefinition::data() const noexcept -> const AnalysisDefinitionData & {
    return data_;
}

AnalysisResult::AnalysisResult(AnalysisResultData data) : data_(std::move(data)) {}

auto AnalysisResult::create(AnalysisResultData data) -> Result<AnalysisResult> {
    if (data.definition_version.empty()) {
        return invalid<AnalysisResult>("analysis result requires a definition version");
    }
    auto scope = validate_scope(data.scope);
    if (!scope) {
        return Result<AnalysisResult>::failure(scope.error());
    }
    auto evidence = validate_evidence(data.evidence,
                                      data.completion != AnalysisCompletion::InsufficientEvidence);
    if (!evidence) {
        return Result<AnalysisResult>::failure(evidence.error());
    }
    if (data.completion == AnalysisCompletion::Complete && data.findings.empty()) {
        return invalid<AnalysisResult>("complete analysis requires at least one finding");
    }
    if (data.completion == AnalysisCompletion::Complete && data.reasoning.empty()) {
        return invalid<AnalysisResult>("complete analysis requires explicit reasoning");
    }
    if (data.completion == AnalysisCompletion::InsufficientEvidence && !data.findings.empty()) {
        return invalid<AnalysisResult>(
            "insufficient-evidence analysis cannot present completed findings");
    }
    for (const auto *texts : {&data.reasoning, &data.assumptions, &data.limitations}) {
        auto validation = validate_texts(*texts, "analysis text");
        if (!validation) {
            return Result<AnalysisResult>::failure(validation.error());
        }
    }
    if (data.uncertainty) {
        auto validation = validate_uncertainty(*data.uncertainty);
        if (!validation) {
            return Result<AnalysisResult>::failure(validation.error());
        }
    }
    std::set<std::string> identities;
    for (const auto &finding : data.findings) {
        if (finding.statement.empty() || !identities.insert(finding.id.to_string()).second) {
            return invalid<AnalysisResult>("findings require unique identities and statements");
        }
        auto finding_evidence = validate_evidence(finding.evidence);
        if (!finding_evidence) {
            return Result<AnalysisResult>::failure(finding_evidence.error());
        }
        auto reasoning = validate_texts(finding.reasoning, "finding reasoning");
        if (!reasoning) {
            return Result<AnalysisResult>::failure(reasoning.error());
        }
        if (finding.claim_strength == ClaimStrength::CausalClaim && finding.reasoning.empty()) {
            return invalid<AnalysisResult>("causal findings require explicit methodology");
        }
        if (finding.uncertainty) {
            auto uncertainty = validate_uncertainty(*finding.uncertainty);
            if (!uncertainty) {
                return Result<AnalysisResult>::failure(uncertainty.error());
            }
        }
    }
    if (data.baseline) {
        if (data.baseline->identity.empty() || data.baseline->description.empty()) {
            return invalid<AnalysisResult>("analysis baseline requires identity and description");
        }
        auto baseline_evidence = validate_evidence(data.baseline->evidence);
        if (!baseline_evidence) {
            return Result<AnalysisResult>::failure(baseline_evidence.error());
        }
    }
    identities.clear();
    for (const auto &hypothesis : data.hypotheses) {
        if (hypothesis.statement.empty() || !identities.insert(hypothesis.id.to_string()).second) {
            return invalid<AnalysisResult>("hypotheses require unique identities and statements");
        }
        auto hypothesis_evidence =
            validate_evidence(hypothesis.evidence, hypothesis.status != HypothesisStatus::Untested);
        if (!hypothesis_evidence) {
            return Result<AnalysisResult>::failure(hypothesis_evidence.error());
        }
    }
    identities.clear();
    for (const auto &prediction : data.predictions) {
        if (prediction.target.empty() || !identities.insert(prediction.id.to_string()).second) {
            return invalid<AnalysisResult>("predictions require unique identities and targets");
        }
        if (prediction.uncertainty) {
            auto uncertainty = validate_uncertainty(*prediction.uncertainty);
            if (!uncertainty) {
                return Result<AnalysisResult>::failure(uncertainty.error());
            }
        }
    }
    auto reproducibility = validate_reproducibility(data.reproducibility);
    if (!reproducibility) {
        return Result<AnalysisResult>::failure(reproducibility.error());
    }
    return Result<AnalysisResult>::success(AnalysisResult(std::move(data)));
}

auto AnalysisResult::data() const noexcept -> const AnalysisResultData & {
    return data_;
}

} // namespace evolution::analysis
