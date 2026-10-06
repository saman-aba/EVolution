#include "evolution/runtime/in_process.hpp"

#include <mutex>
#include <utility>

namespace evolution::runtime {

struct InProcessRuntimeState {
    InProcessRuntimeState(RuntimeManifest runtime_manifest,
                          applications::Application runtime_application)
        : manifest(std::move(runtime_manifest)), application(std::move(runtime_application)) {}

    RuntimeManifest manifest;
    applications::Application application;
    mutable std::mutex mutex;
    RuntimeState state{RuntimeState::Created};
    std::size_t restart_attempts{};
    std::optional<Error> failure;
};

InProcessRuntime::InProcessRuntime(std::shared_ptr<InProcessRuntimeState> state)
    : state_(std::move(state)) {}

auto InProcessRuntime::create(RuntimeManifest manifest, applications::Application application)
    -> Result<InProcessRuntime> {
    if (!manifest.supports_in_process_execution()) {
        return Result<InProcessRuntime>::failure(
            Error(ErrorCode::create("runtime.unsupported_physical_boundary"),
                  ErrorCategory::InvalidConfiguration,
                  "the reference runtime supports in-process placement only"));
    }
    if (manifest.data().application_identity != application.descriptor().id.to_string() ||
        manifest.data().application_version != application.descriptor().version) {
        return Result<InProcessRuntime>::failure(Error(
            ErrorCode::create("runtime.application_mismatch"), ErrorCategory::InvalidConfiguration,
            "runtime manifest application identity or version does not match composition"));
    }
    if (manifest.data().configuration_id != application.effective_configuration().id()) {
        return Result<InProcessRuntime>::failure(
            Error(ErrorCode::create("runtime.configuration_mismatch"),
                  ErrorCategory::InvalidConfiguration,
                  "runtime manifest configuration does not match effective configuration"));
    }
    return Result<InProcessRuntime>::success(InProcessRuntime(
        std::make_shared<InProcessRuntimeState>(std::move(manifest), std::move(application))));
}

auto InProcessRuntime::start(const ExecutionContext &context) -> Result<void> {
    std::lock_guard lock(state_->mutex);
    if (state_->state != RuntimeState::Created && state_->state != RuntimeState::Stopped) {
        return Result<void>::failure(Error(ErrorCode::create("runtime.invalid_start_state"),
                                           ErrorCategory::Conflict,
                                           "runtime can only start from created or stopped state"));
    }
    state_->state = RuntimeState::Starting;
    state_->failure.reset();
    auto started = state_->application.start(context);
    if (!started) {
        state_->state = RuntimeState::Failed;
        state_->failure = started.error();
        return started;
    }
    const auto application_status = state_->application.status();
    if (application_status.state == applications::ApplicationState::Degraded) {
        state_->state = RuntimeState::Degraded;
        if (state_->manifest.data().startup.fail_on_degraded) {
            static_cast<void>(state_->application.shutdown());
            state_->state = RuntimeState::Failed;
            state_->failure = Error(ErrorCode::create("runtime.degraded_start_rejected"),
                                    ErrorCategory::Unavailable,
                                    "runtime startup policy rejects degraded application state");
            return Result<void>::failure(*state_->failure);
        }
        return Result<void>::success();
    }
    state_->state = RuntimeState::Ready;
    return Result<void>::success();
}

auto InProcessRuntime::shutdown() -> Result<void> {
    std::lock_guard lock(state_->mutex);
    if (state_->state != RuntimeState::Ready && state_->state != RuntimeState::Degraded &&
        state_->state != RuntimeState::Failed) {
        return Result<void>::failure(Error(ErrorCode::create("runtime.invalid_shutdown_state"),
                                           ErrorCategory::Conflict,
                                           "runtime is not in a shutdown-capable state"));
    }
    state_->state = RuntimeState::Stopping;
    if (state_->application.status().state == applications::ApplicationState::Stopped) {
        state_->state = RuntimeState::Stopped;
        return Result<void>::success();
    }
    auto stopped = state_->application.shutdown();
    if (!stopped) {
        state_->state = RuntimeState::Failed;
        state_->failure = stopped.error();
        return stopped;
    }
    state_->state = RuntimeState::Stopped;
    return Result<void>::success();
}

auto InProcessRuntime::restart(const ExecutionContext &context) -> Result<void> {
    const auto policy = state_->manifest.data().restart;
    const auto current = status().state;
    {
        std::lock_guard lock(state_->mutex);
        if (policy.mode == RestartMode::Never ||
            state_->restart_attempts >= policy.maximum_attempts) {
            return Result<void>::failure(
                Error(ErrorCode::create("runtime.restart_not_allowed"), ErrorCategory::Conflict,
                      "runtime restart policy does not allow another attempt"));
        }
        if (policy.mode == RestartMode::OnFailure && current != RuntimeState::Failed) {
            return Result<void>::failure(Error(
                ErrorCode::create("runtime.restart_requires_failure"), ErrorCategory::Conflict,
                "on-failure restart policy requires a failed runtime"));
        }
        ++state_->restart_attempts;
    }
    if (current == RuntimeState::Ready || current == RuntimeState::Degraded ||
        current == RuntimeState::Failed) {
        auto stopped = shutdown();
        if (!stopped) {
            return stopped;
        }
    }
    return start(context);
}

auto InProcessRuntime::status() const -> RuntimeOperationalStatus {
    std::lock_guard lock(state_->mutex);
    const auto application_status = state_->application.status();
    const bool degraded_ready = state_->state == RuntimeState::Degraded &&
                                state_->manifest.data().readiness.allow_degraded_application;
    const bool live =
        state_->state == RuntimeState::Starting || state_->state == RuntimeState::Ready ||
        state_->state == RuntimeState::Degraded || state_->state == RuntimeState::Stopping;
    const bool healthy = state_->state == RuntimeState::Ready;
    return {state_->state,
            live,
            healthy,
            state_->state == RuntimeState::Ready || degraded_ready,
            state_->restart_attempts,
            application_status,
            state_->failure};
}

auto InProcessRuntime::manifest() const noexcept -> const RuntimeManifest & {
    return state_->manifest;
}

} // namespace evolution::runtime
