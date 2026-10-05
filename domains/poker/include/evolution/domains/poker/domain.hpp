#pragma once

#include "evolution/domain/contracts.hpp"
#include "evolution/domains/poker/api.hpp"
#include "evolution/extensions/contracts.hpp"

#include <memory>

namespace evolution::domains::poker {

EVOLUTION_DOMAIN_POKER_API auto descriptor() -> Result<domain::DomainDescriptor>;
EVOLUTION_DOMAIN_POKER_API auto extension_descriptor() -> Result<extensions::ExtensionDescriptor>;
EVOLUTION_DOMAIN_POKER_API auto extension_factory()
    -> Result<std::shared_ptr<const extensions::ExtensionFactory>>;

} // namespace evolution::domains::poker
