#include "evolution/extensions/contracts.hpp"

#include <algorithm>
#include <utility>

namespace evolution::extensions {
namespace {

auto invalid_descriptor(std::string message) -> Result<ExtensionDescriptor> {
    return Result<ExtensionDescriptor>::failure(
        Error(ErrorCode::create("extension.invalid_descriptor"), ErrorCategory::InvalidArgument,
              std::move(message)));
}

} // namespace

ExtensionDescriptor::ExtensionDescriptor(ExtensionDescriptorData data) : data_(std::move(data)) {}

auto ExtensionDescriptor::create(ExtensionDescriptorData data) -> Result<ExtensionDescriptor> {
    if (data.name.empty() || data.version.empty()) {
        return invalid_descriptor("extension name and version cannot be empty");
    }
    if (std::any_of(data.capabilities.begin(), data.capabilities.end(),
                    [](const auto &capability) { return capability.empty(); })) {
        return invalid_descriptor("extension capabilities cannot be empty");
    }
    std::set<std::pair<std::string, std::string>> contracts;
    for (const auto &contract : data.contracts) {
        if (contract.identity.empty() || contract.version.empty()) {
            return invalid_descriptor("contract identity and version cannot be empty");
        }
        if (!contracts.emplace(contract.identity, contract.version).second) {
            return invalid_descriptor("extension contracts must be unique");
        }
    }
    std::set<std::pair<std::string, std::string>> dependencies;
    for (const auto &dependency : data.dependencies) {
        if (dependency.version.empty()) {
            return invalid_descriptor("dependency version cannot be empty");
        }
        if (dependency.extension_id == data.id && dependency.version == data.version) {
            return invalid_descriptor("an extension cannot depend on itself");
        }
        if (!dependencies.emplace(dependency.extension_id.to_string(), dependency.version).second) {
            return invalid_descriptor("extension dependencies must be unique");
        }
    }
    if (data.configuration_schema && data.configuration_schema->empty()) {
        return invalid_descriptor("configuration schema identity cannot be empty");
    }
    return Result<ExtensionDescriptor>::success(ExtensionDescriptor(std::move(data)));
}

auto ExtensionDescriptor::data() const noexcept -> const ExtensionDescriptorData & {
    return data_;
}

auto ExtensionDescriptor::supports(std::string_view capability) const -> bool {
    return data_.capabilities.contains(std::string(capability));
}

auto ExtensionDescriptor::implements(std::string_view contract_identity,
                                     std::string_view contract_version) const -> bool {
    return std::any_of(data_.contracts.begin(), data_.contracts.end(), [&](const auto &contract) {
        return contract.identity == contract_identity && contract.version == contract_version;
    });
}

auto ExactExtensionCompatibility::evaluate(const ExtensionDescriptor &descriptor,
                                           const CompatibilityRequest &request) const
    -> CompatibilityResult {
    CompatibilityResult result{CompatibilityStatus::Compatible, {}};
    const auto &data = descriptor.data();
    if (data.kind != request.kind) {
        result.reasons.emplace_back("extension kind does not match");
    }
    for (const auto &capability : request.required_capabilities) {
        if (!descriptor.supports(capability)) {
            result.reasons.push_back("missing capability: " + capability);
        }
    }
    for (const auto &contract : request.required_contracts) {
        if (!descriptor.implements(contract.identity, contract.version)) {
            result.reasons.push_back("missing exact contract: " + contract.identity + '@' +
                                     contract.version);
        }
    }
    if (static_cast<int>(request.available_isolation) <
        static_cast<int>(data.isolation.minimum_level)) {
        result.reasons.emplace_back("required isolation level is unavailable");
    }
    if (data.isolation.trust == TrustRequirement::TrustedOnly && !request.trusted) {
        result.reasons.emplace_back("extension requires a trusted execution boundary");
    }
    if (!result.reasons.empty()) {
        result.status = CompatibilityStatus::Incompatible;
    }
    return result;
}

} // namespace evolution::extensions
