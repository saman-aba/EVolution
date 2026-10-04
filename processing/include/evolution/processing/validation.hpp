#pragma once

#include "evolution/processing/graph.hpp"
#include "evolution/processing/lifecycle.hpp"

#include <optional>
#include <string>

namespace evolution::processing {

enum class ValidationLevel {
    Static,
    Initialization,
    Runtime,
};

struct InitializationValidationContext {
    ResourceSnapshot resources;
};

struct RuntimeValidationContext {
    NodeId node;
    PortId input_port;
    LifecycleState lifecycle;
    bool has_identity{};
    bool has_provenance{};
    std::optional<std::string> partition;
    ResourceSnapshot resources;
};

class EVOLUTION_PROCESSING_API GraphValidator {
  public:
    static auto validate_static(const GraphDefinition &graph) -> Result<void>;
    static auto validate_initialization(const GraphDefinition &graph,
                                        const InitializationValidationContext &context)
        -> Result<void>;
    static auto validate_runtime(const GraphDefinition &graph,
                                 const RuntimeValidationContext &context) -> Result<void>;
};

} // namespace evolution::processing
