#include "chess/Game.hpp"

#include "chess/Fen.hpp"
#include "chess/San.hpp"

namespace chess {

Game::Game()
    : Game(Position::starting())
{
}

Game::Game(const Position& start)
{
    reset(start);
}

void Game::reset(const Position& start)
{
    positions_.assign(1, start);
    repetitionKeys_.assign(1, repetitionKeyFor(start));
    moves_.clear();
    currentPly_ = 0;
}

// Two positions repeat only if the same moves are available in both. The en
// passant square is the trap: a position where a double push just happened but
// no pawn can answer it is, for repetition, the same as one where no push
// happened at all. FEN records the square unconditionally, as most tools do,
// so it is normalised away here instead.
Position Game::repetitionKeyFor(const Position& position)
{
    Position key = position;

    if (isValid(key.enPassantTarget)) {
        bool captureAvailable = false;
        for (const Move& move : generateLegalMoves(position)) {
            if (move.kind == MoveKind::EnPassant) {
                captureAvailable = true;
                break;
            }
        }
        if (!captureAvailable) {
            key.enPassantTarget = Square::None;
        }
    }

    return key;
}

bool Game::play(const Move& move)
{
    // Look the move up rather than trusting the caller's kind: a click on a
    // board produces two squares and nothing more, and only the generator
    // knows whether those squares mean a castle or an en passant capture.
    const std::optional<Move> legal = legalMoves().find(move.from, move.to, move.promotion);
    if (!legal) {
        return false;
    }

    // Playing while rewound abandons the line that was there.
    positions_.resize(currentPly_ + 1);
    repetitionKeys_.resize(currentPly_ + 1);
    moves_.resize(currentPly_);

    const Position& before = positions_[currentPly_];
    moves_.push_back(PlayedMove{*legal, san::toSan(before, *legal)});

    const Position after = applyMove(before, *legal);
    positions_.push_back(after);
    repetitionKeys_.push_back(repetitionKeyFor(after));
    ++currentPly_;

    return true;
}

bool Game::playFrom(Square from, Square to, PieceType promotion)
{
    // Move identity is from, to and promotion, so the placeholder kind here is
    // replaced by the real one during the lookup in play.
    return play(Move{from, to, promotion, MoveKind::Quiet});
}

bool Game::playSan(std::string_view text)
{
    const auto move = san::parse(position(), text);
    if (!move) {
        return false;
    }
    return play(*move);
}

bool Game::playUci(std::string_view text)
{
    const auto move = Move::fromUci(text);
    if (!move) {
        return false;
    }
    return play(*move);
}

bool Game::undo()
{
    if (!canUndo()) {
        return false;
    }
    --currentPly_;
    return true;
}

bool Game::redo()
{
    if (!canRedo()) {
        return false;
    }
    ++currentPly_;
    return true;
}

bool Game::goToPly(std::size_t ply)
{
    if (ply >= positions_.size()) {
        return false;
    }
    currentPly_ = ply;
    return true;
}

const PlayedMove* Game::lastMove() const
{
    if (currentPly_ == 0) {
        return nullptr;
    }
    return &moves_[currentPly_ - 1];
}

int Game::repetitionCount() const
{
    const Position& key = repetitionKeys_[currentPly_];

    int count = 0;
    // Only positions up to where the viewer stands count. Moves ahead in the
    // history belong to a line that has not been played yet.
    for (std::size_t ply = 0; ply <= currentPly_; ++ply) {
        if (repetitionKeys_[ply].sameGameState(key)) {
            ++count;
        }
    }
    return count;
}

TerminalReason Game::terminalReason() const
{
    // Checkmate and stalemate outrank the drawing rules: a game that ends in
    // mate ended in mate, whatever the counters say.
    const TerminalReason fromPosition = chess::terminalReason(position());
    if (fromPosition == TerminalReason::Checkmate || fromPosition == TerminalReason::Stalemate) {
        return fromPosition;
    }

    if (repetitionCount() >= 3) {
        return TerminalReason::ThreefoldRepetition;
    }
    return fromPosition;
}

Outcome Game::outcome() const
{
    return outcomeFor(terminalReason(), sideToMove());
}

std::string Game::fen() const
{
    return fen::serialise(position());
}

} // namespace chess
