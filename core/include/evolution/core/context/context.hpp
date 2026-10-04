#pragma once

#include "evolution/core/api.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace evolution::context {

enum class ContextScope {
    Object,
    Entity,
    Session,
    Domain,
    Application,
};

using ContextValue = std::variant<bool, std::int64_t, double, std::string>;

struct ContextEntity {
    std::string kind;
    std::string identity;

    friend auto operator==(const ContextEntity &, const ContextEntity &) -> bool = default;
};

class EVOLUTION_CORE_API Context {
  public:
    using Dimensions = std::map<std::string, ContextValue, std::less<>>;

    static auto create(ContextScope scope, std::vector<ContextEntity> entities,
                       Dimensions dimensions,
                       std::optional<time::TimeRange> validity = std::nullopt,
                       std::vector<ContextId> parents = {}) -> Result<Context>;

    [[nodiscard]] auto id() const noexcept -> const ContextId &;
    [[nodiscard]] auto scope() const noexcept -> ContextScope;
    [[nodiscard]] auto entities() const noexcept -> const std::vector<ContextEntity> &;
    [[nodiscard]] auto dimensions() const noexcept -> const Dimensions &;
    [[nodiscard]] auto validity() const noexcept -> const std::optional<time::TimeRange> &;
    [[nodiscard]] auto parents() const noexcept -> const std::vector<ContextId> &;

    template <typename T> [[nodiscard]] auto dimension(std::string_view name) const -> Result<T> {
        const auto iterator = dimensions_.find(name);
        if (iterator == dimensions_.end()) {
            return Result<T>::failure(Error(ErrorCode::create("context.dimension_not_found"),
                                            ErrorCategory::NotFound,
                                            "context dimension was not found"));
        }
        if (const auto *value = std::get_if<T>(&iterator->second)) {
            return Result<T>::success(*value);
        }
        return Result<T>::failure(Error(ErrorCode::create("context.dimension_type_mismatch"),
                                        ErrorCategory::InvalidArgument,
                                        "context dimension has a different type"));
    }

    [[nodiscard]] auto project(std::span<const std::string_view> dimension_names) const
        -> Result<Context>;

    friend auto operator==(const Context &, const Context &) -> bool = default;

  private:
    Context(ContextId id, ContextScope scope, std::vector<ContextEntity> entities,
            Dimensions dimensions, std::optional<time::TimeRange> validity,
            std::vector<ContextId> parents);

    ContextId id_;
    ContextScope scope_;
    std::vector<ContextEntity> entities_;
    Dimensions dimensions_;
    std::optional<time::TimeRange> validity_;
    std::vector<ContextId> parents_;
};

} // namespace evolution::context

namespace evolution {

using context::Context;
using context::ContextEntity;
using context::ContextScope;
using context::ContextValue;

} // namespace evolution
