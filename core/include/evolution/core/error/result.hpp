#pragma once

#include "evolution/core/error/error.hpp"

#include <concepts>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace evolution {

template <typename T> class [[nodiscard]] Result {
  public:
    static auto success(T value) -> Result {
        return Result(std::in_place_index<0>, std::move(value));
    }

    static auto failure(Error error) -> Result {
        return Result(std::in_place_index<1>, std::move(error));
    }

    [[nodiscard]] auto has_value() const noexcept -> bool {
        return storage_.index() == 0;
    }

    [[nodiscard]] auto has_error() const noexcept -> bool {
        return !has_value();
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    auto value() & -> T & {
        ensure_value();
        return std::get<0>(storage_);
    }

    auto value() const & -> const T & {
        ensure_value();
        return std::get<0>(storage_);
    }

    auto value() && -> T && {
        ensure_value();
        return std::get<0>(std::move(storage_));
    }

    template <typename U> [[nodiscard]] auto value_or(U &&fallback) const & -> T {
        if (has_value()) {
            return value();
        }
        return static_cast<T>(std::forward<U>(fallback));
    }

    [[nodiscard]] auto error() const & -> const Error & {
        ensure_error();
        return std::get<1>(storage_);
    }

    auto error() && -> Error && {
        ensure_error();
        return std::get<1>(std::move(storage_));
    }

    [[nodiscard]] auto with_error_context(std::string key, std::string value) const & -> Result
        requires std::copy_constructible<T>
    {
        if (has_value()) {
            return success(this->value());
        }
        return failure(error().with_context(std::move(key), std::move(value)));
    }

    template <typename Function> auto and_then(Function &&function) & {
        using Return = std::invoke_result_t<Function, T &>;
        static_assert(requires(Return result) {
            result.has_value();
            result.has_error();
        });
        if (has_error()) {
            return Return::failure(error());
        }
        return std::invoke(std::forward<Function>(function), value());
    }

  private:
    template <std::size_t Index, typename Value>
    Result(std::in_place_index_t<Index> index, Value &&value)
        : storage_(index, std::forward<Value>(value)) {}

    void ensure_value() const {
        if (has_error()) {
            throw std::logic_error("attempted to access the value of an error Result");
        }
    }

    void ensure_error() const {
        if (has_value()) {
            throw std::logic_error("attempted to access the error of a successful Result");
        }
    }

    std::variant<T, Error> storage_;
};

template <> class [[nodiscard]] Result<void> {
  public:
    static auto success() -> Result {
        return Result(std::in_place_index<0>);
    }

    static auto failure(Error error) -> Result {
        return Result(std::in_place_index<1>, std::move(error));
    }

    [[nodiscard]] auto has_value() const noexcept -> bool {
        return storage_.index() == 0;
    }

    [[nodiscard]] auto has_error() const noexcept -> bool {
        return !has_value();
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    void value() const {
        if (has_error()) {
            throw std::logic_error("attempted to access the value of an error Result");
        }
    }

    [[nodiscard]] auto error() const & -> const Error & {
        if (has_value()) {
            throw std::logic_error("attempted to access the error of a successful Result");
        }
        return std::get<1>(storage_);
    }

    auto error() && -> Error && {
        if (has_value()) {
            throw std::logic_error("attempted to access the error of a successful Result");
        }
        return std::get<1>(std::move(storage_));
    }

    [[nodiscard]] auto with_error_context(std::string key, std::string value) const -> Result {
        if (has_value()) {
            return success();
        }
        return failure(error().with_context(std::move(key), std::move(value)));
    }

  private:
    explicit Result(std::in_place_index_t<0>) : storage_(std::in_place_index<0>) {}

    Result(std::in_place_index_t<1>, Error error)
        : storage_(std::in_place_index<1>, std::move(error)) {}

    std::variant<std::monostate, Error> storage_;
};

template <typename T> [[nodiscard]] auto failure(Error error) -> Result<T> {
    return Result<T>::failure(std::move(error));
}

} // namespace evolution
