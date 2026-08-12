#pragma once

#include "chess/Move.hpp"
#include "chess/Position.hpp"
#include "chess/Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace chess {

// A fixed-capacity list of moves.
//
// The most moves any *legal* chess position offers is 218, so 256 is ample for
// any position that can arise in a game. Storage that cannot allocate keeps
// perft honest: the measurement is of the rules, not of the allocator.
//
// "Legal" is doing real work in that sentence. A Position can also be built by
// hand or read from a FEN record, and FEN validates syntax rather than
// legality -- a board of sixty-three queens parses perfectly well and offers
// far more than 256 moves. So the bound is enforced here rather than assumed,
// because a caller cannot be trusted to have supplied a reachable position.
class MoveList {
public:
    static constexpr std::size_t kCapacity = 256;

    // Silently ignores anything past capacity. Only a position that could
    // never occur in a game can reach that point, and quietly generating fewer
    // moves for an impossible board is much better than writing past the end
    // of the array -- which lands on size_ itself and turns the next push into
    // a write at an arbitrary offset.
    //
    // Callers that care whether a position is real should ask
    // validatePosition; see Rules.hpp.
    void push(Move move)
    {
        if (size_ >= kCapacity) {
            return;
        }
        moves_[size_++] = move;
    }

    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] bool empty() const { return size_ == 0; }

    [[nodiscard]] const Move& operator[](std::size_t i) const { return moves_[i]; }

    [[nodiscard]] const Move* begin() const { return moves_.data(); }
    [[nodiscard]] const Move* end() const { return moves_.data() + size_; }

    // Looks a move up by from, to and promotion. This is how the user
    // interface turns a click, which knows nothing about castling or en
    // passant, into a move that carries the right kind.
    //
    // Returns the move by value rather than a pointer into this list. A Move
    // is four bytes, and the obvious way to write the call is
    // generateLegalMoves(position).find(...), where a returned pointer would
    // dangle the moment the temporary list died.
    [[nodiscard]] std::optional<Move> find(
        Square from, Square to, PieceType promotion = PieceType::None) const;

    [[nodiscard]] bool contains(Square from, Square to, PieceType promotion = PieceType::None) const
    {
        return find(from, to, promotion).has_value();
    }

private:
    std::array<Move, kCapacity> moves_{};
    std::size_t size_ = 0;
};

// Every move the side to move may legally play. Empty means the game is over:
// checkmate if the side to move is in check, stalemate otherwise.
[[nodiscard]] MoveList generateLegalMoves(const Position& position);

// Moves that obey the piece's movement rules but may leave the mover's own
// king attacked. Exposed for testing the two stages apart; callers that want
// to know what may be played want generateLegalMoves.
[[nodiscard]] MoveList generatePseudoLegalMoves(const Position& position);

// True when playing this pseudo-legal move would not leave the mover in check.
[[nodiscard]] bool isLegal(const Position& position, const Move& move);

// Counts leaf nodes of the legal move tree to the given depth. Depth 0 counts
// the position itself as one node.
//
// This is the acceptance test for the whole rule set. The published counts are
// exact, so a single wrong castling or en passant case shows up as a wrong
// number rather than as a subtle misbehaviour years later.
[[nodiscard]] std::uint64_t perft(const Position& position, int depth);

} // namespace chess
