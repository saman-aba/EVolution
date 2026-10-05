#include "evolution/analysis/common.hpp"

#include <cmath>
#include <set>
#include <utility>

namespace evolution::analysis {
namespace {

auto invalid(std::string code, std::string message) -> Result<void> {
    return Result<void>::failure(Error(ErrorCode::create(std::move(code)),
                                       ErrorCategory::InvalidArgument, std::move(message)));
}

} // namespace

Confidence::Confidence(double value, std::string semantics)
    : value_(value), semantics_(std::move(semantics)) {}

auto Confidence::create(double value, std::string semantics) -> Result<Confidence> {
    if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
        return Result<Confidence>::failure(Error(ErrorCode::create("analysis.invalid_confidence"),
                                                 ErrorCategory::InvalidArgument,
                                                 "confidence must be finite and within [0, 1]"));
    }
    if (semantics.empty()) {
        return Result<Confidence>::failure(
            Error(ErrorCode::create("analysis.missing_confidence_semantics"),
                  ErrorCategory::InvalidArgument,
                  "confidence requires explicit interpretation semantics"));
    }
    return Result<Confidence>::success(Confidence(value, std::move(semantics)));
}

auto Confidence::value() const noexcept -> double {
    return value_;
}

auto Confidence::semantics() const noexcept -> const std::string & {
    return semantics_;
}

auto validate_evidence(const std::vector<EvidenceReference> &evidence, bool require_non_empty)
    -> Result<void> {
    if (require_non_empty && evidence.empty()) {
        return invalid("analysis.missing_evidence", "analytical evidence cannot be empty");
    }
    std::set<std::pair<std::string, std::string>> identities;
    for (const auto &item : evidence) {
        if (item.kind.empty() || item.identity.empty() || item.role.empty()) {
            return invalid("analysis.invalid_evidence",
                           "evidence requires kind, identity, and role");
        }
        if (item.version && item.version->empty()) {
            return invalid("analysis.invalid_evidence_version", "evidence version cannot be empty");
        }
        if (!identities.emplace(item.kind, item.identity).second) {
            return invalid("analysis.duplicate_evidence",
                           "evidence kind and identity must be unique");
        }
    }
    return Result<void>::success();
}

auto validate_scope(const AnalyticalScope &scope) -> Result<void> {
    if (scope.kind.empty() || scope.identity.empty()) {
        return invalid("analysis.invalid_scope", "analytical scope requires kind and identity");
    }
    for (const auto &[name, value] : scope.dimensions) {
        if (name.empty() || value.empty()) {
            return invalid("analysis.invalid_scope_dimension",
                           "scope dimensions require non-empty names and values");
        }
    }
    return Result<void>::success();
}

auto validate_uncertainty(const Uncertainty &uncertainty) -> Result<void> {
    if (uncertainty.kind.empty() || uncertainty.description.empty()) {
        return invalid("analysis.invalid_uncertainty", "uncertainty requires kind and description");
    }
    if ((uncertainty.lower && !std::isfinite(*uncertainty.lower)) ||
        (uncertainty.upper && !std::isfinite(*uncertainty.upper)) ||
        (uncertainty.lower && uncertainty.upper && *uncertainty.upper < *uncertainty.lower)) {
        return invalid("analysis.invalid_uncertainty_bounds",
                       "uncertainty bounds must be finite and ordered");
    }
    return Result<void>::success();
}

auto validate_reproducibility(const AnalyticalReproducibility &reproducibility) -> Result<void> {
    if (reproducibility.algorithm_version.empty()) {
        return invalid("analysis.invalid_reproducibility",
                       "reproducibility requires an algorithm version");
    }
    for (const auto &version : reproducibility.semantic_versions) {
        if (version.name.empty() || version.version.empty()) {
            return invalid("analysis.invalid_semantic_version",
                           "semantic version references cannot be empty");
        }
    }
    if (reproducibility.semantic_digest && reproducibility.semantic_digest->empty()) {
        return invalid("analysis.invalid_semantic_digest",
                       "semantic result digest cannot be empty");
    }
    return Result<void>::success();
}

} // namespace evolution::analysis
