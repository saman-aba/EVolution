#include "evolution/processing/observability.hpp"

#include <deque>
#include <mutex>
#include <type_traits>
#include <utility>

namespace evolution::processing {
namespace {

void redact(std::vector<TelemetryField> &fields) {
    for (auto &field : fields) {
        if (field.sensitivity == FieldSensitivity::Sensitive) {
            field.value = "[redacted]";
        }
    }
}

void redact_record(TelemetryRecord &record) {
    std::visit(
        [](auto &value) {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, LogRecord> ||
                          std::is_same_v<Value, OperationalMetric> ||
                          std::is_same_v<Value, TraceSpan> || std::is_same_v<Value, StatusRecord>) {
                redact(value.fields);
            }
        },
        record);
}

} // namespace

struct ObservabilityState {
    ObservabilityState(std::size_t maximum_records, std::shared_ptr<ObservabilitySink> output)
        : capacity(maximum_records), sink(std::move(output)) {}

    mutable std::mutex mutex;
    std::size_t capacity;
    std::shared_ptr<ObservabilitySink> sink;
    std::deque<TelemetryRecord> records;
    std::size_t emitted{};
    std::size_t dropped{};
    std::size_t failed{};
};

ObservabilityChannel::ObservabilityChannel(std::shared_ptr<ObservabilityState> state)
    : state_(std::move(state)) {}

auto ObservabilityChannel::create(std::size_t capacity, std::shared_ptr<ObservabilitySink> sink)
    -> Result<ObservabilityChannel> {
    if (capacity == 0) {
        return Result<ObservabilityChannel>::failure(
            Error(ErrorCode::create("processing.invalid_observability_capacity"),
                  ErrorCategory::InvalidArgument,
                  "observability buffering must be explicitly bounded and non-zero"));
    }
    if (!sink) {
        return Result<ObservabilityChannel>::failure(
            Error(ErrorCode::create("processing.missing_observability_sink"),
                  ErrorCategory::InvalidArgument, "observability channel requires a sink"));
    }
    return Result<ObservabilityChannel>::success(
        ObservabilityChannel(std::make_shared<ObservabilityState>(capacity, std::move(sink))));
}

auto ObservabilityChannel::publish(TelemetryRecord record) noexcept -> TelemetryDisposition {
    try {
        redact_record(record);
        const std::scoped_lock lock(state_->mutex);
        if (!state_->sink) {
            ++state_->failed;
            return TelemetryDisposition::Unavailable;
        }
        if (state_->records.size() >= state_->capacity) {
            ++state_->dropped;
            return TelemetryDisposition::Dropped;
        }
        state_->records.push_back(std::move(record));
        return TelemetryDisposition::Accepted;
    } catch (...) {
        const std::scoped_lock lock(state_->mutex);
        ++state_->failed;
        return TelemetryDisposition::Unavailable;
    }
}

auto ObservabilityChannel::flush() noexcept -> TelemetryStats {
    for (;;) {
        std::optional<TelemetryRecord> record;
        std::shared_ptr<ObservabilitySink> sink;
        {
            const std::scoped_lock lock(state_->mutex);
            if (state_->records.empty()) {
                return TelemetryStats{state_->records.size(), state_->emitted, state_->dropped,
                                      state_->failed};
            }
            record.emplace(std::move(state_->records.front()));
            state_->records.pop_front();
            sink = state_->sink;
        }

        try {
            auto result = sink->emit(*record);
            const std::scoped_lock lock(state_->mutex);
            if (result.has_value()) {
                ++state_->emitted;
            } else {
                ++state_->failed;
            }
        } catch (...) {
            const std::scoped_lock lock(state_->mutex);
            ++state_->failed;
        }
    }
}

auto ObservabilityChannel::stats() const noexcept -> TelemetryStats {
    const std::scoped_lock lock(state_->mutex);
    return TelemetryStats{state_->records.size(), state_->emitted, state_->dropped, state_->failed};
}

} // namespace evolution::processing
