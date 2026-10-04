#pragma once

#include "evolution/core/api.hpp"
#include "evolution/core/context/context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/provenance/provenance.hpp"
#include "evolution/core/time/time.hpp"

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace evolution::event {

class EVOLUTION_CORE_API EventType {
  public:
    static auto create(std::string value) -> Result<EventType>;

    [[nodiscard]] auto value() const noexcept -> std::string_view;
    friend auto operator==(const EventType &, const EventType &) -> bool = default;

  private:
    explicit EventType(std::string value);
    std::string value_;
};

class EVOLUTION_CORE_API EventSource {
  public:
    static auto create(SourceId id, std::string name) -> Result<EventSource>;

    [[nodiscard]] auto id() const noexcept -> const SourceId &;
    [[nodiscard]] auto name() const noexcept -> std::string_view;
    friend auto operator==(const EventSource &, const EventSource &) -> bool = default;

  private:
    EventSource(SourceId id, std::string name);
    SourceId id_;
    std::string name_;
};

struct EventSequence {
    std::string scope;
    std::uint64_t value;

    friend auto operator==(const EventSequence &, const EventSequence &) -> bool = default;
};

struct EventMetadata {
    EventId id;
    EventType type;
    EventSource source;
    time::TemporalInformation occurrence_time;
    std::optional<time::TimePoint> ingestion_time;
    std::optional<EventSequence> sequence;
    std::optional<Context> context;
    std::optional<Provenance> provenance;
    std::string schema_version;
    std::string event_version;

    friend auto operator==(const EventMetadata &, const EventMetadata &) -> bool = default;
};

template <typename Payload>
    requires std::movable<Payload>
class Event {
  public:
    static auto create(EventMetadata metadata, Payload payload) -> Result<Event> {
        if (metadata.schema_version.empty()) {
            return invalid("event schema version cannot be empty");
        }
        if (metadata.event_version.empty()) {
            return invalid("event version cannot be empty");
        }
        if (metadata.sequence && metadata.sequence->scope.empty()) {
            return invalid("event sequence requires a non-empty scope");
        }
        return Result<Event>::success(Event(std::move(metadata), std::move(payload)));
    }

    [[nodiscard]] auto metadata() const noexcept -> const EventMetadata & {
        return metadata_;
    }

    [[nodiscard]] auto payload() const noexcept -> const Payload & {
        return payload_;
    }

    friend auto operator==(const Event &, const Event &) -> bool = default;

  private:
    Event(EventMetadata metadata, Payload payload)
        : metadata_(std::move(metadata)), payload_(std::move(payload)) {}

    static auto invalid(std::string message) -> Result<Event> {
        return Result<Event>::failure(Error(ErrorCode::create("event.invalid"),
                                            ErrorCategory::InvalidArgument, std::move(message)));
    }

    EventMetadata metadata_;
    Payload payload_;
};

} // namespace evolution::event

namespace evolution {

using event::Event;
using event::EventMetadata;
using event::EventSequence;
using event::EventSource;
using event::EventType;

} // namespace evolution
