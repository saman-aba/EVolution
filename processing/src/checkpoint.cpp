#include "evolution/processing/checkpoint.hpp"

#include <string_view>
#include <utility>

namespace evolution::processing {
namespace {

auto invalid_manifest(std::string message) -> Result<CheckpointManifest> {
    return Result<CheckpointManifest>::failure(
        Error(ErrorCode::create("processing.invalid_checkpoint_manifest"),
              ErrorCategory::InvalidArgument, std::move(message)));
}

auto incompatible(std::string message) -> Result<void> {
    return Result<void>::failure(Error(ErrorCode::create("processing.incompatible_checkpoint"),
                                       ErrorCategory::Conflict, std::move(message)));
}

auto is_empty(std::string_view value) noexcept -> bool {
    return value.empty();
}

} // namespace

CheckpointManifest::CheckpointManifest(CheckpointManifestData data) : data_(std::move(data)) {}

auto CheckpointManifest::create(CheckpointManifestData data) -> Result<CheckpointManifest> {
    if (is_empty(data.graph.version) || is_empty(data.component.version) ||
        is_empty(data.configuration.version) || is_empty(data.state.version) ||
        is_empty(data.provenance.version)) {
        return invalid_manifest("checkpoint graph, component, configuration, state, and provenance "
                                "versions are required");
    }
    if (data.algorithm && is_empty(data.algorithm->version)) {
        return invalid_manifest("checkpoint algorithm version cannot be empty");
    }
    for (const auto &[schema, version] : data.schema_versions) {
        if (schema.empty() || version.empty()) {
            return invalid_manifest("checkpoint schema names and versions cannot be empty");
        }
    }
    for (const auto &position : data.input_positions) {
        if (position.source.empty() || (position.partition && position.partition->empty())) {
            return invalid_manifest(
                "checkpoint input positions require valid source and partition identities");
        }
    }
    if (data.integrity.algorithm.empty() || data.integrity.digest.empty()) {
        return invalid_manifest("checkpoint integrity algorithm and digest are required");
    }
    return Result<CheckpointManifest>::success(CheckpointManifest(std::move(data)));
}

auto CheckpointManifest::data() const noexcept -> const CheckpointManifestData & {
    return data_;
}

auto CheckpointCompatibility::validate(const CheckpointManifest &manifest,
                                       const CheckpointCompatibilityRequirements &requirements)
    -> Result<void> {
    const auto &data = manifest.data();
    if (data.graph.id != requirements.graph_id ||
        data.graph.version != requirements.graph_version) {
        return incompatible("checkpoint graph identity or version is incompatible");
    }
    if (data.component.id != requirements.component_id ||
        data.component.version != requirements.component_version) {
        return incompatible("checkpoint component identity or version is incompatible");
    }
    if (data.component.processor_id != requirements.processor_id) {
        return incompatible("checkpoint logical processor identity is incompatible");
    }
    if (data.configuration.id != requirements.configuration_id ||
        data.configuration.version != requirements.configuration_version) {
        return incompatible("checkpoint effective configuration is incompatible");
    }
    if (data.state.version != requirements.state_version) {
        return incompatible("checkpoint state version is incompatible");
    }
    if (data.provenance.version != requirements.provenance_version) {
        return incompatible("checkpoint provenance version is incompatible");
    }
    if (data.schema_versions != requirements.schema_versions) {
        return incompatible("checkpoint schema versions are incompatible");
    }
    if (data.algorithm.has_value() != requirements.algorithm.has_value()) {
        return incompatible("checkpoint algorithm requirement is incompatible");
    }
    if (data.algorithm && (data.algorithm->id != requirements.algorithm->id ||
                           data.algorithm->version != requirements.algorithm->version)) {
        return incompatible("checkpoint algorithm identity or version is incompatible");
    }
    return Result<void>::success();
}

} // namespace evolution::processing
