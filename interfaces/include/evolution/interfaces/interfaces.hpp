#pragma once

#include "evolution/applications/operations.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/error.hpp"
#include "evolution/interfaces/api.hpp"
#include "evolution/security/security.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace evolution::interfaces {

struct InterfaceRequestMetadataData {
    std::string api_version;
    std::string external_request_id;
    std::string external_correlation_id;
    std::size_t encoded_bytes{};
    std::size_t requested_items{1};
};

class EVOLUTION_INTERFACES_API InterfaceRequestMetadata {
  public:
    static auto create(InterfaceRequestMetadataData data) -> Result<InterfaceRequestMetadata>;
    [[nodiscard]] auto data() const noexcept -> const InterfaceRequestMetadataData &;

  private:
    explicit InterfaceRequestMetadata(InterfaceRequestMetadataData data);
    InterfaceRequestMetadataData data_;
};

struct InterfaceError {
    std::string code;
    ErrorCategory category;
    std::string message;
    bool retryable{};
};

template <typename Payload> struct CommandDto {
    InterfaceRequestMetadata metadata;
    security::AuthenticationRequest authentication;
    Payload payload;
    std::optional<std::string> idempotency_key;
};

template <typename Payload> struct QueryDto {
    InterfaceRequestMetadata metadata;
    security::AuthenticationRequest authentication;
    Payload payload;
    storage::QueryConsistencyRequest consistency;
};

template <typename Payload> struct InterfaceResponse {
    std::string external_request_id;
    applications::OperationCompletion completion{applications::OperationCompletion::Rejected};
    std::optional<Payload> payload;
    std::optional<InterfaceError> error;
    bool idempotent_replay{};
};

class EVOLUTION_INTERFACES_API InterfaceErrorTranslator {
  public:
    [[nodiscard]] static auto translate(const Error &error) -> InterfaceError;
};

template <typename Dto, typename Model> class DtoMapper {
  public:
    virtual ~DtoMapper() = default;
    [[nodiscard]] virtual auto to_model(const Dto &dto) const -> Result<Model> = 0;
};

template <typename Model, typename Dto> class ResponseMapper {
  public:
    virtual ~ResponseMapper() = default;
    [[nodiscard]] virtual auto to_dto(const Model &model) const -> Result<Dto> = 0;
};

template <typename DtoInput, typename ApplicationInput, typename ApplicationOutput,
          typename DtoOutput>
class CommandAdapter {
  public:
    CommandAdapter(
        const security::Authenticator &authenticator,
        const DtoMapper<DtoInput, ApplicationInput> &input_mapper,
        const ResponseMapper<ApplicationOutput, DtoOutput> &output_mapper,
        const applications::CommandExecutor<ApplicationInput, ApplicationOutput> &executor,
        applications::OperationDescriptor operation, std::string resource)
        : authenticator_(authenticator), input_mapper_(input_mapper), output_mapper_(output_mapper),
          executor_(executor), operation_(std::move(operation)), resource_(std::move(resource)) {}

    [[nodiscard]] auto handle(CommandDto<DtoInput> dto, const ExecutionContext &execution) const
        -> InterfaceResponse<DtoOutput> {
        const auto external_request_id = dto.metadata.data().external_request_id;
        auto security_context = authenticator_.authenticate(dto.authentication);
        if (!security_context) {
            return error(external_request_id, security_context.error());
        }
        auto input = input_mapper_.to_model(dto.payload);
        if (!input) {
            return error(external_request_id, input.error());
        }
        auto request_id = applications::RequestId::from_stable_name(external_request_id);
        auto correlation_id =
            CorrelationId::from_stable_name(dto.metadata.data().external_correlation_id);
        if (!request_id) {
            return error(external_request_id, request_id.error());
        }
        if (!correlation_id) {
            return error(external_request_id, correlation_id.error());
        }
        std::optional<applications::IdempotencyKey> key;
        if (dto.idempotency_key) {
            auto parsed = applications::IdempotencyKey::create(*dto.idempotency_key);
            if (!parsed) {
                return error(external_request_id, parsed.error());
            }
            key = std::move(parsed).value();
        }
        applications::OperationContext context{
            std::move(request_id).value(),       std::move(correlation_id).value(),
            std::move(security_context).value(), execution,
            dto.metadata.data().encoded_bytes,   dto.metadata.data().requested_items};
        auto result = executor_.execute(
            {operation_, resource_, std::move(input).value(), std::move(context), std::move(key)});
        if (!result) {
            return error(external_request_id, result.error());
        }
        InterfaceResponse<DtoOutput> response{external_request_id, result.value()->completion,
                                              std::nullopt, std::nullopt,
                                              result.value()->idempotent_replay};
        if (result.value()->output) {
            auto output = output_mapper_.to_dto(*result.value()->output);
            if (!output) {
                return error(external_request_id, output.error());
            }
            response.payload = std::move(output).value();
        }
        return response;
    }

