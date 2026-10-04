#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/measurement/measurement.hpp"
#include "evolution/core/provenance/provenance.hpp"
#include "evolution/core/time/time.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace evolution::series {

enum class ObservationStatus {
    Observed,
    Missing,
    Unknown,
};

enum class DuplicateTimestampPolicy {
    Reject,
    Allow,
};

enum class ObservationInsertionPolicy {
    OrderedAppend,
    InsertSorted,
};

enum class InterpolationPolicy {
    None,
    ForwardFill,
    Linear,
};

template <typename Value>
    requires std::copy_constructible<Value>
class Observation {
  public:
    static auto observed(time::TemporalInformation temporal_information, Value value,
                         std::optional<Provenance> provenance = std::nullopt)
        -> Result<Observation> {
        if (!temporal_information.value()) {
            return invalid("ordered observations require known or estimated time");
        }
        if constexpr (std::floating_point<Value>) {
            if (!std::isfinite(value)) {
                return invalid("observation value must be finite");
            }
        }
        return Result<Observation>::success(Observation(std::move(temporal_information),
                                                        ObservationStatus::Observed,
                                                        std::move(value), std::move(provenance)));
    }

    static auto missing(time::TemporalInformation temporal_information,
                        std::optional<Provenance> provenance = std::nullopt)
        -> Result<Observation> {
        return absent(std::move(temporal_information), ObservationStatus::Missing,
                      std::move(provenance));
    }

    static auto unknown(time::TemporalInformation temporal_information,
                        std::optional<Provenance> provenance = std::nullopt)
        -> Result<Observation> {
        return absent(std::move(temporal_information), ObservationStatus::Unknown,
                      std::move(provenance));
    }

    [[nodiscard]] auto temporal_information() const noexcept -> const time::TemporalInformation & {
        return temporal_information_;
    }
    [[nodiscard]] auto timestamp() const noexcept -> time::TimePoint {
        return *temporal_information_.value();
    }
    [[nodiscard]] auto status() const noexcept -> ObservationStatus {
        return status_;
    }
    [[nodiscard]] auto value() const noexcept -> const std::optional<Value> & {
        return value_;
    }
    [[nodiscard]] auto provenance() const noexcept -> const std::optional<Provenance> & {
        return provenance_;
    }

    friend auto operator==(const Observation &, const Observation &) -> bool = default;

  private:
    Observation(time::TemporalInformation temporal_information, ObservationStatus status,
                std::optional<Value> value, std::optional<Provenance> provenance)
        : temporal_information_(std::move(temporal_information)), status_(status),
          value_(std::move(value)), provenance_(std::move(provenance)) {}

    static auto absent(time::TemporalInformation temporal_information, ObservationStatus status,
                       std::optional<Provenance> provenance) -> Result<Observation> {
        if (!temporal_information.value()) {
            return invalid("ordered observations require known or estimated time");
        }
        return Result<Observation>::success(Observation(std::move(temporal_information), status,
                                                        std::nullopt, std::move(provenance)));
    }

    static auto invalid(std::string message) -> Result<Observation> {
        return Result<Observation>::failure(
            Error(ErrorCode::create("time_series.invalid_observation"),
                  ErrorCategory::InvalidArgument, std::move(message)));
    }

    time::TemporalInformation temporal_information_;
    ObservationStatus status_;
    std::optional<Value> value_;
    std::optional<Provenance> provenance_;
};

template <typename Value>
    requires std::copy_constructible<Value>
class TimeSeries {
  public:
    using Dimensions = std::map<std::string, std::string, std::less<>>;

    static auto create(TimeSeriesId id, Metric metric, std::string scope, Dimensions dimensions,
                       std::string version, DuplicateTimestampPolicy duplicate_policy,
                       ObservationInsertionPolicy insertion_policy,
                       std::optional<time::Duration> expected_interval, Provenance provenance,
                       std::vector<Observation<Value>> observations = {}) -> Result<TimeSeries> {
        if (scope.empty() || version.empty()) {
            return invalid("time series scope and version are required");
        }
        if (metric.value_kind() != measurement::metric_value_kind<Value>()) {
            return invalid("time series value type does not match the Metric definition");
        }
        if (expected_interval && *expected_interval <= time::Duration::zero()) {
            return invalid("expected sampling interval must be positive");
        }

        TimeSeries series(std::move(id), std::move(metric), std::move(scope), std::move(dimensions),
                          std::move(version), duplicate_policy, insertion_policy, expected_interval,
                          std::move(provenance), {});
        for (auto &observation : observations) {
            auto appended = series.with_observation(std::move(observation));
            if (appended.has_error()) {
                return appended;
            }
            series = std::move(appended).value();
        }
        return Result<TimeSeries>::success(std::move(series));
    }

