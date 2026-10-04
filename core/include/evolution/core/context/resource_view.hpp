#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <utility>

namespace evolution::context {

enum class ResourceKind {
    CpuUnits,
    MemoryBytes,
    ExecutionSlots,
    QueueBytes,
    StorageBytes,
    NetworkBytesPerSecond,
    FileDescriptors,
    ExternalUnits,
};

class ResourceView {
  public:
    using Availability = std::map<ResourceKind, std::uint64_t>;

    ResourceView() = default;
    explicit ResourceView(Availability availability) : availability_(std::move(availability)) {}

    [[nodiscard]] auto available(ResourceKind kind) const noexcept -> std::optional<std::uint64_t> {
        const auto iterator = availability_.find(kind);
        if (iterator == availability_.end()) {
            return std::nullopt;
        }
        return iterator->second;
    }

    [[nodiscard]] auto values() const noexcept -> const Availability & {
        return availability_;
    }

    friend auto operator==(const ResourceView &, const ResourceView &) -> bool = default;

  private:
    Availability availability_;
};

} // namespace evolution::context
