#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/core/event/event.hpp"
#include "evolution/core/identity/identifiers.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace evolution::event {

class StreamPosition {
  public:
    explicit constexpr StreamPosition(std::uint64_t value) noexcept : value_(value) {}

    [[nodiscard]] constexpr auto value() const noexcept -> std::uint64_t {
        return value_;
    }
    friend auto operator==(const StreamPosition &, const StreamPosition &) -> bool = default;
    friend auto operator<=>(const StreamPosition &, const StreamPosition &) = default;

  private:
    std::uint64_t value_;
};

struct StreamCursor {
    StreamId stream_id;
    StreamPosition next_position;

    friend auto operator==(const StreamCursor &, const StreamCursor &) -> bool = default;
};

enum class DuplicateEventPolicy {
    Reject,
    Idempotent,
    Allow,
};

enum class AppendOutcome {
    Appended,
    IdempotentExisting,
};

struct AppendResult {
    AppendOutcome outcome;
    StreamPosition position;

    friend auto operator==(const AppendResult &, const AppendResult &) -> bool = default;
};

template <typename Payload> struct EventRecord {
    StreamPosition position;
    Event<Payload> event;

    friend auto operator==(const EventRecord &, const EventRecord &) -> bool = default;
};

template <typename Payload> struct HistoryBatch {
    std::vector<EventRecord<Payload>> records;
    StreamCursor next_cursor;
    bool end_of_stream;
    std::optional<RunId> replay_run;

    friend auto operator==(const HistoryBatch &, const HistoryBatch &) -> bool = default;
};

template <typename Payload> class EventReader {
  public:
    virtual ~EventReader() = default;

    [[nodiscard]] virtual auto stream_id() const noexcept -> const StreamId & = 0;
    virtual auto read(StreamCursor cursor, std::size_t limit) const
        -> Result<HistoryBatch<Payload>> = 0;
    virtual auto replay(RunId run_id, StreamCursor cursor, std::size_t limit) const
        -> Result<HistoryBatch<Payload>> = 0;
};

template <typename Payload> class EventAppender {
  public:
    virtual ~EventAppender() = default;

    virtual auto append(Event<Payload> event) -> Result<AppendResult> = 0;
};

template <typename Payload>
    requires std::copy_constructible<Payload>
class EventHistory final : public EventReader<Payload>, public EventAppender<Payload> {
  public:
    static auto create(StreamId stream_id, DuplicateEventPolicy duplicate_policy,
                       std::size_t maximum_read_size) -> Result<EventHistory> {
        if (maximum_read_size == 0) {
            return Result<EventHistory>::failure(Error(
                ErrorCode::create("event_stream.invalid_read_limit"),
                ErrorCategory::InvalidArgument, "maximum read size must be greater than zero"));
        }
        return Result<EventHistory>::success(
            EventHistory(std::move(stream_id), duplicate_policy, maximum_read_size));
    }

    EventHistory(EventHistory &&other) noexcept
        : stream_id_(other.stream_id_), duplicate_policy_(other.duplicate_policy_),
          maximum_read_size_(other.maximum_read_size_) {
        const std::scoped_lock lock(other.mutex_);
        records_ = std::move(other.records_);
    }

    auto operator=(EventHistory &&other) noexcept -> EventHistory & {
        if (this == &other) {
            return *this;
        }
        const std::scoped_lock lock(mutex_, other.mutex_);
        stream_id_ = std::move(other.stream_id_);
        duplicate_policy_ = other.duplicate_policy_;
        maximum_read_size_ = other.maximum_read_size_;
        records_ = std::move(other.records_);
        return *this;
    }

    EventHistory(const EventHistory &) = delete;
    auto operator=(const EventHistory &) -> EventHistory & = delete;

    [[nodiscard]] auto stream_id() const noexcept -> const StreamId & override {
        return stream_id_;
    }