  private:
    static auto error(std::string request_id, const Error &source) -> InterfaceResponse<DtoOutput> {
        return {std::move(request_id), applications::OperationCompletion::Rejected, std::nullopt,
                InterfaceErrorTranslator::translate(source), false};
    }

    const security::Authenticator &authenticator_;
    const DtoMapper<DtoInput, ApplicationInput> &input_mapper_;
    const ResponseMapper<ApplicationOutput, DtoOutput> &output_mapper_;
    const applications::CommandExecutor<ApplicationInput, ApplicationOutput> &executor_;
    applications::OperationDescriptor operation_;
    std::string resource_;
};

template <typename DtoInput, typename ApplicationInput, typename ApplicationOutput,
          typename DtoOutput>
class QueryAdapter {
  public:
    QueryAdapter(const security::Authenticator &authenticator,
                 const DtoMapper<DtoInput, ApplicationInput> &input_mapper,
                 const ResponseMapper<ApplicationOutput, DtoOutput> &output_mapper,
                 const applications::QueryExecutor<ApplicationInput, ApplicationOutput> &executor,
                 applications::OperationDescriptor operation, std::string resource)
        : authenticator_(authenticator), input_mapper_(input_mapper), output_mapper_(output_mapper),
          executor_(executor), operation_(std::move(operation)), resource_(std::move(resource)) {}

    [[nodiscard]] auto handle(QueryDto<DtoInput> dto, const ExecutionContext &execution) const
        -> InterfaceResponse<DtoOutput> {
        const auto external_request_id = dto.metadata.data().external_request_id;
        auto security_context = authenticator_.authenticate(dto.authentication);
        if (!security_context) {
            return error(external_request_id, security_context.error());
        }
        auto input = input_mapper_.to_model(dto.payload);
        if (!input) {
            return error(external_request_id, input.error());
        }
        auto request_id = applications::RequestId::from_stable_name(external_request_id);
        auto correlation_id =
            CorrelationId::from_stable_name(dto.metadata.data().external_correlation_id);
        if (!request_id) {
            return error(external_request_id, request_id.error());
        }
        if (!correlation_id) {
            return error(external_request_id, correlation_id.error());
        }
        applications::OperationContext context{
            std::move(request_id).value(),       std::move(correlation_id).value(),
            std::move(security_context).value(), execution,
            dto.metadata.data().encoded_bytes,   dto.metadata.data().requested_items};
        auto result = executor_.execute({operation_, resource_, std::move(input).value(),
                                         std::move(context), std::move(dto.consistency)});
        if (!result) {
            return error(external_request_id, result.error());
        }
        InterfaceResponse<DtoOutput> response{external_request_id, result.value().completion,
                                              std::nullopt, std::nullopt, false};
        if (result.value().output) {
            auto output = output_mapper_.to_dto(*result.value().output);
            if (!output) {
                return error(external_request_id, output.error());
            }
            response.payload = std::move(output).value();
        }
        return response;
    }

  private:
    static auto error(std::string request_id, const Error &source) -> InterfaceResponse<DtoOutput> {
        return {std::move(request_id), applications::OperationCompletion::Rejected, std::nullopt,
                InterfaceErrorTranslator::translate(source), false};
    }

    const security::Authenticator &authenticator_;
    const DtoMapper<DtoInput, ApplicationInput> &input_mapper_;
    const ResponseMapper<ApplicationOutput, DtoOutput> &output_mapper_;
    const applications::QueryExecutor<ApplicationInput, ApplicationOutput> &executor_;
    applications::OperationDescriptor operation_;
    std::string resource_;
};

} // namespace evolution::interfaces
