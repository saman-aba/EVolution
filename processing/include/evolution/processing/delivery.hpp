#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/processing/api.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

namespace evolution::processing {

enum class DeliveryGuarantee {
    AtMostOnce,
    AtLeastOnce,
    ExactlyOnce,
};

enum class AcknowledgementPoint {
    Admission,
    Execution,
    DurableCompletion,
};

enum class IdempotencyMode {
    None,
    Natural,
    Keyed,
};

enum class ExactlyOnceMechanism {
    None,
    Transactional,
    IdempotentEffectWithDurableDeduplication,
};

class EVOLUTION_PROCESSING_API DeliveryContract {
  public:
    static auto create(DeliveryGuarantee guarantee, AcknowledgementPoint acknowledgement,
                       IdempotencyMode idempotency, std::uint32_t maximum_attempts,
                       ExactlyOnceMechanism exactly_once_mechanism = ExactlyOnceMechanism::None)
        -> Result<DeliveryContract>;

    [[nodiscard]] auto guarantee() const noexcept -> DeliveryGuarantee;
    [[nodiscard]] auto acknowledgement() const noexcept -> AcknowledgementPoint;
    [[nodiscard]] auto idempotency() const noexcept -> IdempotencyMode;
    [[nodiscard]] auto maximum_attempts() const noexcept -> std::uint32_t;
    [[nodiscard]] auto exactly_once_mechanism() const noexcept -> ExactlyOnceMechanism;

  private:
    DeliveryContract(DeliveryGuarantee guarantee, AcknowledgementPoint acknowledgement,
                     IdempotencyMode idempotency, std::uint32_t maximum_attempts,
                     ExactlyOnceMechanism exactly_once_mechanism);

    DeliveryGuarantee guarantee_;
    AcknowledgementPoint acknowledgement_;
    IdempotencyMode idempotency_;
    std::uint32_t maximum_attempts_;
    ExactlyOnceMechanism exactly_once_mechanism_;
};

class EVOLUTION_PROCESSING_API IdempotencyKey {
  public:
    static auto create(std::string value) -> Result<IdempotencyKey>;
    [[nodiscard]] auto value() const noexcept -> std::string_view;
    friend auto operator==(const IdempotencyKey &, const IdempotencyKey &) -> bool = default;

  private:
    explicit IdempotencyKey(std::string value);
    std::string value_;
};

class EVOLUTION_PROCESSING_API DeduplicationStore {
  public:
    virtual ~DeduplicationStore() = default;
    [[nodiscard]] virtual auto is_durable() const noexcept -> bool = 0;
    [[nodiscard]] virtual auto contains(const IdempotencyKey &key) const -> Result<bool> = 0;
    virtual auto record_completed(IdempotencyKey key) -> Result<void> = 0;
};

class EVOLUTION_PROCESSING_API InMemoryDeduplicationStore final : public DeduplicationStore {
  public:
    [[nodiscard]] auto is_durable() const noexcept -> bool override;
    [[nodiscard]] auto contains(const IdempotencyKey &key) const -> Result<bool> override;
    auto record_completed(IdempotencyKey key) -> Result<void> override;

  private:
    mutable std::mutex mutex_;
    std::unordered_set<std::string> completed_;
};

enum class DeliveryDecision {
    Execute,
    AlreadyCompleted,
};

class EVOLUTION_PROCESSING_API DeliveryGuard {
  public:
    static auto create(DeliveryContract contract,
                       std::shared_ptr<DeduplicationStore> deduplication_store = nullptr)
        -> Result<DeliveryGuard>;

    [[nodiscard]] auto begin(const std::optional<IdempotencyKey> &key) const
        -> Result<DeliveryDecision>;
    auto complete(const std::optional<IdempotencyKey> &key) const -> Result<void>;

  private:
    DeliveryGuard(DeliveryContract contract,
                  std::shared_ptr<DeduplicationStore> deduplication_store);

    DeliveryContract contract_;
    std::shared_ptr<DeduplicationStore> deduplication_store_;
};

} // namespace evolution::processing
