#include "evolution/domains/poker/domain.hpp"

#include <utility>

namespace evolution::domains::poker {
namespace {

template <typename Id> auto stable(std::string_view name) -> Id {
    return Id::from_stable_name(name).value();
}

class PokerDomainInstance final : public extensions::ExtensionInstance {
  public:
    PokerDomainInstance(ComponentId instance_id, extensions::ExtensionId extension_id)
        : instance_id_(std::move(instance_id)), extension_id_(std::move(extension_id)) {}

    [[nodiscard]] auto instance_id() const noexcept -> const ComponentId & override {
        return instance_id_;
    }

    [[nodiscard]] auto extension_id() const noexcept -> const extensions::ExtensionId & override {
        return extension_id_;
    }

    [[nodiscard]] auto extension_version() const noexcept -> std::string_view override {
        return "1";
    }

  private:
    ComponentId instance_id_;
    extensions::ExtensionId extension_id_;
};

class PokerDomainFactory final : public extensions::ExtensionFactory {
  public:
    explicit PokerDomainFactory(extensions::ExtensionDescriptor descriptor)
        : descriptor_(std::move(descriptor)) {}

    [[nodiscard]] auto descriptor() const noexcept
        -> const extensions::ExtensionDescriptor & override {
        return descriptor_;
    }

    auto construct(const Configuration &, const ExecutionContext &) const
        -> Result<std::unique_ptr<extensions::ExtensionInstance>> override {
        auto instance_id = ComponentId::generate();
        if (!instance_id) {
            return Result<std::unique_ptr<extensions::ExtensionInstance>>::failure(
                instance_id.error());
        }
        return Result<std::unique_ptr<extensions::ExtensionInstance>>::success(
            std::make_unique<PokerDomainInstance>(std::move(instance_id).value(),
                                                  descriptor_.data().id));
    }

  private:
    extensions::ExtensionDescriptor descriptor_;
};

} // namespace

auto descriptor() -> Result<domain::DomainDescriptor> {
    using domain::DomainConceptDescriptor;
    using domain::DomainConceptKind;
    std::vector<DomainConceptDescriptor> concepts{
        {stable<domain::DomainConceptId>("poker.entity.player"), "player", "1",
         DomainConceptKind::Entity, std::nullopt},
        {stable<domain::DomainConceptId>("poker.entity.hand"), "hand", "1",
         DomainConceptKind::Entity, std::nullopt},
        {stable<domain::DomainConceptId>("poker.event.hand-started"), "hand-started", "1",
         DomainConceptKind::Event, std::string("poker.hand-started.v1")},
        {stable<domain::DomainConceptId>("poker.event.player-acted"), "player-acted", "1",
         DomainConceptKind::Event, std::string("poker.player-acted.v1")},
        {stable<domain::DomainConceptId>("poker.event.hand-finished"), "hand-finished", "1",
         DomainConceptKind::Event, std::string("poker.hand-finished.v1")},
        {stable<domain::DomainConceptId>("poker.state.hand"), "hand-state", "1",
         DomainConceptKind::State, std::string("poker.hand-state.v1")},
        {stable<domain::DomainConceptId>("poker.metric.vpip"), "vpip", "1",
         DomainConceptKind::Metric, std::nullopt},
        {stable<domain::DomainConceptId>("poker.pattern.aggression"), "aggression", "1",
         DomainConceptKind::Pattern, std::nullopt},
        {stable<domain::DomainConceptId>("poker.policy.legal-action"), "legal-action", "1",
         DomainConceptKind::Policy, std::nullopt},
        {stable<domain::DomainConceptId>("poker.invariant.hand"), "hand-invariants", "1",
         DomainConceptKind::Invariant, std::nullopt}};
    return domain::DomainDescriptor::create(
        {stable<domain::DomainId>("poker.domain"),
         stable<extensions::ExtensionId>("poker.domain.extension"), "poker", "1",
         std::move(concepts)});
}

auto extension_descriptor() -> Result<extensions::ExtensionDescriptor> {
    auto poker_domain = descriptor();
    if (!poker_domain) {
        return Result<extensions::ExtensionDescriptor>::failure(poker_domain.error());
    }
    return domain::make_domain_extension_descriptor(
        poker_domain.value(), {"poker.hand-state", "poker.vpip"},
        {{"evolution.domain", "1"}, {"evolution.core.event", "1"}});
}

auto extension_factory() -> Result<std::shared_ptr<const extensions::ExtensionFactory>> {
    auto poker_descriptor = extension_descriptor();
    if (!poker_descriptor) {
        return Result<std::shared_ptr<const extensions::ExtensionFactory>>::failure(
            poker_descriptor.error());
    }
    return Result<std::shared_ptr<const extensions::ExtensionFactory>>::success(
        std::make_shared<PokerDomainFactory>(std::move(poker_descriptor).value()));
}

} // namespace evolution::domains::poker
