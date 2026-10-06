#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/packaging/api.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace evolution::packaging {

struct BuildTag;
struct ReleaseTag;
struct PackageTag;
struct ArtifactTag;
using BuildId = identity::Id<BuildTag>;
using ReleaseId = identity::Id<ReleaseTag>;
using PackageId = identity::Id<PackageTag>;
using ArtifactId = identity::Id<ArtifactTag>;

enum class ArtifactRole { Runtime, Development, Documentation, Metadata };
enum class DependencyKind { Build, Runtime, ExternalService };
enum class MigrationKind { Configuration, PersistentData };

struct BuildProvenanceData {
    BuildId id;
    std::string source_revision;
    std::string compiler;
    std::string compiler_version;
    std::string build_configuration;
    std::string build_epoch;
    std::map<std::string, std::string> dependency_versions;
    bool reproducible{};
};

class EVOLUTION_PACKAGING_API BuildProvenance {
  public:
    static auto create(BuildProvenanceData data) -> Result<BuildProvenance>;
    [[nodiscard]] auto data() const noexcept -> const BuildProvenanceData &;

  private:
    explicit BuildProvenance(BuildProvenanceData data);
    BuildProvenanceData data_;
};

struct IntegrityDigest {
    std::string algorithm;
    std::string value;
};

struct ArtifactDescriptor {
    ArtifactId id;
    std::string relative_path;
    ArtifactRole role{ArtifactRole::Runtime};
    IntegrityDigest integrity;
};

struct PackageDependency {
    std::string identity;
    DependencyKind kind{DependencyKind::Runtime};
    std::optional<std::string> version_requirement;
    bool required{true};
};

struct RuntimeLayout {
    std::string installation;
    std::string configuration_templates;
    std::string runtime_state;
    std::string persistent_data;
    std::string observability_data;
    std::string temporary_data;
};

struct PackageManifestData {
    PackageId id;
    std::string version;
    std::string target;
    std::vector<ArtifactDescriptor> artifacts;
    std::vector<PackageDependency> dependencies;
    RuntimeLayout layout;
};

class EVOLUTION_PACKAGING_API PackageManifest {
  public:
    static auto create(PackageManifestData data) -> Result<PackageManifest>;
    [[nodiscard]] auto data() const noexcept -> const PackageManifestData &;
    [[nodiscard]] auto
    verify_integrity(const std::map<std::string, IntegrityDigest> &observed) const -> Result<void>;

  private:
    explicit PackageManifest(PackageManifestData data);
    PackageManifestData data_;
};

struct ReleaseIdentityData {
    ReleaseId id;
    std::string version;
    BuildProvenance build;
    PackageManifest package;
};

class EVOLUTION_PACKAGING_API ReleaseIdentity {
  public:
    static auto create(ReleaseIdentityData data) -> Result<ReleaseIdentity>;
    [[nodiscard]] auto data() const noexcept -> const ReleaseIdentityData &;

  private:
    explicit ReleaseIdentity(ReleaseIdentityData data);
    ReleaseIdentityData data_;
};

struct MigrationDescriptor {
    std::string identity;
    MigrationKind kind{MigrationKind::Configuration};
    std::string from_version;
    std::string to_version;
    bool reversible{};
};

class MigrationHook {
  public:
    virtual ~MigrationHook() = default;
    [[nodiscard]] virtual auto descriptor() const noexcept -> const MigrationDescriptor & = 0;
    virtual auto validate() const -> Result<void> = 0;
    virtual auto apply() -> Result<void> = 0;
    virtual auto rollback() -> Result<void> = 0;
};

struct UpgradePlanData {
    std::string from_release;
    std::string to_release;
    std::vector<MigrationDescriptor> migrations;
    bool downgrade_supported{};
};

class EVOLUTION_PACKAGING_API UpgradePlan {
  public:
    static auto create(UpgradePlanData data) -> Result<UpgradePlan>;
    [[nodiscard]] auto data() const noexcept -> const UpgradePlanData &;
    auto execute(const std::vector<MigrationHook *> &hooks) const -> Result<void>;

  private:
    explicit UpgradePlan(UpgradePlanData data);
    UpgradePlanData data_;
};

} // namespace evolution::packaging
