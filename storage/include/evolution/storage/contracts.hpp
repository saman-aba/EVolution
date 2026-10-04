#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/core/event/event.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/measurement/measurement.hpp"
#include "evolution/core/state/state.hpp"
#include "evolution/storage/api.hpp"

#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace evolution::storage {

struct StorageTransactionTag;
struct ReadModelTag;
struct QueryTag;
struct MaterializationTag;

using StorageTransactionId = identity::Id<StorageTransactionTag>;
using ReadModelId = identity::Id<ReadModelTag>;
using QueryId = identity::Id<QueryTag>;
using MaterializationId = identity::Id<MaterializationTag>;

enum class PersistenceCapability {
    Append,
    PointRead,
    OrderedScan,
    RangeRead,
    BatchWrite,
    AtomicBatch,
    Transactions,
    StreamingRead,
    SnapshotRead,
    VersionedRead,
    Indexing,
    Retention,
};

enum class DurabilityGuarantee {
    Volatile,
    ProcessLifetime,
    Durable,
};

enum class VisibilityGuarantee {
    Immediate,
    Eventual,
};

enum class AtomicityGuarantee {
    None,
    SingleRecord,
    Batch,
    Transactional,
};

class EVOLUTION_STORAGE_API PersistenceProfile {
  public:
    static auto create(std::set<PersistenceCapability> capabilities, DurabilityGuarantee durability,
                       VisibilityGuarantee visibility, AtomicityGuarantee atomicity)
        -> Result<PersistenceProfile>;

    [[nodiscard]] auto supports(PersistenceCapability capability) const noexcept -> bool;
    [[nodiscard]] auto capabilities() const noexcept -> const std::set<PersistenceCapability> &;
    [[nodiscard]] auto durability() const noexcept -> DurabilityGuarantee;
    [[nodiscard]] auto visibility() const noexcept -> VisibilityGuarantee;
    [[nodiscard]] auto atomicity() const noexcept -> AtomicityGuarantee;

  private:
    PersistenceProfile(std::set<PersistenceCapability> capabilities, DurabilityGuarantee durability,
                       VisibilityGuarantee visibility, AtomicityGuarantee atomicity);

    std::set<PersistenceCapability> capabilities_;
    DurabilityGuarantee durability_;
    VisibilityGuarantee visibility_;
    AtomicityGuarantee atomicity_;
};

enum class RetentionKind {
    Forever,
    CountBounded,
    TimeBounded,
    ExternalPolicy,
};

class EVOLUTION_STORAGE_API RetentionPolicy {
  public:
    static auto forever() -> RetentionPolicy;
    static auto count_bounded(std::size_t maximum_records) -> Result<RetentionPolicy>;
    static auto time_bounded(std::chrono::seconds duration) -> Result<RetentionPolicy>;
    static auto external(std::string policy_id) -> Result<RetentionPolicy>;

    [[nodiscard]] auto kind() const noexcept -> RetentionKind;
    [[nodiscard]] auto maximum_records() const noexcept -> std::optional<std::size_t>;
    [[nodiscard]] auto duration() const noexcept -> std::optional<std::chrono::seconds>;
    [[nodiscard]] auto external_policy_id() const noexcept -> const std::optional<std::string> &;

  private:
    RetentionPolicy(RetentionKind kind, std::optional<std::size_t> maximum_records,
                    std::optional<std::chrono::seconds> duration,
                    std::optional<std::string> external_policy_id);

    RetentionKind kind_;
    std::optional<std::size_t> maximum_records_;
    std::optional<std::chrono::seconds> duration_;
    std::optional<std::string> external_policy_id_;
};

struct SerializationContract {
    std::string schema;
    std::string schema_version;
    std::string representation;
    bool canonical{};

    friend auto operator==(const SerializationContract &, const SerializationContract &)
        -> bool = default;
};

struct IndexContract {
    std::string name;
    std::vector<std::string> fields;
    bool unique{};

    friend auto operator==(const IndexContract &, const IndexContract &) -> bool = default;
};

class EVOLUTION_STORAGE_API StorageLocation {
  public:
    static auto create(std::string value) -> Result<StorageLocation>;
    [[nodiscard]] auto value() const noexcept -> const std::string &;

  private:
    explicit StorageLocation(std::string value);
    std::string value_;
};

struct PersistenceMetadata {
    std::string logical_identity;
    SerializationContract serialization;
    RetentionPolicy retention;
    std::vector<IndexContract> indexes;
    std::optional<ProvenanceId> provenance_id;
};

enum class WriteOutcome {
    Stored,
    IdempotentExisting,
    Rejected,
};

struct WriteReceipt {
    std::string logical_identity;
    WriteOutcome outcome;
    DurabilityGuarantee durability;
    VisibilityGuarantee visibility;
    std::optional<StorageLocation> physical_location;
};

enum class BatchCompletion {
    Complete,
    Partial,
};

struct BatchWriteReceipt {
    BatchCompletion completion;
    std::vector<WriteReceipt> writes;
    std::vector<Error> failures;
};

class EVOLUTION_STORAGE_API CapabilityProvider {
  public:
    virtual ~CapabilityProvider() = default;
    [[nodiscard]] virtual auto profile() const noexcept -> const PersistenceProfile & = 0;
};

template <typename Record> class PersistentAppender {
  public:
    virtual ~PersistentAppender() = default;
    virtual auto append(Record record, PersistenceMetadata metadata) -> Result<WriteReceipt> = 0;
};

template <typename Record> class PersistentPointReader {
  public:
    virtual ~PersistentPointReader() = default;
    virtual auto read(std::string_view logical_identity) const -> Result<std::optional<Record>> = 0;
};

template <typename Record> class PersistentBatchWriter {
  public:
    virtual ~PersistentBatchWriter() = default;
    virtual auto append_batch(std::vector<std::pair<Record, PersistenceMetadata>> records)
        -> Result<BatchWriteReceipt> = 0;
};

template <typename Payload> using EventPersistenceWriter = PersistentAppender<Event<Payload>>;
template <typename Payload> using EventPersistenceReader = PersistentPointReader<Event<Payload>>;
template <typename StateValue> using StateSnapshotWriter = PersistentAppender<State<StateValue>>;
template <typename StateValue> using StateSnapshotReader = PersistentPointReader<State<StateValue>>;
template <typename MeasurementValue>
using MeasurementWriter = PersistentAppender<Measurement<MeasurementValue>>;
template <typename MeasurementValue>
using MeasurementReader = PersistentPointReader<Measurement<MeasurementValue>>;
template <typename Derived> using DerivedDataWriter = PersistentAppender<Derived>;
template <typename Derived> using DerivedDataReader = PersistentPointReader<Derived>;
template <typename CheckpointValue> using CheckpointWriter = PersistentAppender<CheckpointValue>;
template <typename CheckpointValue> using CheckpointReader = PersistentPointReader<CheckpointValue>;

} // namespace evolution::storage
