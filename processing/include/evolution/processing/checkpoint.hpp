#pragma once

#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"
#include "evolution/processing/api.hpp"
#include "evolution/processing/graph.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace evolution::processing {

struct CheckpointTag;
using CheckpointId = identity::Id<CheckpointTag>;

enum class CheckpointScope {
    Processor,
    Partition,
    Graph,
    Execution,
    Batch,
};

struct InputPosition {
    std::string source;
    std::optional<std::string> partition;
    std::uint64_t sequence{};
    std::optional<EventId> event_id;
    time::TemporalInformation historical_time{time::TemporalInformation::unknown()};

    friend auto operator==(const InputPosition &, const InputPosition &) -> bool = default;
};

struct CheckpointGraphVersion {
    GraphId id;
    std::string version;
};

struct CheckpointComponentVersion {
    ComponentId id;
    std::optional<ProcessorId> processor_id;
    std::string version;
};

struct CheckpointConfigurationVersion {
    ConfigurationId id;
    std::string version;
};

struct CheckpointAlgorithmVersion {
    AlgorithmId id;
    std::string version;
};

struct CheckpointStateVersion {
    StateId id;
    std::string version;
};

struct CheckpointProvenanceVersion {
    ProvenanceId id;
    std::string version;
};

struct CheckpointIntegrity {
    std::string algorithm;
    std::string digest;
};

struct CheckpointManifestData {
    CheckpointId checkpoint_id;
    CheckpointScope scope;
    CheckpointGraphVersion graph;
    CheckpointComponentVersion component;
    CheckpointConfigurationVersion configuration;
    std::map<std::string, std::string> schema_versions;
    std::optional<CheckpointAlgorithmVersion> algorithm;
    CheckpointStateVersion state;
    std::vector<InputPosition> input_positions;
    CheckpointProvenanceVersion provenance;
    time::TemporalInformation created_at;
    CheckpointIntegrity integrity;
};

class EVOLUTION_PROCESSING_API CheckpointManifest {
  public:
    static auto create(CheckpointManifestData data) -> Result<CheckpointManifest>;

    [[nodiscard]] auto data() const noexcept -> const CheckpointManifestData &;

  private:
    explicit CheckpointManifest(CheckpointManifestData data);
    CheckpointManifestData data_;
};

struct CheckpointCompatibilityRequirements {
    GraphId graph_id;
    std::string graph_version;
    ComponentId component_id;
    std::optional<ProcessorId> processor_id;
    std::string component_version;
    ConfigurationId configuration_id;
    std::string configuration_version;
    std::optional<CheckpointAlgorithmVersion> algorithm;
    std::string state_version;
    std::string provenance_version;
    std::map<std::string, std::string> schema_versions;
};

class EVOLUTION_PROCESSING_API CheckpointCompatibility {
  public:
    static auto validate(const CheckpointManifest &manifest,
                         const CheckpointCompatibilityRequirements &requirements) -> Result<void>;
};

template <typename State> struct StateSnapshot {
    State value;
    std::string state_version;
    std::string integrity_digest;
    bool complete{true};
};

template <typename State> struct Checkpoint {
    CheckpointManifest manifest;
    StateSnapshot<State> snapshot;
};

template <typename State> class SnapshotRestorer {
  public:
    virtual ~SnapshotRestorer() = default;
    virtual auto snapshot() const -> Result<StateSnapshot<State>> = 0;
    virtual auto restore(const StateSnapshot<State> &snapshot,
                         const CancellationToken &cancellation) -> Result<void> = 0;
};

template <typename State>
auto restore_checkpoint(const Checkpoint<State> &checkpoint,
                        const CheckpointCompatibilityRequirements &requirements,
                        SnapshotRestorer<State> &restorer,
                        const CancellationToken &cancellation = {}) -> Result<void> {
    if (cancellation.is_cancelled()) {
        return Result<void>::failure(Error(ErrorCode::create("processing.recovery_cancelled"),
                                           ErrorCategory::Cancelled,
                                           "checkpoint recovery was cancelled"));
    }
    auto compatibility = CheckpointCompatibility::validate(checkpoint.manifest, requirements);
    if (compatibility.has_error()) {
        return compatibility;
    }
    const auto &manifest = checkpoint.manifest.data();
    if (!checkpoint.snapshot.complete ||
        checkpoint.snapshot.state_version != manifest.state.version ||
        checkpoint.snapshot.integrity_digest != manifest.integrity.digest) {
        return Result<void>::failure(
            Error(ErrorCode::create("processing.corrupt_checkpoint"), ErrorCategory::Serialization,
                  "checkpoint state is incomplete or fails integrity validation"));
    }
    auto restored = restorer.restore(checkpoint.snapshot, cancellation);
    if (restored.has_error()) {
        return restored;
    }
    return Result<void>::success();
}

