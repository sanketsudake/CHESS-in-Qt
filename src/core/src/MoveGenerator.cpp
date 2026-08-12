#include "chess/MoveGenerator.hpp"

#include "Geometry.hpp"
#include "chess/Rules.hpp"

namespace chess {
namespace {

using geometry::Offset;

constexpr std::array<PieceType, 4> kPromotionChoices{
    PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight};

// Adds a pawn move, expanding it into the four promotion choices when it lands
// on the far rank. Every pawn move goes through here so that no path can
// forget that a pawn reaching the eighth rank must become something.
void addPawnMove(MoveList& moves, Color mover, Square from, Square to, MoveKind kind)
{
    if (rankOf(to) != promotionRank(mover)) {
        moves.push(Move{from, to, PieceType::None, kind});
        return;
    }
    for (const PieceType promotion : kPromotionChoices) {
        moves.push(Move{from, to, promotion, kind});
    }
}

void generatePawnMoves(const Position& position, Square from, MoveList& moves)
{
    const Board& board = position.board;
    const Color mover = position.sideToMove;
    const int step = pawnPush(mover);
    const int file = static_cast<int>(fileOf(from));
    const int rank = static_cast<int>(rankOf(from));

    const Square oneForward = makeSquare(file, rank + step);
    if (isValid(oneForward) && board.isEmpty(oneForward)) {
        addPawnMove(moves, mover, from, oneForward, MoveKind::Quiet);

        // The two-square opening is only available from the pawn's home rank,
        // and only when both squares are clear -- a pawn does not jump.
        if (rankOf(from) == pawnStartRank(mover)) {
            const Square twoForward = makeSquare(file, rank + (2 * step));
            if (isValid(twoForward) && board.isEmpty(twoForward)) {
                moves.push(Move{from, twoForward, PieceType::None, MoveKind::DoublePawnPush});
            }
        }
    }

    for (const int fileStep : {-1, 1}) {
        const Square target = makeSquare(file + fileStep, rank + step);
        if (!isValid(target)) {
            continue;
        }
        if (board.hasPieceOf(target, opposite(mover))) {
            addPawnMove(moves, mover, from, target, MoveKind::Capture);
        } else if (target == position.enPassantTarget) {
            // The destination is empty; the pawn being taken is beside it.
            moves.push(Move{from, target, PieceType::None, MoveKind::EnPassant});
        }
    }
}

void generateSteppingMoves(const Position& position, Square from, PieceType type, MoveList& moves)
{
    const Board& board = position.board;
    const Color mover = position.sideToMove;
    const geometry::Movement movement = geometry::movementFor(type);

    for (const Offset direction : movement.directions) {
        Square current = geometry::offsetFrom(from, direction);
        while (isValid(current)) {
            const Piece occupant = board.pieceAt(current);
            if (occupant.isEmpty()) {
                moves.push(Move{from, current, PieceType::None, MoveKind::Quiet});
                if (!movement.sliding) {
                    break;
                }
                current = geometry::offsetFrom(current, direction);
                continue;
            }
            if (occupant.color != mover) {
                moves.push(Move{from, current, PieceType::None, MoveKind::Capture});
            }
            // A piece of either colour ends the ray.
            break;
        }
    }
}

// Castling has four conditions beyond the right still being held: the squares
// between king and rook are empty, the rook is actually there, the king is not
// in check, and it does not pass through an attacked square. The 2012 code
// implemented none of them, because it had no castling at all.
void generateCastles(const Position& position, MoveList& moves)
{
    const Color mover = position.sideToMove;
    const Square kingFrom = kingStartSquare(mover);
    const Color enemy = opposite(mover);
    const Board& board = position.board;
    const int backRank = mover == Color::White ? 0 : 7;

    if (board.pieceAt(kingFrom) != Piece{PieceType::King, mover}) {
        return;
    }
    if (isSquareAttacked(board, kingFrom, enemy)) {
        return;
    }

    struct Side {
        CastlingRights::Flag right;
        Square rookSquare;
        std::array<int, 3> mustBeEmptyFiles;
        int emptyCount;
        int crossedFile;
        int kingDestinationFile;
        MoveKind kind;
    };

    const std::array<Side, 2> sides{
        Side{CastlingRights::kingSideFor(mover), kingSideRookSquare(mover), {5, 6, 0}, 2, 5, 6,
            MoveKind::CastleKingSide},
        // The b-file square must be empty for the rook to pass, but the king
        // never stands on it, so it may be attacked.
        Side{CastlingRights::queenSideFor(mover), queenSideRookSquare(mover), {1, 2, 3}, 3, 3, 2,
            MoveKind::CastleQueenSide},
    };

    for (const Side& side : sides) {
        if (!position.castling.has(side.right)) {
            continue;
        }
        if (board.pieceAt(side.rookSquare) != Piece{PieceType::Rook, mover}) {
            continue;
        }

        bool pathIsClear = true;
        for (int i = 0; i < side.emptyCount; ++i) {
            if (!board.isEmpty(makeSquare(side.mustBeEmptyFiles[static_cast<std::size_t>(i)], backRank))) {
                pathIsClear = false;
                break;
            }
        }
        if (!pathIsClear) {
            continue;
        }

        // The square the king crosses. Its destination is left to the general
        // legality filter, which rejects any move ending in check.
        if (isSquareAttacked(board, makeSquare(side.crossedFile, backRank), enemy)) {
            continue;
        }

        moves.push(
            Move{kingFrom, makeSquare(side.kingDestinationFile, backRank), PieceType::None, side.kind});
    }
}

} // namespace

std::optional<Move> MoveList::find(Square from, Square to, PieceType promotion) const
{
    for (const Move& move : *this) {
        if (move.from == from && move.to == to && move.promotion == promotion) {
            return move;
        }
    }
    return std::nullopt;
}

std::optional<Move> MoveList::findAnyBetween(Square from, Square to) const
{
    for (const Move& move : *this) {
        if (move.from == from && move.to == to) {
            return move;
        }
    }
    return std::nullopt;
}

MoveList generatePseudoLegalMoves(const Position& position)
{
    MoveList moves;
    const Color mover = position.sideToMove;

    for (int i = 0; i < kSquareCount; ++i) {
        const auto from = static_cast<Square>(i);
        const Piece piece = position.board.pieceAt(from);
        if (piece.isEmpty() || piece.color != mover) {
            continue;
        }

        if (piece.type == PieceType::Pawn) {
            generatePawnMoves(position, from, moves);
        } else {
            generateSteppingMoves(position, from, piece.type, moves);
        }
    }

    generateCastles(position, moves);
    return moves;
}

bool isLegal(const Position& position, const Move& move)
{
    const Color mover = position.sideToMove;
    const Position next = applyMove(position, move);
    return !isInCheck(next, mover);
}

MoveList generateLegalMoves(const Position& position)
{
    MoveList legal;
    for (const Move& move : generatePseudoLegalMoves(position)) {
        if (isLegal(position, move)) {
            legal.push(move);
        }
    }
    return legal;
}

std::uint64_t perft(const Position& position, int depth)
{
    if (depth <= 0) {
        return 1;
    }

    const MoveList moves = generateLegalMoves(position);
    if (depth == 1) {
        return moves.size();
    }

    std::uint64_t nodes = 0;
    for (const Move& move : moves) {
        nodes += perft(applyMove(position, move), depth - 1);
    }
    return nodes;
}

} // namespace chess
