#pragma once

#include "evolution/applications/api.hpp"
#include "evolution/core/configuration/configuration.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace evolution::applications {

struct ApplicationTag;
using ApplicationId = identity::Id<ApplicationTag>;

enum class ApplicationState { Created, Starting, Running, Degraded, Stopping, Stopped, Failed };
enum class ComponentOperationalState { Created, Configured, Running, Degraded, Stopped, Failed };

struct ApplicationComponentDescriptor {
    ComponentId id;
    std::string name;
    std::vector<ComponentId> dependencies;
    bool required{true};
};

struct ComponentOperationalStatus {
    ComponentId id;
    ComponentOperationalState state{ComponentOperationalState::Created};
    std::optional<Error> failure;
};

class ApplicationComponent {
  public:
    virtual ~ApplicationComponent() = default;
    [[nodiscard]] virtual auto descriptor() const noexcept
        -> const ApplicationComponentDescriptor & = 0;
    virtual auto configure(const Configuration &configuration) -> Result<void> = 0;
    virtual auto start(const ExecutionContext &context) -> Result<void> = 0;
    virtual auto stop() -> Result<void> = 0;
    [[nodiscard]] virtual auto status() const -> ComponentOperationalStatus = 0;
};

struct ApplicationDescriptor {
    ApplicationId id;
    std::string name;
    std::string version;
    bool degraded_start_allowed{};
};

struct ApplicationOperationalStatus {
    ApplicationState state{ApplicationState::Created};
    std::vector<ComponentOperationalStatus> components;
    std::optional<Error> failure;
};

struct ApplicationStateData;

class EVOLUTION_APPLICATIONS_API Application {
  public:
    static auto create(ApplicationDescriptor descriptor, Configuration effective_configuration,
                       std::vector<std::shared_ptr<ApplicationComponent>> components)
        -> Result<Application>;

    auto start(const ExecutionContext &context) -> Result<void>;
    auto shutdown() -> Result<void>;

    [[nodiscard]] auto descriptor() const noexcept -> const ApplicationDescriptor &;
    [[nodiscard]] auto effective_configuration() const noexcept -> const Configuration &;
    [[nodiscard]] auto status() const -> ApplicationOperationalStatus;

  private:
    explicit Application(std::shared_ptr<ApplicationStateData> state);
    std::shared_ptr<ApplicationStateData> state_;
};

} // namespace evolution::applications
