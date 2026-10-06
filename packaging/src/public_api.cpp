#include "evolution/packaging/public_api.hpp"

#include <utility>

namespace evolution::packaging {

PublicApiContract::PublicApiContract(PublicApiContractData data) : data_(std::move(data)) {}

auto PublicApiContract::create(PublicApiContractData data) -> Result<PublicApiContract> {
    if (data.name.empty() || data.version.empty()) {
        return Result<PublicApiContract>::failure(
            Error(ErrorCode::create("packaging.invalid_public_api"), ErrorCategory::InvalidArgument,
                  "public API contract requires name and version"));
    }
    if (data.stability == ApiStabilityLevel::Internal &&
        data.symbols == SymbolVisibility::Exported) {
        return Result<PublicApiContract>::failure(
            Error(ErrorCode::create("packaging.internal_api_exported"),
                  ErrorCategory::InvalidConfiguration,
                  "internal API contracts must not declare exported symbols"));
    }
    if (data.cxx_abi == CxxAbiPolicy::ExplicitStableBoundary &&
        data.representation != RepresentationPolicy::Hidden) {
        return Result<PublicApiContract>::failure(
            Error(ErrorCode::create("packaging.stable_abi_exposes_representation"),
                  ErrorCategory::InvalidConfiguration,
                  "an explicitly stable binary boundary must hide C++ representation"));
    }
    if (data.exposes_borrowed_views && data.borrowed_view_lifetime.empty()) {
        return Result<PublicApiContract>::failure(
            Error(ErrorCode::create("packaging.borrowed_lifetime_missing"),
                  ErrorCategory::InvalidConfiguration,
                  "borrowed views require an explicit lifetime contract"));
    }
    if (data.asynchronous == AsyncPolicy::ExplicitAsyncContract &&
        data.asynchronous_lifetime.empty()) {
        return Result<PublicApiContract>::failure(
            Error(ErrorCode::create("packaging.async_lifetime_missing"),
                  ErrorCategory::InvalidConfiguration,
                  "asynchronous APIs require explicit lifetime and completion semantics"));
    }
    return Result<PublicApiContract>::success(PublicApiContract(std::move(data)));
}

auto PublicApiContract::data() const noexcept -> const PublicApiContractData & {
    return data_;
}

} // namespace evolution::packaging
