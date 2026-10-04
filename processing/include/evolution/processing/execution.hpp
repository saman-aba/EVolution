#pragma once

#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/processing/api.hpp"
#include "evolution/processing/resources.hpp"
#include "evolution/processing/scheduler.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

namespace evolution::processing {

struct ExecutionTag;
using ExecutionId = identity::Id<ExecutionTag>;

enum class ExecutionOutcome {
    Success,
    OperationFailure,
    Cancelled,
    ResourceExhausted,
    BackendFailure,
};

using ExecutionOperation =
    std::function<Result<void>(const ExecutionContext &, const CancellationToken &)>;

struct ExecutableWork {
    WorkId work_id;
    NodeId node_id;
    ExecutionContext context;
    ResourceRequirements resources;
    ExecutionOperation operation;
};

struct ExecutionCompletion {
    ExecutionId execution_id;
    WorkId work_id;
    ExecutionOutcome outcome;
    std::optional<Error> error;
};

class EVOLUTION_PROCESSING_API ExecutionBackend {
  public:
    virtual ~ExecutionBackend() = default;
    virtual auto submit(ExecutableWork work) -> Result<ExecutionId> = 0;
    virtual auto run_next() -> Result<std::optional<ExecutionCompletion>> = 0;
    virtual auto request_cancel(const ExecutionId &execution_id) -> Result<void> = 0;
    virtual auto shutdown() -> Result<void> = 0;
    [[nodiscard]] virtual auto pending() const noexcept -> std::size_t = 0;
};

struct DeterministicBackendState;

class EVOLUTION_PROCESSING_API DeterministicSingleThreadBackend final : public ExecutionBackend {
  public:
    static auto create(std::size_t maximum_pending, ResourcePool resources)
        -> Result<DeterministicSingleThreadBackend>;

    auto submit(ExecutableWork work) -> Result<ExecutionId> override;
    auto run_next() -> Result<std::optional<ExecutionCompletion>> override;
    auto request_cancel(const ExecutionId &execution_id) -> Result<void> override;
    auto shutdown() -> Result<void> override;
    [[nodiscard]] auto pending() const noexcept -> std::size_t override;

  private:
    explicit DeterministicSingleThreadBackend(std::shared_ptr<DeterministicBackendState> state);
    std::shared_ptr<DeterministicBackendState> state_;
};

} // namespace evolution::processing
