#pragma once

#include "evolution/core/context/resource_view.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/runtime/api.hpp"

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace evolution::runtime {

struct DeploymentTag;
struct RuntimeInstanceTag;
using DeploymentId = identity::Id<DeploymentTag>;
using RuntimeInstanceId = identity::Id<RuntimeInstanceTag>;

enum class PlacementKind { InProcess, Process, Remote };
enum class DependencyRequirement { Required, Optional, Conditional };
enum class ShutdownMode { Drain, Cancel, Immediate };
enum class RestartMode { Never, OnFailure, Always };
enum class ConfigurationDeliveryKind { ApplicationOwnedEffective, Referenced };

struct ComponentPlacement {
    PlacementKind kind{PlacementKind::InProcess};
    std::string deployment_unit;
    std::optional<std::string> communication_contract;
};

struct RuntimeDependency {
    std::string identity;
    std::string capability;
    DependencyRequirement requirement{DependencyRequirement::Required};
    std::optional<std::string> required_version;
    std::optional<std::string> condition;
};

struct ConfigurationDelivery {
    ConfigurationDeliveryKind kind{ConfigurationDeliveryKind::ApplicationOwnedEffective};
    std::optional<std::string> source_reference;
    bool secrets_externalized{true};
};

struct HealthPolicy {
    std::chrono::milliseconds evaluation_interval{};
    std::size_t unhealthy_threshold{};
};

struct ReadinessPolicy {
    bool require_all_required_dependencies{true};
    bool allow_degraded_application{};
};

struct StartupPolicy {
    std::chrono::milliseconds timeout{};
    bool fail_on_degraded{true};
};

struct ShutdownPolicy {
    ShutdownMode mode{ShutdownMode::Drain};
    std::chrono::milliseconds timeout{};
};

struct RestartPolicy {
    RestartMode mode{RestartMode::Never};
    std::size_t maximum_attempts{};
    std::chrono::milliseconds backoff{};
};

struct RuntimeComponentManifest {
    ComponentId component_id;
    std::string component_version;
    ComponentPlacement placement;
    context::ResourceView resources;
    std::vector<RuntimeDependency> dependencies;
};

struct RuntimeManifestData {
    DeploymentId deployment_id;
    RuntimeInstanceId instance_id;
    std::string deployment_version;
    std::string application_identity;
    std::string application_version;
    ConfigurationId configuration_id;
    ConfigurationDelivery configuration_delivery;
    std::vector<RuntimeComponentManifest> components;
    HealthPolicy health;
    ReadinessPolicy readiness;
    StartupPolicy startup;
    ShutdownPolicy shutdown;
    RestartPolicy restart;
};

class EVOLUTION_RUNTIME_API RuntimeManifest {
  public:
    static auto create(RuntimeManifestData data) -> Result<RuntimeManifest>;
    [[nodiscard]] auto data() const noexcept -> const RuntimeManifestData &;
    [[nodiscard]] auto supports_in_process_execution() const noexcept -> bool;

  private:
    explicit RuntimeManifest(RuntimeManifestData data);
    RuntimeManifestData data_;
};

} // namespace evolution::runtime
