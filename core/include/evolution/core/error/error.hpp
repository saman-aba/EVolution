#pragma once

#include "evolution/core/api.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace evolution::error {

enum class ErrorCategory {
    InvalidArgument,
    InvalidConfiguration,
    NotFound,
    Conflict,
    Unavailable,
    ResourceExhausted,
    DeadlineExceeded,
    Cancelled,
    Serialization,
    Persistence,
    ExternalDependency,
    Unauthorized,
    Internal,
};

class EVOLUTION_CORE_API ErrorCode {
  public:
    static auto create(std::string value) -> ErrorCode;

    [[nodiscard]] auto value() const noexcept -> std::string_view;

    friend auto operator==(const ErrorCode &, const ErrorCode &) -> bool = default;

  private:
    explicit ErrorCode(std::string value);

    std::string value_;
};

struct ErrorContext {
    std::string key;
    std::string value;

    friend auto operator==(const ErrorContext &, const ErrorContext &) -> bool = default;
};

class EVOLUTION_CORE_API Error {
  public:
    Error(ErrorCode code, ErrorCategory category, std::string message);

    [[nodiscard]] auto code() const noexcept -> const ErrorCode &;
    [[nodiscard]] auto category() const noexcept -> ErrorCategory;
    [[nodiscard]] auto message() const noexcept -> std::string_view;
    [[nodiscard]] auto context() const noexcept -> const std::vector<ErrorContext> &;
    [[nodiscard]] auto cause() const noexcept -> const Error *;

    [[nodiscard]] auto with_context(std::string key, std::string value) const -> Error;
    [[nodiscard]] auto translated(ErrorCode code, ErrorCategory category, std::string message) const
        -> Error;

  private:
    Error(ErrorCode code, ErrorCategory category, std::string message,
          std::vector<ErrorContext> context, std::shared_ptr<const Error> cause);

    ErrorCode code_;
    ErrorCategory category_;
    std::string message_;
    std::vector<ErrorContext> context_;
    std::shared_ptr<const Error> cause_;
};

} // namespace evolution::error

namespace evolution {

using Error = error::Error;
using ErrorCategory = error::ErrorCategory;
using ErrorCode = error::ErrorCode;
using ErrorContext = error::ErrorContext;

} // namespace evolution
