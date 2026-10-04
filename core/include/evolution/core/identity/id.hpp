#pragma once

#include "evolution/core/error/result.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <span>
#include <string>
#include <string_view>

namespace evolution::identity {

template <typename Tag> class Id {
  public:
    using Bytes = std::array<std::byte, 16>;

    Id() = delete;

    [[nodiscard]] static auto generate() -> Result<Id> {
        try {
            std::random_device random_device;
            Bytes bytes{};
            for (auto &byte : bytes) {
                byte = static_cast<std::byte>(random_device() & 0xffU);
            }
            bytes[6] =
                static_cast<std::byte>((std::to_integer<unsigned int>(bytes[6]) & 0x0fU) | 0x40U);
            bytes[8] =
                static_cast<std::byte>((std::to_integer<unsigned int>(bytes[8]) & 0x3fU) | 0x80U);
            return from_bytes(bytes);
        } catch (const std::exception &exception) {
            return Result<Id>::failure(Error(ErrorCode::create("identity.generation_failed"),
                                             ErrorCategory::Unavailable, exception.what()));
        }
    }

    [[nodiscard]] static auto parse(std::string_view text) -> Result<Id> {
        if (text.size() != 36 || text[8] != '-' || text[13] != '-' || text[18] != '-' ||
            text[23] != '-') {
            return invalid("identifier must use canonical 8-4-4-4-12 hexadecimal form");
        }

        Bytes bytes{};
        std::size_t byte_index = 0;
        for (std::size_t index = 0; index < text.size();) {
            if (text[index] == '-') {
                ++index;
                continue;
            }
            if (index + 1 >= text.size() || byte_index >= bytes.size()) {
                return invalid("identifier has an invalid length");
            }
            const auto high = hex_value(text[index]);
            const auto low = hex_value(text[index + 1]);
            if (high < 0 || low < 0) {
                return invalid("identifier contains a non-hexadecimal character");
            }
            bytes[byte_index++] = static_cast<std::byte>((high << 4) | low);
            index += 2;
        }
        return from_bytes(bytes);
    }

    [[nodiscard]] static auto from_bytes(Bytes bytes) -> Result<Id> {
        const auto valid = std::any_of(bytes.begin(), bytes.end(),
                                       [](std::byte byte) { return byte != std::byte{0}; });
        if (!valid) {
            return invalid("the all-zero identifier is reserved as invalid");
        }
        return Result<Id>::success(Id(bytes));
    }

    [[nodiscard]] static auto from_stable_name(std::string_view name) -> Result<Id> {
        if (name.empty()) {
            return invalid("stable identity input cannot be empty");
        }

        std::uint64_t first = 1469598103934665603ULL;
        std::uint64_t second = 1099511628211ULL;
        for (const auto character : name) {
            const auto value = static_cast<unsigned char>(character);
            first = (first ^ value) * 1099511628211ULL;
            second = (second ^ value) * 1469598103934665603ULL;
        }

        Bytes bytes{};
        for (std::size_t index = 0; index < 8; ++index) {
            bytes[index] = static_cast<std::byte>((first >> (index * 8U)) & 0xffU);
            bytes[index + 8] = static_cast<std::byte>((second >> (index * 8U)) & 0xffU);
        }
        bytes[6] =
            static_cast<std::byte>((std::to_integer<unsigned int>(bytes[6]) & 0x0fU) | 0x50U);
        bytes[8] =
            static_cast<std::byte>((std::to_integer<unsigned int>(bytes[8]) & 0x3fU) | 0x80U);
        return from_bytes(bytes);
    }

    [[nodiscard]] auto bytes() const noexcept -> const Bytes & {
        return bytes_;
    }

    [[nodiscard]] auto to_string() const -> std::string {
        static constexpr char digits[] = "0123456789abcdef";
        std::string result;
        result.reserve(36);
        for (std::size_t index = 0; index < bytes_.size(); ++index) {
            if (index == 4 || index == 6 || index == 8 || index == 10) {
                result.push_back('-');
            }
            const auto value = std::to_integer<unsigned int>(bytes_[index]);
            result.push_back(digits[(value >> 4U) & 0x0fU]);
            result.push_back(digits[value & 0x0fU]);
        }
        return result;
    }

    friend auto operator==(const Id &, const Id &) -> bool = default;

  private:
    explicit Id(Bytes bytes) : bytes_(bytes) {}

    [[nodiscard]] static auto invalid(std::string message) -> Result<Id> {
        return Result<Id>::failure(Error(ErrorCode::create("identity.invalid"),
                                         ErrorCategory::InvalidArgument, std::move(message)));
    }

    [[nodiscard]] static auto hex_value(char character) noexcept -> int {
        if (character >= '0' && character <= '9') {
            return character - '0';
        }
        if (character >= 'a' && character <= 'f') {
            return character - 'a' + 10;
        }
        if (character >= 'A' && character <= 'F') {
            return character - 'A' + 10;
        }
        return -1;
    }

    Bytes bytes_;
};

} // namespace evolution::identity

namespace std {

template <typename Tag> struct hash<evolution::identity::Id<Tag>> {
    auto operator()(const evolution::identity::Id<Tag> &id) const noexcept -> std::size_t {
        std::size_t result = 1469598103934665603ULL;
        for (const auto byte : id.bytes()) {
            result ^= std::to_integer<unsigned char>(byte);
            result *= 1099511628211ULL;
        }
        return result;
    }
};

} // namespace std
