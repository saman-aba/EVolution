#include "evolution/core/time/time.hpp"

#include <utility>

namespace evolution::time {

TemporalInformation::TemporalInformation(TemporalStatus status, std::optional<TimePoint> value,
                                         Precision precision, std::optional<Duration> uncertainty,
                                         std::string estimation_method)
    : status_(status), value_(value), precision_(precision), uncertainty_(uncertainty),
      estimation_method_(std::move(estimation_method)) {}

auto TemporalInformation::known(TimePoint value, Precision precision) -> TemporalInformation {
    return TemporalInformation(TemporalStatus::Known, value, precision, std::nullopt, {});
}

auto TemporalInformation::estimated(TimePoint value, Precision precision, Duration uncertainty,
                                    std::string method) -> Result<TemporalInformation> {
    if (uncertainty < Duration::zero()) {
        return Result<TemporalInformation>::failure(
            Error(ErrorCode::create("time.negative_uncertainty"), ErrorCategory::InvalidArgument,
                  "estimated time uncertainty cannot be negative"));
    }
    if (method.empty()) {
        return Result<TemporalInformation>::failure(
            Error(ErrorCode::create("time.missing_estimation_method"),
                  ErrorCategory::InvalidArgument, "estimated time requires an estimation method"));
    }
    return Result<TemporalInformation>::success(TemporalInformation(
        TemporalStatus::Estimated, value, precision, uncertainty, std::move(method)));
}

auto TemporalInformation::unknown() -> TemporalInformation {
    return TemporalInformation(TemporalStatus::Unknown, std::nullopt, Precision::Unknown,
                               std::nullopt, {});
}

auto TemporalInformation::status() const noexcept -> TemporalStatus {
    return status_;
}

auto TemporalInformation::value() const noexcept -> std::optional<TimePoint> {
    return value_;
}

auto TemporalInformation::precision() const noexcept -> Precision {
    return precision_;
}

auto TemporalInformation::uncertainty() const noexcept -> std::optional<Duration> {
    return uncertainty_;
}

auto TemporalInformation::estimation_method() const noexcept -> std::string_view {
    return estimation_method_;
}

TimeRange::TimeRange(std::optional<TimePoint> start, std::optional<TimePoint> end)
    : start_(start), end_(end) {}

auto TimeRange::create(std::optional<TimePoint> start, std::optional<TimePoint> end)
    -> Result<TimeRange> {
    if (start && end && *end < *start) {
        return Result<TimeRange>::failure(Error(ErrorCode::create("time.invalid_range"),
                                                ErrorCategory::InvalidArgument,
                                                "time range end cannot precede its start"));
    }
    return Result<TimeRange>::success(TimeRange(start, end));
}

auto TimeRange::start() const noexcept -> std::optional<TimePoint> {
    return start_;
}

auto TimeRange::end() const noexcept -> std::optional<TimePoint> {
    return end_;
}

auto TimeRange::is_empty() const noexcept -> bool {
    return start_ && end_ && *start_ == *end_;
}

auto TimeRange::contains(TimePoint point) const noexcept -> bool {
    return (!start_ || point >= *start_) && (!end_ || point < *end_);
}

auto SystemTimeSource::now() const -> TimePoint {
    return Clock::now();
}

auto SystemTimeSource::source_id() const noexcept -> std::string_view {
    return "system_clock";
}

} // namespace evolution::time