    [[nodiscard]] auto id() const noexcept -> const TimeSeriesId & {
        return id_;
    }
    [[nodiscard]] auto metric() const noexcept -> const Metric & {
        return metric_;
    }
    [[nodiscard]] auto scope() const noexcept -> std::string_view {
        return scope_;
    }
    [[nodiscard]] auto dimensions() const noexcept -> const Dimensions & {
        return dimensions_;
    }
    [[nodiscard]] auto version() const noexcept -> std::string_view {
        return version_;
    }
    [[nodiscard]] auto duplicate_policy() const noexcept -> DuplicateTimestampPolicy {
        return duplicate_policy_;
    }
    [[nodiscard]] auto insertion_policy() const noexcept -> ObservationInsertionPolicy {
        return insertion_policy_;
    }
    [[nodiscard]] auto expected_interval() const noexcept -> std::optional<time::Duration> {
        return expected_interval_;
    }
    [[nodiscard]] auto provenance() const noexcept -> const Provenance & {
        return provenance_;
    }
    [[nodiscard]] auto observations() const noexcept -> const std::vector<Observation<Value>> & {
        return observations_;
    }

    [[nodiscard]] auto with_observation(Observation<Value> observation) const
        -> Result<TimeSeries> {
        auto updated = observations_;
        const auto position =
            std::lower_bound(updated.begin(), updated.end(), observation.timestamp(),
                             [](const auto &existing, time::TimePoint timestamp) {
                                 return existing.timestamp() < timestamp;
                             });
        const auto duplicate =
            position != updated.end() && position->timestamp() == observation.timestamp();
        if (duplicate && duplicate_policy_ == DuplicateTimestampPolicy::Reject) {
            return invalid("duplicate observation timestamp is not allowed by this series");
        }
        if (insertion_policy_ == ObservationInsertionPolicy::OrderedAppend && !updated.empty() &&
            observation.timestamp() < updated.back().timestamp()) {
            return invalid("out-of-order observation violates ordered append policy");
        }

        if (insertion_policy_ == ObservationInsertionPolicy::InsertSorted) {
            updated.insert(position, std::move(observation));
        } else {
            updated.push_back(std::move(observation));
        }
        return Result<TimeSeries>::success(copy_with(std::move(updated)));
    }

    [[nodiscard]] auto window(const time::TimeRange &range) const -> TimeSeries {
        std::vector<Observation<Value>> selected;
        for (const auto &observation : observations_) {
            if (range.contains(observation.timestamp())) {
                selected.push_back(observation);
            }
        }
        return copy_with(std::move(selected));
    }

    [[nodiscard]] auto gaps(time::Duration expected) const -> Result<std::vector<time::TimeRange>> {
        if (expected <= time::Duration::zero()) {
            return Result<std::vector<time::TimeRange>>::failure(
                Error(ErrorCode::create("time_series.invalid_interval"),
                      ErrorCategory::InvalidArgument, "gap interval must be positive"));
        }
        std::vector<time::TimeRange> result;
        for (std::size_t index = 1; index < observations_.size(); ++index) {
            const auto previous = observations_[index - 1].timestamp();
            const auto current = observations_[index].timestamp();
            if (current - previous > expected) {
                result.push_back(time::TimeRange::create(previous + expected, current).value());
            }
        }
        return Result<std::vector<time::TimeRange>>::success(std::move(result));
    }

