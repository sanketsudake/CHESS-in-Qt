#pragma once

#include "chess/Types.hpp"

#include <array>
#include <cassert>

namespace chess {

// Piece placement, and nothing else.
//
// Side to move, castling rights and the en passant target live in Position.
// Keeping them apart is what lets move generation reason about "what is on
// this square" without carrying the rest of the game state around.
class Board {
public:
    Board() = default;

    // Square::None indexes at -1, which as an unsigned subscript reads or
    // writes far outside the array. Several things legitimately produce
    // Square::None -- makeSquare off the edge of the board, kingSquare with no
    // king, an absent en passant target -- so callers must reject it before
    // asking about a square, and this asserts that they did.
    [[nodiscard]] Piece pieceAt(Square s) const
    {
        assert(isValid(s));
        return squares_[static_cast<std::size_t>(index(s))];
    }

    void setPiece(Square s, Piece piece)
    {
        assert(isValid(s));
        squares_[static_cast<std::size_t>(index(s))] = piece;
    }

    void clearSquare(Square s) { setPiece(s, Piece{}); }

    [[nodiscard]] bool isEmpty(Square s) const { return pieceAt(s).isEmpty(); }

    // True when the square holds a piece of the given colour. An empty square
    // is neither colour, which is the check move generation actually wants.
    [[nodiscard]] bool hasPieceOf(Square s, Color color) const
    {
        const Piece piece = pieceAt(s);
        return !piece.isEmpty() && piece.color == color;
    }

    void clear() { squares_.fill(Piece{}); }

    // Square::None when that side has no king. A legal position always has
    // one, but FEN can describe positions that do not, and tests use them.
    [[nodiscard]] Square kingSquare(Color color) const;

    [[nodiscard]] static Board startingPosition();

    friend bool operator==(const Board& a, const Board& b) { return a.squares_ == b.squares_; }

private:
    std::array<Piece, kSquareCount> squares_{};
};

} // namespace chess
