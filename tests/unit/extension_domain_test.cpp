#include "evolution/core/configuration/configuration.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/domain/contracts.hpp"
#include "evolution/domains/poker/domain.hpp"
#include "evolution/domains/poker/model.hpp"
#include "evolution/extensions/registry.hpp"

#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

class TestRunner {
  public:
    void expect(bool condition, std::string message) {
        if (!condition) {
            ++failures_;
            std::cerr << "FAIL: " << message << '\n';
        }
    }

    [[nodiscard]] auto failures() const noexcept -> int {
        return failures_;
    }

  private:
    int failures_{};
};

template <typename Value> auto require_value(evolution::Result<Value> result) -> Value {
    if (!result) {
        throw std::runtime_error(std::string(result.error().message()));
    }
    return std::move(result).value();
}

auto stable_extension(std::string_view name) -> evolution::extensions::ExtensionId {
    return require_value(evolution::extensions::ExtensionId::from_stable_name(name));
}

auto make_descriptor(std::string name, std::string version = "1",
                     std::vector<evolution::extensions::ExtensionDependency> dependencies = {},
                     evolution::extensions::ExtensionIsolationContract isolation = {})
    -> evolution::extensions::ExtensionDescriptor {
    return require_value(evolution::extensions::ExtensionDescriptor::create(
        {stable_extension(name),
         std::move(name),
         std::move(version),
         evolution::extensions::ExtensionKind::Processor,
         {"batch", "deterministic"},
         {{"test.input", "1"}, {"test.output", "1"}},
         std::move(dependencies),
         {true, true, true, false},
         isolation,
         std::string("test.configuration.v1")}));
}

class TestInstance final : public evolution::extensions::ExtensionInstance {
  public:
    TestInstance(evolution::ComponentId instance_id,
                 evolution::extensions::ExtensionId extension_id, std::string version)
        : instance_id_(std::move(instance_id)), extension_id_(std::move(extension_id)),
          version_(std::move(version)) {}

    [[nodiscard]] auto instance_id() const noexcept -> const evolution::ComponentId & override {
        return instance_id_;
    }

    [[nodiscard]] auto extension_id() const noexcept
        -> const evolution::extensions::ExtensionId & override {
        return extension_id_;
    }

    [[nodiscard]] auto extension_version() const noexcept -> std::string_view override {
        return version_;
    }

  private:
    evolution::ComponentId instance_id_;
    evolution::extensions::ExtensionId extension_id_;
    std::string version_;
};

class CountingFactory final : public evolution::extensions::ExtensionFactory {
  public:
    explicit CountingFactory(evolution::extensions::ExtensionDescriptor descriptor)
        : descriptor_(std::move(descriptor)) {}

    [[nodiscard]] auto descriptor() const noexcept
        -> const evolution::extensions::ExtensionDescriptor & override {
        return descriptor_;
    }

    auto construct(const evolution::Configuration &, const evolution::ExecutionContext &) const
        -> evolution::Result<std::unique_ptr<evolution::extensions::ExtensionInstance>> override {
        ++constructions_;
        auto instance_id = evolution::ComponentId::generate();
        if (!instance_id) {
            return evolution::Result<std::unique_ptr<evolution::extensions::ExtensionInstance>>::
                failure(instance_id.error());
        }
        return evolution::Result<std::unique_ptr<evolution::extensions::ExtensionInstance>>::
            success(std::make_unique<TestInstance>(
                std::move(instance_id).value(), descriptor_.data().id, descriptor_.data().version));
    }

    [[nodiscard]] auto constructions() const noexcept -> int {
        return constructions_.load();
    }

  private:
    evolution::extensions::ExtensionDescriptor descriptor_;
    mutable std::atomic_int constructions_{};
};

auto configuration() -> evolution::Configuration {
    return require_value(evolution::Configuration::create({}, "1"));
}

auto execution_context() -> evolution::ExecutionContext {
    return evolution::ExecutionContext::create(
        evolution::ExecutionMode::Test,
        require_value(evolution::RunId::from_stable_name("extension-test-run")), std::nullopt,
        nullptr, std::make_shared<evolution::DeterministicRandomSource>(7), {});
}

void test_registration(TestRunner &test) {
    auto factory = std::make_shared<CountingFactory>(make_descriptor("processor-a"));
    evolution::extensions::ExtensionRegistry registry;
    test.expect(registry.register_factory(factory).has_value(),
                "explicit extension registration succeeds");
    test.expect(factory->constructions() == 0,
                "registration does not construct or activate an extension");

    const auto discovered = registry.discover(
        {evolution::extensions::ExtensionKind::Processor, {"batch"}, {{"test.input", "1"}}});
    test.expect(discovered.size() == 1 && factory->constructions() == 0,
                "metadata discovery does not initialize an extension");
    const auto duplicate = registry.register_factory(factory);
    test.expect(!duplicate && duplicate.error().category() == evolution::ErrorCategory::Conflict,
                "duplicate identity and version registration is rejected");

    auto selected = require_value(
        registry.factory(factory->descriptor().data().id, factory->descriptor().data().version));
    auto instance = require_value(selected->construct(configuration(), execution_context()));
    test.expect(factory->constructions() == 1 &&
                    instance->extension_id() == factory->descriptor().data().id,
                "construction occurs only after explicit factory selection");

    const auto missing = registry.factory(stable_extension("missing"), "1");
    test.expect(!missing && missing.error().category() == evolution::ErrorCategory::NotFound,
                "missing extensions remain distinct from construction failure");
}

