#include "evolution/processing/graph.hpp"

#include <utility>

namespace evolution::processing {

GraphDefinition::GraphDefinition(GraphId id, std::string version, std::vector<NodeDefinition> nodes,
                                 std::vector<ConnectionDefinition> connections,
                                 std::vector<GraphBoundaryPort> inputs,
                                 std::vector<GraphBoundaryPort> outputs,
                                 CycleContract cycle_contract)
    : id_(std::move(id)), version_(std::move(version)), nodes_(std::move(nodes)),
      connections_(std::move(connections)), inputs_(std::move(inputs)),
      outputs_(std::move(outputs)), cycle_contract_(std::move(cycle_contract)) {}

auto GraphDefinition::create(GraphId id, std::string version, std::vector<NodeDefinition> nodes,
                             std::vector<ConnectionDefinition> connections,
                             std::vector<GraphBoundaryPort> inputs,
                             std::vector<GraphBoundaryPort> outputs, CycleContract cycle_contract)
    -> Result<GraphDefinition> {
    if (version.empty()) {
        return Result<GraphDefinition>::failure(
            Error(ErrorCode::create("processing.invalid_graph_version"),
                  ErrorCategory::InvalidArgument, "graph version cannot be empty"));
    }
    if (cycle_contract.cycles_allowed &&
        (!cycle_contract.termination_condition || cycle_contract.termination_condition->empty())) {
        return Result<GraphDefinition>::failure(
            Error(ErrorCode::create("processing.incomplete_cycle_contract"),
                  ErrorCategory::InvalidArgument,
                  "graphs permitting cycles require explicit termination semantics"));
    }
    if (!cycle_contract.cycles_allowed && cycle_contract.termination_condition) {
        return Result<GraphDefinition>::failure(Error(
            ErrorCode::create("processing.unused_cycle_contract"), ErrorCategory::InvalidArgument,
            "acyclic graphs cannot declare cycle termination semantics"));
    }
    return Result<GraphDefinition>::success(
        GraphDefinition(std::move(id), std::move(version), std::move(nodes), std::move(connections),
                        std::move(inputs), std::move(outputs), std::move(cycle_contract)));
}

auto GraphDefinition::id() const noexcept -> const GraphId & {
    return id_;
}

auto GraphDefinition::version() const noexcept -> const std::string & {
    return version_;
}

auto GraphDefinition::nodes() const noexcept -> const std::vector<NodeDefinition> & {
    return nodes_;
}

auto GraphDefinition::connections() const noexcept -> const std::vector<ConnectionDefinition> & {
    return connections_;
}

auto GraphDefinition::inputs() const noexcept -> const std::vector<GraphBoundaryPort> & {
    return inputs_;
}

auto GraphDefinition::outputs() const noexcept -> const std::vector<GraphBoundaryPort> & {
    return outputs_;
}

auto GraphDefinition::cycle_contract() const noexcept -> const CycleContract & {
    return cycle_contract_;
}

} // namespace evolution::processing
