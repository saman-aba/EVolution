#include "evolution/core/error/error.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace evolution::error {

auto ErrorCode::create(std::string value) -> ErrorCode {
    const auto valid =
        !value.empty() && std::all_of(value.begin(), value.end(), [](char character) {
            const auto byte = static_cast<unsigned char>(character);
            return std::islower(byte) != 0 || std::isdigit(byte) != 0 || character == '.' ||
                   character == '_';
        });
    if (!valid) {
        throw std::invalid_argument(
            "error codes must contain lowercase ASCII letters, digits, dots, or underscores");
    }
    return ErrorCode(std::move(value));
}

ErrorCode::ErrorCode(std::string value) : value_(std::move(value)) {}

auto ErrorCode::value() const noexcept -> std::string_view {
    return value_;
}

Error::Error(ErrorCode code, ErrorCategory category, std::string message)
    : Error(std::move(code), category, std::move(message), {}, nullptr) {}

Error::Error(ErrorCode code, ErrorCategory category, std::string message,
             std::vector<ErrorContext> context, std::shared_ptr<const Error> cause)
    : code_(std::move(code)), category_(category), message_(std::move(message)),
      context_(std::move(context)), cause_(std::move(cause)) {}

auto Error::code() const noexcept -> const ErrorCode & {
    return code_;
}

auto Error::category() const noexcept -> ErrorCategory {
    return category_;
}

auto Error::message() const noexcept -> std::string_view {
    return message_;
}

auto Error::context() const noexcept -> const std::vector<ErrorContext> & {
    return context_;
}

auto Error::cause() const noexcept -> const Error * {
    return cause_.get();
}

auto Error::with_context(std::string key, std::string value) const -> Error {
    auto enriched_context = context_;
    enriched_context.push_back(ErrorContext{std::move(key), std::move(value)});
    return Error(code_, category_, message_, std::move(enriched_context), cause_);
}

auto Error::translated(ErrorCode code, ErrorCategory category, std::string message) const -> Error {
    return Error(std::move(code), category, std::move(message), {},
                 std::make_shared<const Error>(*this));
}

} // namespace evolution::error
