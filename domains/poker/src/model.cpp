#include "evolution/domains/poker/model.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <utility>

namespace evolution::domains::poker {
namespace {

auto invalid(std::string code, std::string message) -> Error {
    return Error(ErrorCode::create(std::move(code)), ErrorCategory::InvalidArgument,
                 std::move(message));
}

auto contains_player(const HandState &state, const PlayerId &player) -> bool {
    return std::find(state.players.begin(), state.players.end(), player) != state.players.end();
}

} // namespace

auto create_player(PlayerId id, std::string display_name) -> Result<Player> {
    if (display_name.empty()) {
        return Result<Player>::failure(
            invalid("poker.invalid_player", "player display name cannot be empty"));
    }
    return Result<Player>::success(Player{std::move(id), std::move(display_name)});
}

auto start_hand(const HandStarted &event) -> Result<HandState> {
    if (event.players.size() < 2) {
        return Result<HandState>::failure(
            invalid("poker.insufficient_players", "a poker hand requires at least two players"));
    }
    if (event.initial_pot < 0) {
        return Result<HandState>::failure(
            invalid("poker.negative_pot", "initial pot cannot be negative"));
    }
    std::set<std::string> players;
    for (const auto &player : event.players) {
        players.insert(player.to_string());
    }
    if (players.size() != event.players.size()) {
        return Result<HandState>::failure(
            invalid("poker.duplicate_player", "a player cannot appear twice in one hand"));
    }
    return Result<HandState>::success(HandState{event.hand_id, event.players, event.initial_pot, 0,
                                                HandStatus::InProgress, std::nullopt});
}

auto apply_event(const HandState &state, const PokerEventPayload &event) -> Result<HandState> {
    if (state.status == HandStatus::Finished) {
        return Result<HandState>::failure(
            invalid("poker.hand_finished", "finished poker hands cannot accept more events"));
    }
    return std::visit(
        [&](const auto &payload) -> Result<HandState> {
            using Payload = std::decay_t<decltype(payload)>;
            if (payload.hand_id != state.hand_id) {
                return Result<HandState>::failure(
                    invalid("poker.hand_mismatch", "event belongs to a different poker hand"));
            }
            if constexpr (std::is_same_v<Payload, HandStarted>) {
                return Result<HandState>::failure(
                    invalid("poker.hand_already_started", "a hand cannot be started twice"));
            } else if constexpr (std::is_same_v<Payload, PlayerActed>) {
                if (!contains_player(state, payload.player_id)) {
                    return Result<HandState>::failure(invalid(
                        "poker.unknown_player", "action player is not participating in the hand"));
                }
                const bool contributes = payload.action == ActionKind::Call ||
                                         payload.action == ActionKind::Bet ||
                                         payload.action == ActionKind::Raise;
                if ((contributes && payload.amount <= 0) || (!contributes && payload.amount != 0)) {
                    return Result<HandState>::failure(invalid(
                        "poker.invalid_action_amount", "poker action amount is inconsistent"));
                }
                if (payload.amount > 0 &&
                    state.pot > std::numeric_limits<std::int64_t>::max() - payload.amount) {
                    return Result<HandState>::failure(
                        invalid("poker.pot_overflow", "poker pot would exceed its value range"));
                }
                auto next = state;
                next.pot += payload.amount;
                ++next.action_count;
                return Result<HandState>::success(std::move(next));
            } else {
                if (payload.winner && !contains_player(state, *payload.winner)) {
                    return Result<HandState>::failure(
                        invalid("poker.invalid_winner", "winner is not participating in the hand"));
                }
                auto next = state;
                next.status = HandStatus::Finished;
                next.winner = payload.winner;
                return Result<HandState>::success(std::move(next));
            }
        },
        event);
}

auto validate_hand(const HandState &state) -> Result<domain::DomainValidationReport> {
    std::vector<domain::DomainViolation> violations;
    if (state.players.size() < 2) {
        violations.push_back({"poker.hand.participants", "poker.insufficient_players", "players",
                              "a poker hand requires at least two players",
                              domain::DomainViolationSeverity::Error});
    }
    if (state.pot < 0) {
        violations.push_back({"poker.hand.pot", "poker.negative_pot", "pot",
                              "poker pot cannot be negative",
                              domain::DomainViolationSeverity::Error});
    }
    if (state.winner && !contains_player(state, *state.winner)) {
        violations.push_back({"poker.hand.winner", "poker.invalid_winner", "winner",
                              "winner must participate in the hand",
                              domain::DomainViolationSeverity::Error});
    }
    return domain::DomainValidationReport::create(std::move(violations));
}

auto voluntary_put_money_in_pot_metric() -> Result<Metric> {
    auto id = MetricId::from_stable_name("poker.metric.vpip");
    if (!id) {
        return Result<Metric>::failure(id.error());
    }
    return Metric::create(std::move(id).value(), "VPIP",
                          "whether a player voluntarily contributed chips before the flop",
                          measurement::MetricValueKind::Boolean, std::nullopt, "1");
}

} // namespace evolution::domains::poker
