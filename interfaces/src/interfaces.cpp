#include "evolution/interfaces/interfaces.hpp"

#include <utility>

namespace evolution::interfaces {

InterfaceRequestMetadata::InterfaceRequestMetadata(InterfaceRequestMetadataData data)
    : data_(std::move(data)) {}

auto InterfaceRequestMetadata::create(InterfaceRequestMetadataData data)
    -> Result<InterfaceRequestMetadata> {
    if (data.api_version.empty() || data.external_request_id.empty() ||
        data.external_correlation_id.empty() || data.encoded_bytes == 0 ||
        data.requested_items == 0) {
        return Result<InterfaceRequestMetadata>::failure(Error(
            ErrorCode::create("interface.invalid_request_metadata"), ErrorCategory::InvalidArgument,
            "interface metadata fields and resource counts must be non-empty"));
    }
    return Result<InterfaceRequestMetadata>::success(InterfaceRequestMetadata(std::move(data)));
}

auto InterfaceRequestMetadata::data() const noexcept -> const InterfaceRequestMetadataData & {
    return data_;
}

auto InterfaceErrorTranslator::translate(const Error &error) -> InterfaceError {
    const bool retryable = error.category() == ErrorCategory::Unavailable ||
                           error.category() == ErrorCategory::ResourceExhausted ||
                           error.category() == ErrorCategory::DeadlineExceeded;
    return {std::string(error.code().value()), error.category(), std::string(error.message()),
            retryable};
}

} // namespace evolution::interfaces
