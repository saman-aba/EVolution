#include "evolution/core/provenance/provenance.hpp"

#include <utility>

namespace evolution::provenance {
namespace {

auto calculate_id(const SourceReference &source, const std::vector<ProvenanceInput> &inputs,
                  const TransformationReference &transformation,
                  const std::optional<ConfigurationId> &configuration_id,
                  const std::optional<RunId> &run_id, const time::TemporalInformation &produced_at,
                  const std::vector<VersionReference> &environment,
                  const std::vector<ProvenanceId> &parents, bool replay) -> ProvenanceId {
    std::string canonical =
        "source:" + source.id.to_string() + ':' + source.kind + ':' + source.locator + '\n';
    for (const auto &input : inputs) {
        canonical +=
            "input:" + input.kind + ':' + input.identity + ':' + input.version.value_or("") + '\n';
    }
    canonical += "component:" + transformation.component_id.to_string() + ':' +
                 transformation.component_name + ':' + transformation.component_version + '\n';
    if (transformation.algorithm_id) {
        canonical += "algorithm:" + transformation.algorithm_id->to_string() + ':' +
                     transformation.algorithm_version.value_or("") + '\n';
    }
    if (transformation.processor_id) {
        canonical += "processor:" + transformation.processor_id->to_string() + '\n';
    }
    if (configuration_id) {
        canonical += "configuration:" + configuration_id->to_string() + '\n';
    }
    if (run_id) {
        canonical += "run:" + run_id->to_string() + '\n';
    }
    if (const auto value = produced_at.value()) {
        canonical += "produced_at:" + std::to_string(value->time_since_epoch().count()) + '\n';
    } else {
        canonical += "produced_at:unknown\n";
    }
    for (const auto &dependency : environment) {
        canonical += "environment:" + dependency.name + ':' + dependency.version + '\n';
    }
    for (const auto &parent : parents) {
        canonical += "parent:" + parent.to_string() + '\n';
    }
    canonical += replay ? "replay:true\n" : "replay:false\n";
    return ProvenanceId::from_stable_name(canonical).value();
}

auto validate(const SourceReference &source, const std::vector<ProvenanceInput> &inputs,
              const TransformationReference &transformation,
              const std::vector<VersionReference> &environment) -> Result<void> {
    if (source.kind.empty()) {
        return Result<void>::failure(Error(ErrorCode::create("provenance.invalid_source"),
                                           ErrorCategory::InvalidArgument,
                                           "provenance source kind cannot be empty"));
    }
    if (transformation.component_name.empty() || transformation.component_version.empty()) {
        return Result<void>::failure(Error(
            ErrorCode::create("provenance.invalid_transformation"), ErrorCategory::InvalidArgument,
            "transformation component name and version are required"));
    }
    for (const auto &input : inputs) {
        if (input.kind.empty() || input.identity.empty()) {
            return Result<void>::failure(Error(ErrorCode::create("provenance.invalid_input"),
                                               ErrorCategory::InvalidArgument,
                                               "provenance input kind and identity are required"));
        }
    }
    for (const auto &dependency : environment) {
        if (dependency.name.empty() || dependency.version.empty()) {
            return Result<void>::failure(Error(
                ErrorCode::create("provenance.invalid_environment"), ErrorCategory::InvalidArgument,
                "environment dependency name and version are required"));
        }
    }
    return Result<void>::success();
}

} // namespace

Provenance::Provenance(ProvenanceId id, SourceReference source, std::vector<ProvenanceInput> inputs,
                       TransformationReference transformation,
                       std::optional<ConfigurationId> configuration_id, std::optional<RunId> run_id,
                       time::TemporalInformation produced_at,
                       std::vector<VersionReference> environment, std::vector<ProvenanceId> parents,
                       bool replay)
    : id_(std::move(id)), source_(std::move(source)), inputs_(std::move(inputs)),
      transformation_(std::move(transformation)), configuration_id_(std::move(configuration_id)),
      run_id_(std::move(run_id)), produced_at_(std::move(produced_at)),
      environment_(std::move(environment)), parents_(std::move(parents)), replay_(replay) {}

auto Provenance::create(SourceReference source, std::vector<ProvenanceInput> inputs,
                        TransformationReference transformation,
                        std::optional<ConfigurationId> configuration_id,
                        std::optional<RunId> run_id, time::TemporalInformation produced_at,
                        std::vector<VersionReference> environment,
                        std::vector<ProvenanceId> parents, bool replay) -> Result<Provenance> {
    auto validation = validate(source, inputs, transformation, environment);
    if (validation.has_error()) {
        return Result<Provenance>::failure(validation.error());
    }
    auto id = calculate_id(source, inputs, transformation, configuration_id, run_id, produced_at,
                           environment, parents, replay);
    return Result<Provenance>::success(
        Provenance(std::move(id), std::move(source), std::move(inputs), std::move(transformation),
                   std::move(configuration_id), std::move(run_id), std::move(produced_at),
                   std::move(environment), std::move(parents), replay));
}

auto Provenance::id() const noexcept -> const ProvenanceId & {
    return id_;
}

auto Provenance::source() const noexcept -> const SourceReference & {
    return source_;
}

auto Provenance::inputs() const noexcept -> const std::vector<ProvenanceInput> & {
    return inputs_;
}

auto Provenance::transformation() const noexcept -> const TransformationReference & {
    return transformation_;
}

auto Provenance::configuration_id() const noexcept -> const std::optional<ConfigurationId> & {
    return configuration_id_;
}

auto Provenance::run_id() const noexcept -> const std::optional<RunId> & {
    return run_id_;
}

auto Provenance::produced_at() const noexcept -> const time::TemporalInformation & {
    return produced_at_;
}

auto Provenance::environment() const noexcept -> const std::vector<VersionReference> & {
    return environment_;
}

auto Provenance::parents() const noexcept -> const std::vector<ProvenanceId> & {
    return parents_;
}

auto Provenance::is_replay() const noexcept -> bool {
    return replay_;
}

auto Provenance::with_environment(VersionReference dependency) const -> Result<Provenance> {
    auto environment = environment_;
    environment.push_back(std::move(dependency));
    return create(source_, inputs_, transformation_, configuration_id_, run_id_, produced_at_,
                  std::move(environment), parents_, replay_);
}

auto Provenance::with_parent(ProvenanceId parent) const -> Result<Provenance> {
    auto parents = parents_;
    parents.push_back(std::move(parent));
    return create(source_, inputs_, transformation_, configuration_id_, run_id_, produced_at_,
                  environment_, std::move(parents), replay_);
}

} // namespace evolution::provenance
