#include "evolution/packaging/release.hpp"

#include <set>
#include <utility>

namespace evolution::packaging {
namespace {

template <typename Value> auto invalid(std::string code, std::string message) -> Result<Value> {
    return Result<Value>::failure(Error(ErrorCode::create(std::move(code)),
                                        ErrorCategory::InvalidArgument, std::move(message)));
}

auto invalid_path(const std::string &path) -> bool {
    return path.empty() || path.front() == '/' || path.find("..") != std::string::npos;
}

} // namespace

BuildProvenance::BuildProvenance(BuildProvenanceData data) : data_(std::move(data)) {}

auto BuildProvenance::create(BuildProvenanceData data) -> Result<BuildProvenance> {
    if (data.source_revision.empty() || data.compiler.empty() || data.compiler_version.empty() ||
        data.build_configuration.empty() || data.build_epoch.empty()) {
        return invalid<BuildProvenance>("packaging.invalid_build_provenance",
                                        "build provenance fields cannot be empty");
    }
    return Result<BuildProvenance>::success(BuildProvenance(std::move(data)));
}

auto BuildProvenance::data() const noexcept -> const BuildProvenanceData & {
    return data_;
}

PackageManifest::PackageManifest(PackageManifestData data) : data_(std::move(data)) {}

auto PackageManifest::create(PackageManifestData data) -> Result<PackageManifest> {
    if (data.version.empty() || data.target.empty() || data.artifacts.empty()) {
        return invalid<PackageManifest>("packaging.invalid_manifest",
                                        "package manifest requires version, target, and artifacts");
    }
    const std::vector<std::string> layout_locations{
        data.layout.installation,       data.layout.configuration_templates,
        data.layout.runtime_state,      data.layout.persistent_data,
        data.layout.observability_data, data.layout.temporary_data};
    std::set<std::string> distinct_locations;
    for (const auto &location : layout_locations) {
        if (location.empty() || !distinct_locations.insert(location).second) {
            return invalid<PackageManifest>(
                "packaging.invalid_runtime_layout",
                "installation, configuration, runtime, data, observability, and temporary "
                "locations must be explicit and distinct");
        }
    }
    std::set<std::string> artifact_paths;
    for (const auto &artifact : data.artifacts) {
        if (invalid_path(artifact.relative_path) || artifact.integrity.algorithm.empty() ||
            artifact.integrity.value.empty() ||
            !artifact_paths.insert(artifact.relative_path).second) {
            return invalid<PackageManifest>(
                "packaging.invalid_artifact",
                "artifacts require unique relative paths and explicit integrity digests");
        }
    }
    for (const auto &dependency : data.dependencies) {
        if (dependency.identity.empty()) {
            return invalid<PackageManifest>("packaging.invalid_dependency",
                                            "package dependency identity cannot be empty");
        }
    }
    return Result<PackageManifest>::success(PackageManifest(std::move(data)));
}

auto PackageManifest::data() const noexcept -> const PackageManifestData & {
    return data_;
}

auto PackageManifest::verify_integrity(const std::map<std::string, IntegrityDigest> &observed) const
    -> Result<void> {
    for (const auto &artifact : data_.artifacts) {
        const auto iterator = observed.find(artifact.relative_path);
        if (iterator == observed.end() ||
            iterator->second.algorithm != artifact.integrity.algorithm ||
            iterator->second.value != artifact.integrity.value) {
            return Result<void>::failure(
                Error(ErrorCode::create("packaging.integrity_mismatch"), ErrorCategory::Conflict,
                      "package artifact integrity does not match the release manifest"));
        }
    }
    return Result<void>::success();
}

ReleaseIdentity::ReleaseIdentity(ReleaseIdentityData data) : data_(std::move(data)) {}

auto ReleaseIdentity::create(ReleaseIdentityData data) -> Result<ReleaseIdentity> {
    if (data.version.empty() || data.version != data.package.data().version) {
        return invalid<ReleaseIdentity>("packaging.invalid_release_identity",
                                        "release and package versions must be explicit and equal");
    }
    return Result<ReleaseIdentity>::success(ReleaseIdentity(std::move(data)));
}

auto ReleaseIdentity::data() const noexcept -> const ReleaseIdentityData & {
    return data_;
}

UpgradePlan::UpgradePlan(UpgradePlanData data) : data_(std::move(data)) {}

auto UpgradePlan::create(UpgradePlanData data) -> Result<UpgradePlan> {
    if (data.from_release.empty() || data.to_release.empty() ||
        data.from_release == data.to_release) {
        return invalid<UpgradePlan>("packaging.invalid_upgrade_plan",
                                    "upgrade plan requires distinct release identities");
    }
    std::set<std::string> identities;
    for (const auto &migration : data.migrations) {
        if (migration.identity.empty() || migration.from_version.empty() ||
            migration.to_version.empty() || migration.from_version == migration.to_version ||
            !identities.insert(migration.identity).second) {
            return invalid<UpgradePlan>(
                "packaging.invalid_migration",
                "migration identities and version transitions must be valid");
        }
    }
    return Result<UpgradePlan>::success(UpgradePlan(std::move(data)));
}

auto UpgradePlan::data() const noexcept -> const UpgradePlanData & {
    return data_;
}

auto UpgradePlan::execute(const std::vector<MigrationHook *> &hooks) const -> Result<void> {
    if (hooks.size() != data_.migrations.size()) {
        return Result<void>::failure(
            Error(ErrorCode::create("packaging.migration_hook_mismatch"),
                  ErrorCategory::InvalidConfiguration,
                  "upgrade plan requires one migration hook per descriptor"));
    }
    std::vector<MigrationHook *> applied;
    for (std::size_t index = 0; index < hooks.size(); ++index) {
        auto *hook = hooks[index];
        if (hook == nullptr || hook->descriptor().identity != data_.migrations[index].identity ||
            hook->descriptor().kind != data_.migrations[index].kind ||
            hook->descriptor().from_version != data_.migrations[index].from_version ||
            hook->descriptor().to_version != data_.migrations[index].to_version) {
            return Result<void>::failure(
                Error(ErrorCode::create("packaging.migration_hook_mismatch"),
                      ErrorCategory::InvalidConfiguration,
                      "migration hook does not match upgrade plan ordering"));
        }
        auto validated = hook->validate();
        auto applied_result = validated ? hook->apply() : validated;
        if (!applied_result) {
            for (auto iterator = applied.rbegin(); iterator != applied.rend(); ++iterator) {
                if ((*iterator)->descriptor().reversible) {
                    static_cast<void>((*iterator)->rollback());
                }
            }
            return applied_result;
        }
        applied.push_back(hook);
    }
    return Result<void>::success();
}

} // namespace evolution::packaging
