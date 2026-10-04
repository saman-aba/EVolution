#pragma once

#include "evolution/core/api.hpp"
#include "evolution/core/context/context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/provenance/provenance.hpp"
#include "evolution/core/time/time.hpp"

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace evolution::processing {

struct OrderingMetadata {
    std::string scope;
    std::uint64_t sequence;

    friend auto operator==(const OrderingMetadata &, const OrderingMetadata &) -> bool = default;
};

class EVOLUTION_CORE_API ProcessingMetadata {
  public:
    static auto create(EnvelopeId id, time::TemporalInformation processing_time,
                       std::optional<CorrelationId> correlation_id = std::nullopt,
                       std::optional<EnvelopeId> causation_id = std::nullopt,
                       std::optional<OrderingMetadata> ordering = std::nullopt,
                       std::optional<std::string> partition_key = std::nullopt,
                       std::optional<Context> context = std::nullopt,
                       std::optional<Provenance> provenance = std::nullopt,
                       std::uint32_t delivery_attempt = 1, bool replay = false)
        -> Result<ProcessingMetadata>;

    [[nodiscard]] auto id() const noexcept -> const EnvelopeId &;
    [[nodiscard]] auto processing_time() const noexcept -> const time::TemporalInformation &;
    [[nodiscard]] auto correlation_id() const noexcept -> const std::optional<CorrelationId> &;
    [[nodiscard]] auto causation_id() const noexcept -> const std::optional<EnvelopeId> &;
    [[nodiscard]] auto ordering() const noexcept -> const std::optional<OrderingMetadata> &;
    [[nodiscard]] auto partition_key() const noexcept -> const std::optional<std::string> &;
    [[nodiscard]] auto context() const noexcept -> const std::optional<Context> &;
    [[nodiscard]] auto provenance() const noexcept -> const std::optional<Provenance> &;
    [[nodiscard]] auto delivery_attempt() const noexcept -> std::uint32_t;
    [[nodiscard]] auto is_replay() const noexcept -> bool;

    [[nodiscard]] auto for_retry() const -> ProcessingMetadata;
    [[nodiscard]] auto for_output(EnvelopeId output_id, time::TemporalInformation output_time,
                                  std::optional<Provenance> output_provenance) const
        -> ProcessingMetadata;

    friend auto operator==(const ProcessingMetadata &, const ProcessingMetadata &)
        -> bool = default;

  private:
    ProcessingMetadata(EnvelopeId id, time::TemporalInformation processing_time,
                       std::optional<CorrelationId> correlation_id,
                       std::optional<EnvelopeId> causation_id,
                       std::optional<OrderingMetadata> ordering,
                       std::optional<std::string> partition_key, std::optional<Context> context,
                       std::optional<Provenance> provenance, std::uint32_t delivery_attempt,
                       bool replay);

    EnvelopeId id_;
    time::TemporalInformation processing_time_;
    std::optional<CorrelationId> correlation_id_;
    std::optional<EnvelopeId> causation_id_;
    std::optional<OrderingMetadata> ordering_;
    std::optional<std::string> partition_key_;
    std::optional<Context> context_;
    std::optional<Provenance> provenance_;
    std::uint32_t delivery_attempt_;
    bool replay_;
};

template <typename Payload>
    requires std::movable<Payload>
class Envelope {
  public:
    static auto create(Payload payload, ProcessingMetadata metadata) -> Envelope {
        return Envelope(std::move(payload), std::move(metadata));
    }

    [[nodiscard]] auto payload() const noexcept -> const Payload & {
        return payload_;
    }
    [[nodiscard]] auto metadata() const noexcept -> const ProcessingMetadata & {
        return metadata_;
    }

    friend auto operator==(const Envelope &, const Envelope &) -> bool = default;

  private:
    Envelope(Payload payload, ProcessingMetadata metadata)
        : payload_(std::move(payload)), metadata_(std::move(metadata)) {}

    Payload payload_;
    ProcessingMetadata metadata_;
};

} // namespace evolution::processing

namespace evolution {

using processing::Envelope;
using processing::OrderingMetadata;
using processing::ProcessingMetadata;

} // namespace evolution
