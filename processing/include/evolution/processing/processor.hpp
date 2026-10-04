#pragma once

#include "evolution/core/configuration/configuration.hpp"
#include "evolution/core/context/execution_context.hpp"
#include "evolution/core/error/result.hpp"
#include "evolution/core/processing/envelope.hpp"

#include <concepts>
#include <optional>
#include <utility>
#include <vector>

namespace evolution::processing {

enum class OperationOutcome {
    Success,
    NoOutput,
    PartialSuccess,
    Cancelled,
};

template <typename Output, typename State> struct ProcessingOutcome {
    OperationOutcome outcome;
    std::vector<Envelope<Output>> outputs;
    std::optional<State> updated_state;
    std::vector<Error> partial_failures;

    static auto success(std::vector<Envelope<Output>> outputs,
                        std::optional<State> state = std::nullopt) -> ProcessingOutcome {
        return ProcessingOutcome{
            outputs.empty() ? OperationOutcome::NoOutput : OperationOutcome::Success,
            std::move(outputs),
            std::move(state),
            {},
        };
    }

    static auto partial(std::vector<Envelope<Output>> outputs, std::optional<State> state,
                        std::vector<Error> failures) -> ProcessingOutcome {
        return ProcessingOutcome{
            OperationOutcome::PartialSuccess,
            std::move(outputs),
            std::move(state),
            std::move(failures),
        };
    }

    static auto cancelled(std::optional<State> state = std::nullopt) -> ProcessingOutcome {
        return ProcessingOutcome{
            OperationOutcome::Cancelled,
            {},
            std::move(state),
            {},
        };
    }
};

template <typename Input, typename Output, typename State> class Processor {
  public:
    virtual ~Processor() = default;

    virtual auto process(Envelope<Input> input, std::optional<State> state,
                         const Configuration &configuration,
                         const ExecutionContext &execution_context) const
        -> Result<ProcessingOutcome<Output, State>> = 0;
};

} // namespace evolution::processing
