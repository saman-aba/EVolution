#pragma once

#include "evolution/applications/api.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/security/security.hpp"
#include "evolution/storage/query.hpp"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace evolution::applications {

struct OperationTag;
struct RequestTag;
using OperationId = identity::Id<OperationTag>;
using RequestId = identity::Id<RequestTag>;

class EVOLUTION_APPLICATIONS_API IdempotencyKey {
  public:
    static auto create(std::string value) -> Result<IdempotencyKey>;
    [[nodiscard]] auto value() const noexcept -> const std::string &;

  private:
    explicit IdempotencyKey(std::string value);
    std::string value_;
};

enum class OperationKind { Command, Query };
enum class OperationCompletion { Accepted, Completed, Partial, NoResult, Rejected, Cancelled };

struct OperationDescriptorData {
    std::string name;
    std::string version;
    OperationKind kind;
    std::string authorization_action;
    bool idempotent{};
    bool side_effecting{};
};

class EVOLUTION_APPLICATIONS_API OperationDescriptor {
  public:
    static auto create(OperationDescriptorData data) -> Result<OperationDescriptor>;
    [[nodiscard]] auto data() const noexcept -> const OperationDescriptorData &;

  private:
    explicit OperationDescriptor(OperationDescriptorData data);
    OperationDescriptorData data_;
};

struct OperationContext {
    RequestId request_id;
    CorrelationId correlation_id;
    security::SecurityContext security;
    ExecutionContext execution;
    std::size_t request_bytes{};
    std::size_t requested_items{1};
};

template <typename Input> struct CommandRequest {
    OperationDescriptor operation;
    std::string resource;
    Input input;
    OperationContext context;
    std::optional<IdempotencyKey> idempotency_key;
};

template <typename Input> struct QueryRequest {
    OperationDescriptor operation;
    std::string resource;
    Input input;
    OperationContext context;
    storage::QueryConsistencyRequest consistency;
};

struct SideEffectReference {
    std::string kind;
    std::string identity;
    std::string delivery_semantics;
};

template <typename Output> struct CommandExecution {
    OperationCompletion completion{OperationCompletion::Completed};
    std::optional<Output> output;
    std::vector<SideEffectReference> side_effects;
};

template <typename Output> struct CommandResult {
    OperationId operation_id;
    RequestId request_id;
    OperationCompletion completion;
    std::optional<Output> output;
    std::vector<SideEffectReference> side_effects;
    bool idempotent_replay{};
};

template <typename Output> struct QueryExecution {
    OperationCompletion completion{OperationCompletion::Completed};
    std::optional<Output> output;
    std::string consistency_observed;
};

template <typename Output> struct QueryResult {
    OperationId operation_id;
    RequestId request_id;
    OperationCompletion completion;
    std::optional<Output> output;
    std::string consistency_observed;
};

template <typename Input, typename Output> class CommandHandler {
  public:
    virtual ~CommandHandler() = default;
    virtual auto execute(const Input &input, const OperationContext &context) const
        -> Result<CommandExecution<Output>> = 0;
};

template <typename Input, typename Output> class QueryHandler {
  public:
    virtual ~QueryHandler() = default;
    [[nodiscard]] virtual auto execute(const Input &input,
                                       const storage::QueryConsistencyRequest &consistency,
                                       const OperationContext &context) const
        -> Result<QueryExecution<Output>> = 0;
};

template <typename Output> class IdempotencyStore {
  public:
    virtual ~IdempotencyStore() = default;
    [[nodiscard]] virtual auto get(std::string_view scope, const IdempotencyKey &key) const
        -> Result<std::shared_ptr<const CommandResult<Output>>> = 0;
    virtual auto put(std::string scope, IdempotencyKey key,
                     std::shared_ptr<const CommandResult<Output>> result) -> Result<void> = 0;
};

template <typename Output> class InMemoryIdempotencyStore final : public IdempotencyStore<Output> {
  public:
    [[nodiscard]] auto get(std::string_view scope, const IdempotencyKey &key) const
        -> Result<std::shared_ptr<const CommandResult<Output>>> override {
        std::lock_guard lock(mutex_);
        const auto iterator = results_.find(std::string(scope) + '\n' + key.value());
        return Result<std::shared_ptr<const CommandResult<Output>>>::success(
            iterator == results_.end() ? nullptr : iterator->second);
    }

    auto put(std::string scope, IdempotencyKey key,
             std::shared_ptr<const CommandResult<Output>> result) -> Result<void> override {
        if (!result) {
            return Result<void>::failure(
                Error(ErrorCode::create("application.null_idempotency_result"),
                      ErrorCategory::InvalidArgument, "idempotency result cannot be null"));
        }
        std::lock_guard lock(mutex_);
        results_.insert_or_assign(std::move(scope) + '\n' + key.value(), std::move(result));
        return Result<void>::success();
    }

  private:
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<const CommandResult<Output>>> results_;
};

