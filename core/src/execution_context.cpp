#include "evolution/core/context/execution_context.hpp"

#include <utility>

namespace evolution::context {

DeterministicRandomSource::DeterministicRandomSource(std::uint64_t seed) : engine_(seed) {}

auto DeterministicRandomSource::next_u64() -> std::uint64_t {
    const std::scoped_lock lock(mutex_);
    return engine_();
}

auto DeterministicRandomSource::source_id() const noexcept -> std::string_view {
    return "mt19937_64";
}

SystemRandomSource::SystemRandomSource() : engine_(std::random_device{}()) {}

auto SystemRandomSource::next_u64() -> std::uint64_t {
    const std::scoped_lock lock(mutex_);
    return engine_();
}

auto SystemRandomSource::source_id() const noexcept -> std::string_view {
    return "system_random";
}

ExecutionContext::ExecutionContext(ExecutionMode mode, RunId run_id,
                                   std::optional<TraceId> trace_id,
                                   std::shared_ptr<const time::TimeSource> time_source,
                                   std::shared_ptr<RandomSource> random_source,
                                   std::optional<CancellationToken> cancellation,
                                   std::optional<time::MonotonicTimePoint> deadline,
                                   std::optional<ResourceView> resources)
    : mode_(mode), run_id_(std::move(run_id)), trace_id_(std::move(trace_id)),
      time_source_(std::move(time_source)), random_source_(std::move(random_source)),
      cancellation_(std::move(cancellation)), deadline_(deadline),
      resources_(std::move(resources)) {}

auto ExecutionContext::create(ExecutionMode mode, RunId run_id, std::optional<TraceId> trace_id,
                              std::shared_ptr<const time::TimeSource> time_source,
                              std::shared_ptr<RandomSource> random_source,
                              std::optional<CancellationToken> cancellation,
                              std::optional<time::MonotonicTimePoint> deadline,
                              std::optional<ResourceView> resources) -> ExecutionContext {
    return ExecutionContext(mode, std::move(run_id), std::move(trace_id), std::move(time_source),
                            std::move(random_source), std::move(cancellation), deadline,
                            std::move(resources));
}

auto ExecutionContext::mode() const noexcept -> ExecutionMode {
    return mode_;
}

auto ExecutionContext::run_id() const noexcept -> const RunId & {
    return run_id_;
}

auto ExecutionContext::trace_id() const noexcept -> const std::optional<TraceId> & {
    return trace_id_;
}

auto ExecutionContext::time_source() const noexcept
    -> const std::shared_ptr<const time::TimeSource> & {
    return time_source_;
}

auto ExecutionContext::random_source() const noexcept -> const std::shared_ptr<RandomSource> & {
    return random_source_;
}

auto ExecutionContext::cancellation() const noexcept -> const std::optional<CancellationToken> & {
    return cancellation_;
}

auto ExecutionContext::deadline() const noexcept -> std::optional<time::MonotonicTimePoint> {
    return deadline_;
}

auto ExecutionContext::resources() const noexcept -> const std::optional<ResourceView> & {
    return resources_;
}

auto ExecutionContext::current_time() const -> Result<time::TimePoint> {
    if (!time_source_) {
        return Result<time::TimePoint>::failure(
            Error(ErrorCode::create("context.time_unavailable"), ErrorCategory::Unavailable,
                  "execution context does not provide a time source"));
    }
    return Result<time::TimePoint>::success(time_source_->now());
}

auto ExecutionContext::next_random_u64() const -> Result<std::uint64_t> {
    if (!random_source_) {
        return Result<std::uint64_t>::failure(
            Error(ErrorCode::create("context.randomness_unavailable"), ErrorCategory::Unavailable,
                  "execution context does not provide a randomness source"));
    }
    return Result<std::uint64_t>::success(random_source_->next_u64());
}

auto ExecutionContext::is_cancelled() const noexcept -> bool {
    return cancellation_ && cancellation_->is_cancelled();
}

auto ExecutionContext::deadline_exceeded(time::MonotonicTimePoint now) const noexcept -> bool {
    return deadline_ && now >= *deadline_;
}

auto ExecutionContext::project(ExecutionCapability capabilities) const -> ExecutionContext {
    return ExecutionContext(
        mode_, run_id_,
        has_capability(capabilities, ExecutionCapability::Trace) ? trace_id_ : std::nullopt,
        has_capability(capabilities, ExecutionCapability::Time) ? time_source_ : nullptr,
        has_capability(capabilities, ExecutionCapability::Randomness) ? random_source_ : nullptr,
        has_capability(capabilities, ExecutionCapability::Cancellation) ? cancellation_
                                                                        : std::nullopt,
        has_capability(capabilities, ExecutionCapability::Deadline) ? deadline_ : std::nullopt,
        has_capability(capabilities, ExecutionCapability::Resources) ? resources_ : std::nullopt);
}

} // namespace evolution::context
