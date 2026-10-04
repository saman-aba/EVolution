#pragma once

#include "evolution/core/context/cancellation.hpp"
#include "evolution/core/error/result.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace evolution::processing {

enum class AdmissionPolicyKind {
    Reject,
    Wait,
    Drop,
    Sample,
    Spill,
};

enum class AdmissionDecision {
    Accepted,
    Rejected,
    Dropped,
    SampledOut,
    Spilled,
    Cancelled,
    TimedOut,
};

class AdmissionPolicy {
  public:
    static auto reject() -> AdmissionPolicy {
        return AdmissionPolicy(AdmissionPolicyKind::Reject);
    }
    static auto drop() -> AdmissionPolicy {
        return AdmissionPolicy(AdmissionPolicyKind::Drop);
    }
    static auto spill() -> AdmissionPolicy {
        return AdmissionPolicy(AdmissionPolicyKind::Spill);
    }

    static auto wait(std::chrono::milliseconds timeout) -> Result<AdmissionPolicy> {
        if (timeout <= std::chrono::milliseconds::zero()) {
            return invalid("wait timeout must be positive");
        }
        auto policy = AdmissionPolicy(AdmissionPolicyKind::Wait);
        policy.timeout_ = timeout;
        return Result<AdmissionPolicy>::success(policy);
    }

    static auto sample(std::uint64_t every_nth) -> Result<AdmissionPolicy> {
        if (every_nth == 0) {
            return invalid("sample interval must be greater than zero");
        }
        auto policy = AdmissionPolicy(AdmissionPolicyKind::Sample);
        policy.sample_every_nth_ = every_nth;
        return Result<AdmissionPolicy>::success(policy);
    }

    [[nodiscard]] auto kind() const noexcept -> AdmissionPolicyKind {
        return kind_;
    }
    [[nodiscard]] auto timeout() const noexcept -> std::optional<std::chrono::milliseconds> {
        return timeout_;
    }
    [[nodiscard]] auto sample_every_nth() const noexcept -> std::optional<std::uint64_t> {
        return sample_every_nth_;
    }

  private:
    explicit AdmissionPolicy(AdmissionPolicyKind kind) : kind_(kind) {}

    static auto invalid(std::string message) -> Result<AdmissionPolicy> {
        return Result<AdmissionPolicy>::failure(
            Error(ErrorCode::create("processing.invalid_admission_policy"),
                  ErrorCategory::InvalidArgument, std::move(message)));
    }

    AdmissionPolicyKind kind_;
    std::optional<std::chrono::milliseconds> timeout_;
    std::optional<std::uint64_t> sample_every_nth_;
};

struct QueueCapacity {
    std::size_t maximum_items;
    std::size_t maximum_bytes;
    std::uint64_t maximum_weight;
};

struct QueuePressure {
    std::size_t items;
    std::size_t bytes;
    std::uint64_t weight;
    double item_ratio;
    double byte_ratio;
    double weight_ratio;
    bool closed;
};

enum class QueueOrdering {
    Fifo,
};

enum class QueueCloseMode {
    Drain,
    Discard,
};

template <typename T> struct QueueItem {
    T value;
    std::size_t size_bytes;
    std::uint64_t weight;
};

template <typename T> struct AdmissionResult {
    AdmissionDecision decision;
    std::optional<QueueItem<T>> returned_item;
};

template <typename T> using SpillHandler = std::function<Result<void>(const QueueItem<T> &)>;

} // namespace evolution::processing