template <typename Input> struct ReplayItem {
    Input input;
    InputPosition position;
};

template <typename Input> class RetainedInputSource {
  public:
    virtual ~RetainedInputSource() = default;
    virtual auto read_after(const std::vector<InputPosition> &positions, std::size_t maximum_items)
        -> Result<std::vector<ReplayItem<Input>>> = 0;
};

template <typename Input> class ReplayTarget {
  public:
    virtual ~ReplayTarget() = default;
    virtual auto apply(const ReplayItem<Input> &item, const ExecutionContext &context)
        -> Result<void> = 0;
};

struct ReplaySummary {
    std::size_t replayed{};
    std::vector<InputPosition> final_positions;
};

template <typename Input>
auto replay_retained_input(RetainedInputSource<Input> &source, ReplayTarget<Input> &target,
                           const std::vector<InputPosition> &positions, std::size_t maximum_items,
                           const ExecutionContext &context) -> Result<ReplaySummary> {
    if (maximum_items == 0) {
        return Result<ReplaySummary>::failure(
            Error(ErrorCode::create("processing.invalid_replay_limit"),
                  ErrorCategory::InvalidArgument, "replay limit must be explicitly bounded"));
    }
    if (context.mode() != ExecutionMode::Replay) {
        return Result<ReplaySummary>::failure(Error(
            ErrorCode::create("processing.invalid_replay_context"), ErrorCategory::InvalidArgument,
            "retained-input replay requires replay execution mode"));
    }
    auto items = source.read_after(positions, maximum_items);
    if (items.has_error()) {
        return Result<ReplaySummary>::failure(items.error());
    }

    if (items.value().size() > maximum_items) {
        return Result<ReplaySummary>::failure(Error(
            ErrorCode::create("processing.replay_limit_exceeded"), ErrorCategory::ResourceExhausted,
            "retained-input source exceeded the bounded replay request"));
    }

    std::map<std::pair<std::string, std::optional<std::string>>, std::uint64_t> last_sequence;
    std::map<std::pair<std::string, std::optional<std::string>>, InputPosition> final_positions;
    for (const auto &position : positions) {
        const auto key = std::make_pair(position.source, position.partition);
        last_sequence[key] = position.sequence;
        final_positions[key] = position;
    }
    std::size_t replayed = 0;
    for (const auto &item : items.value()) {
        if (context.is_cancelled()) {
            return Result<ReplaySummary>::failure(
                Error(ErrorCode::create("processing.replay_cancelled"), ErrorCategory::Cancelled,
                      "retained-input replay was cancelled"));
        }
        const auto key = std::make_pair(item.position.source, item.position.partition);
        const auto previous = last_sequence.find(key);
        if (previous != last_sequence.end() && item.position.sequence <= previous->second) {
            return Result<ReplaySummary>::failure(
                Error(ErrorCode::create("processing.invalid_replay_order"), ErrorCategory::Conflict,
                      "retained input is not ordered after the checkpoint position"));
        }
        auto applied = target.apply(item, context);
        if (applied.has_error()) {
            return Result<ReplaySummary>::failure(applied.error());
        }
        last_sequence[key] = item.position.sequence;
        final_positions[key] = item.position;
        ++replayed;
    }

    std::vector<InputPosition> final;
    final.reserve(final_positions.size());
    for (auto &[key, position] : final_positions) {
        static_cast<void>(key);
        final.push_back(std::move(position));
    }
    return Result<ReplaySummary>::success(ReplaySummary{replayed, std::move(final)});
}

} // namespace evolution::processing