    [[nodiscard]] auto align(TimeSeriesId output_id, std::span<const time::TimePoint> timestamps,
                             InterpolationPolicy policy, Provenance derivation) const
        -> Result<TimeSeries> {
        if (!std::is_sorted(timestamps.begin(), timestamps.end())) {
            return invalid("alignment timestamps must be ordered");
        }

        std::vector<Observation<Value>> aligned;
        aligned.reserve(timestamps.size());
        for (const auto timestamp : timestamps) {
            const auto exact =
                std::lower_bound(observations_.begin(), observations_.end(), timestamp,
                                 [](const auto &observation, time::TimePoint value) {
                                     return observation.timestamp() < value;
                                 });
            if (exact != observations_.end() && exact->timestamp() == timestamp) {
                aligned.push_back(*exact);
                continue;
            }

            const auto temporal =
                time::TemporalInformation::known(timestamp, time::Precision::Unknown);
            if (policy == InterpolationPolicy::None) {
                aligned.push_back(Observation<Value>::missing(temporal, derivation).value());
                continue;
            }
            if (policy == InterpolationPolicy::ForwardFill) {
                if (exact == observations_.begin()) {
                    aligned.push_back(Observation<Value>::missing(temporal, derivation).value());
                    continue;
                }
                const auto &previous = *(exact - 1);
                if (previous.status() != ObservationStatus::Observed) {
                    aligned.push_back(Observation<Value>::unknown(temporal, derivation).value());
                    continue;
                }
                aligned.push_back(
                    Observation<Value>::observed(temporal, *previous.value(), derivation).value());
                continue;
            }

            if constexpr (!std::floating_point<Value>) {
                return invalid("linear interpolation requires a floating-point series value");
            } else {
                if (exact == observations_.begin() || exact == observations_.end()) {
                    aligned.push_back(Observation<Value>::missing(temporal, derivation).value());
                    continue;
                }
                const auto &lower = *(exact - 1);
                const auto &upper = *exact;
                if (lower.status() != ObservationStatus::Observed ||
                    upper.status() != ObservationStatus::Observed) {
                    aligned.push_back(Observation<Value>::unknown(temporal, derivation).value());
                    continue;
                }
                const auto total = upper.timestamp() - lower.timestamp();
                const auto offset = timestamp - lower.timestamp();
                const auto ratio = std::chrono::duration<double>(offset).count() /
                                   std::chrono::duration<double>(total).count();
                const auto value = *lower.value() + (*upper.value() - *lower.value()) * ratio;
                aligned.push_back(
                    Observation<Value>::observed(temporal, value, derivation).value());
            }
        }

        return create(std::move(output_id), metric_, scope_, dimensions_, version_,
                      DuplicateTimestampPolicy::Reject, ObservationInsertionPolicy::OrderedAppend,
                      expected_interval_, std::move(derivation), std::move(aligned));
    }

    friend auto operator==(const TimeSeries &, const TimeSeries &) -> bool = default;

  private:
    TimeSeries(TimeSeriesId id, Metric metric, std::string scope, Dimensions dimensions,
               std::string version, DuplicateTimestampPolicy duplicate_policy,
               ObservationInsertionPolicy insertion_policy,
               std::optional<time::Duration> expected_interval, Provenance provenance,
               std::vector<Observation<Value>> observations)
        : id_(std::move(id)), metric_(std::move(metric)), scope_(std::move(scope)),
          dimensions_(std::move(dimensions)), version_(std::move(version)),
          duplicate_policy_(duplicate_policy), insertion_policy_(insertion_policy),
          expected_interval_(expected_interval), provenance_(std::move(provenance)),
          observations_(std::move(observations)) {}

    [[nodiscard]] auto copy_with(std::vector<Observation<Value>> observations) const -> TimeSeries {
        return TimeSeries(id_, metric_, scope_, dimensions_, version_, duplicate_policy_,
                          insertion_policy_, expected_interval_, provenance_,
                          std::move(observations));
    }

    static auto invalid(std::string message) -> Result<TimeSeries> {
        return Result<TimeSeries>::failure(Error(ErrorCode::create("time_series.invalid"),
                                                 ErrorCategory::InvalidArgument,
                                                 std::move(message)));
    }

    TimeSeriesId id_;
    Metric metric_;
    std::string scope_;
    Dimensions dimensions_;
    std::string version_;
    DuplicateTimestampPolicy duplicate_policy_;
    ObservationInsertionPolicy insertion_policy_;
    std::optional<time::Duration> expected_interval_;
    Provenance provenance_;
    std::vector<Observation<Value>> observations_;
};

} // namespace evolution::series

namespace evolution {

using series::DuplicateTimestampPolicy;
using series::InterpolationPolicy;
using series::Observation;
using series::ObservationInsertionPolicy;
using series::ObservationStatus;
using series::TimeSeries;

} // namespace evolution
