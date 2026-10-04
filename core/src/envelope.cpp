#include "evolution/core/processing/envelope.hpp"

#include <limits>
#include <utility>

namespace evolution::processing {

ProcessingMetadata::ProcessingMetadata(EnvelopeId id, time::TemporalInformation processing_time,
                                       std::optional<CorrelationId> correlation_id,
                                       std::optional<EnvelopeId> causation_id,
                                       std::optional<OrderingMetadata> ordering,
                                       std::optional<std::string> partition_key,
                                       std::optional<Context> context,
                                       std::optional<Provenance> provenance,
                                       std::uint32_t delivery_attempt, bool replay)
    : id_(std::move(id)), processing_time_(std::move(processing_time)),
      correlation_id_(std::move(correlation_id)), causation_id_(std::move(causation_id)),
      ordering_(std::move(ordering)), partition_key_(std::move(partition_key)),
      context_(std::move(context)), provenance_(std::move(provenance)),
      delivery_attempt_(delivery_attempt), replay_(replay) {}

auto ProcessingMetadata::create(
    EnvelopeId id, time::TemporalInformation processing_time,
    std::optional<CorrelationId> correlation_id, std::optional<EnvelopeId> causation_id,
    std::optional<OrderingMetadata> ordering, std::optional<std::string> partition_key,
    std::optional<Context> context, std::optional<Provenance> provenance,
    std::uint32_t delivery_attempt, bool replay) -> Result<ProcessingMetadata> {
    if (delivery_attempt == 0) {
        return Result<ProcessingMetadata>::failure(
            Error(ErrorCode::create("processing.invalid_delivery_attempt"),
                  ErrorCategory::InvalidArgument, "delivery attempt numbering starts at one"));
    }
    if (ordering && ordering->scope.empty()) {
        return Result<ProcessingMetadata>::failure(
            Error(ErrorCode::create("processing.invalid_ordering"), ErrorCategory::InvalidArgument,
                  "ordering metadata requires a non-empty scope"));
    }
    if (partition_key && partition_key->empty()) {
        return Result<ProcessingMetadata>::failure(
            Error(ErrorCode::create("processing.invalid_partition"), ErrorCategory::InvalidArgument,
                  "partition key cannot be empty when present"));
    }
    return Result<ProcessingMetadata>::success(
        ProcessingMetadata(std::move(id), std::move(processing_time), std::move(correlation_id),
                           std::move(causation_id), std::move(ordering), std::move(partition_key),
                           std::move(context), std::move(provenance), delivery_attempt, replay));
}

auto ProcessingMetadata::id() const noexcept -> const EnvelopeId & {
    return id_;
}

auto ProcessingMetadata::processing_time() const noexcept -> const time::TemporalInformation & {
    return processing_time_;
}

auto ProcessingMetadata::correlation_id() const noexcept -> const std::optional<CorrelationId> & {
    return correlation_id_;
}

auto ProcessingMetadata::causation_id() const noexcept -> const std::optional<EnvelopeId> & {
    return causation_id_;
}

auto ProcessingMetadata::ordering() const noexcept -> const std::optional<OrderingMetadata> & {
    return ordering_;
}

auto ProcessingMetadata::partition_key() const noexcept -> const std::optional<std::string> & {
    return partition_key_;
}

auto ProcessingMetadata::context() const noexcept -> const std::optional<Context> & {
    return context_;
}

auto ProcessingMetadata::provenance() const noexcept -> const std::optional<Provenance> & {
    return provenance_;
}

auto ProcessingMetadata::delivery_attempt() const noexcept -> std::uint32_t {
    return delivery_attempt_;
}

auto ProcessingMetadata::is_replay() const noexcept -> bool {
    return replay_;
}

auto ProcessingMetadata::for_retry() const -> ProcessingMetadata {
    const auto next_attempt = delivery_attempt_ == std::numeric_limits<std::uint32_t>::max()
                                  ? delivery_attempt_
                                  : delivery_attempt_ + 1;
    return ProcessingMetadata(id_, processing_time_, correlation_id_, causation_id_, ordering_,
                              partition_key_, context_, provenance_, next_attempt, replay_);
}

auto ProcessingMetadata::for_output(EnvelopeId output_id, time::TemporalInformation output_time,
                                    std::optional<Provenance> output_provenance) const
    -> ProcessingMetadata {
    return ProcessingMetadata(std::move(output_id), std::move(output_time), correlation_id_, id_,
                              std::nullopt, partition_key_, context_, std::move(output_provenance),
                              1, replay_);
}

} // namespace evolution::processing
