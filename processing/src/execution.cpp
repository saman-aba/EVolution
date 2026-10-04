#include "evolution/processing/execution.hpp"

#include <deque>
#include <exception>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace evolution::processing {

namespace {

struct PendingExecution {
    PendingExecution(ExecutionId execution_identity, ExecutableWork executable_work)
        : execution_id(std::move(execution_identity)), work(std::move(executable_work)) {}

    ExecutionId execution_id;
    ExecutableWork work;
    CancellationSource cancellation;
};

auto backend_error(std::string code, ErrorCategory category, std::string message) -> Error {
    return Error(ErrorCode::create(std::move(code)), category, std::move(message));
}

} // namespace

struct DeterministicBackendState {
    DeterministicBackendState(std::size_t maximum_pending_work, ResourcePool resource_pool)
        : maximum_pending(maximum_pending_work), resources(std::move(resource_pool)) {}

    mutable std::mutex mutex;
    std::size_t maximum_pending;
    ResourcePool resources;
    bool shutdown{};
    std::deque<std::shared_ptr<PendingExecution>> queue;
    std::unordered_map<ExecutionId, std::shared_ptr<PendingExecution>> known;
};

DeterministicSingleThreadBackend::DeterministicSingleThreadBackend(
    std::shared_ptr<DeterministicBackendState> state)
    : state_(std::move(state)) {}

auto DeterministicSingleThreadBackend::create(std::size_t maximum_pending, ResourcePool resources)
    -> Result<DeterministicSingleThreadBackend> {
    if (maximum_pending == 0) {
        return Result<DeterministicSingleThreadBackend>::failure(
            backend_error("processing.invalid_backend_capacity", ErrorCategory::InvalidArgument,
                          "execution backend pending capacity must be bounded and non-zero"));
    }
    return Result<DeterministicSingleThreadBackend>::success(DeterministicSingleThreadBackend(
        std::make_shared<DeterministicBackendState>(maximum_pending, std::move(resources))));
}

auto DeterministicSingleThreadBackend::submit(ExecutableWork work) -> Result<ExecutionId> {
    if (!work.operation) {
        return Result<ExecutionId>::failure(backend_error("processing.invalid_executable_work",
                                                          ErrorCategory::InvalidArgument,
                                                          "executable work requires an operation"));
    }
    const std::scoped_lock lock(state_->mutex);
    if (state_->shutdown) {
        return Result<ExecutionId>::failure(
            backend_error("processing.backend_shutdown", ErrorCategory::Conflict,
                          "execution backend no longer accepts work"));
    }
    if (state_->queue.size() >= state_->maximum_pending) {
        return Result<ExecutionId>::failure(
            backend_error("processing.backend_capacity_exhausted", ErrorCategory::ResourceExhausted,
                          "execution backend pending capacity is exhausted"));
    }
    auto execution_id = ExecutionId::generate();
    if (execution_id.has_error()) {
        return Result<ExecutionId>::failure(execution_id.error());
    }
    auto pending = std::make_shared<PendingExecution>(execution_id.value(), std::move(work));
    state_->queue.push_back(pending);
    state_->known.emplace(pending->execution_id, pending);
    return Result<ExecutionId>::success(std::move(execution_id).value());
}

auto DeterministicSingleThreadBackend::run_next() -> Result<std::optional<ExecutionCompletion>> {
    std::shared_ptr<PendingExecution> pending_execution;
    {
        const std::scoped_lock lock(state_->mutex);
        if (state_->queue.empty()) {
            return Result<std::optional<ExecutionCompletion>>::success(std::nullopt);
        }
        pending_execution = state_->queue.front();
        state_->queue.pop_front();
    }

    const auto finish = [&](ExecutionOutcome outcome, std::optional<Error> error = std::nullopt) {
        const std::scoped_lock lock(state_->mutex);
        state_->known.erase(pending_execution->execution_id);
        return Result<std::optional<ExecutionCompletion>>::success(ExecutionCompletion{
            pending_execution->execution_id,
            pending_execution->work.work_id,
            outcome,
            std::move(error),
        });
    };

    if (pending_execution->cancellation.token().is_cancelled()) {
        return finish(ExecutionOutcome::Cancelled);
    }

    auto reservation = state_->resources.reserve(pending_execution->work.resources);
    if (reservation.has_error()) {
        return finish(ExecutionOutcome::ResourceExhausted, reservation.error());
    }

    try {
        auto result = pending_execution->work.operation(pending_execution->work.context,
                                                        pending_execution->cancellation.token());
        if (result.has_value()) {
            return finish(ExecutionOutcome::Success);
        }
        if (result.error().category() == ErrorCategory::Cancelled) {
            return finish(ExecutionOutcome::Cancelled, result.error());
        }
        return finish(ExecutionOutcome::OperationFailure, result.error());
    } catch (const std::exception &exception) {
        return finish(ExecutionOutcome::BackendFailure,
                      backend_error("processing.execution_exception", ErrorCategory::Internal,
                                    exception.what()));
    } catch (...) {
        return finish(ExecutionOutcome::BackendFailure,
                      backend_error("processing.execution_exception", ErrorCategory::Internal,
                                    "execution operation threw an unknown exception"));
    }
}

auto DeterministicSingleThreadBackend::request_cancel(const ExecutionId &execution_id)
    -> Result<void> {
    const std::scoped_lock lock(state_->mutex);
    const auto iterator = state_->known.find(execution_id);
    if (iterator == state_->known.end()) {
        return Result<void>::failure(backend_error("processing.execution_not_found",
                                                   ErrorCategory::NotFound,
                                                   "execution identity is not pending or running"));
    }
    iterator->second->cancellation.cancel();
    return Result<void>::success();
}

auto DeterministicSingleThreadBackend::shutdown() -> Result<void> {
    const std::scoped_lock lock(state_->mutex);
    if (state_->shutdown) {
        return Result<void>::success();
    }
    state_->shutdown = true;
    for (const auto &pending_execution : state_->queue) {
        pending_execution->cancellation.cancel();
    }
    return Result<void>::success();
}

auto DeterministicSingleThreadBackend::pending() const noexcept -> std::size_t {
    const std::scoped_lock lock(state_->mutex);
    return state_->queue.size();
}

} // namespace evolution::processing
