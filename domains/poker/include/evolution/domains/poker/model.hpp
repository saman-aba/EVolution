#pragma once

#include "evolution/core/error/result.hpp"
#include "evolution/core/event/event.hpp"
#include "evolution/core/identity/id.hpp"
#include "evolution/core/measurement/measurement.hpp"
#include "evolution/domain/contracts.hpp"
#include "evolution/domains/poker/api.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace evolution::domains::poker {

struct PlayerTag;
struct HandTag;

using PlayerId = identity::Id<PlayerTag>;
using HandId = identity::Id<HandTag>;

struct Player {
    PlayerId id;
    std::string display_name;
};

enum class ActionKind {
    Fold,
    Check,
    Call,
    Bet,
    Raise,
};

struct HandStarted {
    HandId hand_id;
    std::vector<PlayerId> players;
    std::int64_t initial_pot{};
};

struct PlayerActed {
    HandId hand_id;
    PlayerId player_id;
    ActionKind action;
    std::int64_t amount{};
};

struct HandFinished {
    HandId hand_id;
    std::optional<PlayerId> winner;
};

using PokerEventPayload = std::variant<HandStarted, PlayerActed, HandFinished>;
using PokerEvent = Event<PokerEventPayload>;

enum class HandStatus {
    InProgress,
    Finished,
};

struct HandState {
    HandId hand_id;
    std::vector<PlayerId> players;
    std::int64_t pot{};
    std::size_t action_count{};
    HandStatus status{HandStatus::InProgress};
    std::optional<PlayerId> winner;
};

EVOLUTION_DOMAIN_POKER_API auto create_player(PlayerId id, std::string display_name)
    -> Result<Player>;
EVOLUTION_DOMAIN_POKER_API auto start_hand(const HandStarted &event) -> Result<HandState>;
EVOLUTION_DOMAIN_POKER_API auto apply_event(const HandState &state, const PokerEventPayload &event)
    -> Result<HandState>;
EVOLUTION_DOMAIN_POKER_API auto validate_hand(const HandState &state)
    -> Result<domain::DomainValidationReport>;
EVOLUTION_DOMAIN_POKER_API auto voluntary_put_money_in_pot_metric() -> Result<Metric>;

} // namespace evolution::domains::poker
