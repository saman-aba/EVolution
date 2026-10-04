#pragma once

#include "evolution/core/context/context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/event/event.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/provenance/provenance.hpp"
#include "evolution/core/time/time.hpp"

#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace evolution::state {

enum class StateCompleteness {
    Complete,
    Partial,
    Unknown,
};

struct StateVersion {
    std::uint64_t revision;
    std::optional<EventId> through_event;

    friend auto operator==(const StateVersion &, const StateVersion &) -> bool = default;
};

struct SnapshotBoundary {
    StreamId stream_id;
    std::uint64_t next_position;

    friend auto operator==(const SnapshotBoundary &, const SnapshotBoundary &) -> bool = default;
};

template <typename Value>
    requires std::movable<Value>
class State {
  public:
    static auto create(StateId id, std::string entity_identity, StateVersion version,
                       ProjectionId projection_id, std::string projection_version, Value value,
                       StateCompleteness completeness, time::TemporalInformation effective_time,
                       std::optional<time::TimeRange> validity, std::optional<Context> context,
                       std::optional<Provenance> provenance) -> Result<State> {
        if (entity_identity.empty()) {
            return invalid("state entity identity cannot be empty");
        }
        if (projection_version.empty()) {
            return invalid("state projection version cannot be empty");
        }
        return Result<State>::success(
            State(std::move(id), std::move(entity_identity), std::move(version),
                  std::move(projection_id), std::move(projection_version), std::move(value),
                  completeness, std::move(effective_time), std::move(validity), std::move(context),
                  std::move(provenance)));
    }

    [[nodiscard]] auto id() const noexcept -> const StateId & {
        return id_;
    }
    [[nodiscard]] auto entity_identity() const noexcept -> std::string_view {
        return entity_identity_;
    }
    [[nodiscard]] auto version() const noexcept -> const StateVersion & {
        return version_;
    }
    [[nodiscard]] auto projection_id() const noexcept -> const ProjectionId & {
        return projection_id_;
    }
    [[nodiscard]] auto projection_version() const noexcept -> std::string_view {
        return projection_version_;
    }
    [[nodiscard]] auto value() const noexcept -> const Value & {
        return value_;
    }
    [[nodiscard]] auto completeness() const noexcept -> StateCompleteness {
        return completeness_;
    }
    [[nodiscard]] auto effective_time() const noexcept -> const time::TemporalInformation & {
        return effective_time_;
    }
    [[nodiscard]] auto validity() const noexcept -> const std::optional<time::TimeRange> & {
        return validity_;
    }
    [[nodiscard]] auto context() const noexcept -> const std::optional<Context> & {
        return context_;
    }
    [[nodiscard]] auto provenance() const noexcept -> const std::optional<Provenance> & {
        return provenance_;
    }

    friend auto operator==(const State &, const State &) -> bool = default;

  private:
    State(StateId id, std::string entity_identity, StateVersion version, ProjectionId projection_id,
          std::string projection_version, Value value, StateCompleteness completeness,
          time::TemporalInformation effective_time, std::optional<time::TimeRange> validity,
          std::optional<Context> context, std::optional<Provenance> provenance)
        : id_(std::move(id)), entity_identity_(std::move(entity_identity)),
          version_(std::move(version)), projection_id_(std::move(projection_id)),
          projection_version_(std::move(projection_version)), value_(std::move(value)),
          completeness_(completeness), effective_time_(std::move(effective_time)),
          validity_(std::move(validity)), context_(std::move(context)),
          provenance_(std::move(provenance)) {}

    static auto invalid(std::string message) -> Result<State> {
        return Result<State>::failure(Error(ErrorCode::create("state.invalid"),
                                            ErrorCategory::InvalidArgument, std::move(message)));
    }

    StateId id_;
    std::string entity_identity_;
    StateVersion version_;
    ProjectionId projection_id_;
    std::string projection_version_;
    Value value_;
    StateCompleteness completeness_;
    time::TemporalInformation effective_time_;
    std::optional<time::TimeRange> validity_;
    std::optional<Context> context_;
    std::optional<Provenance> provenance_;
};

template <typename Value> struct StateSnapshot {
    State<Value> state;
    SnapshotBoundary boundary;
    std::string snapshot_version;
    Provenance provenance;

    friend auto operator==(const StateSnapshot &, const StateSnapshot &) -> bool = default;
};

template <typename StateValue, typename EventPayload> class Projection {
  public:
    virtual ~Projection() = default;

    [[nodiscard]] virtual auto id() const noexcept -> const ProjectionId & = 0;
    [[nodiscard]] virtual auto version() const noexcept -> std::string_view = 0;
    virtual auto initial_state() const -> Result<State<StateValue>> = 0;
    virtual auto apply(const State<StateValue> &current, const Event<EventPayload> &event) const
        -> Result<State<StateValue>> = 0;
};

template <typename StateValue, typename EventPayload>
auto reconstruct(const Projection<StateValue, EventPayload> &projection,
                 std::span<const Event<EventPayload>> events) -> Result<State<StateValue>> {
    auto current = projection.initial_state();
    if (current.has_error()) {
        return current;
    }
    for (const auto &event : events) {
        auto next = projection.apply(current.value(), event);
        if (next.has_error()) {
            return Result<State<StateValue>>::failure(
                next.error().with_context("projection", std::string(projection.version())));
        }
        current = std::move(next);
    }
    return current;
}

} // namespace evolution::state

namespace evolution {

using state::Projection;
using state::reconstruct;
using state::SnapshotBoundary;
using state::State;
using state::StateCompleteness;
using state::StateSnapshot;
using state::StateVersion;

} // namespace evolution
