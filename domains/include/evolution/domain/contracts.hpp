#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/provenance/provenance.hpp"
#include "evolution/domain/api.hpp"
#include "evolution/extensions/contracts.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace evolution::domain {

struct DomainTag;
struct DomainConceptTag;

using DomainId = identity::Id<DomainTag>;
using DomainConceptId = identity::Id<DomainConceptTag>;

enum class DomainConceptKind {
    Entity,
    Event,
    State,
    Metric,
    Pattern,
    Analysis,
    Policy,
    Invariant,
    Serializer,
    IngestionMapping,
};

struct DomainConceptDescriptor {
    DomainConceptId id;
    std::string name;
    std::string version;
    DomainConceptKind kind;
    std::optional<std::string> schema;

    friend auto operator==(const DomainConceptDescriptor &, const DomainConceptDescriptor &)
        -> bool = default;
};

struct DomainDescriptorData {
    DomainId id;
    extensions::ExtensionId extension_id;
    std::string name;
    std::string version;
    std::vector<DomainConceptDescriptor> concepts;
};

class EVOLUTION_DOMAIN_API DomainDescriptor {
  public:
    static auto create(DomainDescriptorData data) -> Result<DomainDescriptor>;

    [[nodiscard]] auto data() const noexcept -> const DomainDescriptorData &;
    [[nodiscard]] auto concepts(DomainConceptKind kind) const
        -> std::vector<DomainConceptDescriptor>;
    [[nodiscard]] auto semantic_versions() const -> std::vector<VersionReference>;

  private:
    explicit DomainDescriptor(DomainDescriptorData data);
    DomainDescriptorData data_;
};

enum class DomainViolationSeverity {
    Warning,
    Error,
};

struct DomainViolation {
    std::string invariant;
    std::string code;
    std::string path;
    std::string message;
    DomainViolationSeverity severity{DomainViolationSeverity::Error};

    friend auto operator==(const DomainViolation &, const DomainViolation &) -> bool = default;
};

class EVOLUTION_DOMAIN_API DomainValidationReport {
  public:
    static auto create(std::vector<DomainViolation> violations) -> Result<DomainValidationReport>;

    [[nodiscard]] auto valid() const noexcept -> bool;
    [[nodiscard]] auto violations() const noexcept -> const std::vector<DomainViolation> &;

  private:
    explicit DomainValidationReport(std::vector<DomainViolation> violations);
    std::vector<DomainViolation> violations_;
};

template <typename Value> class DomainInvariant {
  public:
    virtual ~DomainInvariant() = default;
    [[nodiscard]] virtual auto identity() const noexcept -> std::string_view = 0;
    [[nodiscard]] virtual auto version() const noexcept -> std::string_view = 0;
    [[nodiscard]] virtual auto evaluate(const Value &value) const
        -> Result<std::vector<DomainViolation>> = 0;
};

template <typename Value> class DomainValidator {
  public:
    virtual ~DomainValidator() = default;
    [[nodiscard]] virtual auto validate(const Value &value) const
        -> Result<DomainValidationReport> = 0;
};

template <typename Value> class DomainInvariantSet final : public DomainValidator<Value> {
  public:
    static auto create(std::vector<std::shared_ptr<const DomainInvariant<Value>>> invariants)
        -> Result<DomainInvariantSet> {
        std::set<std::string> identities;
        for (const auto &invariant : invariants) {
            if (!invariant || invariant->identity().empty() || invariant->version().empty()) {
                return Result<DomainInvariantSet>::failure(Error(
                    ErrorCode::create("domain.invalid_invariant"), ErrorCategory::InvalidArgument,
                    "domain invariants require non-empty identity and version"));
            }
            if (!identities.emplace(invariant->identity()).second) {
                return Result<DomainInvariantSet>::failure(
                    Error(ErrorCode::create("domain.duplicate_invariant"), ErrorCategory::Conflict,
                          "domain invariant identities must be unique"));
            }
        }
        return Result<DomainInvariantSet>::success(DomainInvariantSet(std::move(invariants)));
    }

    [[nodiscard]] auto validate(const Value &value) const
        -> Result<DomainValidationReport> override {
        std::vector<DomainViolation> violations;
        for (const auto &invariant : invariants_) {
            auto evaluated = invariant->evaluate(value);
            if (!evaluated) {
                return Result<DomainValidationReport>::failure(evaluated.error());
            }
            auto current = std::move(evaluated).value();
            violations.insert(violations.end(), std::make_move_iterator(current.begin()),
                              std::make_move_iterator(current.end()));
        }
        return DomainValidationReport::create(std::move(violations));
    }

  private:
    explicit DomainInvariantSet(
        std::vector<std::shared_ptr<const DomainInvariant<Value>>> invariants)
        : invariants_(std::move(invariants)) {}

    std::vector<std::shared_ptr<const DomainInvariant<Value>>> invariants_;
};

struct DomainRepresentation {
    std::string schema;
    std::string schema_version;
    std::string representation;
    bool canonical{};
};

template <typename Value> class DomainSerializationAdapter {
  public:
    virtual ~DomainSerializationAdapter() = default;
    [[nodiscard]] virtual auto representation() const noexcept -> const DomainRepresentation & = 0;
    [[nodiscard]] virtual auto serialize(const Value &value) const
        -> Result<std::vector<std::byte>> = 0;
    [[nodiscard]] virtual auto deserialize(std::span<const std::byte> bytes) const
        -> Result<Value> = 0;
};

EVOLUTION_DOMAIN_API auto
make_domain_extension_descriptor(const DomainDescriptor &domain,
                                 std::set<std::string> capabilities = {},
                                 std::vector<extensions::ContractReference> contracts = {})
    -> Result<extensions::ExtensionDescriptor>;

} // namespace evolution::domain
