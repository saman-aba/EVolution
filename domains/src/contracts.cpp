#include "evolution/domain/contracts.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace evolution::domain {
namespace {

auto invalid_domain(std::string message) -> Result<DomainDescriptor> {
    return Result<DomainDescriptor>::failure(Error(ErrorCode::create("domain.invalid_descriptor"),
                                                   ErrorCategory::InvalidArgument,
                                                   std::move(message)));
}

} // namespace

DomainDescriptor::DomainDescriptor(DomainDescriptorData data) : data_(std::move(data)) {}

auto DomainDescriptor::create(DomainDescriptorData data) -> Result<DomainDescriptor> {
    if (data.name.empty() || data.version.empty()) {
        return invalid_domain("domain name and version cannot be empty");
    }
    std::set<std::string> identities;
    std::set<std::pair<std::string, DomainConceptKind>> names;
    for (const auto &concept_descriptor : data.concepts) {
        if (concept_descriptor.name.empty() || concept_descriptor.version.empty()) {
            return invalid_domain("domain concept name and version cannot be empty");
        }
        if (concept_descriptor.schema && concept_descriptor.schema->empty()) {
            return invalid_domain("domain concept schema identity cannot be empty");
        }
        if (!identities.emplace(concept_descriptor.id.to_string()).second ||
            !names.emplace(concept_descriptor.name, concept_descriptor.kind).second) {
            return invalid_domain("domain concepts must have unique identities and names per kind");
        }
    }
    return Result<DomainDescriptor>::success(DomainDescriptor(std::move(data)));
}

auto DomainDescriptor::data() const noexcept -> const DomainDescriptorData & {
    return data_;
}

auto DomainDescriptor::concepts(DomainConceptKind kind) const
    -> std::vector<DomainConceptDescriptor> {
    std::vector<DomainConceptDescriptor> matches;
    std::copy_if(data_.concepts.begin(), data_.concepts.end(), std::back_inserter(matches),
                 [&](const auto &concept_descriptor) { return concept_descriptor.kind == kind; });
    return matches;
}

auto DomainDescriptor::semantic_versions() const -> std::vector<VersionReference> {
    std::vector<VersionReference> versions{{"domain:" + data_.name, data_.version}};
    versions.reserve(data_.concepts.size() + 1);
    for (const auto &concept_descriptor : data_.concepts) {
        versions.push_back(
            {"domain-concept:" + concept_descriptor.name, concept_descriptor.version});
    }
    return versions;
}

DomainValidationReport::DomainValidationReport(std::vector<DomainViolation> violations)
    : violations_(std::move(violations)) {}

auto DomainValidationReport::create(std::vector<DomainViolation> violations)
    -> Result<DomainValidationReport> {
    for (const auto &violation : violations) {
        if (violation.invariant.empty() || violation.code.empty() || violation.message.empty()) {
            return Result<DomainValidationReport>::failure(
                Error(ErrorCode::create("domain.invalid_violation"), ErrorCategory::InvalidArgument,
                      "domain violations require invariant, code, and message"));
        }
    }
    return Result<DomainValidationReport>::success(DomainValidationReport(std::move(violations)));
}

auto DomainValidationReport::valid() const noexcept -> bool {
    return std::none_of(violations_.begin(), violations_.end(), [](const auto &violation) {
        return violation.severity == DomainViolationSeverity::Error;
    });
}

auto DomainValidationReport::violations() const noexcept -> const std::vector<DomainViolation> & {
    return violations_;
}

auto make_domain_extension_descriptor(const DomainDescriptor &domain,
                                      std::set<std::string> capabilities,
                                      std::vector<extensions::ContractReference> contracts)
    -> Result<extensions::ExtensionDescriptor> {
    capabilities.insert("domain");
    for (const auto &concept_descriptor : domain.data().concepts) {
        capabilities.insert("domain-concept:" + concept_descriptor.name);
    }
    return extensions::ExtensionDescriptor::create(
        {domain.data().extension_id,
         domain.data().name,
         domain.data().version,
         extensions::ExtensionKind::Domain,
         std::move(capabilities),
         std::move(contracts),
         {},
         {false, false, false, false},
         {extensions::IsolationLevel::InProcess, extensions::TrustRequirement::TrustedOnly, false,
          false},
         std::nullopt});
}

} // namespace evolution::domain