    auto append(Event<Payload> event) -> Result<AppendResult> override {
        const std::scoped_lock lock(mutex_);
        const auto existing =
            std::find_if(records_.begin(), records_.end(), [&](const auto &record) {
                return record.event.metadata().id == event.metadata().id;
            });
        if (existing != records_.end()) {
            if (duplicate_policy_ == DuplicateEventPolicy::Reject) {
                return Result<AppendResult>::failure(
                    Error(ErrorCode::create("event_stream.duplicate_event"),
                          ErrorCategory::Conflict, "event identity already exists in the stream"));
            }
            if (duplicate_policy_ == DuplicateEventPolicy::Idempotent) {
                return Result<AppendResult>::success(
                    AppendResult{AppendOutcome::IdempotentExisting, existing->position});
            }
        }

        const auto position = StreamPosition(static_cast<std::uint64_t>(records_.size()));
        records_.push_back(EventRecord<Payload>{position, std::move(event)});
        return Result<AppendResult>::success(AppendResult{AppendOutcome::Appended, position});
    }

    auto read(StreamCursor cursor, std::size_t limit) const
        -> Result<HistoryBatch<Payload>> override {
        return read_impl(std::move(cursor), limit, std::nullopt);
    }

    auto replay(RunId run_id, StreamCursor cursor, std::size_t limit) const
        -> Result<HistoryBatch<Payload>> override {
        return read_impl(std::move(cursor), limit, std::move(run_id));
    }

  private:
    EventHistory(StreamId stream_id, DuplicateEventPolicy duplicate_policy,
                 std::size_t maximum_read_size)
        : stream_id_(std::move(stream_id)), duplicate_policy_(duplicate_policy),
          maximum_read_size_(maximum_read_size) {}

    auto read_impl(StreamCursor cursor, std::size_t limit, std::optional<RunId> replay_run) const
        -> Result<HistoryBatch<Payload>> {
        if (cursor.stream_id != stream_id_) {
            return Result<HistoryBatch<Payload>>::failure(Error(
                ErrorCode::create("event_stream.cursor_mismatch"), ErrorCategory::InvalidArgument,
                "cursor belongs to a different event stream"));
        }
        if (limit == 0 || limit > maximum_read_size_) {
            return Result<HistoryBatch<Payload>>::failure(
                Error(ErrorCode::create("event_stream.invalid_read_limit"),
                      ErrorCategory::InvalidArgument,
                      "read limit must be within the configured bounded range"));
        }

        const std::scoped_lock lock(mutex_);
        const auto start = cursor.next_position.value();
        if (start > records_.size()) {
            return Result<HistoryBatch<Payload>>::failure(
                Error(ErrorCode::create("event_stream.position_out_of_range"),
                      ErrorCategory::InvalidArgument,
                      "cursor position is beyond the current stream end"));
        }
        const auto available = records_.size() - static_cast<std::size_t>(start);
        const auto count = std::min(limit, available);
        std::vector<EventRecord<Payload>> batch;
        batch.reserve(count);
        const auto first = records_.begin() + static_cast<std::ptrdiff_t>(start);
        batch.insert(batch.end(), first, first + static_cast<std::ptrdiff_t>(count));
        const auto next = start + static_cast<std::uint64_t>(count);
        return Result<HistoryBatch<Payload>>::success(HistoryBatch<Payload>{
            std::move(batch),
            StreamCursor{stream_id_, StreamPosition(next)},
            next == records_.size(),
            std::move(replay_run),
        });
    }

    mutable std::mutex mutex_;
    StreamId stream_id_;
    DuplicateEventPolicy duplicate_policy_;
    std::size_t maximum_read_size_;
    std::vector<EventRecord<Payload>> records_;
};

} // namespace evolution::event

namespace evolution {

using event::AppendOutcome;
using event::AppendResult;
using event::DuplicateEventPolicy;
using event::EventAppender;
using event::EventHistory;
using event::EventReader;
using event::EventRecord;
using event::HistoryBatch;
using event::StreamCursor;
using event::StreamPosition;

} // namespace evolution
