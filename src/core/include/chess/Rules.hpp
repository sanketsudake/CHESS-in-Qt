#pragma once

#include "chess/Board.hpp"
#include "chess/Position.hpp"
#include "chess/Types.hpp"

#include <array>
#include <cstddef>
#include <string_view>

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

// How much of each kind of piece one side has on the board.
//
// One pass produces the whole count, which is what callers want: deciding
// whether a draw is forced, whether a position is possible, and what has been
// captured are all questions about the same tally.
struct Material {
    // Indexed by PieceType. The None slot is always zero.
    std::array<int, 7> counts{};

    // Bishops split by the colour of square they stand on, because two
    // bishops on the same colour can never attack the same squares.
    int lightSquareBishops = 0;
    int darkSquareBishops = 0;

    [[nodiscard]] int of(PieceType type) const { return counts[static_cast<std::size_t>(type)]; }

    [[nodiscard]] int minors() const { return of(PieceType::Knight) + of(PieceType::Bishop); }

    // Everything except the king, which is counted separately because an
    // invalid position can have the wrong number of those.
    [[nodiscard]] int total() const;
};

[[nodiscard]] Material countMaterial(const Board& board, Color color);

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

// Why a position could not occur in a game of chess.
enum class PositionError : std::uint8_t {
    None,
    MissingKing,
    TooManyKings,
    PawnOnBackRank,
    TooManyPieces,
    OpponentAlreadyInCheck,
};

// Checks that a position could actually arise in a game.
//
// FEN validates syntax, not legality: "BQQQQQQQ/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/BQQQQQQB"
// is a well-formed record and an impossible board. Anything that accepts a
// position from outside -- a pasted record, a file, a command line argument --
// should ask this before playing from it, because the rest of the library is
// written for positions that can occur.
//
// This is deliberately not folded into fen::parse. Parsing and judging are
// different jobs, and the tests need to build boards with no king on them to
// check that the rules cope.
//
// Checked: one king each, no pawns on the first or eighth rank, no more than
// sixteen pieces or eight pawns a side, and the side *not* to move not already
// in check -- which would mean the previous move left its own king attacked.
[[nodiscard]] PositionError validatePosition(const Position& position);

// A short phrase naming the problem, for a message to the player.
[[nodiscard]] std::string_view describe(PositionError error);

} // namespace chess
