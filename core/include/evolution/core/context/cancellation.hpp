#pragma once

#include <atomic>
#include <memory>

namespace evolution::context {
namespace detail {

struct CancellationState {
    std::atomic_bool cancelled{false};
};

} // namespace detail

class CancellationToken {
  public:
    CancellationToken() = default;

    [[nodiscard]] auto is_cancelled() const noexcept -> bool {
        return state_ && state_->cancelled.load(std::memory_order_acquire);
    }

  private:
    explicit CancellationToken(std::shared_ptr<const detail::CancellationState> state)
        : state_(std::move(state)) {}

    std::shared_ptr<const detail::CancellationState> state_;

    friend class CancellationSource;
};

class CancellationSource {
  public:
    CancellationSource() : state_(std::make_shared<detail::CancellationState>()) {}

    [[nodiscard]] auto token() const noexcept -> CancellationToken {
        return CancellationToken(state_);
    }

    void cancel() const noexcept {
        state_->cancelled.store(true, std::memory_order_release);
    }

  private:
    std::shared_ptr<detail::CancellationState> state_;
};

} // namespace evolution::context
