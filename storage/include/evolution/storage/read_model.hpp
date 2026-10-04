#pragma once

#include "evolution/core/configuration/configuration.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"
#include "evolution/storage/api.hpp"
#include "evolution/storage/contracts.hpp"
#include "evolution/storage/query.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace evolution::storage {

enum class ReadModelAuthority {
    Authoritative,
    Derived,
};

enum class FreshnessStatus {
    Current,
    Stale,
    Rebuilding,
    Unknown,
};

struct ReadModelFreshness {
    FreshnessStatus status{FreshnessStatus::Unknown};
    time::TemporalInformation as_of{time::TemporalInformation::unknown()};
    std::optional<std::string> source_position;
    std::optional<std::chrono::seconds> lag;
};

struct ReadModelDescriptorData {
    ReadModelId id;
    std::string name;
    std::string version;
    ReadModelAuthority authority;
    std::optional<ComponentId> projection_component;
    std::optional<std::string> projection_version;
    std::optional<ConfigurationId> configuration_id;
    std::optional<ProvenanceId> provenance_id;
    std::vector<std::string> authoritative_sources;
    ReadModelFreshness freshness;
    bool rebuildable{};
    bool cache{};
};

class EVOLUTION_STORAGE_API ReadModelDescriptor {
  public:
    static auto create(ReadModelDescriptorData data) -> Result<ReadModelDescriptor>;
    [[nodiscard]] auto data() const noexcept -> const ReadModelDescriptorData &;

  private:
    explicit ReadModelDescriptor(ReadModelDescriptorData data);
    ReadModelDescriptorData data_;
};

struct ReadModelRebuildRequest {
    std::vector<std::string> source_versions;
    std::string projection_version;
    ConfigurationId configuration_id;
    std::optional<time::TimePoint> historical_cutoff;
};

template <typename Record> class ReadModelRebuilder {
  public:
    virtual ~ReadModelRebuilder() = default;
    virtual auto rebuild(const ReadModelRebuildRequest &request,
                         const QueryExecutionContext &context)
        -> Result<std::vector<QueryRecord<Record>>> = 0;
};

struct MaterializationDescriptor {
    MaterializationId id;
    ReadModelId read_model_id;
    std::string read_model_version;
    std::string query_fingerprint;
    std::optional<ProvenanceId> provenance_id;
    RetentionPolicy retention;
    ReadModelFreshness freshness;
};

template <typename Record> class QueryCache {
  public:
    virtual ~QueryCache() = default;
    virtual auto get(const QueryCacheKey &key) const
        -> Result<std::optional<QueryPage<Record>>> = 0;
    virtual auto put(QueryCacheKey key, QueryPage<Record> page) -> Result<void> = 0;
    virtual void invalidate_read_model(const ReadModelId &read_model_id) = 0;
};

template <typename Record> using ReadModelWriter = PersistentAppender<QueryRecord<Record>>;
template <typename Record> using ReadModelReader = PersistentPointReader<QueryRecord<Record>>;

} // namespace evolution::storage
