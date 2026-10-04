#pragma once

#include "evolution/core/identity/identifiers.hpp"
#include "evolution/processing/admission.hpp"
#include "evolution/processing/api.hpp"
#include "evolution/processing/concurrency.hpp"
#include "evolution/processing/resources.hpp"

#include <optional>
#include <string>
#include <vector>

namespace evolution::processing {

struct GraphTag;
struct GraphExecutionTag;
struct NodeTag;
struct PortTag;
struct ConnectionTag;

using GraphId = identity::Id<GraphTag>;
using GraphExecutionId = identity::Id<GraphExecutionTag>;
using NodeId = identity::Id<NodeTag>;
using PortId = identity::Id<PortTag>;
using ConnectionId = identity::Id<ConnectionTag>;

enum class PortDirection {
    Input,
    Output,
};

enum class PortCardinality {
    Single,
    Multiple,
};

enum class TemporalSemantics {
    Unspecified,
    EventTime,
    ProcessingTime,
};

enum class FanOutDelivery {
    Independent,
    AllOrNothing,
};

struct PortDefinition {
    PortId id;
    std::string name;
    PortDirection direction;
    PortCardinality cardinality;
    std::string semantic_type;
    std::string schema_version;
    OrderingRequirement ordering;
    TemporalSemantics temporal_semantics;
    bool required{};
    bool carries_identity{};
    bool carries_provenance{};
};

struct ConfigurationBinding {
    ConfigurationId configuration_id;
    std::string scope;
};

struct NodeDefinition {
    NodeId id;
    ProcessorId processor_id;
    std::string processor_version;
    std::optional<ConfigurationBinding> configuration;
    bool configuration_required{};
    ConcurrencyContract concurrency;
    ResourceRequirements resources;
    std::vector<PortDefinition> ports;
};

struct ConnectionDefinition {
    ConnectionId id;
    NodeId source_node;
    PortId source_port;
    NodeId destination_node;
    PortId destination_port;
    QueueCapacity capacity;
    AdmissionPolicyKind admission_policy;
    OrderingRequirement ordering;
    FanOutDelivery fan_out_delivery;
};

struct GraphBoundaryPort {
    std::string name;
    NodeId node;
    PortId port;
    bool required{};
};

struct CycleContract {
    bool cycles_allowed{};
    std::optional<std::string> termination_condition;
};

class EVOLUTION_PROCESSING_API GraphDefinition {
  public:
    static auto create(GraphId id, std::string version, std::vector<NodeDefinition> nodes,
                       std::vector<ConnectionDefinition> connections,
                       std::vector<GraphBoundaryPort> inputs = {},
                       std::vector<GraphBoundaryPort> outputs = {},
                       CycleContract cycle_contract = {}) -> Result<GraphDefinition>;

    [[nodiscard]] auto id() const noexcept -> const GraphId &;
    [[nodiscard]] auto version() const noexcept -> const std::string &;
    [[nodiscard]] auto nodes() const noexcept -> const std::vector<NodeDefinition> &;
    [[nodiscard]] auto connections() const noexcept -> const std::vector<ConnectionDefinition> &;
    [[nodiscard]] auto inputs() const noexcept -> const std::vector<GraphBoundaryPort> &;
    [[nodiscard]] auto outputs() const noexcept -> const std::vector<GraphBoundaryPort> &;
    [[nodiscard]] auto cycle_contract() const noexcept -> const CycleContract &;

  private:
    GraphDefinition(GraphId id, std::string version, std::vector<NodeDefinition> nodes,
                    std::vector<ConnectionDefinition> connections,
                    std::vector<GraphBoundaryPort> inputs, std::vector<GraphBoundaryPort> outputs,
                    CycleContract cycle_contract);

    GraphId id_;
    std::string version_;
    std::vector<NodeDefinition> nodes_;
    std::vector<ConnectionDefinition> connections_;
    std::vector<GraphBoundaryPort> inputs_;
    std::vector<GraphBoundaryPort> outputs_;
    CycleContract cycle_contract_;
};

struct GraphExecutionDescriptor {
    GraphExecutionId execution_id;
    GraphId graph_id;
    std::string graph_version;
    RunId run_id;
};

} // namespace evolution::processing
