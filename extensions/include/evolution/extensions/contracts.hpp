#pragma once

#include "evolution/core/configuration/configuration.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/identity/identifiers.hpp"
#include "evolution/extensions/api.hpp"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace evolution::extensions {

struct ExtensionTag;
using ExtensionId = identity::Id<ExtensionTag>;

enum class ExtensionKind {
    Domain,
    Processor,
    Analysis,
    Storage,
    Ingestion,
    Interface,
    Serializer,
    Application,
};

enum class DependencyRequirement {
    Required,
    Optional,
    Conditional,
};

enum class IsolationLevel {
    InProcess,
    SeparateProcess,
    Remote,
};

enum class TrustRequirement {
    TrustedOnly,
    UntrustedAllowed,
};

enum class CompatibilityStatus {
    Compatible,
    Incompatible,
    RequiresMigration,
    RequiresAdapter,
};

struct ContractReference {
    std::string identity;
    std::string version;

    friend auto operator==(const ContractReference &, const ContractReference &) -> bool = default;
};

struct ExtensionDependency {
    ExtensionId extension_id;
    std::string version;
    DependencyRequirement requirement{DependencyRequirement::Required};

    friend auto operator==(const ExtensionDependency &, const ExtensionDependency &)
        -> bool = default;
};

struct ExtensionLifecycleContract {
    bool configuration_required{};
    bool initialization_required{};
    bool activation_required{};
    bool supports_reconfiguration{};

    friend auto operator==(const ExtensionLifecycleContract &, const ExtensionLifecycleContract &)
        -> bool = default;
};

struct ExtensionIsolationContract {
    IsolationLevel minimum_level{IsolationLevel::InProcess};
    TrustRequirement trust{TrustRequirement::TrustedOnly};
    bool failure_containment_required{};
    bool resource_isolation_required{};

    friend auto operator==(const ExtensionIsolationContract &, const ExtensionIsolationContract &)
        -> bool = default;
};

struct ExtensionDescriptorData {
    ExtensionId id;
    std::string name;
    std::string version;
    ExtensionKind kind;
    std::set<std::string> capabilities;
    std::vector<ContractReference> contracts;
    std::vector<ExtensionDependency> dependencies;
    ExtensionLifecycleContract lifecycle;
    ExtensionIsolationContract isolation;
    std::optional<std::string> configuration_schema;
};

class EVOLUTION_EXTENSIONS_API ExtensionDescriptor {
  public:
    static auto create(ExtensionDescriptorData data) -> Result<ExtensionDescriptor>;

    [[nodiscard]] auto data() const noexcept -> const ExtensionDescriptorData &;
    [[nodiscard]] auto supports(std::string_view capability) const -> bool;
    [[nodiscard]] auto implements(std::string_view contract_identity,
                                  std::string_view contract_version) const -> bool;

  private:
    explicit ExtensionDescriptor(ExtensionDescriptorData data);
    ExtensionDescriptorData data_;
};

struct CompatibilityRequest {
    ExtensionKind kind;
    std::set<std::string> required_capabilities;
    std::vector<ContractReference> required_contracts;
    IsolationLevel available_isolation{IsolationLevel::InProcess};
    bool trusted{};
};

struct CompatibilityResult {
    CompatibilityStatus status{CompatibilityStatus::Incompatible};
    std::vector<std::string> reasons;
};

class EVOLUTION_EXTENSIONS_API ExtensionCompatibilityEvaluator {
  public:
    virtual ~ExtensionCompatibilityEvaluator() = default;
    [[nodiscard]] virtual auto evaluate(const ExtensionDescriptor &descriptor,
                                        const CompatibilityRequest &request) const
        -> CompatibilityResult = 0;
};

class EVOLUTION_EXTENSIONS_API ExactExtensionCompatibility final
    : public ExtensionCompatibilityEvaluator {
  public:
    [[nodiscard]] auto evaluate(const ExtensionDescriptor &descriptor,
                                const CompatibilityRequest &request) const
        -> CompatibilityResult override;
};

class ExtensionInstance {
  public:
    virtual ~ExtensionInstance() = default;
    [[nodiscard]] virtual auto instance_id() const noexcept -> const ComponentId & = 0;
    [[nodiscard]] virtual auto extension_id() const noexcept -> const ExtensionId & = 0;
    [[nodiscard]] virtual auto extension_version() const noexcept -> std::string_view = 0;
};

class ExtensionFactory {
  public:
    virtual ~ExtensionFactory() = default;
    [[nodiscard]] virtual auto descriptor() const noexcept -> const ExtensionDescriptor & = 0;
    virtual auto construct(const Configuration &configuration,
                           const ExecutionContext &execution_context) const
        -> Result<std::unique_ptr<ExtensionInstance>> = 0;
};

class ExtensionActivator {
  public:
    virtual ~ExtensionActivator() = default;
    virtual auto activate(ExtensionInstance &instance) -> Result<void> = 0;
};

class ExtensionTrustEvaluator {
  public:
    virtual ~ExtensionTrustEvaluator() = default;
    [[nodiscard]] virtual auto is_trusted(const ExtensionDescriptor &descriptor) const
        -> Result<bool> = 0;
};

class ExtensionAuthorizer {
  public:
    virtual ~ExtensionAuthorizer() = default;
    [[nodiscard]] virtual auto authorize(std::string_view principal,
                                         const ExtensionDescriptor &descriptor) const
        -> Result<bool> = 0;
};

class ExtensionLoader {
  public:
    virtual ~ExtensionLoader() = default;
    virtual auto load(std::string_view locator)
        -> Result<std::vector<std::shared_ptr<const ExtensionFactory>>> = 0;
};

} // namespace evolution::extensions
