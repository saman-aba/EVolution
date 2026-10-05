#pragma once

#include "evolution/extensions/contracts.hpp"

#include <memory>
#include <shared_mutex>
#include <vector>

namespace evolution::extensions {

struct DiscoveryQuery {
    std::optional<ExtensionKind> kind;
    std::set<std::string> required_capabilities;
    std::vector<ContractReference> required_contracts;
};

class EVOLUTION_EXTENSIONS_API ExtensionRegistry {
  public:
    auto register_factory(std::shared_ptr<const ExtensionFactory> factory) -> Result<void>;

    [[nodiscard]] auto discover(const DiscoveryQuery &query = {}) const
        -> std::vector<ExtensionDescriptor>;
    [[nodiscard]] auto factory(const ExtensionId &id, std::string_view version) const
        -> Result<std::shared_ptr<const ExtensionFactory>>;
    [[nodiscard]] auto validate_dependencies() const -> Result<void>;
    [[nodiscard]] auto size() const -> std::size_t;

  private:
    struct Entry {
        ExtensionDescriptor descriptor;
        std::shared_ptr<const ExtensionFactory> factory;
    };

    mutable std::shared_mutex mutex_;
    std::vector<Entry> entries_;
};

} // namespace evolution::extensions
