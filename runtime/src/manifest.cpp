#include "evolution/runtime/manifest.hpp"

#include <set>
#include <utility>

namespace evolution::runtime {
namespace {

template <typename Value> auto invalid(std::string code, std::string message) -> Result<Value> {
    return Result<Value>::failure(Error(ErrorCode::create(std::move(code)),
                                        ErrorCategory::InvalidConfiguration, std::move(message)));
}

} // namespace

RuntimeManifest::RuntimeManifest(RuntimeManifestData data) : data_(std::move(data)) {}

auto RuntimeManifest::create(RuntimeManifestData data) -> Result<RuntimeManifest> {
    if (data.deployment_version.empty() || data.application_identity.empty() ||
        data.application_version.empty() || data.components.empty()) {
        return invalid<RuntimeManifest>("runtime.invalid_manifest",
                                        "runtime manifest requires deployment, application, "
                                        "configuration, and component information");
    }
    if (data.configuration_delivery.kind == ConfigurationDeliveryKind::Referenced &&
        (!data.configuration_delivery.source_reference ||
         data.configuration_delivery.source_reference->empty())) {
        return invalid<RuntimeManifest>(
            "runtime.missing_configuration_reference",
            "referenced configuration delivery requires an explicit source reference");
    }
    if (!data.configuration_delivery.secrets_externalized) {
        return invalid<RuntimeManifest>(
            "runtime.embedded_secrets",
            "runtime manifests cannot embed secret values in configuration delivery");
    }
    if (data.health.evaluation_interval <= std::chrono::milliseconds::zero() ||
        data.health.unhealthy_threshold == 0 ||
        data.startup.timeout <= std::chrono::milliseconds::zero() ||
        data.shutdown.timeout <= std::chrono::milliseconds::zero()) {
        return invalid<RuntimeManifest>("runtime.invalid_policy",
                                        "runtime timing and threshold policies must be positive");
    }
    if (data.restart.mode == RestartMode::Never && data.restart.maximum_attempts != 0) {
        return invalid<RuntimeManifest>("runtime.invalid_restart_policy",
                                        "restart attempts must be zero when restart is disabled");
    }
    if (data.restart.mode != RestartMode::Never && data.restart.maximum_attempts == 0) {
        return invalid<RuntimeManifest>("runtime.invalid_restart_policy",
                                        "enabled restart policy requires a positive attempt limit");
    }

    std::set<std::string> component_ids;
    for (const auto &component : data.components) {
        if (component.component_version.empty() || component.placement.deployment_unit.empty()) {
            return invalid<RuntimeManifest>("runtime.invalid_component",
                                            "runtime component requires version and placement");
        }
        if (!component_ids.insert(component.component_id.to_string()).second) {
            return invalid<RuntimeManifest>("runtime.duplicate_component",
                                            "runtime component identities must be unique");
        }
        if (component.placement.kind != PlacementKind::InProcess &&
            (!component.placement.communication_contract ||
             component.placement.communication_contract->empty())) {
            return invalid<RuntimeManifest>(
                "runtime.missing_communication_contract",
                "physical boundaries require an explicit communication contract");
        }
        for (const auto &dependency : component.dependencies) {
            if (dependency.identity.empty() || dependency.capability.empty()) {
                return invalid<RuntimeManifest>(
                    "runtime.invalid_dependency",
                    "runtime dependency identity and capability are required");
            }
            if (dependency.requirement == DependencyRequirement::Conditional &&
                (!dependency.condition || dependency.condition->empty())) {
                return invalid<RuntimeManifest>(
                    "runtime.missing_dependency_condition",
                    "conditional runtime dependencies require an explicit condition");
            }
        }
    }
    return Result<RuntimeManifest>::success(RuntimeManifest(std::move(data)));
}

auto RuntimeManifest::data() const noexcept -> const RuntimeManifestData & {
    return data_;
}

auto RuntimeManifest::supports_in_process_execution() const noexcept -> bool {
    for (const auto &component : data_.components) {
        if (component.placement.kind != PlacementKind::InProcess) {
            return false;
        }
    }
    return true;
}

} // namespace evolution::runtime
