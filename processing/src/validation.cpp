#include "evolution/processing/validation.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace evolution::processing {
namespace {

auto invalid(std::string code, std::string message) -> Result<void> {
    return Result<void>::failure(Error(ErrorCode::create(std::move(code)),
                                       ErrorCategory::InvalidConfiguration, std::move(message)));
}

auto find_node(const GraphDefinition &graph, const NodeId &id) -> const NodeDefinition * {
    for (const auto &node : graph.nodes()) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

auto find_port(const NodeDefinition &node, const PortId &id) -> const PortDefinition * {
    for (const auto &port : node.ports) {
        if (port.id == id) {
            return &port;
        }
    }
    return nullptr;
}

auto ordering_compatible(OrderingRequirement provided, OrderingRequirement required) noexcept
    -> bool {
    return required == OrderingRequirement::Unordered || provided == required;
}

auto has_cycle(const GraphDefinition &graph) -> bool {
    enum class VisitState {
        Unvisited,
        Visiting,
        Visited,
    };

    std::unordered_map<NodeId, std::vector<NodeId>> adjacency;
    std::unordered_map<NodeId, VisitState> state;
    for (const auto &node : graph.nodes()) {
        adjacency[node.id];
        state[node.id] = VisitState::Unvisited;
    }
    for (const auto &connection : graph.connections()) {
        adjacency[connection.source_node].push_back(connection.destination_node);
    }

    std::function<bool(const NodeId &)> visit = [&](const NodeId &node) {
        state[node] = VisitState::Visiting;
        for (const auto &destination : adjacency[node]) {
            if (state[destination] == VisitState::Visiting) {
                return true;
            }
            if (state[destination] == VisitState::Unvisited && visit(destination)) {
                return true;
            }
        }
        state[node] = VisitState::Visited;
        return false;
    };

    for (const auto &node : graph.nodes()) {
        if (state[node.id] == VisitState::Unvisited && visit(node.id)) {
            return true;
        }
    }
    return false;
}

auto validate_boundary(const GraphDefinition &graph, const GraphBoundaryPort &boundary,
                       PortDirection expected_direction) -> Result<void> {
    if (boundary.name.empty()) {
        return invalid("processing.invalid_graph_boundary", "graph boundary name cannot be empty");
    }
    const auto *node = find_node(graph, boundary.node);
    if (!node) {
        return invalid("processing.invalid_graph_boundary",
                       "graph boundary references an unknown node");
    }
    const auto *port = find_port(*node, boundary.port);
    if (!port || port->direction != expected_direction) {
        return invalid("processing.invalid_graph_boundary",
                       "graph boundary references an incompatible port");
    }
    return Result<void>::success();
}

} // namespace

auto GraphValidator::validate_static(const GraphDefinition &graph) -> Result<void> {
    if (graph.nodes().empty()) {
        return invalid("processing.empty_graph", "processing graph must contain at least one node");
    }

    std::unordered_set<NodeId> node_ids;
    for (const auto &node : graph.nodes()) {
        if (!node_ids.insert(node.id).second) {
            return invalid("processing.duplicate_node",
                           "processing graph contains duplicate node identities");
        }
        if (node.processor_version.empty()) {
            return invalid("processing.invalid_processor_version",
                           "graph nodes require a processor version");
        }
        if (node.configuration && node.configuration->scope.empty()) {
            return invalid("processing.invalid_configuration_binding",
                           "configuration binding scope cannot be empty");
        }
        std::unordered_set<PortId> port_ids;
        std::unordered_set<std::string> port_names;
        for (const auto &port : node.ports) {
            if (!port_ids.insert(port.id).second || !port_names.insert(port.name).second) {
                return invalid("processing.duplicate_port",
                               "node contains duplicate port identity or name");
            }
            if (port.name.empty() || port.semantic_type.empty() || port.schema_version.empty()) {
                return invalid("processing.invalid_port_contract",
                               "ports require name, semantic type, and schema version");
            }
        }
    }

    std::unordered_set<ConnectionId> connection_ids;
    std::unordered_map<PortId, std::size_t> incoming_connections;
    for (const auto &connection : graph.connections()) {
        if (!connection_ids.insert(connection.id).second) {
            return invalid("processing.duplicate_connection",
                           "processing graph contains duplicate connection identities");
        }
        if (connection.capacity.maximum_items == 0 || connection.capacity.maximum_bytes == 0 ||
            connection.capacity.maximum_weight == 0) {
            return invalid("processing.unbounded_connection",
                           "every graph connection requires explicit bounded capacity");
        }
        const auto *source_node = find_node(graph, connection.source_node);
        const auto *destination_node = find_node(graph, connection.destination_node);
        if (!source_node || !destination_node) {
            return invalid("processing.invalid_connection_endpoint",
                           "connection references an unknown node");
        }
        const auto *source_port = find_port(*source_node, connection.source_port);
        const auto *destination_port = find_port(*destination_node, connection.destination_port);
        if (!source_port || !destination_port || source_port->direction != PortDirection::Output ||
            destination_port->direction != PortDirection::Input) {
            return invalid("processing.invalid_connection_endpoint",
                           "connection must link an output port to an input port");
        }
        if (source_port->semantic_type != destination_port->semantic_type ||
            source_port->schema_version != destination_port->schema_version) {
            return invalid("processing.incompatible_connection_type",
                           "connection endpoints have incompatible semantic types or schemas");
        }
        if (!ordering_compatible(connection.ordering, destination_port->ordering) ||
            !ordering_compatible(connection.ordering, destination_node->concurrency.ordering())) {
            return invalid("processing.incompatible_ordering",
                           "connection does not preserve required destination ordering");
        }
        if (source_port->temporal_semantics != TemporalSemantics::Unspecified &&
            destination_port->temporal_semantics != TemporalSemantics::Unspecified &&
            source_port->temporal_semantics != destination_port->temporal_semantics) {
            return invalid("processing.incompatible_temporal_semantics",
                           "connection changes required temporal semantics");
        }
        if (destination_port->carries_identity && !source_port->carries_identity) {
            return invalid("processing.identity_not_preserved",
                           "connection source does not provide required identity");
        }
        if (destination_port->carries_provenance && !source_port->carries_provenance) {
            return invalid("processing.provenance_not_preserved",
                           "connection source does not provide required provenance");
        }
        ++incoming_connections[destination_port->id];
        if (destination_port->cardinality == PortCardinality::Single &&
            incoming_connections[destination_port->id] > 1) {
            return invalid("processing.invalid_fan_in",
                           "single-cardinality input has multiple producers");
        }
    }

    std::unordered_set<std::string> boundary_names;
    std::unordered_set<PortId> external_inputs;
    for (const auto &input : graph.inputs()) {
        if (!boundary_names.insert("input:" + input.name).second) {
            return invalid("processing.duplicate_graph_boundary",
                           "graph contains duplicate input boundary names");
        }
        auto result = validate_boundary(graph, input, PortDirection::Input);
        if (result.has_error()) {
            return result;
        }
        external_inputs.insert(input.port);
    }
    for (const auto &output : graph.outputs()) {
        if (!boundary_names.insert("output:" + output.name).second) {
            return invalid("processing.duplicate_graph_boundary",
                           "graph contains duplicate output boundary names");
        }
        auto result = validate_boundary(graph, output, PortDirection::Output);
        if (result.has_error()) {
            return result;
        }
    }

    for (const auto &node : graph.nodes()) {
        for (const auto &port : node.ports) {
            if (port.direction == PortDirection::Input && port.required &&
                !incoming_connections.contains(port.id) && !external_inputs.contains(port.id)) {
                return invalid("processing.missing_required_input",
                               "required graph input has no producer or graph boundary");
            }
        }
    }

    if (has_cycle(graph) && !graph.cycle_contract().cycles_allowed) {
        return invalid("processing.graph_cycle",
                       "processing graph is cyclic without an explicit cycle contract");
    }
    return Result<void>::success();
}

auto GraphValidator::validate_initialization(const GraphDefinition &graph,
                                             const InitializationValidationContext &context)
    -> Result<void> {
    auto static_result = validate_static(graph);
    if (static_result.has_error()) {
        return static_result;
    }
    for (const auto &node : graph.nodes()) {
        if (node.configuration_required && !node.configuration) {
            return invalid("processing.missing_node_configuration",
                           "node requires an effective configuration binding");
        }
        if (!context.resources.can_satisfy(node.resources)) {
            return Result<void>::failure(
                Error(ErrorCode::create("processing.insufficient_initialization_resources"),
                      ErrorCategory::ResourceExhausted,
                      "initialization resources cannot satisfy graph requirements"));
        }
    }
    return Result<void>::success();
}

auto GraphValidator::validate_runtime(const GraphDefinition &graph,
                                      const RuntimeValidationContext &context) -> Result<void> {
    const auto *node = find_node(graph, context.node);
    if (!node) {
        return invalid("processing.unknown_runtime_node",
                       "runtime work references an unknown node");
    }
    const auto *port = find_port(*node, context.input_port);
    if (!port || port->direction != PortDirection::Input) {
        return invalid("processing.unknown_runtime_port",
                       "runtime work references an unknown input port");
    }
    if (context.lifecycle != LifecycleState::Active) {
        return Result<void>::failure(
            Error(ErrorCode::create("processing.runtime_lifecycle_blocked"),
                  ErrorCategory::Conflict, "runtime work requires an active processor"));
    }
    if (port->carries_identity && !context.has_identity) {
        return invalid("processing.runtime_identity_missing",
                       "runtime input is missing required identity");
    }
    if (port->carries_provenance && !context.has_provenance) {
        return invalid("processing.runtime_provenance_missing",
                       "runtime input is missing required provenance");
    }
    if (node->concurrency.mode() == ConcurrencyMode::Partitioned &&
        (!context.partition || context.partition->empty())) {
        return invalid("processing.runtime_partition_missing",
                       "partitioned processor work requires a partition value");
    }
    if (!context.resources.can_satisfy(node->resources)) {
        return Result<void>::failure(
            Error(ErrorCode::create("processing.runtime_resource_exhausted"),
                  ErrorCategory::ResourceExhausted,
                  "runtime resources cannot satisfy processor requirements"));
    }
    return Result<void>::success();
}

} // namespace evolution::processing
