#pragma once

#include "chess/Board.hpp"
#include "chess/Move.hpp"
#include "chess/Types.hpp"

#include <cstdint>

namespace chess {

// Which castles are still available, as a bitmask. A right is lost when the
// king moves, when the rook on that side moves, or when that rook is captured.
// It is never regained, so this only ever shrinks over a game.
class CastlingRights {
public:
    enum Flag : std::uint8_t {
        None = 0,
        WhiteKingSide = 1U << 0U,
        WhiteQueenSide = 1U << 1U,
        BlackKingSide = 1U << 2U,
        BlackQueenSide = 1U << 3U,
        All = WhiteKingSide | WhiteQueenSide | BlackKingSide | BlackQueenSide,
    };

    constexpr CastlingRights() = default;
    constexpr explicit CastlingRights(std::uint8_t bits)
        : bits_(bits)
    {
    }

    [[nodiscard]] constexpr bool has(Flag flag) const
    {
        return (bits_ & static_cast<std::uint8_t>(flag)) != 0U;
    }
    constexpr void add(Flag flag) { bits_ |= static_cast<std::uint8_t>(flag); }
    constexpr void remove(Flag flag)
    {
        bits_ = static_cast<std::uint8_t>(bits_ & ~static_cast<std::uint8_t>(flag));
    }
    [[nodiscard]] constexpr std::uint8_t bits() const { return bits_; }

    [[nodiscard]] static constexpr Flag kingSideFor(Color c)
    {
        return c == Color::White ? WhiteKingSide : BlackKingSide;
    }

    [[nodiscard]] static constexpr Flag queenSideFor(Color c)
    {
        return c == Color::White ? WhiteQueenSide : BlackQueenSide;
    }

    friend constexpr bool operator==(CastlingRights a, CastlingRights b) { return a.bits_ == b.bits_; }

private:
    std::uint8_t bits_ = None;
};

// A complete chess position: everything needed to decide what moves are legal
// and whether the game has ended. Two Positions comparing equal are the same
// position for every rule purpose except the move counters.
struct Position {
    Board board;
    Color sideToMove = Color::White;
    CastlingRights castling;

    // The square a pawn just skipped over, which an enemy pawn may capture
    // onto. Square::None when the previous move was not a double pawn push.
    // Set unconditionally after a double push, matching FEN as written by
    // most tools, so a position compares unequal to one where no push happened.
    Square enPassantTarget = Square::None;

    // Plies since the last capture or pawn move. The game is drawn at 100.
    int halfmoveClock = 0;

    // Starts at 1 and increments after Black moves.
    int fullmoveNumber = 1;

    [[nodiscard]] static Position starting();

    // Equality for threefold repetition: placement, side to move, castling
    // rights and en passant target, deliberately excluding the two counters.
    // Two positions that differ only in move number are the same position.
    [[nodiscard]] bool sameGameState(const Position& other) const;

    friend bool operator==(const Position& a, const Position& b);
};

// The squares a rook starts on, and so the squares whose occupant leaving or
// being captured costs a castling right.
[[nodiscard]] constexpr Square kingSideRookSquare(Color c)
{
    return c == Color::White ? Square::H1 : Square::H8;
}

[[nodiscard]] constexpr Square queenSideRookSquare(Color c)
{
    return c == Color::White ? Square::A1 : Square::A8;
}

[[nodiscard]] constexpr Square kingStartSquare(Color c)
{
    return c == Color::White ? Square::E1 : Square::E8;
}

// Applies a move and returns the resulting position. Pure: the input is not
// modified, which is what lets the legality filter try a move and throw the
// result away, and what lets Game keep a history by value.
//
// The move must carry the right MoveKind, which is what the generator
// produces. Re-deriving "was that an en passant capture?" from the board here
// would duplicate the generator's reasoning, and that duplication is where the
// 2012 code went wrong.
[[nodiscard]] Position applyMove(const Position& position, const Move& move);

} // namespace chess
