#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/processing/api.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace evolution::processing {

struct SpanTag;
using SpanId = identity::Id<SpanTag>;

enum class LogSeverity {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

enum class FieldSensitivity {
    Public,
    Sensitive,
};

struct TelemetryField {
    std::string key;
    std::string value;
    FieldSensitivity sensitivity{FieldSensitivity::Public};
};

struct LogRecord {
    std::chrono::system_clock::time_point timestamp;
    LogSeverity severity;
    ComponentId component;
    std::string message;
    std::vector<TelemetryField> fields;
    std::optional<CorrelationId> correlation_id;
};

struct OperationalMetric {
    std::chrono::system_clock::time_point timestamp;
    ComponentId component;
    std::string name;
    double value{};
    std::string unit;
    std::vector<TelemetryField> fields;
};

struct TraceSpan {
    TraceId trace_id;
    SpanId span_id;
    std::optional<SpanId> parent_span_id;
    ComponentId component;
    std::string operation;
    std::chrono::nanoseconds elapsed{};
    std::vector<TelemetryField> fields;
};

enum class HealthState {
    Healthy,
    Degraded,
    Unhealthy,
    Unknown,
};

struct HealthRecord {
    ComponentId component;
    HealthState state;
    std::string reason;
};

struct StatusRecord {
    ComponentId component;
    std::string state;
    std::vector<TelemetryField> fields;
};

using TelemetryRecord =
    std::variant<LogRecord, OperationalMetric, TraceSpan, HealthRecord, StatusRecord>;

class EVOLUTION_PROCESSING_API ObservabilitySink {
  public:
    virtual ~ObservabilitySink() = default;
    virtual auto emit(const TelemetryRecord &record) -> Result<void> = 0;
};

enum class TelemetryDisposition {
    Accepted,
    Dropped,
    Unavailable,
};

struct TelemetryStats {
    std::size_t buffered{};
    std::size_t emitted{};
    std::size_t dropped{};
    std::size_t failed{};
};

struct ObservabilityState;

class EVOLUTION_PROCESSING_API ObservabilityChannel {
  public:
    static auto create(std::size_t capacity, std::shared_ptr<ObservabilitySink> sink)
        -> Result<ObservabilityChannel>;

    auto publish(TelemetryRecord record) noexcept -> TelemetryDisposition;
    auto flush() noexcept -> TelemetryStats;
    [[nodiscard]] auto stats() const noexcept -> TelemetryStats;

  private:
    explicit ObservabilityChannel(std::shared_ptr<ObservabilityState> state);
    std::shared_ptr<ObservabilityState> state_;
};

} // namespace evolution::processing
