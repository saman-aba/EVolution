#pragma once

#include "evolution/core/context/cancellation.hpp"
#include "evolution/processing/admission.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace evolution::processing {

template <typename T> class BoundedQueue {
  public:
    static auto create(QueueCapacity capacity, QueueOrdering ordering = QueueOrdering::Fifo,
                       SpillHandler<T> spill_handler = {})
        -> Result<std::unique_ptr<BoundedQueue>> {
        if (capacity.maximum_items == 0 || capacity.maximum_bytes == 0 ||
            capacity.maximum_weight == 0) {
            return Result<std::unique_ptr<BoundedQueue>>::failure(
                Error(ErrorCode::create("processing.invalid_queue_capacity"),
                      ErrorCategory::InvalidArgument,
                      "queue item, byte, and weight capacities must all be explicitly bounded"));
        }
        return Result<std::unique_ptr<BoundedQueue>>::success(std::unique_ptr<BoundedQueue>(
            new BoundedQueue(capacity, ordering, std::move(spill_handler))));
    }

    auto admit(QueueItem<T> item, const AdmissionPolicy &policy,
               const context::CancellationToken &cancellation = {}) -> AdmissionResult<T> {
        if (!can_ever_fit(item)) {
            return AdmissionResult<T>{AdmissionDecision::Rejected, std::move(item)};
        }

        std::unique_lock lock(mutex_);
        ++admission_sequence_;
        if (policy.kind() == AdmissionPolicyKind::Sample &&
            admission_sequence_ % *policy.sample_every_nth() != 0) {
            return AdmissionResult<T>{AdmissionDecision::SampledOut, std::nullopt};
        }

        if (fits(item)) {
            push(std::move(item));
            return AdmissionResult<T>{AdmissionDecision::Accepted, std::nullopt};
        }

        switch (policy.kind()) {
        case AdmissionPolicyKind::Reject:
        case AdmissionPolicyKind::Sample:
            return AdmissionResult<T>{AdmissionDecision::Rejected, std::move(item)};
        case AdmissionPolicyKind::Drop:
            return AdmissionResult<T>{AdmissionDecision::Dropped, std::nullopt};
        case AdmissionPolicyKind::Spill: {
            if (!spill_handler_) {
                return AdmissionResult<T>{AdmissionDecision::Rejected, std::move(item)};
            }
            lock.unlock();
            auto spilled = spill_handler_(item);
            if (spilled.has_error()) {
                return AdmissionResult<T>{AdmissionDecision::Rejected, std::move(item)};
            }
            return AdmissionResult<T>{AdmissionDecision::Spilled, std::nullopt};
        }
        case AdmissionPolicyKind::Wait:
            break;
        }

        const auto deadline = std::chrono::steady_clock::now() + *policy.timeout();
        while (!fits(item) && !closed_) {
            if (cancellation.is_cancelled()) {
                return AdmissionResult<T>{AdmissionDecision::Cancelled, std::move(item)};
            }
            if (condition_.wait_until(lock, std::min(deadline, std::chrono::steady_clock::now() +
                                                                   std::chrono::milliseconds(5))) ==
                    std::cv_status::timeout &&
                std::chrono::steady_clock::now() >= deadline) {
                return AdmissionResult<T>{AdmissionDecision::TimedOut, std::move(item)};
            }
        }
        if (closed_) {
            return AdmissionResult<T>{AdmissionDecision::Rejected, std::move(item)};
        }
        push(std::move(item));
        return AdmissionResult<T>{AdmissionDecision::Accepted, std::nullopt};
    }

    [[nodiscard]] auto try_pop() -> std::optional<QueueItem<T>> {
        std::unique_lock lock(mutex_);
        if (items_.empty()) {
            return std::nullopt;
        }
        auto item = std::move(items_.front());
        items_.pop_front();
        release(item);
        lock.unlock();
        condition_.notify_all();
        return item;
    }

    [[nodiscard]] auto wait_pop(std::chrono::milliseconds timeout,
                                const context::CancellationToken &cancellation = {})
        -> Result<std::optional<QueueItem<T>>> {
        if (timeout <= std::chrono::milliseconds::zero()) {
            return Result<std::optional<QueueItem<T>>>::failure(
                Error(ErrorCode::create("processing.invalid_queue_timeout"),
                      ErrorCategory::InvalidArgument, "queue wait timeout must be positive"));
        }
        std::unique_lock lock(mutex_);
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (items_.empty() && !closed_) {
            if (cancellation.is_cancelled()) {
                return Result<std::optional<QueueItem<T>>>::failure(
                    Error(ErrorCode::create("processing.queue_wait_cancelled"),
                          ErrorCategory::Cancelled, "queue wait was cancelled"));
            }
            if (condition_.wait_until(lock, std::min(deadline, std::chrono::steady_clock::now() +
                                                                   std::chrono::milliseconds(5))) ==
                    std::cv_status::timeout &&
                std::chrono::steady_clock::now() >= deadline) {
                return Result<std::optional<QueueItem<T>>>::success(std::nullopt);
            }
        }
        if (items_.empty()) {
            return Result<std::optional<QueueItem<T>>>::success(std::nullopt);
        }
        auto item = std::move(items_.front());
        items_.pop_front();
        release(item);
        lock.unlock();
        condition_.notify_all();
        return Result<std::optional<QueueItem<T>>>::success(std::move(item));
    }

    [[nodiscard]] auto pressure() const -> QueuePressure {
        const std::scoped_lock lock(mutex_);
        return QueuePressure{
            items_.size(),
            used_bytes_,
            used_weight_,
            static_cast<double>(items_.size()) / static_cast<double>(capacity_.maximum_items),
            static_cast<double>(used_bytes_) / static_cast<double>(capacity_.maximum_bytes),
            static_cast<double>(used_weight_) / static_cast<double>(capacity_.maximum_weight),
            closed_,
        };
    }

    [[nodiscard]] auto ordering() const noexcept -> QueueOrdering {
        return ordering_;
    }

    auto close(QueueCloseMode mode) -> std::vector<QueueItem<T>> {
        std::unique_lock lock(mutex_);
        closed_ = true;
        std::vector<QueueItem<T>> discarded;
        if (mode == QueueCloseMode::Discard) {
            discarded.reserve(items_.size());
            while (!items_.empty()) {
                discarded.push_back(std::move(items_.front()));
                items_.pop_front();
            }
            used_bytes_ = 0;
            used_weight_ = 0;
        }
        lock.unlock();
        condition_.notify_all();
        return discarded;
    }

    [[nodiscard]] auto drain() -> std::vector<QueueItem<T>> {
        std::unique_lock lock(mutex_);
        std::vector<QueueItem<T>> drained;
        drained.reserve(items_.size());
        while (!items_.empty()) {
            drained.push_back(std::move(items_.front()));
            items_.pop_front();
        }
        used_bytes_ = 0;
        used_weight_ = 0;
        lock.unlock();
        condition_.notify_all();
        return drained;
    }

  private:
    BoundedQueue(QueueCapacity capacity, QueueOrdering ordering, SpillHandler<T> spill_handler)
        : capacity_(capacity), ordering_(ordering), spill_handler_(std::move(spill_handler)) {}

    [[nodiscard]] auto can_ever_fit(const QueueItem<T> &item) const noexcept -> bool {
        return item.size_bytes <= capacity_.maximum_bytes &&
               item.weight <= capacity_.maximum_weight;
    }

    [[nodiscard]] auto fits(const QueueItem<T> &item) const noexcept -> bool {
        return !closed_ && items_.size() < capacity_.maximum_items &&
               used_bytes_ <= capacity_.maximum_bytes - item.size_bytes &&
               used_weight_ <= capacity_.maximum_weight - item.weight;
    }

    void push(QueueItem<T> item) {
        used_bytes_ += item.size_bytes;
        used_weight_ += item.weight;
        items_.push_back(std::move(item));
        condition_.notify_all();
    }

    void release(const QueueItem<T> &item) noexcept {
        used_bytes_ -= item.size_bytes;
        used_weight_ -= item.weight;
    }

    QueueCapacity capacity_;
    QueueOrdering ordering_;
    SpillHandler<T> spill_handler_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<QueueItem<T>> items_;
    std::size_t used_bytes_{};
    std::uint64_t used_weight_{};
    std::uint64_t admission_sequence_{};
    bool closed_{};
};

} // namespace evolution::processing
