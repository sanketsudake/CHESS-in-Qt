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

namespace {

// A castling right is lost when the king moves, when its rook moves, or when
// that rook is captured where it stands. The last case is the one that is easy
// to forget: capturing a rook on h8 ends Black's king-side castling even
// though Black never moved anything.
void revokeRightsTouching(CastlingRights& rights, Square square)
{
    for (const Color color : {Color::White, Color::Black}) {
        if (square == kingStartSquare(color)) {
            rights.remove(CastlingRights::kingSideFor(color));
            rights.remove(CastlingRights::queenSideFor(color));
        }
        if (square == kingSideRookSquare(color)) {
            rights.remove(CastlingRights::kingSideFor(color));
        }
        if (square == queenSideRookSquare(color)) {
            rights.remove(CastlingRights::queenSideFor(color));
        }
    }
}

} // namespace

RookTravel rookTravelFor(Color mover, MoveKind kind)
{
    const int backRank = mover == Color::White ? 0 : 7;
    if (kind == MoveKind::CastleKingSide) {
        return {makeSquare(7, backRank), makeSquare(5, backRank)};
    }
    return {makeSquare(0, backRank), makeSquare(3, backRank)};
}

Position applyMove(const Position& position, const Move& move)
{
    Position next = position;

    const Piece mover = position.board.pieceAt(move.from);
    const bool isPawnMove = mover.type == PieceType::Pawn;

    // A capture or a pawn move resets the fifty-move counter. Everything else
    // advances it, which is what eventually forces a draw in a dead position.
    next.halfmoveClock = (isPawnMove || move.isCapture()) ? 0 : position.halfmoveClock + 1;

    next.board.clearSquare(move.from);

    if (move.kind == MoveKind::EnPassant) {
        // The captured pawn is beside the destination, not on it.
        const Square captured
            = makeSquare(static_cast<int>(fileOf(move.to)), static_cast<int>(rankOf(move.from)));
        next.board.clearSquare(captured);
    }

    next.board.setPiece(move.to, move.isPromotion() ? Piece{move.promotion, mover.color} : mover);

    if (move.isCastle()) {
        const RookTravel rook = rookTravelFor(mover.color, move.kind);
        const Piece rookPiece = next.board.pieceAt(rook.from);
        next.board.clearSquare(rook.from);
        next.board.setPiece(rook.to, rookPiece);
    }

    revokeRightsTouching(next.castling, move.from);
    revokeRightsTouching(next.castling, move.to);

    next.enPassantTarget = Square::None;
    if (move.kind == MoveKind::DoublePawnPush) {
        const int skipped = (static_cast<int>(rankOf(move.from)) + static_cast<int>(rankOf(move.to))) / 2;
        next.enPassantTarget = makeSquare(static_cast<int>(fileOf(move.from)), skipped);
    }

    if (position.sideToMove == Color::Black) {
        ++next.fullmoveNumber;
    }
    next.sideToMove = opposite(position.sideToMove);

    return next;
}

} // namespace chess
