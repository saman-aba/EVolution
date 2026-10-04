#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/processing/api.hpp"

#include <mutex>
#include <optional>

namespace evolution::processing {

enum class LifecycleState {
    Created,
    Configured,
    Initialized,
    Active,
    Stopping,
    Stopped,
    Failed,
};

enum class TerminationMode {
    None,
    Stop,
    Cancel,
    Abort,
};

class EVOLUTION_PROCESSING_API ProcessorLifecycle {
  public:
    auto configure() -> Result<void>;
    auto initialize() -> Result<void>;
    auto activate() -> Result<void>;

    auto request_stop() -> Result<void>;
    auto request_cancel() -> Result<void>;
    auto abort() -> Result<void>;
    auto complete_termination() -> Result<void>;

    auto fail(Error error) -> Result<void>;

    [[nodiscard]] auto state() const noexcept -> LifecycleState;
    [[nodiscard]] auto termination_mode() const noexcept -> TerminationMode;
    [[nodiscard]] auto failure() const -> std::optional<Error>;
    [[nodiscard]] auto accepts_work() const noexcept -> bool;

  private:
    auto invalid_state(const char *operation) const -> Result<void>;
    void set_state(LifecycleState state);

    mutable std::mutex mutex_;
    LifecycleState state_{LifecycleState::Created};
    TerminationMode termination_mode_{TerminationMode::None};
    std::optional<Error> failure_;
};

} // namespace evolution::processing