void test_compatibility_and_dependencies(TestRunner &test) {
    evolution::extensions::ExactExtensionCompatibility compatibility;
    const auto isolated =
        make_descriptor("isolated", "1", {},
                        {evolution::extensions::IsolationLevel::SeparateProcess,
                         evolution::extensions::TrustRequirement::TrustedOnly, true, true});
    const auto incompatible =
        compatibility.evaluate(isolated, {evolution::extensions::ExtensionKind::Processor,
                                          {"batch"},
                                          {{"test.input", "1"}},
                                          evolution::extensions::IsolationLevel::InProcess,
                                          false});
    test.expect(incompatible.status == evolution::extensions::CompatibilityStatus::Incompatible &&
                    incompatible.reasons.size() == 2,
                "compatibility reports trust and isolation mismatches explicitly");
    const auto compatible =
        compatibility.evaluate(isolated, {evolution::extensions::ExtensionKind::Processor,
                                          {"batch"},
                                          {{"test.input", "1"}},
                                          evolution::extensions::IsolationLevel::SeparateProcess,
                                          true});
    test.expect(compatible.status == evolution::extensions::CompatibilityStatus::Compatible,
                "exact contract compatibility accepts an adequate boundary");

    const auto first_id = stable_extension("dependency-first");
    const auto second_id = stable_extension("dependency-second");
    auto first = std::make_shared<CountingFactory>(
        require_value(evolution::extensions::ExtensionDescriptor::create(
            {first_id,
             "dependency-first",
             "1",
             evolution::extensions::ExtensionKind::Processor,
             {},
             {},
             {{second_id, "1", evolution::extensions::DependencyRequirement::Required}},
             {},
             {},
             std::nullopt})));
    evolution::extensions::ExtensionRegistry missing_registry;
    require_value(missing_registry.register_factory(first));
    const auto missing = missing_registry.validate_dependencies();
    test.expect(!missing && missing.error().category() == evolution::ErrorCategory::NotFound,
                "required dependencies are validated without implicit construction");

    auto second = std::make_shared<CountingFactory>(
        require_value(evolution::extensions::ExtensionDescriptor::create(
            {second_id,
             "dependency-second",
             "1",
             evolution::extensions::ExtensionKind::Processor,
             {},
             {},
             {{first_id, "1", evolution::extensions::DependencyRequirement::Required}},
             {},
             {},
             std::nullopt})));
    evolution::extensions::ExtensionRegistry cycle_registry;
    require_value(cycle_registry.register_factory(first));
    require_value(cycle_registry.register_factory(second));
    const auto cycle = cycle_registry.validate_dependencies();
    test.expect(!cycle && cycle.error().category() == evolution::ErrorCategory::Conflict,
                "dependency cycles are rejected before activation");
}

class PositiveInvariant final : public evolution::domain::DomainInvariant<int> {
  public:
    [[nodiscard]] auto identity() const noexcept -> std::string_view override {
        return "test.positive";
    }

    [[nodiscard]] auto version() const noexcept -> std::string_view override {
        return "1";
    }

    [[nodiscard]] auto evaluate(const int &value) const
        -> evolution::Result<std::vector<evolution::domain::DomainViolation>> override {
        if (value > 0) {
            return evolution::Result<std::vector<evolution::domain::DomainViolation>>::success({});
        }
        return evolution::Result<std::vector<evolution::domain::DomainViolation>>::success(
            {{"test.positive", "test.not_positive", "value", "value must be positive",
              evolution::domain::DomainViolationSeverity::Error}});
    }
};

class IntegerAdapter final : public evolution::domain::DomainSerializationAdapter<int> {
  public:
    [[nodiscard]] auto representation() const noexcept
        -> const evolution::domain::DomainRepresentation & override {
        return representation_;
    }

    [[nodiscard]] auto serialize(const int &value) const
        -> evolution::Result<std::vector<std::byte>> override {
        if (value < 0 || value > 255) {
            return evolution::Result<std::vector<std::byte>>::failure(evolution::Error(
                evolution::ErrorCode::create("test.out_of_range"),
                evolution::ErrorCategory::Serialization, "test integer is outside one byte"));
        }
        return evolution::Result<std::vector<std::byte>>::success({static_cast<std::byte>(value)});
    }