template <typename Input, typename Output> class CommandExecutor {
  public:
    CommandExecutor(const security::Authorizer &authorizer, security::AuditSink &audit,
                    security::ResourceProtector resource_protector,
                    const CommandHandler<Input, Output> &handler,
                    IdempotencyStore<Output> &idempotency_store)
        : authorizer_(authorizer), audit_(audit),
          resource_protector_(std::move(resource_protector)), handler_(handler),
          idempotency_store_(idempotency_store) {}

    auto execute(CommandRequest<Input> request) const
        -> Result<std::shared_ptr<const CommandResult<Output>>> {
        if (request.operation.data().kind != OperationKind::Command) {
            return failure("application.operation_kind_mismatch", ErrorCategory::InvalidArgument,
                           "command executor requires a command descriptor");
        }
        if (request.context.execution.is_cancelled()) {
            return failure("application.operation_cancelled", ErrorCategory::Cancelled,
                           "command was cancelled before execution");
        }
        if (request.context.execution.deadline_exceeded(time::MonotonicClock::now())) {
            return failure("application.operation_deadline_exceeded",
                           ErrorCategory::DeadlineExceeded,
                           "command deadline was exceeded before execution");
        }
        auto resource = resource_protector_.admit(
            {request.context.request_bytes, request.context.requested_items});
        if (!resource) {
            return Result<std::shared_ptr<const CommandResult<Output>>>::failure(resource.error());
        }
        auto authorization = authorizer_.authorize(request.context.security,
                                                   {request.operation.data().name, request.resource,
                                                    request.operation.data().authorization_action,
                                                    request.context.security.principal.tenant});
        if (!authorization) {
            return Result<std::shared_ptr<const CommandResult<Output>>>::failure(
                authorization.error());
        }
        if (authorization.value().outcome != security::AuthorizationOutcome::Allowed) {
            static_cast<void>(record_audit(request, security::AuditOutcome::Denied,
                                           authorization.value().reason_code));
            return failure("application.operation_unauthorized", ErrorCategory::Unauthorized,
                           "command authorization was denied");
        }
        const auto idempotency_scope = request.operation.data().name + '|' + request.resource +
                                       '|' + request.context.security.principal.id.to_string();
        if (request.operation.data().idempotent) {
            if (!request.idempotency_key) {
                return failure("application.missing_idempotency_key",
                               ErrorCategory::InvalidArgument,
                               "idempotent command requires an idempotency key");
            }
            auto existing = idempotency_store_.get(idempotency_scope, *request.idempotency_key);
            if (!existing) {
                return Result<std::shared_ptr<const CommandResult<Output>>>::failure(
                    existing.error());
            }
            if (existing.value()) {
                auto replay = std::make_shared<CommandResult<Output>>(*existing.value());
                replay->idempotent_replay = true;
                return Result<std::shared_ptr<const CommandResult<Output>>>::success(
                    std::move(replay));
            }
        }
        auto executed = handler_.execute(request.input, request.context);
        if (!executed) {
            static_cast<void>(
                record_audit(request, security::AuditOutcome::Failed, "handler_failed"));
            return Result<std::shared_ptr<const CommandResult<Output>>>::failure(executed.error());
        }
        auto operation_id = OperationId::generate();
        if (!operation_id) {
            return Result<std::shared_ptr<const CommandResult<Output>>>::failure(
                operation_id.error());
        }
        auto result = std::make_shared<CommandResult<Output>>(
            CommandResult<Output>{std::move(operation_id).value(), request.context.request_id,
                                  executed.value().completion, std::move(executed.value().output),
                                  std::move(executed.value().side_effects), false});
        if (request.operation.data().idempotent) {
            auto stored =
                idempotency_store_.put(idempotency_scope, *request.idempotency_key, result);
            if (!stored) {
                return Result<std::shared_ptr<const CommandResult<Output>>>::failure(
                    stored.error());
            }
        }
        auto audited = record_audit(request, security::AuditOutcome::Succeeded, "completed");
        if (!audited) {
            return Result<std::shared_ptr<const CommandResult<Output>>>::failure(audited.error());
        }
        return Result<std::shared_ptr<const CommandResult<Output>>>::success(std::move(result));
    }

  private:
    auto record_audit(const CommandRequest<Input> &request, security::AuditOutcome outcome,
                      std::string reason) const -> Result<void> {
        auto id = security::AuditEventId::generate();
        if (!id) {
            return Result<void>::failure(id.error());
        }
        auto event = security::AuditEvent::create({std::move(id).value(),
                                                   request.context.security.principal.id,
                                                   request.operation.data().name,
                                                   request.resource,
                                                   outcome,
                                                   std::move(reason),
                                                   evolution::time::TemporalInformation::unknown(),
                                                   request.context.correlation_id,
                                                   {}});
        if (!event) {
            return Result<void>::failure(event.error());
        }
        return audit_.record(std::move(event).value());
    }

    static auto failure(std::string code, ErrorCategory category, std::string message)
        -> Result<std::shared_ptr<const CommandResult<Output>>> {
        return Result<std::shared_ptr<const CommandResult<Output>>>::failure(
            Error(ErrorCode::create(std::move(code)), category, std::move(message)));
    }

    const security::Authorizer &authorizer_;
    security::AuditSink &audit_;
    security::ResourceProtector resource_protector_;
    const CommandHandler<Input, Output> &handler_;
    IdempotencyStore<Output> &idempotency_store_;
};

