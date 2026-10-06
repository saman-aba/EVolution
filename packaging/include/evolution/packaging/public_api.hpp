#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/packaging/api.hpp"

#include <string>

namespace evolution::packaging {

enum class ApiStabilityLevel { Internal, Experimental, Public, Stable, Deprecated };
enum class SymbolVisibility { Hidden, Exported };
enum class RepresentationPolicy { ValueExposed, Hidden };
enum class OwnershipPolicy { Value, Unique, Shared, Borrowed };
enum class ExceptionPolicy { ResultForOperationalFailure, DocumentedExceptions };
enum class AsyncPolicy { Synchronous, ExplicitAsyncContract };
enum class TemplatePolicy { None, DeliberateTypeSafeAbstraction };
enum class CxxAbiPolicy { CompatibleToolchainRequired, ExplicitStableBoundary };

struct PublicApiContractData {
    std::string name;
    std::string version;
    ApiStabilityLevel stability{ApiStabilityLevel::Experimental};
    SymbolVisibility symbols{SymbolVisibility::Exported};
    RepresentationPolicy representation{RepresentationPolicy::Hidden};
    OwnershipPolicy ownership{OwnershipPolicy::Value};
    ExceptionPolicy exceptions{ExceptionPolicy::ResultForOperationalFailure};
    AsyncPolicy asynchronous{AsyncPolicy::Synchronous};
    TemplatePolicy templates{TemplatePolicy::None};
    CxxAbiPolicy cxx_abi{CxxAbiPolicy::CompatibleToolchainRequired};
    bool exposes_borrowed_views{};
    std::string borrowed_view_lifetime;
    std::string asynchronous_lifetime;
};

class EVOLUTION_PACKAGING_API PublicApiContract {
  public:
    static auto create(PublicApiContractData data) -> Result<PublicApiContract>;
    [[nodiscard]] auto data() const noexcept -> const PublicApiContractData &;

  private:
    explicit PublicApiContract(PublicApiContractData data);
    PublicApiContractData data_;
};

} // namespace evolution::packaging