    [[nodiscard]] auto deserialize(std::span<const std::byte> bytes) const
        -> evolution::Result<int> override {
        if (bytes.size() != 1) {
            return evolution::Result<int>::failure(evolution::Error(
                evolution::ErrorCode::create("test.invalid_bytes"),
                evolution::ErrorCategory::Serialization, "test integer requires one byte"));
        }
        return evolution::Result<int>::success(std::to_integer<int>(bytes.front()));
    }

  private:
    evolution::domain::DomainRepresentation representation_{"test.integer", "1", "byte", true};
};

void test_domain_infrastructure(TestRunner &test) {
    auto validator = require_value(evolution::domain::DomainInvariantSet<int>::create(
        {std::make_shared<PositiveInvariant>()}));
    test.expect(require_value(validator.validate(1)).valid(),
                "domain invariant sets accept valid domain values");
    const auto invalid = require_value(validator.validate(0));
    test.expect(!invalid.valid() && invalid.violations().front().code == "test.not_positive",
                "domain semantic violations remain structured and domain-owned");

    IntegerAdapter adapter;
    const auto bytes = require_value(adapter.serialize(42));
    test.expect(require_value(adapter.deserialize(bytes)) == 42 &&
                    adapter.representation().schema == "test.integer",
                "domain serialization adapters declare and preserve representation semantics");

    const auto poker = require_value(evolution::domains::poker::descriptor());
    test.expect(poker.concepts(evolution::domain::DomainConceptKind::Entity).size() == 2 &&
                    !poker.concepts(evolution::domain::DomainConceptKind::Pattern).empty() &&
                    !poker.concepts(evolution::domain::DomainConceptKind::Policy).empty(),
                "Poker owns entities, patterns, and policies outside Core");
    test.expect(poker.semantic_versions().size() == poker.data().concepts.size() + 1,
                "domain provenance metadata includes domain and concept versions");

    const auto other_domain = require_value(evolution::domain::DomainDescriptor::create(
        {require_value(evolution::domain::DomainId::from_stable_name("market.domain")),
         stable_extension("market.domain.extension"),
         "market",
         "1",
         {{require_value(evolution::domain::DomainConceptId::from_stable_name("market.player")),
           "player", "1", evolution::domain::DomainConceptKind::Entity, std::nullopt}}}));
    test.expect(other_domain.data().id != poker.data().id &&
                    other_domain.data().concepts.front().id != poker.data().concepts.front().id,
                "similar names in different domains retain distinct logical identities");
}

void test_poker_domain(TestRunner &test) {
    using namespace evolution::domains::poker;
    static_assert(!std::is_same_v<PlayerId, HandId>);
    static_assert(!std::is_same_v<PlayerId, evolution::domain::DomainId>);

    const auto player_a = require_value(PlayerId::from_stable_name("player-a"));
    const auto player_b = require_value(PlayerId::from_stable_name("player-b"));
    const auto hand_id = require_value(HandId::from_stable_name("hand-a"));
    test.expect(create_player(player_a, "Alice").has_value() &&
                    !create_player(player_b, "").has_value(),
                "Poker entity validation remains domain-specific");

    auto state = require_value(start_hand({hand_id, {player_a, player_b}, 3}));
    state = require_value(apply_event(
        state, PokerEventPayload{PlayerActed{hand_id, player_a, ActionKind::Raise, 7}}));
    test.expect(state.pot == 10 && state.action_count == 1,
                "Poker events project deterministic hand state");
    const auto invalid_check =
        apply_event(state, PokerEventPayload{PlayerActed{hand_id, player_b, ActionKind::Check, 1}});
    test.expect(!invalid_check &&
                    invalid_check.error().code().value() == "poker.invalid_action_amount",
                "Poker semantic invariants reject illegal action amounts");
    state = require_value(apply_event(state, PokerEventPayload{HandFinished{hand_id, player_a}}));
    test.expect(state.status == HandStatus::Finished && require_value(validate_hand(state)).valid(),
                "Poker hand completion preserves a valid domain state");
    test.expect(require_value(voluntary_put_money_in_pot_metric()).name() == "VPIP",
                "Poker metric meaning is defined by the Poker domain");

    evolution::extensions::ExtensionRegistry registry;
    auto poker_factory = require_value(extension_factory());
    require_value(registry.register_factory(poker_factory));
    test.expect(
        registry.discover({evolution::extensions::ExtensionKind::Domain, {"domain"}, {}}).size() ==
            1,
        "Poker participates through generic explicit domain registration");
}

} // namespace

int main() {
    TestRunner test;
    try {
        test_registration(test);
        test_compatibility_and_dependencies(test);
        test_domain_infrastructure(test);
        test_poker_domain(test);
    } catch (const std::exception &exception) {
        std::cerr << "UNEXPECTED: " << exception.what() << '\n';
        return 1;
    }
    return test.failures() == 0 ? 0 : 1;
}