template <typename Input, typename Output> class QueryExecutor {
  public:
    QueryExecutor(const security::Authorizer &authorizer,
                  security::ResourceProtector resource_protector,
                  const QueryHandler<Input, Output> &handler)
        : authorizer_(authorizer), resource_protector_(std::move(resource_protector)),
          handler_(handler) {}

    [[nodiscard]] auto execute(QueryRequest<Input> request) const -> Result<QueryResult<Output>> {
        if (request.operation.data().kind != OperationKind::Query ||
            request.operation.data().side_effecting) {
            return failure("application.invalid_query_operation", ErrorCategory::InvalidArgument,
                           "query descriptor must be non-side-effecting");
        }
        if (request.context.execution.is_cancelled()) {
            return failure("application.operation_cancelled", ErrorCategory::Cancelled,
                           "query was cancelled before execution");
        }
        if (request.context.execution.deadline_exceeded(time::MonotonicClock::now())) {
            return failure("application.operation_deadline_exceeded",
                           ErrorCategory::DeadlineExceeded,
                           "query deadline was exceeded before execution");
        }
        auto resource = resource_protector_.admit(
            {request.context.request_bytes, request.context.requested_items});
        if (!resource) {
            return Result<QueryResult<Output>>::failure(resource.error());
        }
        auto authorization = authorizer_.authorize(request.context.security,
                                                   {request.operation.data().name, request.resource,
                                                    request.operation.data().authorization_action,
                                                    request.context.security.principal.tenant});
        if (!authorization) {
            return Result<QueryResult<Output>>::failure(authorization.error());
        }
        if (authorization.value().outcome != security::AuthorizationOutcome::Allowed) {
            return failure("application.operation_unauthorized", ErrorCategory::Unauthorized,
                           "query authorization was denied");
        }
        auto executed = handler_.execute(request.input, request.consistency, request.context);
        if (!executed) {
            return Result<QueryResult<Output>>::failure(executed.error());
        }
        auto operation_id = OperationId::generate();
        if (!operation_id) {
            return Result<QueryResult<Output>>::failure(operation_id.error());
        }
        return Result<QueryResult<Output>>::success(
            {std::move(operation_id).value(), request.context.request_id,
             executed.value().completion, std::move(executed.value().output),
             std::move(executed.value().consistency_observed)});
    }

  private:
    static auto failure(std::string code, ErrorCategory category, std::string message)
        -> Result<QueryResult<Output>> {
        return Result<QueryResult<Output>>::failure(
            Error(ErrorCode::create(std::move(code)), category, std::move(message)));
    }

    const security::Authorizer &authorizer_;
    security::ResourceProtector resource_protector_;
    const QueryHandler<Input, Output> &handler_;
};

} // namespace evolution::applications
