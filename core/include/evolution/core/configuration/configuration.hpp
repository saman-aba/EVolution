#pragma once

#include "evolution/core/api.hpp"
#include "evolution/core/configuration/secret_reference.hpp"
#include "evolution/core/identity/identifiers.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace evolution::configuration {

using ConfigurationValue = std::variant<bool, std::int64_t, double, std::string, SecretReference>;

class EVOLUTION_CORE_API Configuration {
  public:
    using Entries = std::map<std::string, ConfigurationValue, std::less<>>;

    static auto create(Entries entries, std::string version = "1") -> Result<Configuration>;

    [[nodiscard]] auto id() const noexcept -> const ConfigurationId &;
    [[nodiscard]] auto version() const noexcept -> std::string_view;
    [[nodiscard]] auto entries() const noexcept -> const Entries &;
    [[nodiscard]] auto contains(std::string_view path) const -> bool;

    template <typename T> [[nodiscard]] auto get(std::string_view path) const -> Result<T> {
        static_assert(std::is_same_v<T, bool> || std::is_same_v<T, std::int64_t> ||
                          std::is_same_v<T, double> || std::is_same_v<T, std::string> ||
                          std::is_same_v<T, SecretReference>,
                      "unsupported configuration value type");

        const auto iterator = entries_.find(path);
        if (iterator == entries_.end()) {
            return Result<T>::failure(Error(ErrorCode::create("configuration.not_found"),
                                            ErrorCategory::NotFound,
                                            "configuration path was not found"));
        }
        if (const auto *value = std::get_if<T>(&iterator->second)) {
            return Result<T>::success(*value);
        }
        return Result<T>::failure(Error(ErrorCode::create("configuration.type_mismatch"),
                                        ErrorCategory::InvalidConfiguration,
                                        "configuration value has a different type"));
    }

    [[nodiscard]] auto child(std::string_view prefix) const -> Result<Configuration>;
    [[nodiscard]] auto overlay(const Configuration &higher_precedence) const -> Configuration;

    friend auto operator==(const Configuration &, const Configuration &) -> bool = default;

  private:
    Configuration(ConfigurationId id, Entries entries, std::string version);

    [[nodiscard]] static auto validate_path(std::string_view path) -> Result<void>;
    [[nodiscard]] static auto calculate_id(const Entries &entries, std::string_view version)
        -> ConfigurationId;

    ConfigurationId id_;
    Entries entries_;
    std::string version_;
};

} // namespace evolution::configuration

namespace evolution {

using configuration::Configuration;
using configuration::ConfigurationValue;
using configuration::SecretReference;

} // namespace evolution
