#pragma once

#include "chess/Board.hpp"
#include "chess/Position.hpp"
#include "chess/Types.hpp"

namespace chess {

// Why a game stopped. Ongoing means it has not.
enum class TerminalReason : std::uint8_t {
    None,
    Checkmate,
    Stalemate,
    FiftyMoveRule,
    ThreefoldRepetition,
    InsufficientMaterial,
};

enum class Outcome : std::uint8_t { Ongoing, WhiteWins, BlackWins, Draw };

// True when any piece of `attacker` could capture onto `square`, whether or
// not doing so would be legal for the attacker. This is the primitive behind
// check detection, castling restrictions and the legality filter.
//
// It works outwards from the square rather than iterating every enemy piece,
// so the cost is a fixed handful of rays instead of a full board scan. That
// matters: it is the innermost operation in perft.
[[nodiscard]] bool isSquareAttacked(const Board& board, Square square, Color attacker);

// True when `color`'s king stands on an attacked square. A position with no
// king of that colour is not in check.
[[nodiscard]] bool isInCheck(const Position& position, Color color);

// True when the side to move is in check.
[[nodiscard]] bool isInCheck(const Position& position);

// Neither side has the material to force mate: king against king, king and a
// single minor piece against king, or king and bishop against king and bishop
// with both bishops on squares of the same colour.
[[nodiscard]] bool hasInsufficientMaterial(const Position& position);

// Why this position is terminal, considering only what a single position can
// show. Threefold repetition needs the move history, so it is Game's job and
// is never returned here.
[[nodiscard]] TerminalReason terminalReason(const Position& position);

// The result implied by a reason, given whose turn it is. Checkmate is a loss
// for the side to move; every other terminal reason is a draw.
[[nodiscard]] Outcome outcomeFor(TerminalReason reason, Color sideToMove);

} // namespace chess
