#pragma once

#include "evolution/core/api.hpp"
#include "evolution/core/context/cancellation.hpp"
#include "evolution/core/context/random_source.hpp"
#include "evolution/core/context/resource_view.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace evolution::context {

enum class ExecutionMode {
    Live,
    Batch,
    Replay,
    Test,
};

enum class ExecutionCapability : std::uint32_t {
    None = 0,
    Trace = 1U << 0U,
    Time = 1U << 1U,
    Randomness = 1U << 2U,
    Cancellation = 1U << 3U,
    Deadline = 1U << 4U,
    Resources = 1U << 5U,
    All = (1U << 6U) - 1U,
};

constexpr auto operator|(ExecutionCapability left, ExecutionCapability right) noexcept
    -> ExecutionCapability {
    return static_cast<ExecutionCapability>(static_cast<std::uint32_t>(left) |
                                            static_cast<std::uint32_t>(right));
}

constexpr auto has_capability(ExecutionCapability value, ExecutionCapability capability) noexcept
    -> bool {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(capability)) != 0U;
}

class EVOLUTION_CORE_API ExecutionContext {
  public:
    static auto create(ExecutionMode mode, RunId run_id,
                       std::optional<TraceId> trace_id = std::nullopt,
                       std::shared_ptr<const time::TimeSource> time_source = nullptr,
                       std::shared_ptr<RandomSource> random_source = nullptr,
                       std::optional<CancellationToken> cancellation = std::nullopt,
                       std::optional<time::MonotonicTimePoint> deadline = std::nullopt,
                       std::optional<ResourceView> resources = std::nullopt) -> ExecutionContext;

    [[nodiscard]] auto mode() const noexcept -> ExecutionMode;
    [[nodiscard]] auto run_id() const noexcept -> const RunId &;
    [[nodiscard]] auto trace_id() const noexcept -> const std::optional<TraceId> &;
    [[nodiscard]] auto time_source() const noexcept
        -> const std::shared_ptr<const time::TimeSource> &;
    [[nodiscard]] auto random_source() const noexcept -> const std::shared_ptr<RandomSource> &;
    [[nodiscard]] auto cancellation() const noexcept -> const std::optional<CancellationToken> &;
    [[nodiscard]] auto deadline() const noexcept -> std::optional<time::MonotonicTimePoint>;
    [[nodiscard]] auto resources() const noexcept -> const std::optional<ResourceView> &;

    [[nodiscard]] auto current_time() const -> Result<time::TimePoint>;
    [[nodiscard]] auto next_random_u64() const -> Result<std::uint64_t>;
    [[nodiscard]] auto is_cancelled() const noexcept -> bool;
    [[nodiscard]] auto deadline_exceeded(time::MonotonicTimePoint now) const noexcept -> bool;

    [[nodiscard]] auto project(ExecutionCapability capabilities) const -> ExecutionContext;

  private:
    ExecutionContext(ExecutionMode mode, RunId run_id, std::optional<TraceId> trace_id,
                     std::shared_ptr<const time::TimeSource> time_source,
                     std::shared_ptr<RandomSource> random_source,
                     std::optional<CancellationToken> cancellation,
                     std::optional<time::MonotonicTimePoint> deadline,
                     std::optional<ResourceView> resources);

    ExecutionMode mode_;
    RunId run_id_;
    std::optional<TraceId> trace_id_;
    std::shared_ptr<const time::TimeSource> time_source_;
    std::shared_ptr<RandomSource> random_source_;
    std::optional<CancellationToken> cancellation_;
    std::optional<time::MonotonicTimePoint> deadline_;
    std::optional<ResourceView> resources_;
};

} // namespace evolution::context

namespace evolution {

using context::CancellationSource;
using context::CancellationToken;
using context::DeterministicRandomSource;
using context::ExecutionCapability;
using context::ExecutionContext;
using context::ExecutionMode;
using context::RandomSource;
using context::ResourceKind;
using context::ResourceView;
using context::SystemRandomSource;

} // namespace evolution
