#pragma once

#include "evolution/core/api.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/core/time/time.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace evolution::provenance {

struct VersionReference {
    std::string name;
    std::string version;

    friend auto operator==(const VersionReference &, const VersionReference &) -> bool = default;
};

struct SourceReference {
    SourceId id;
    std::string kind;
    std::string locator;

    friend auto operator==(const SourceReference &, const SourceReference &) -> bool = default;
};

struct ProvenanceInput {
    std::string kind;
    std::string identity;
    std::optional<std::string> version;

    friend auto operator==(const ProvenanceInput &, const ProvenanceInput &) -> bool = default;
};

struct TransformationReference {
    ComponentId component_id;
    std::string component_name;
    std::string component_version;
    std::optional<AlgorithmId> algorithm_id;
    std::optional<std::string> algorithm_version;
    std::optional<ProcessorId> processor_id;

    friend auto operator==(const TransformationReference &, const TransformationReference &)
        -> bool = default;
};

class EVOLUTION_CORE_API Provenance {
  public:
    static auto create(SourceReference source, std::vector<ProvenanceInput> inputs,
                       TransformationReference transformation,
                       std::optional<ConfigurationId> configuration_id = std::nullopt,
                       std::optional<RunId> run_id = std::nullopt,
                       time::TemporalInformation produced_at = time::TemporalInformation::unknown(),
                       std::vector<VersionReference> environment = {},
                       std::vector<ProvenanceId> parents = {}, bool replay = false)
        -> Result<Provenance>;

    [[nodiscard]] auto id() const noexcept -> const ProvenanceId &;
    [[nodiscard]] auto source() const noexcept -> const SourceReference &;
    [[nodiscard]] auto inputs() const noexcept -> const std::vector<ProvenanceInput> &;
    [[nodiscard]] auto transformation() const noexcept -> const TransformationReference &;
    [[nodiscard]] auto configuration_id() const noexcept -> const std::optional<ConfigurationId> &;
    [[nodiscard]] auto run_id() const noexcept -> const std::optional<RunId> &;
    [[nodiscard]] auto produced_at() const noexcept -> const time::TemporalInformation &;
    [[nodiscard]] auto environment() const noexcept -> const std::vector<VersionReference> &;
    [[nodiscard]] auto parents() const noexcept -> const std::vector<ProvenanceId> &;
    [[nodiscard]] auto is_replay() const noexcept -> bool;

    [[nodiscard]] auto with_environment(VersionReference dependency) const -> Result<Provenance>;
    [[nodiscard]] auto with_parent(ProvenanceId parent) const -> Result<Provenance>;

    friend auto operator==(const Provenance &, const Provenance &) -> bool = default;

  private:
    Provenance(ProvenanceId id, SourceReference source, std::vector<ProvenanceInput> inputs,
               TransformationReference transformation,
               std::optional<ConfigurationId> configuration_id, std::optional<RunId> run_id,
               time::TemporalInformation produced_at, std::vector<VersionReference> environment,
               std::vector<ProvenanceId> parents, bool replay);

    ProvenanceId id_;
    SourceReference source_;
    std::vector<ProvenanceInput> inputs_;
    TransformationReference transformation_;
    std::optional<ConfigurationId> configuration_id_;
    std::optional<RunId> run_id_;
    time::TemporalInformation produced_at_;
    std::vector<VersionReference> environment_;
    std::vector<ProvenanceId> parents_;
    bool replay_;
};

} // namespace evolution::provenance

namespace evolution {

using provenance::Provenance;
using provenance::ProvenanceInput;
using provenance::SourceReference;
using provenance::TransformationReference;
using provenance::VersionReference;

} // namespace evolution
