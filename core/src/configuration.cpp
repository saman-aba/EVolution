#include "evolution/core/configuration/configuration.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <utility>

namespace evolution::configuration {
namespace {

auto value_text(const ConfigurationValue &value) -> std::string {
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
            } else if constexpr (std::is_same_v<Value, std::string>) {
                return "string:" + item;
            } else {
                return "secret:" + std::string(item.provider()) + ':' + std::string(item.key());
            }
        },
        value);
}

} // namespace

SecretReference::SecretReference(std::string provider, std::string key)
    : provider_(std::move(provider)), key_(std::move(key)) {}

auto SecretReference::create(std::string provider, std::string key) -> Result<SecretReference> {
    if (provider.empty() || key.empty()) {
        return Result<SecretReference>::failure(Error(
            ErrorCode::create("configuration.invalid_secret_reference"),
            ErrorCategory::InvalidConfiguration, "secret provider and key must both be non-empty"));
    }
    return Result<SecretReference>::success(SecretReference(std::move(provider), std::move(key)));
}

auto SecretReference::provider() const noexcept -> std::string_view {
    return provider_;
}

auto SecretReference::key() const noexcept -> std::string_view {
    return key_;
}

Configuration::Configuration(ConfigurationId id, Entries entries, std::string version)
    : id_(std::move(id)), entries_(std::move(entries)), version_(std::move(version)) {}

auto Configuration::create(Entries entries, std::string version) -> Result<Configuration> {
    if (version.empty()) {
        return Result<Configuration>::failure(
            Error(ErrorCode::create("configuration.invalid_version"),
                  ErrorCategory::InvalidConfiguration, "configuration version cannot be empty"));
    }
    for (const auto &[path, value] : entries) {
        static_cast<void>(value);
        auto validation = validate_path(path);
        if (validation.has_error()) {
            return Result<Configuration>::failure(validation.error().with_context("path", path));
        }
    }
    auto id = calculate_id(entries, version);
    return Result<Configuration>::success(
        Configuration(std::move(id), std::move(entries), std::move(version)));
}

auto Configuration::id() const noexcept -> const ConfigurationId & {
    return id_;
}

auto Configuration::version() const noexcept -> std::string_view {
    return version_;
}

auto Configuration::entries() const noexcept -> const Entries & {
    return entries_;
}

auto Configuration::contains(std::string_view path) const -> bool {
    return entries_.contains(path);
}

auto Configuration::child(std::string_view prefix) const -> Result<Configuration> {
    auto validation = validate_path(prefix);
    if (validation.has_error()) {
        return Result<Configuration>::failure(validation.error());
    }

    Entries child_entries;
    const auto nested_prefix = std::string(prefix) + '.';
    for (const auto &[path, value] : entries_) {
        if (path.starts_with(nested_prefix)) {
            child_entries.emplace(path.substr(nested_prefix.size()), value);
        }
    }
    return create(std::move(child_entries), version_);
}

auto Configuration::overlay(const Configuration &higher_precedence) const -> Configuration {
    auto merged = entries_;
    for (const auto &[path, value] : higher_precedence.entries_) {
        merged.insert_or_assign(path, value);
    }
    return Configuration(calculate_id(merged, higher_precedence.version_), std::move(merged),
                         higher_precedence.version_);
}

auto Configuration::validate_path(std::string_view path) -> Result<void> {
    if (path.empty() || path.front() == '.' || path.back() == '.' ||
        path.find("..") != std::string_view::npos) {
        return Result<void>::failure(Error(
            ErrorCode::create("configuration.invalid_path"), ErrorCategory::InvalidConfiguration,
            "configuration paths must contain non-empty dot-separated segments"));
    }
    const auto valid = std::all_of(path.begin(), path.end(), [](char character) {
        const auto byte = static_cast<unsigned char>(character);
        return std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.';
    });
    if (!valid) {
        return Result<void>::failure(Error(ErrorCode::create("configuration.invalid_path"),
                                           ErrorCategory::InvalidConfiguration,
                                           "configuration path contains an unsupported character"));
    }
    return Result<void>::success();
}

auto Configuration::calculate_id(const Entries &entries, std::string_view version)
    -> ConfigurationId {
    std::string canonical = "version:" + std::string(version) + '\n';
    for (const auto &[path, value] : entries) {
        canonical += path;
        canonical += '=';
        canonical += value_text(value);
        canonical += '\n';
    }
    return ConfigurationId::from_stable_name(canonical).value();
}

} // namespace evolution::configuration
