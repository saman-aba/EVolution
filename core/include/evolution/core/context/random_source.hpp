#pragma once

#include "evolution/core/api.hpp"

#include <cstdint>
#include <mutex>
#include <random>
#include <string_view>

namespace evolution::context {

class EVOLUTION_CORE_API RandomSource {
  public:
    virtual ~RandomSource() = default;

    virtual auto next_u64() -> std::uint64_t = 0;
    [[nodiscard]] virtual auto source_id() const noexcept -> std::string_view = 0;
};

class EVOLUTION_CORE_API DeterministicRandomSource final : public RandomSource {
  public:
    explicit DeterministicRandomSource(std::uint64_t seed);

    auto next_u64() -> std::uint64_t override;
    [[nodiscard]] auto source_id() const noexcept -> std::string_view override;

  private:
    std::mutex mutex_;
    std::mt19937_64 engine_;
};

class EVOLUTION_CORE_API SystemRandomSource final : public RandomSource {
  public:
    SystemRandomSource();

    auto next_u64() -> std::uint64_t override;
    [[nodiscard]] auto source_id() const noexcept -> std::string_view override;

  private:
    std::mutex mutex_;
    std::mt19937_64 engine_;
};

} // namespace evolution::context
