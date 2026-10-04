#include "evolution/core/event/event.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace evolution::event {

EventType::EventType(std::string value) : value_(std::move(value)) {}

auto EventType::create(std::string value) -> Result<EventType> {
    const auto valid = !value.empty() && value.find('.') != std::string::npos &&
                       std::all_of(value.begin(), value.end(), [](char character) {
                           const auto byte = static_cast<unsigned char>(character);
                           return std::isalnum(byte) != 0 || character == '.' || character == '_' ||
                                  character == '-';
                       });
    if (!valid) {
        return Result<EventType>::failure(
            Error(ErrorCode::create("event.invalid_type"), ErrorCategory::InvalidArgument,
                  "event type must be a qualified stable identifier"));
    }
    return Result<EventType>::success(EventType(std::move(value)));
}

auto EventType::value() const noexcept -> std::string_view {
    return value_;
}

EventSource::EventSource(SourceId id, std::string name)
    : id_(std::move(id)), name_(std::move(name)) {}

auto EventSource::create(SourceId id, std::string name) -> Result<EventSource> {
    if (name.empty()) {
        return Result<EventSource>::failure(Error(ErrorCode::create("event.invalid_source"),
                                                  ErrorCategory::InvalidArgument,
                                                  "event source name cannot be empty"));
    }
    return Result<EventSource>::success(EventSource(std::move(id), std::move(name)));
}

auto EventSource::id() const noexcept -> const SourceId & {
    return id_;
}

auto EventSource::name() const noexcept -> std::string_view {
    return name_;
}

} // namespace evolution::event
