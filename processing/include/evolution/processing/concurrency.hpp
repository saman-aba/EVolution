#pragma once

#include "evolution/core/error/result.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace evolution::processing {

enum class ConcurrencyMode {
    Serial,
    Concurrent,
    Partitioned,
};

enum class OrderingRequirement {
    Unordered,
    Arrival,
    Sequence,
    Partition,
};

enum class StateOwnership {
    Stateless,
    ProcessorOwned,
    PartitionOwned,
    ExternalSynchronized,
};

class ConcurrencyContract {
  public:
    static auto create(ConcurrencyMode mode, std::size_t maximum_parallelism,
                       OrderingRequirement ordering, StateOwnership state_ownership,
                       std::optional<std::string> partition_key = std::nullopt)
        -> Result<ConcurrencyContract> {
        if (maximum_parallelism == 0) {
            return invalid("maximum parallelism must be greater than zero");
        }
        if (mode == ConcurrencyMode::Serial && maximum_parallelism != 1) {
            return invalid("serial processors require maximum parallelism of one");
        }
        if (mode == ConcurrencyMode::Concurrent &&
            state_ownership == StateOwnership::ProcessorOwned) {
            return invalid("concurrent processors cannot use unsynchronized processor-owned state");
        }
        if (mode == ConcurrencyMode::Partitioned) {
            if (!partition_key || partition_key->empty()) {
                return invalid("partitioned concurrency requires a partition key contract");
            }
            if (state_ownership != StateOwnership::PartitionOwned &&
                state_ownership != StateOwnership::Stateless) {
                return invalid(
                    "partitioned concurrency requires partition-owned or stateless work");
            }
        } else if (partition_key) {
            return invalid("only partitioned concurrency may declare a partition key");
        }
        if (mode == ConcurrencyMode::Concurrent && ordering != OrderingRequirement::Unordered) {
            return invalid("unpartitioned concurrent processing cannot promise ordered execution");
        }
        if (ordering == OrderingRequirement::Partition && mode != ConcurrencyMode::Partitioned) {
            return invalid("partition ordering requires partitioned concurrency");
        }
        return Result<ConcurrencyContract>::success(ConcurrencyContract(
            mode, maximum_parallelism, ordering, state_ownership, std::move(partition_key)));
    }

    static auto serial() -> ConcurrencyContract {
        return ConcurrencyContract(ConcurrencyMode::Serial, 1, OrderingRequirement::Arrival,
                                   StateOwnership::ProcessorOwned, std::nullopt);
    }

    [[nodiscard]] auto mode() const noexcept -> ConcurrencyMode {
        return mode_;
    }
    [[nodiscard]] auto maximum_parallelism() const noexcept -> std::size_t {
        return maximum_parallelism_;
    }
    [[nodiscard]] auto ordering() const noexcept -> OrderingRequirement {
        return ordering_;
    }
    [[nodiscard]] auto state_ownership() const noexcept -> StateOwnership {
        return state_ownership_;
    }
    [[nodiscard]] auto partition_key() const noexcept -> const std::optional<std::string> & {
        return partition_key_;
    }

    [[nodiscard]] auto
    may_run_concurrently(std::optional<std::string_view> left_partition,
                         std::optional<std::string_view> right_partition) const noexcept -> bool {
        if (mode_ == ConcurrencyMode::Serial) {
            return false;
        }
        if (mode_ == ConcurrencyMode::Concurrent) {
            return true;
        }
        return left_partition && right_partition && *left_partition != *right_partition;
    }

  private:
    ConcurrencyContract(ConcurrencyMode mode, std::size_t maximum_parallelism,
                        OrderingRequirement ordering, StateOwnership state_ownership,
                        std::optional<std::string> partition_key)
        : mode_(mode), maximum_parallelism_(maximum_parallelism), ordering_(ordering),
          state_ownership_(state_ownership), partition_key_(std::move(partition_key)) {}

    static auto invalid(std::string message) -> Result<ConcurrencyContract> {
        return Result<ConcurrencyContract>::failure(
            Error(ErrorCode::create("processing.invalid_concurrency_contract"),
                  ErrorCategory::InvalidArgument, std::move(message)));
    }

    ConcurrencyMode mode_;
    std::size_t maximum_parallelism_;
    OrderingRequirement ordering_;
    StateOwnership state_ownership_;
    std::optional<std::string> partition_key_;
};

} // namespace evolution::processing
