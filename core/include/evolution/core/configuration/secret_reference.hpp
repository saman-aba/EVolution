#pragma once

#include "evolution/core/error/result.hpp"

#include <string>
#include <string_view>

namespace evolution::configuration {

class SecretReference {
  public:
    static auto create(std::string provider, std::string key) -> Result<SecretReference>;

    [[nodiscard]] auto provider() const noexcept -> std::string_view;
    [[nodiscard]] auto key() const noexcept -> std::string_view;

    friend auto operator==(const SecretReference &, const SecretReference &) -> bool = default;

  private:
    SecretReference(std::string provider, std::string key);

    std::string provider_;
    std::string key_;
};

} // namespace evolution::configuration
