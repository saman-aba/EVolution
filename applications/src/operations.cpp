#include "evolution/applications/operations.hpp"

#include <utility>

namespace evolution::applications {

IdempotencyKey::IdempotencyKey(std::string value) : value_(std::move(value)) {}

auto IdempotencyKey::create(std::string value) -> Result<IdempotencyKey> {
    if (value.empty()) {
        return Result<IdempotencyKey>::failure(
            Error(ErrorCode::create("application.invalid_idempotency_key"),
                  ErrorCategory::InvalidArgument, "idempotency key cannot be empty"));
    }
    return Result<IdempotencyKey>::success(IdempotencyKey(std::move(value)));
}

auto IdempotencyKey::value() const noexcept -> const std::string & {
    return value_;
}

OperationDescriptor::OperationDescriptor(OperationDescriptorData data) : data_(std::move(data)) {}

auto OperationDescriptor::create(OperationDescriptorData data) -> Result<OperationDescriptor> {
    if (data.name.empty() || data.version.empty() || data.authorization_action.empty()) {
        return Result<OperationDescriptor>::failure(
            Error(ErrorCode::create("application.invalid_operation_descriptor"),
                  ErrorCategory::InvalidArgument,
                  "operation requires name, version, and authorization action"));
    }
    if (data.kind == OperationKind::Query && data.side_effecting) {
        return Result<OperationDescriptor>::failure(Error(
            ErrorCode::create("application.side_effecting_query"), ErrorCategory::InvalidArgument,
            "query operation cannot declare semantic side effects"));
    }
    if (data.kind == OperationKind::Query && data.idempotent) {
        return Result<OperationDescriptor>::failure(
            Error(ErrorCode::create("application.idempotent_query"), ErrorCategory::InvalidArgument,
                  "query repeatability is not command idempotency"));
    }
    return Result<OperationDescriptor>::success(OperationDescriptor(std::move(data)));
}

auto OperationDescriptor::data() const noexcept -> const OperationDescriptorData & {
    return data_;
}

} // namespace evolution::applications
