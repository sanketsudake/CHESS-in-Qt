#include "chess/Position.hpp"

namespace chess {

Position Position::starting()
{
    Position position;
    position.board = Board::startingPosition();
    position.sideToMove = Color::White;
    position.castling = CastlingRights{CastlingRights::All};
    position.enPassantTarget = Square::None;
    position.halfmoveClock = 0;
    position.fullmoveNumber = 1;
    return position;
}

bool Position::sameGameState(const Position& other) const
{
    return board == other.board && sideToMove == other.sideToMove && castling == other.castling
        && enPassantTarget == other.enPassantTarget;
}

bool operator==(const Position& a, const Position& b)
{
    return a.sameGameState(b) && a.halfmoveClock == b.halfmoveClock && a.fullmoveNumber == b.fullmoveNumber;
}

} // namespace chess
