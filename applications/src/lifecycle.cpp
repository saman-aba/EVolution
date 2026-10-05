#include "evolution/applications/lifecycle.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <utility>

namespace evolution::applications {

struct ApplicationStateData {
    ApplicationStateData(ApplicationDescriptor application_descriptor,
                         Configuration effective_configuration,
                         std::vector<std::shared_ptr<ApplicationComponent>> application_components,
                         std::vector<std::size_t> order)
        : descriptor(std::move(application_descriptor)),
          configuration(std::move(effective_configuration)),
          components(std::move(application_components)), startup_order(std::move(order)) {}

    ApplicationDescriptor descriptor;
    Configuration configuration;
    std::vector<std::shared_ptr<ApplicationComponent>> components;
    std::vector<std::size_t> startup_order;
    std::vector<std::size_t> started_components;
    mutable std::mutex mutex;
    ApplicationState state{ApplicationState::Created};
    std::optional<Error> failure;
};

namespace {

template <typename Value> auto invalid(std::string code, std::string message) -> Result<Value> {
    return Result<Value>::failure(Error(ErrorCode::create(std::move(code)),
                                        ErrorCategory::InvalidArgument, std::move(message)));
}

auto calculate_order(const std::vector<std::shared_ptr<ApplicationComponent>> &components)
    -> Result<std::vector<std::size_t>> {
    std::map<std::string, std::size_t> indexes;
    for (std::size_t index = 0; index < components.size(); ++index) {
        if (!components[index] || components[index]->descriptor().name.empty() ||
            !indexes.emplace(components[index]->descriptor().id.to_string(), index).second) {
            return invalid<std::vector<std::size_t>>("application.invalid_component",
                                                     "components must be non-null and unique");
        }
    }
    enum class Visit { Visiting, Complete };
    std::map<std::string, Visit> visits;
    std::vector<std::size_t> order;
    std::function<Result<void>(std::size_t)> visit;
    visit = [&](std::size_t index) -> Result<void> {
        const auto key = components[index]->descriptor().id.to_string();
        const auto existing = visits.find(key);
        if (existing != visits.end()) {
            if (existing->second == Visit::Visiting) {
                return Result<void>::failure(Error(
                    ErrorCode::create("application.dependency_cycle"), ErrorCategory::Conflict,
                    "application component dependency graph contains a cycle"));
            }
            return Result<void>::success();
        }
        visits.emplace(key, Visit::Visiting);
        for (const auto &dependency : components[index]->descriptor().dependencies) {
            const auto dependency_index = indexes.find(dependency.to_string());
            if (dependency_index == indexes.end()) {
                return Result<void>::failure(Error(
                    ErrorCode::create("application.missing_dependency"), ErrorCategory::NotFound,
                    "application component dependency is not present"));
            }
            auto result = visit(dependency_index->second);
            if (!result) {
                return result;
            }
        }
        visits[key] = Visit::Complete;
        order.push_back(index);
        return Result<void>::success();
    };
    for (std::size_t index = 0; index < components.size(); ++index) {
        auto result = visit(index);
        if (!result) {
            return Result<std::vector<std::size_t>>::failure(result.error());
        }
    }
    return Result<std::vector<std::size_t>>::success(std::move(order));
}

} // namespace

Application::Application(std::shared_ptr<ApplicationStateData> state) : state_(std::move(state)) {}

auto Application::create(ApplicationDescriptor descriptor, Configuration effective_configuration,
                         std::vector<std::shared_ptr<ApplicationComponent>> components)
    -> Result<Application> {
    if (descriptor.name.empty() || descriptor.version.empty() || components.empty()) {
        return invalid<Application>("application.invalid_descriptor",
                                    "application requires name, version, and components");
    }
    auto order = calculate_order(components);
    if (!order) {
        return Result<Application>::failure(order.error());
    }
    return Result<Application>::success(Application(std::make_shared<ApplicationStateData>(
        std::move(descriptor), std::move(effective_configuration), std::move(components),
        std::move(order).value())));
}

auto Application::start(const ExecutionContext &context) -> Result<void> {
    std::lock_guard lock(state_->mutex);
    if (state_->state != ApplicationState::Created && state_->state != ApplicationState::Stopped) {
        return Result<void>::failure(
            Error(ErrorCode::create("application.invalid_start_state"), ErrorCategory::Conflict,
                  "application can only start from created or stopped state"));
    }
    state_->state = ApplicationState::Starting;
    state_->failure.reset();
    state_->started_components.clear();
    std::vector<std::size_t> started;
    bool degraded{};
    for (const auto index : state_->startup_order) {
        auto configured = state_->components[index]->configure(state_->configuration);
        auto started_result = configured ? state_->components[index]->start(context) : configured;
        if (started_result) {
            started.push_back(index);
            continue;
        }
        const bool may_degrade = state_->descriptor.degraded_start_allowed &&
                                 !state_->components[index]->descriptor().required;
        if (may_degrade) {
            degraded = true;
            continue;
        }
        for (auto iterator = started.rbegin(); iterator != started.rend(); ++iterator) {
            static_cast<void>(state_->components[*iterator]->stop());
        }
        state_->started_components.clear();
        state_->state = ApplicationState::Failed;
        state_->failure = started_result.error();
        return Result<void>::failure(started_result.error());
    }
    state_->started_components = std::move(started);
    state_->state = degraded ? ApplicationState::Degraded : ApplicationState::Running;
    return Result<void>::success();
}

auto Application::shutdown() -> Result<void> {
    std::lock_guard lock(state_->mutex);
    if (state_->state != ApplicationState::Running && state_->state != ApplicationState::Degraded &&
        state_->state != ApplicationState::Failed) {
        return Result<void>::failure(Error(ErrorCode::create("application.invalid_shutdown_state"),
                                           ErrorCategory::Conflict,
                                           "application is not in a shutdown-capable state"));
    }
    state_->state = ApplicationState::Stopping;
    std::optional<Error> failure;
    for (auto iterator = state_->started_components.rbegin();
         iterator != state_->started_components.rend(); ++iterator) {
        auto stopped = state_->components[*iterator]->stop();
        if (!stopped && !failure) {
            failure = stopped.error();
        }
    }
    state_->started_components.clear();
    if (failure) {
        state_->state = ApplicationState::Failed;
        state_->failure = failure;
        return Result<void>::failure(*failure);
    }
    state_->state = ApplicationState::Stopped;
    return Result<void>::success();
}

auto Application::descriptor() const noexcept -> const ApplicationDescriptor & {
    return state_->descriptor;
}

auto Application::effective_configuration() const noexcept -> const Configuration & {
    return state_->configuration;
}

auto Application::status() const -> ApplicationOperationalStatus {
    std::lock_guard lock(state_->mutex);
    ApplicationOperationalStatus status{state_->state, {}, state_->failure};
    status.components.reserve(state_->components.size());
    for (const auto &component : state_->components) {
        status.components.push_back(component->status());
    }
    return status;
}

} // namespace evolution::applications
