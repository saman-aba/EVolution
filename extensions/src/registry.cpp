#include "evolution/extensions/registry.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <utility>

namespace evolution::extensions {
namespace {

auto key(const ExtensionDescriptor &descriptor) -> std::pair<std::string, std::string> {
    return {descriptor.data().id.to_string(), descriptor.data().version};
}

auto registry_error(std::string code, ErrorCategory category, std::string message) -> Error {
    return Error(ErrorCode::create(std::move(code)), category, std::move(message));
}

} // namespace

auto ExtensionRegistry::register_factory(std::shared_ptr<const ExtensionFactory> factory)
    -> Result<void> {
    if (!factory) {
        return Result<void>::failure(registry_error("extension.null_factory",
                                                    ErrorCategory::InvalidArgument,
                                                    "extension factory cannot be null"));
    }
    const auto descriptor_key = key(factory->descriptor());
    std::unique_lock lock(mutex_);
    const auto duplicate = std::any_of(entries_.begin(), entries_.end(), [&](const auto &entry) {
        return key(entry.descriptor) == descriptor_key;
    });
    if (duplicate) {
        return Result<void>::failure(
            registry_error("extension.registration_conflict", ErrorCategory::Conflict,
                           "extension identity and version are already registered"));
    }
    entries_.push_back(Entry{factory->descriptor(), std::move(factory)});
    return Result<void>::success();
}

auto ExtensionRegistry::discover(const DiscoveryQuery &query) const
    -> std::vector<ExtensionDescriptor> {
    std::shared_lock lock(mutex_);
    std::vector<ExtensionDescriptor> matches;
    for (const auto &entry : entries_) {
        const auto &descriptor = entry.descriptor;
        if (query.kind && descriptor.data().kind != *query.kind) {
            continue;
        }
        if (!std::all_of(query.required_capabilities.begin(), query.required_capabilities.end(),
                         [&](const auto &capability) { return descriptor.supports(capability); })) {
            continue;
        }
        if (!std::all_of(query.required_contracts.begin(), query.required_contracts.end(),
                         [&](const auto &contract) {
                             return descriptor.implements(contract.identity, contract.version);
                         })) {
            continue;
        }
        matches.push_back(descriptor);
    }
    std::sort(matches.begin(), matches.end(),
              [](const auto &left, const auto &right) { return key(left) < key(right); });
    return matches;
}

auto ExtensionRegistry::factory(const ExtensionId &id, std::string_view version) const
    -> Result<std::shared_ptr<const ExtensionFactory>> {
    std::shared_lock lock(mutex_);
    const auto iterator = std::find_if(entries_.begin(), entries_.end(), [&](const auto &entry) {
        return entry.descriptor.data().id == id && entry.descriptor.data().version == version;
    });
    if (iterator == entries_.end()) {
        return Result<std::shared_ptr<const ExtensionFactory>>::failure(
            registry_error("extension.not_found", ErrorCategory::NotFound,
                           "extension identity and version are not registered"));
    }
    return Result<std::shared_ptr<const ExtensionFactory>>::success(iterator->factory);
}

auto ExtensionRegistry::validate_dependencies() const -> Result<void> {
    std::shared_lock lock(mutex_);
    std::map<std::pair<std::string, std::string>, const ExtensionDescriptor *> descriptors;
    for (const auto &entry : entries_) {
        descriptors.emplace(key(entry.descriptor), &entry.descriptor);
    }
    for (const auto &entry : entries_) {
        for (const auto &dependency : entry.descriptor.data().dependencies) {
            if (dependency.requirement != DependencyRequirement::Required) {
                continue;
            }
            const auto dependency_key =
                std::pair{dependency.extension_id.to_string(), dependency.version};
            if (!descriptors.contains(dependency_key)) {
                return Result<void>::failure(registry_error(
                    "extension.missing_dependency", ErrorCategory::NotFound,
                    "required or conditional extension dependency is not registered"));
            }
        }
    }

    enum class VisitState { Visiting, Complete };
    std::map<std::pair<std::string, std::string>, VisitState> states;
    std::function<bool(const std::pair<std::string, std::string> &)> visit;
    visit = [&](const auto &descriptor_key) {
        const auto state = states.find(descriptor_key);
        if (state != states.end()) {
            return state->second == VisitState::Visiting;
        }
        states.emplace(descriptor_key, VisitState::Visiting);
        const auto descriptor = descriptors.at(descriptor_key);
        for (const auto &dependency : descriptor->data().dependencies) {
            const auto dependency_key =
                std::pair{dependency.extension_id.to_string(), dependency.version};
            if (descriptors.contains(dependency_key) && visit(dependency_key)) {
                return true;
            }
        }
        states[descriptor_key] = VisitState::Complete;
        return false;
    };
    for (const auto &[descriptor_key, descriptor] : descriptors) {
        static_cast<void>(descriptor);
        if (visit(descriptor_key)) {
            return Result<void>::failure(
                registry_error("extension.dependency_cycle", ErrorCategory::Conflict,
                               "extension dependency graph contains a cycle"));
        }
    }
    return Result<void>::success();
}

auto ExtensionRegistry::size() const -> std::size_t {
    std::shared_lock lock(mutex_);
    return entries_.size();
}

} // namespace evolution::extensions
