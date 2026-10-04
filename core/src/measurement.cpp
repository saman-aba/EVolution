#include "evolution/core/measurement/measurement.hpp"

#include <cmath>
#include <utility>

namespace evolution::measurement {

Unit::Unit(std::string symbol, std::string dimension, double scale)
    : symbol_(std::move(symbol)), dimension_(std::move(dimension)), scale_(scale) {}

auto Unit::create(std::string symbol, std::string dimension, double scale) -> Result<Unit> {
    if (symbol.empty() || dimension.empty() || !std::isfinite(scale) || scale <= 0.0) {
        return Result<Unit>::failure(
            Error(ErrorCode::create("measurement.invalid_unit"), ErrorCategory::InvalidArgument,
                  "unit symbol and dimension are required and scale must be finite and positive"));
    }
    return Result<Unit>::success(Unit(std::move(symbol), std::move(dimension), scale));
}

auto Unit::symbol() const noexcept -> std::string_view {
    return symbol_;
}

auto Unit::dimension() const noexcept -> std::string_view {
    return dimension_;
}

auto Unit::scale() const noexcept -> double {
    return scale_;
}

auto Unit::compatible_with(const Unit &other) const noexcept -> bool {
    return dimension_ == other.dimension_;
}

Metric::Metric(MetricId id, std::string name, std::string definition, MetricValueKind value_kind,
               std::optional<Unit> unit, std::string version)
    : id_(std::move(id)), name_(std::move(name)), definition_(std::move(definition)),
      value_kind_(value_kind), unit_(std::move(unit)), version_(std::move(version)) {}

auto Metric::create(MetricId id, std::string name, std::string definition,
                    MetricValueKind value_kind, std::optional<Unit> unit, std::string version)
    -> Result<Metric> {
    if (name.empty() || definition.empty() || version.empty()) {
        return Result<Metric>::failure(Error(ErrorCode::create("measurement.invalid_metric"),
                                             ErrorCategory::InvalidArgument,
                                             "metric name, definition, and version are required"));
    }
    return Result<Metric>::success(Metric(std::move(id), std::move(name), std::move(definition),
                                          value_kind, std::move(unit), std::move(version)));
}

auto Metric::id() const noexcept -> const MetricId & {
    return id_;
}

auto Metric::name() const noexcept -> std::string_view {
    return name_;
}

auto Metric::definition() const noexcept -> std::string_view {
    return definition_;
}

auto Metric::value_kind() const noexcept -> MetricValueKind {
    return value_kind_;
}

auto Metric::unit() const noexcept -> const std::optional<Unit> & {
    return unit_;
}

auto Metric::version() const noexcept -> std::string_view {
    return version_;
}

MeasurementPeriod::MeasurementPeriod(time::TemporalInformation observation_time,
                                     std::optional<time::TimeRange> range)
    : observation_time_(std::move(observation_time)), range_(std::move(range)) {}

auto MeasurementPeriod::point(time::TemporalInformation observation_time) -> MeasurementPeriod {
    return MeasurementPeriod(std::move(observation_time), std::nullopt);
}

auto MeasurementPeriod::interval(time::TimeRange range, time::TemporalInformation observation_time)
    -> MeasurementPeriod {
    return MeasurementPeriod(std::move(observation_time), std::move(range));
}

auto MeasurementPeriod::observation_time() const noexcept -> const time::TemporalInformation & {
    return observation_time_;
}

auto MeasurementPeriod::range() const noexcept -> const std::optional<time::TimeRange> & {
    return range_;
}

} // namespace evolution::measurement
