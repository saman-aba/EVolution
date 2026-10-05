#pragma once

#include "evolution/analysis/common.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/time/time.hpp"

#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace evolution::analysis {

struct AggregationTag;
using AggregationId = identity::Id<AggregationTag>;

enum class AggregationWindowKind { None, Time, ObservationCount, Event, Session, Domain };
enum class AggregationWindowMode { Tumbling, Sliding, Rolling, Expanding };

struct AggregationWindow {
    AggregationWindowKind kind{AggregationWindowKind::None};
    AggregationWindowMode mode{AggregationWindowMode::Tumbling};
    std::optional<std::chrono::nanoseconds> duration;
    std::optional<std::size_t> count;
    std::optional<std::string> boundary_key;
    std::optional<std::chrono::nanoseconds> step;
};

enum class MissingValuePolicy { Exclude, Propagate, Fail, UseIdentity };
enum class UnknownValuePolicy { Exclude, Propagate, Fail };
enum class EmptyInputPolicy { NoResult, Identity, Fail };
enum class AggregationOrderDirection { Ascending, Descending };

struct AggregationOrdering {
    bool required{};
    std::optional<std::string> field;
    AggregationOrderDirection direction{AggregationOrderDirection::Ascending};
    std::optional<std::string> tie_breaker;
};

struct AggregationCapabilities {
    bool incremental{};
    bool mergeable{};
    bool retractable{};
};

struct AlgebraicProperties {
    bool associative{};
    bool commutative{};
    bool deterministic{};
    bool order_sensitive{};
};

struct AggregationDefinitionData {
    AggregationId id;
    std::string name;
    std::string version;
    AlgorithmId function_id;
    std::string function_version;
    AggregationWindow window;
    std::vector<std::string> grouping_dimensions;
    AggregationOrdering ordering;
    MissingValuePolicy missing_values{MissingValuePolicy::Exclude};
    UnknownValuePolicy unknown_values{UnknownValuePolicy::Propagate};
    EmptyInputPolicy empty_input{EmptyInputPolicy::NoResult};
    AggregationCapabilities capabilities;
    AlgebraicProperties algebra;
};

class EVOLUTION_ANALYSIS_API AggregationDefinition {
  public:
    static auto create(AggregationDefinitionData data) -> Result<AggregationDefinition>;
    [[nodiscard]] auto data() const noexcept -> const AggregationDefinitionData &;

  private:
    explicit AggregationDefinition(AggregationDefinitionData data);
    AggregationDefinitionData data_;
};

enum class AggregationCompletion { Complete, Partial, Empty, Unknown };

template <typename Value> struct AggregationResultData {
    AggregationId definition_id;
    std::string definition_version;
    AggregationCompletion completion{AggregationCompletion::Complete};
    std::optional<Value> value;
    std::map<std::string, std::string> group;
    AggregationWindow window;
    std::size_t input_count{};
    std::size_t included_count{};
    std::size_t missing_count{};
    std::size_t unknown_count{};
    time::TemporalInformation output_time{time::TemporalInformation::unknown()};
    std::optional<time::TimeRange> output_interval;
    ProvenanceId provenance_id;
    AnalyticalReproducibility reproducibility;
};

template <typename Value> class AggregationResult {
  public:
    static auto create(AggregationResultData<Value> data) -> Result<AggregationResult> {
        if (data.definition_version.empty()) {
            return failure("aggregation result requires a definition version");
        }
        if (data.included_count + data.missing_count + data.unknown_count > data.input_count) {
            return failure("aggregation result counts exceed total input count");
        }
        const bool requires_value = data.completion == AggregationCompletion::Complete ||
                                    data.completion == AggregationCompletion::Partial;
        if (requires_value != data.value.has_value()) {
            return failure("aggregation completion and value presence are inconsistent");
        }
        if (data.completion == AggregationCompletion::Complete &&
            data.included_count != data.input_count - data.missing_count - data.unknown_count) {
            return failure("complete aggregation result has inconsistent included input count");
        }
        for (const auto &[dimension, group_value] : data.group) {
            if (dimension.empty() || group_value.empty()) {
                return failure("aggregation group dimensions cannot be empty");
            }
        }
        auto reproducibility = validate_reproducibility(data.reproducibility);
        if (!reproducibility) {
            return Result<AggregationResult>::failure(reproducibility.error());
        }
        return Result<AggregationResult>::success(AggregationResult(std::move(data)));
    }

    [[nodiscard]] auto data() const noexcept -> const AggregationResultData<Value> & {
        return data_;
    }

  private:
    explicit AggregationResult(AggregationResultData<Value> data) : data_(std::move(data)) {}

    static auto failure(std::string message) -> Result<AggregationResult> {
        return Result<AggregationResult>::failure(
            Error(ErrorCode::create("analysis.invalid_aggregation_result"),
                  ErrorCategory::InvalidArgument, std::move(message)));
    }

    AggregationResultData<Value> data_;
};

template <typename Input, typename State, typename Output> class IncrementalAggregator {
  public:
    virtual ~IncrementalAggregator() = default;
    [[nodiscard]] virtual auto definition() const noexcept -> const AggregationDefinition & = 0;
    virtual auto initial_state() const -> Result<State> = 0;
    virtual auto add(State state, const Input &input) const -> Result<State> = 0;
    virtual auto merge(State left, const State &right) const -> Result<State> = 0;
    virtual auto retract(State state, const Input &input) const -> Result<State> = 0;
    virtual auto finish(const State &state) const -> Result<Output> = 0;
};

} // namespace evolution::analysis
