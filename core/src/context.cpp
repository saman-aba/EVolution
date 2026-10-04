#include "evolution/core/context/context.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <utility>

namespace evolution::context {
namespace {

auto valid_name(std::string_view name) -> bool {
    return !name.empty() && std::all_of(name.begin(), name.end(), [](char character) {
        const auto byte = static_cast<unsigned char>(character);
        return std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.';
    });
}

auto value_text(const ContextValue &value) -> std::string {
    return std::visit(
        [](const auto &item) -> std::string {
            using Value = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<Value, bool>) {
                return item ? "bool:true" : "bool:false";
            } else if constexpr (std::is_same_v<Value, std::int64_t>) {
                return "int:" + std::to_string(item);
            } else if constexpr (std::is_same_v<Value, double>) {
                std::ostringstream stream;
                stream.imbue(std::locale::classic());
                stream << "double:" << std::setprecision(std::numeric_limits<double>::max_digits10)
                       << item;
                return stream.str();
            } else {
                return "string:" + item;
            }
        },
        value);
}

auto calculate_id(ContextScope scope, const std::vector<ContextEntity> &entities,
                  const Context::Dimensions &dimensions,
                  const std::optional<time::TimeRange> &validity,
                  const std::vector<ContextId> &parents) -> ContextId {
    std::string canonical = "scope:" + std::to_string(static_cast<int>(scope)) + '\n';
    for (const auto &entity : entities) {
        canonical += "entity:" + entity.kind + ':' + entity.identity + '\n';
    }
    for (const auto &[name, value] : dimensions) {
        canonical += "dimension:" + name + '=' + value_text(value) + '\n';
    }
    if (validity) {
        const auto encode = [](const std::optional<time::TimePoint> &value) {
            if (!value) {
                return std::string("open");
            }
            return std::to_string(value->time_since_epoch().count());
        };
        canonical += "validity:" + encode(validity->start()) + ':' + encode(validity->end()) + '\n';
    }
    for (const auto &parent : parents) {
        canonical += "parent:" + parent.to_string() + '\n';
    }
    return ContextId::from_stable_name(canonical).value();
}

} // namespace

Context::Context(ContextId id, ContextScope scope, std::vector<ContextEntity> entities,
                 Dimensions dimensions, std::optional<time::TimeRange> validity,
                 std::vector<ContextId> parents)
    : id_(std::move(id)), scope_(scope), entities_(std::move(entities)),
      dimensions_(std::move(dimensions)), validity_(std::move(validity)),
      parents_(std::move(parents)) {}

auto Context::create(ContextScope scope, std::vector<ContextEntity> entities, Dimensions dimensions,
                     std::optional<time::TimeRange> validity, std::vector<ContextId> parents)
    -> Result<Context> {
    for (const auto &entity : entities) {
        if (!valid_name(entity.kind) || entity.identity.empty()) {
            return Result<Context>::failure(
                Error(ErrorCode::create("context.invalid_entity"), ErrorCategory::InvalidArgument,
                      "context entities require a valid kind and non-empty identity"));
        }
    }
    for (const auto &[name, value] : dimensions) {
        static_cast<void>(value);
        if (!valid_name(name)) {
            return Result<Context>::failure(Error(ErrorCode::create("context.invalid_dimension"),
                                                  ErrorCategory::InvalidArgument,
                                                  "context dimension name is invalid"));
        }
    }

    auto id = calculate_id(scope, entities, dimensions, validity, parents);
    return Result<Context>::success(Context(std::move(id), scope, std::move(entities),
                                            std::move(dimensions), std::move(validity),
                                            std::move(parents)));
}

auto Context::id() const noexcept -> const ContextId & {
    return id_;
}

auto Context::scope() const noexcept -> ContextScope {
    return scope_;
}

auto Context::entities() const noexcept -> const std::vector<ContextEntity> & {
    return entities_;
}

auto Context::dimensions() const noexcept -> const Dimensions & {
    return dimensions_;
}

auto Context::validity() const noexcept -> const std::optional<time::TimeRange> & {
    return validity_;
}

auto Context::parents() const noexcept -> const std::vector<ContextId> & {
    return parents_;
}

auto Context::project(std::span<const std::string_view> dimension_names) const -> Result<Context> {
    Dimensions projected;
    for (const auto name : dimension_names) {
        const auto iterator = dimensions_.find(name);
        if (iterator == dimensions_.end()) {
            return Result<Context>::failure(
                Error(ErrorCode::create("context.dimension_not_found"), ErrorCategory::NotFound,
                      "context projection requested an unavailable dimension")
                    .with_context("dimension", std::string(name)));
        }
        projected.emplace(iterator->first, iterator->second);
    }
    return create(scope_, entities_, std::move(projected), validity_, {id_});
}

} // namespace evolution::context
