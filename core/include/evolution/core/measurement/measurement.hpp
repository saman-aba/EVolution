#pragma once

#include "evolution/core/api.hpp"
#include "evolution/core/context/context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/provenance/provenance.hpp"
#include "evolution/core/time/time.hpp"

#include <cmath>
#include <compare>
#include <concepts>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace evolution::measurement {

enum class MetricValueKind {
    Integer,
    Real,
    Boolean,
    Text,
    Structured,
};

template <typename Value>
[[nodiscard]] constexpr auto metric_value_kind() noexcept -> MetricValueKind {
    using Type = std::remove_cvref_t<Value>;
    if constexpr (std::same_as<Type, bool>) {
        return MetricValueKind::Boolean;
    } else if constexpr (std::integral<Type>) {
        return MetricValueKind::Integer;
    } else if constexpr (std::floating_point<Type>) {
        return MetricValueKind::Real;
    } else if constexpr (std::same_as<Type, std::string>) {
        return MetricValueKind::Text;
    } else {
        return MetricValueKind::Structured;
    }
}

class EVOLUTION_CORE_API Unit {
  public:
    static auto create(std::string symbol, std::string dimension, double scale = 1.0)
        -> Result<Unit>;

    [[nodiscard]] auto symbol() const noexcept -> std::string_view;
    [[nodiscard]] auto dimension() const noexcept -> std::string_view;
    [[nodiscard]] auto scale() const noexcept -> double;
    [[nodiscard]] auto compatible_with(const Unit &other) const noexcept -> bool;

    friend auto operator==(const Unit &, const Unit &) -> bool = default;

  private:
    Unit(std::string symbol, std::string dimension, double scale);
    std::string symbol_;
    std::string dimension_;
    double scale_;
};

class EVOLUTION_CORE_API Metric {
  public:
    static auto create(MetricId id, std::string name, std::string definition,
                       MetricValueKind value_kind, std::optional<Unit> unit, std::string version)
        -> Result<Metric>;

    [[nodiscard]] auto id() const noexcept -> const MetricId &;
    [[nodiscard]] auto name() const noexcept -> std::string_view;
    [[nodiscard]] auto definition() const noexcept -> std::string_view;
    [[nodiscard]] auto value_kind() const noexcept -> MetricValueKind;
    [[nodiscard]] auto unit() const noexcept -> const std::optional<Unit> &;
    [[nodiscard]] auto version() const noexcept -> std::string_view;

    friend auto operator==(const Metric &, const Metric &) -> bool = default;

  private:
    Metric(MetricId id, std::string name, std::string definition, MetricValueKind value_kind,
           std::optional<Unit> unit, std::string version);

    MetricId id_;
    std::string name_;
    std::string definition_;
    MetricValueKind value_kind_;
    std::optional<Unit> unit_;
    std::string version_;
};

enum class MeasurementStatus {
    Observed,
    Missing,
    Unknown,
};

enum class QualityLevel {
    Good,
    Suspect,
    Poor,
};

struct MeasurementQuality {
    QualityLevel level{QualityLevel::Good};
    std::optional<double> confidence;
    std::optional<double> absolute_uncertainty;
    std::vector<std::string> flags;

    friend auto operator==(const MeasurementQuality &, const MeasurementQuality &)
        -> bool = default;
};

class EVOLUTION_CORE_API MeasurementPeriod {
  public:
    static auto point(time::TemporalInformation observation_time) -> MeasurementPeriod;
    static auto interval(time::TimeRange range, time::TemporalInformation observation_time)
        -> MeasurementPeriod;

    [[nodiscard]] auto observation_time() const noexcept -> const time::TemporalInformation &;
    [[nodiscard]] auto range() const noexcept -> const std::optional<time::TimeRange> &;

    friend auto operator==(const MeasurementPeriod &, const MeasurementPeriod &) -> bool = default;

  private:
    MeasurementPeriod(time::TemporalInformation observation_time,
                      std::optional<time::TimeRange> range);

    time::TemporalInformation observation_time_;
    std::optional<time::TimeRange> range_;
};

template <typename Value>
    requires std::movable<Value>
class Measurement {
  public:
    using Dimensions = std::map<std::string, std::string, std::less<>>;

    static auto observed(MeasurementId id, Metric metric, Value value, std::string scope,
                         Dimensions dimensions, MeasurementPeriod period,
                         MeasurementQuality quality, std::optional<Context> context,
                         Provenance provenance) -> Result<Measurement> {
        if constexpr (std::floating_point<Value>) {
            if (!std::isfinite(value)) {
                return invalid("measurement value must be finite");
            }
        }
        auto validation = validate(scope, quality);
        if (validation.has_error()) {
            return Result<Measurement>::failure(validation.error());
        }
        auto metric_validation = validate_metric(metric);
        if (metric_validation.has_error()) {
            return Result<Measurement>::failure(metric_validation.error());
        }
        return Result<Measurement>::success(Measurement(
            std::move(id), std::move(metric), MeasurementStatus::Observed, std::move(value),
            std::move(scope), std::move(dimensions), std::move(period), std::move(quality),
            std::move(context), std::move(provenance)));
    }

    static auto missing(MeasurementId id, Metric metric, std::string scope, Dimensions dimensions,
                        MeasurementPeriod period, Provenance provenance) -> Result<Measurement> {
        return absent(std::move(id), std::move(metric), MeasurementStatus::Missing,
                      std::move(scope), std::move(dimensions), std::move(period),
                      std::move(provenance));
    }

    static auto unknown(MeasurementId id, Metric metric, std::string scope, Dimensions dimensions,
                        MeasurementPeriod period, Provenance provenance) -> Result<Measurement> {
        return absent(std::move(id), std::move(metric), MeasurementStatus::Unknown,
                      std::move(scope), std::move(dimensions), std::move(period),
                      std::move(provenance));
    }

