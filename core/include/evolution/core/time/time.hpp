#pragma once

#include "evolution/core/api.hpp"
#include "evolution/core/error/result.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace evolution::time {

using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;
using MonotonicClock = std::chrono::steady_clock;
using MonotonicTimePoint = MonotonicClock::time_point;

enum class Precision {
    Nanosecond,
    Microsecond,
    Millisecond,
    Second,
    Minute,
    Hour,
    Day,
    Unknown,
};

enum class TemporalStatus {
    Known,
    Estimated,
    Unknown,
};

class EVOLUTION_CORE_API TemporalInformation {
  public:
    static auto known(TimePoint value, Precision precision) -> TemporalInformation;
    static auto estimated(TimePoint value, Precision precision, Duration uncertainty,
                          std::string method) -> Result<TemporalInformation>;
    static auto unknown() -> TemporalInformation;

    [[nodiscard]] auto status() const noexcept -> TemporalStatus;
    [[nodiscard]] auto value() const noexcept -> std::optional<TimePoint>;
    [[nodiscard]] auto precision() const noexcept -> Precision;
    [[nodiscard]] auto uncertainty() const noexcept -> std::optional<Duration>;
    [[nodiscard]] auto estimation_method() const noexcept -> std::string_view;

    friend auto operator==(const TemporalInformation &, const TemporalInformation &)
        -> bool = default;

  private:
    TemporalInformation(TemporalStatus status, std::optional<TimePoint> value, Precision precision,
                        std::optional<Duration> uncertainty, std::string estimation_method);

    TemporalStatus status_;
    std::optional<TimePoint> value_;
    Precision precision_;
    std::optional<Duration> uncertainty_;
    std::string estimation_method_;
};

class EVOLUTION_CORE_API TimeRange {
  public:
    static auto create(std::optional<TimePoint> start, std::optional<TimePoint> end)
        -> Result<TimeRange>;

    [[nodiscard]] auto start() const noexcept -> std::optional<TimePoint>;
    [[nodiscard]] auto end() const noexcept -> std::optional<TimePoint>;
    [[nodiscard]] auto is_empty() const noexcept -> bool;
    [[nodiscard]] auto contains(TimePoint point) const noexcept -> bool;

    friend auto operator==(const TimeRange &, const TimeRange &) -> bool = default;

  private:
    TimeRange(std::optional<TimePoint> start, std::optional<TimePoint> end);

    std::optional<TimePoint> start_;
    std::optional<TimePoint> end_;
};

class EVOLUTION_CORE_API TimeSource {
  public:
    virtual ~TimeSource() = default;

    [[nodiscard]] virtual auto now() const -> TimePoint = 0;
    [[nodiscard]] virtual auto source_id() const noexcept -> std::string_view = 0;
};

class EVOLUTION_CORE_API SystemTimeSource final : public TimeSource {
  public:
    [[nodiscard]] auto now() const -> TimePoint override;
    [[nodiscard]] auto source_id() const noexcept -> std::string_view override;
};

} // namespace evolution::time
