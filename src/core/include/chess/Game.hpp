#pragma once

#include "chess/Move.hpp"
#include "chess/MoveGenerator.hpp"
#include "chess/Position.hpp"
#include "chess/Rules.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace chess {

// A move as it was played, kept with the notation it was written in at the
// time. Recomputing the notation later would need the position it was played
// from, so it is recorded once, when that position is at hand.
struct PlayedMove {
    Move move;
    std::string san;
};

// A game in progress: a starting position, the moves played from it, and where
// in that sequence the viewer currently is.
//
// Every position along the way is kept rather than recomputed, which is what
// makes undo, redo and jumping to an arbitrary ply all the same operation, and
// what lets threefold repetition be answered by looking rather than guessing.
// A long game is a few hundred positions of eighty bytes; the memory is not
// worth optimising away.
class Game {
public:
    Game();
    explicit Game(const Position& start);

    // Discards all history and starts again from the given position.
    void reset(const Position& start = Position::starting());

    [[nodiscard]] const Position& position() const { return positions_[currentPly_]; }
    [[nodiscard]] const Position& startPosition() const { return positions_.front(); }
    [[nodiscard]] Color sideToMove() const { return position().sideToMove; }

    [[nodiscard]] MoveList legalMoves() const { return generateLegalMoves(position()); }

    // Plays a move from the current position. Returns false, changing nothing,
    // when the move is not legal here.
    //
    // Playing while rewound discards the moves that came after, which is what
    // taking a different line means.
    bool play(const Move& move);

    // Convenience entry points for the same thing. playFrom is what a click on
    // a board calls: it looks the squares up in the legal move list, so the
    // caller never has to know whether the click meant a castle or an en
    // passant capture.
    bool playFrom(Square from, Square to, PieceType promotion = PieceType::None);
    bool playSan(std::string_view text);
    bool playUci(std::string_view text);

    [[nodiscard]] bool canUndo() const { return currentPly_ > 0; }
    [[nodiscard]] bool canRedo() const { return currentPly_ < moves_.size(); }
    bool undo();
    bool redo();

    // Moves the viewer to a ply without discarding anything, which is what
    // clicking an entry in the move list does. Ply 0 is the starting position.
    bool goToPly(std::size_t ply);

    [[nodiscard]] std::size_t currentPly() const { return currentPly_; }
    [[nodiscard]] const std::vector<PlayedMove>& history() const { return moves_; }

    // The move that led to the current position, or nullptr at the start. The
    // board uses this to highlight where the last piece came from.
    [[nodiscard]] const PlayedMove* lastMove() const;

    // How many times the current position has occurred in this game, counting
    // the present occurrence. Three means a draw may be claimed.
    [[nodiscard]] int repetitionCount() const;

    // Why the game is over, including threefold repetition, which needs the
    // history and so cannot be answered by Rules alone.
    [[nodiscard]] TerminalReason terminalReason() const;
    [[nodiscard]] Outcome outcome() const;
    [[nodiscard]] bool isOver() const { return terminalReason() != TerminalReason::None; }

    [[nodiscard]] std::string fen() const;

private:
    // A position stripped of what does not distinguish it for repetition
    // purposes. See Game.cpp for why the en passant square needs care.
    [[nodiscard]] static Position repetitionKeyFor(const Position& position);

    std::vector<Position> positions_;
    std::vector<Position> repetitionKeys_;
    std::vector<PlayedMove> moves_;
    std::size_t currentPly_ = 0;
};

} // namespace chess