    [[nodiscard]] auto id() const noexcept -> const MeasurementId & {
        return id_;
    }
    [[nodiscard]] auto metric() const noexcept -> const Metric & {
        return metric_;
    }
    [[nodiscard]] auto status() const noexcept -> MeasurementStatus {
        return status_;
    }
    [[nodiscard]] auto value() const noexcept -> const std::optional<Value> & {
        return value_;
    }
    [[nodiscard]] auto scope() const noexcept -> std::string_view {
        return scope_;
    }
    [[nodiscard]] auto dimensions() const noexcept -> const Dimensions & {
        return dimensions_;
    }
    [[nodiscard]] auto period() const noexcept -> const MeasurementPeriod & {
        return period_;
    }
    [[nodiscard]] auto quality() const noexcept -> const MeasurementQuality & {
        return quality_;
    }
    [[nodiscard]] auto context() const noexcept -> const std::optional<Context> & {
        return context_;
    }
    [[nodiscard]] auto provenance() const noexcept -> const Provenance & {
        return provenance_;
    }

    [[nodiscard]] auto comparable_with(const Measurement &other) const noexcept -> bool {
        return status_ == MeasurementStatus::Observed &&
               other.status_ == MeasurementStatus::Observed && metric_.id() == other.metric_.id() &&
               scope_ == other.scope_ && dimensions_ == other.dimensions_;
    }

    [[nodiscard]] auto compare(const Measurement &other) const -> Result<std::partial_ordering>
        requires std::three_way_comparable<Value>
    {
        if (!comparable_with(other)) {
            return Result<std::partial_ordering>::failure(Error(
                ErrorCode::create("measurement.not_comparable"), ErrorCategory::InvalidArgument,
                "measurements require the same metric, scope, dimensions, and observed status"));
        }
        return Result<std::partial_ordering>::success(*value_ <=> *other.value_);
    }

    friend auto operator==(const Measurement &, const Measurement &) -> bool = default;

  private:
    Measurement(MeasurementId id, Metric metric, MeasurementStatus status,
                std::optional<Value> value, std::string scope, Dimensions dimensions,
                MeasurementPeriod period, MeasurementQuality quality,
                std::optional<Context> context, Provenance provenance)
        : id_(std::move(id)), metric_(std::move(metric)), status_(status), value_(std::move(value)),
          scope_(std::move(scope)), dimensions_(std::move(dimensions)), period_(std::move(period)),
          quality_(std::move(quality)), context_(std::move(context)),
          provenance_(std::move(provenance)) {}

    static auto validate(std::string_view scope, const MeasurementQuality &quality)
        -> Result<void> {
        if (scope.empty()) {
            return Result<void>::failure(Error(ErrorCode::create("measurement.invalid_scope"),
                                               ErrorCategory::InvalidArgument,
                                               "measurement scope cannot be empty"));
        }
        if (quality.confidence && (*quality.confidence < 0.0 || *quality.confidence > 1.0 ||
                                   !std::isfinite(*quality.confidence))) {
            return Result<void>::failure(Error(
                ErrorCode::create("measurement.invalid_confidence"), ErrorCategory::InvalidArgument,
                "measurement confidence must be finite and within [0, 1]"));
        }
        if (quality.absolute_uncertainty && (*quality.absolute_uncertainty < 0.0 ||
                                             !std::isfinite(*quality.absolute_uncertainty))) {
            return Result<void>::failure(
                Error(ErrorCode::create("measurement.invalid_uncertainty"),
                      ErrorCategory::InvalidArgument,
                      "measurement uncertainty must be finite and non-negative"));
        }
        return Result<void>::success();
    }

    static auto validate_metric(const Metric &metric) -> Result<void> {
        if (metric.value_kind() != metric_value_kind<Value>()) {
            return Result<void>::failure(
                Error(ErrorCode::create("measurement.metric_type_mismatch"),
                      ErrorCategory::InvalidArgument,
                      "measurement value type does not match the Metric definition"));
        }
        return Result<void>::success();
    }

    static auto absent(MeasurementId id, Metric metric, MeasurementStatus status, std::string scope,
                       Dimensions dimensions, MeasurementPeriod period, Provenance provenance)
        -> Result<Measurement> {
        auto validation = validate(scope, {});
        if (validation.has_error()) {
            return Result<Measurement>::failure(validation.error());
        }
        auto metric_validation = validate_metric(metric);
        if (metric_validation.has_error()) {
            return Result<Measurement>::failure(metric_validation.error());
        }
        return Result<Measurement>::success(Measurement(
            std::move(id), std::move(metric), status, std::nullopt, std::move(scope),
            std::move(dimensions), std::move(period), {}, std::nullopt, std::move(provenance)));
    }

    static auto invalid(std::string message) -> Result<Measurement> {
        return Result<Measurement>::failure(Error(ErrorCode::create("measurement.invalid"),
                                                  ErrorCategory::InvalidArgument,
                                                  std::move(message)));
    }

    MeasurementId id_;
    Metric metric_;
    MeasurementStatus status_;
    std::optional<Value> value_;
    std::string scope_;
    Dimensions dimensions_;
    MeasurementPeriod period_;
    MeasurementQuality quality_;
    std::optional<Context> context_;
    Provenance provenance_;
};

} // namespace evolution::measurement

namespace evolution {

using measurement::Measurement;
using measurement::MeasurementPeriod;
using measurement::MeasurementQuality;
using measurement::MeasurementStatus;
using measurement::Metric;
using measurement::MetricValueKind;
using measurement::QualityLevel;
using measurement::Unit;

} // namespace evolution
