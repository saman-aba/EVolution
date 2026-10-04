#include "evolution/processing/lifecycle.hpp"

#include <string>
#include <utility>

namespace evolution::processing {

auto ProcessorLifecycle::configure() -> Result<void> {
    const std::scoped_lock lock(mutex_);
    if (state_ != LifecycleState::Created) {
        return invalid_state("configure");
    }
    set_state(LifecycleState::Configured);
    return Result<void>::success();
}

auto ProcessorLifecycle::initialize() -> Result<void> {
    const std::scoped_lock lock(mutex_);
    if (state_ != LifecycleState::Configured) {
        return invalid_state("initialize");
    }
    set_state(LifecycleState::Initialized);
    return Result<void>::success();
}

auto ProcessorLifecycle::activate() -> Result<void> {
    const std::scoped_lock lock(mutex_);
    if (state_ != LifecycleState::Initialized) {
        return invalid_state("activate");
    }
    set_state(LifecycleState::Active);
    return Result<void>::success();
}

auto ProcessorLifecycle::request_stop() -> Result<void> {
    const std::scoped_lock lock(mutex_);
    if (state_ == LifecycleState::Stopped ||
        (state_ == LifecycleState::Stopping && termination_mode_ == TerminationMode::Stop)) {
        return Result<void>::success();
    }
    if (state_ != LifecycleState::Active) {
        return invalid_state("stop");
    }
    termination_mode_ = TerminationMode::Stop;
    set_state(LifecycleState::Stopping);
    return Result<void>::success();
}

auto ProcessorLifecycle::request_cancel() -> Result<void> {
    const std::scoped_lock lock(mutex_);
    if (state_ == LifecycleState::Stopped ||
        (state_ == LifecycleState::Stopping && termination_mode_ == TerminationMode::Cancel)) {
        return Result<void>::success();
    }
    if (state_ != LifecycleState::Active && state_ != LifecycleState::Stopping) {
        return invalid_state("cancel");
    }
    termination_mode_ = TerminationMode::Cancel;
    set_state(LifecycleState::Stopping);
    return Result<void>::success();
}

auto ProcessorLifecycle::abort() -> Result<void> {
    const std::scoped_lock lock(mutex_);
    if (state_ == LifecycleState::Failed && termination_mode_ == TerminationMode::Abort) {
        return Result<void>::success();
    }
    if (state_ != LifecycleState::Active && state_ != LifecycleState::Stopping) {
        return invalid_state("abort");
    }
    termination_mode_ = TerminationMode::Abort;
    failure_ = Error(ErrorCode::create("processing.aborted"), ErrorCategory::Cancelled,
                     "processor was aborted");
    set_state(LifecycleState::Failed);
    return Result<void>::success();
}

auto ProcessorLifecycle::complete_termination() -> Result<void> {
    const std::scoped_lock lock(mutex_);
    if (state_ == LifecycleState::Stopped) {
        return Result<void>::success();
    }
    if (state_ != LifecycleState::Stopping) {
        return invalid_state("complete termination");
    }
    set_state(LifecycleState::Stopped);
    return Result<void>::success();
}

auto ProcessorLifecycle::fail(Error error) -> Result<void> {
    const std::scoped_lock lock(mutex_);
    if (state_ == LifecycleState::Stopped || state_ == LifecycleState::Failed) {
        return invalid_state("fail");
    }
    failure_ = std::move(error);
    set_state(LifecycleState::Failed);
    return Result<void>::success();
}

auto ProcessorLifecycle::state() const noexcept -> LifecycleState {
    const std::scoped_lock lock(mutex_);
    return state_;
}

auto ProcessorLifecycle::termination_mode() const noexcept -> TerminationMode {
    const std::scoped_lock lock(mutex_);
    return termination_mode_;
}

auto ProcessorLifecycle::failure() const -> std::optional<Error> {
    const std::scoped_lock lock(mutex_);
    return failure_;
}

auto ProcessorLifecycle::accepts_work() const noexcept -> bool {
    const std::scoped_lock lock(mutex_);
    return state_ == LifecycleState::Active;
}

auto ProcessorLifecycle::invalid_state(const char *operation) const -> Result<void> {
    return Result<void>::failure(
        Error(ErrorCode::create("processing.invalid_lifecycle_state"), ErrorCategory::Conflict,
              std::string(operation) + " is invalid in the current processor lifecycle state"));
}

void ProcessorLifecycle::set_state(LifecycleState state) {
    state_ = state;
}

} // namespace evolution::processing
