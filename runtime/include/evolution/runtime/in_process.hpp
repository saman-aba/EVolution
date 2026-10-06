#pragma once

#include "evolution/applications/lifecycle.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/runtime/api.hpp"
#include "evolution/runtime/manifest.hpp"

#include <cstddef>
#include <memory>
#include <optional>

namespace evolution::runtime {

enum class RuntimeState { Created, Starting, Ready, Degraded, Stopping, Stopped, Failed };

struct RuntimeOperationalStatus {
    RuntimeState state{RuntimeState::Created};
    bool live{};
    bool healthy{};
    bool ready{};
    std::size_t restart_attempts{};
    applications::ApplicationOperationalStatus application;
    std::optional<Error> failure;
};

struct InProcessRuntimeState;

class EVOLUTION_RUNTIME_API InProcessRuntime {
  public:
    static auto create(RuntimeManifest manifest, applications::Application application)
        -> Result<InProcessRuntime>;

    auto start(const ExecutionContext &context) -> Result<void>;
    auto shutdown() -> Result<void>;
    auto restart(const ExecutionContext &context) -> Result<void>;
    [[nodiscard]] auto status() const -> RuntimeOperationalStatus;
    [[nodiscard]] auto manifest() const noexcept -> const RuntimeManifest &;

  private:
    explicit InProcessRuntime(std::shared_ptr<InProcessRuntimeState> state);
    std::shared_ptr<InProcessRuntimeState> state_;
};

} // namespace evolution::runtime
