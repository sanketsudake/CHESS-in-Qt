#include "chess/Rules.hpp"

#include "Geometry.hpp"
#include "chess/MoveGenerator.hpp"

#include <algorithm>

namespace chess {
namespace {

using geometry::Offset;

// Walks outwards along `directions` looking for an enemy slider that could
// reach the origin. The first piece met in each direction blocks the rest of
// that ray, whichever side it belongs to.
bool isAttackedBySlider(
    const Board& board, Square origin, Color attacker, std::span<const Offset> directions, PieceType slider)
{
    for (const Offset direction : directions) {
        Square current = geometry::offsetFrom(origin, direction);
        while (isValid(current)) {
            const Piece piece = board.pieceAt(current);
            if (!piece.isEmpty()) {
                if (piece.color == attacker && (piece.type == slider || piece.type == PieceType::Queen)) {
                    return true;
                }
                break;
            }
            current = geometry::offsetFrom(current, direction);
        }
    }
    return false;
}

bool isAttackedByHopper(
    const Board& board, Square origin, Color attacker, std::span<const Offset> hops, PieceType hopper)
{
    return std::ranges::any_of(hops, [&](const Offset hop) {
        const Square candidate = geometry::offsetFrom(origin, hop);
        return isValid(candidate) && board.pieceAt(candidate) == Piece{hopper, attacker};
    });
}

// A White pawn on rank r attacks rank r+1, so a White pawn attacking `origin`
// stands one rank behind it. Working backwards like this costs two square
// lookups instead of a scan for every enemy pawn.
bool isAttackedByPawn(const Board& board, Square origin, Color attacker)
{
    const int sourceRank = static_cast<int>(rankOf(origin)) - pawnPush(attacker);
    const int originFile = static_cast<int>(fileOf(origin));

    for (const int fileStep : {-1, 1}) {
        const Square candidate = makeSquare(originFile + fileStep, sourceRank);
        if (isValid(candidate) && board.pieceAt(candidate) == Piece{PieceType::Pawn, attacker}) {
            return true;
        }
    }
    return false;
}

} // namespace

int Material::total() const
{
    return of(PieceType::Pawn) + of(PieceType::Knight) + of(PieceType::Bishop) + of(PieceType::Rook)
        + of(PieceType::Queen);
}

Material countMaterial(const Board& board, Color color)
{
    Material material;
    for (int i = 0; i < kSquareCount; ++i) {
        const auto square = static_cast<Square>(i);
        const Piece piece = board.pieceAt(square);
        if (piece.isEmpty() || piece.color != color) {
            continue;
        }

        ++material.counts[static_cast<std::size_t>(piece.type)];
        if (piece.type == PieceType::Bishop) {
            (isLightSquare(square) ? material.lightSquareBishops : material.darkSquareBishops)++;
        }
    }
    return material;
}

bool isSquareAttacked(const Board& board, Square square, Color attacker)
{
    if (!isValid(square)) {
        return false;
    }

    // Ordered cheapest and most likely first.
    return isAttackedByPawn(board, square, attacker)
        || isAttackedByHopper(board, square, attacker, geometry::kKnightHops, PieceType::Knight)
        || isAttackedByHopper(board, square, attacker, geometry::kAllDirections, PieceType::King)
        || isAttackedBySlider(board, square, attacker, geometry::kOrthogonal, PieceType::Rook)
        || isAttackedBySlider(board, square, attacker, geometry::kDiagonal, PieceType::Bishop);
}

bool isInCheck(const Position& position, Color color)
{
    const Square king = position.board.kingSquare(color);
    if (!isValid(king)) {
        return false;
    }
    return isSquareAttacked(position.board, king, opposite(color));
}

bool isInCheck(const Position& position)
{
    return isInCheck(position, position.sideToMove);
}

bool hasInsufficientMaterial(const Position& position)
{
    const Material white = countMaterial(position.board, Color::White);
    const Material black = countMaterial(position.board, Color::Black);

    // A pawn, rook or queen can force mate on its own, so the position is
    // still playable whatever else is on the board.
    const auto canMateAlone = [](const Material& side) {
        return side.of(PieceType::Pawn) > 0 || side.of(PieceType::Rook) > 0 || side.of(PieceType::Queen) > 0;
    };
    if (canMateAlone(white) || canMateAlone(black)) {
        return false;
    }

    const int minors = white.minors() + black.minors();

    // King against king, and king with one minor piece against king. Neither
    // can be forced to mate.
    if (minors <= 1) {
        return true;
    }

    // King and bishop against king and bishop, both bishops on the same colour
    // of square: they can never attack the same squares, so mate is impossible.
    if (minors == 2 && white.of(PieceType::Bishop) == 1 && black.of(PieceType::Bishop) == 1) {
        const bool bothLight = white.lightSquareBishops == 1 && black.lightSquareBishops == 1;
        const bool bothDark = white.darkSquareBishops == 1 && black.darkSquareBishops == 1;
        return bothLight || bothDark;
    }

    // Two knights against a lone king cannot force mate, but mate is still
    // reachable if the defender helps, so it is not a dead position.
    return false;
}

TerminalReason terminalReason(const Position& position)
{
    // Having no legal move is checked first: a mate delivered on the hundredth
    // ply ends the game as a mate, not as a fifty-move draw.
    if (generateLegalMoves(position).empty()) {
        return isInCheck(position) ? TerminalReason::Checkmate : TerminalReason::Stalemate;
    }
    if (hasInsufficientMaterial(position)) {
        return TerminalReason::InsufficientMaterial;
    }
    if (position.halfmoveClock >= 100) {
        return TerminalReason::FiftyMoveRule;
    }
    return TerminalReason::None;
}

PositionError validatePosition(const Position& position)
{
    const Board& board = position.board;

    for (const Color color : {Color::White, Color::Black}) {
        int kings = 0;
        for (int i = 0; i < kSquareCount; ++i) {
            const Piece piece = board.pieceAt(static_cast<Square>(i));
            if (piece.type == PieceType::King && piece.color == color) {
                ++kings;
            }
        }
        if (kings == 0) {
            return PositionError::MissingKing;
        }
        if (kings > 1) {
            return PositionError::TooManyKings;
        }

        const Material material = countMaterial(board, color);
        // Sixteen pieces a side including the king, and eight of those pawns.
        // Promotions can turn pawns into pieces but never add to the total.
        if (material.total() + 1 > 16 || material.of(PieceType::Pawn) > kBoardSize) {
            return PositionError::TooManyPieces;
        }
    }

    // A pawn cannot be on the rank it would have promoted from, nor on the one
    // it started behind.
    for (int file = 0; file < kBoardSize; ++file) {
        for (const int rank : {0, kBoardSize - 1}) {
            if (board.pieceAt(makeSquare(file, rank)).type == PieceType::Pawn) {
                return PositionError::PawnOnBackRank;
            }
        }
    }

    // If the side that just moved left its own king attacked, the move that
    // produced this position was itself illegal.
    if (isInCheck(position, opposite(position.sideToMove))) {
        return PositionError::OpponentAlreadyInCheck;
    }

    return PositionError::None;
}

std::string_view describe(PositionError error)
{
    switch (error) {
    case PositionError::None:
        return "the position is legal";
    case PositionError::MissingKing:
        return "each side needs a king";
    case PositionError::TooManyKings:
        return "a side has more than one king";
    case PositionError::PawnOnBackRank:
        return "a pawn cannot stand on the first or eighth rank";
    case PositionError::TooManyPieces:
        return "a side has more pieces than it could ever have";
    case PositionError::OpponentAlreadyInCheck:
        return "the side that just moved left its own king in check";
    }
    return "the position is not legal";
}

Outcome outcomeFor(TerminalReason reason, Color sideToMove)
{
    switch (reason) {
    case TerminalReason::None:
        return Outcome::Ongoing;
    case TerminalReason::Checkmate:
        return sideToMove == Color::White ? Outcome::BlackWins : Outcome::WhiteWins;
    case TerminalReason::Stalemate:
    case TerminalReason::FiftyMoveRule:
    case TerminalReason::ThreefoldRepetition:
    case TerminalReason::InsufficientMaterial:
        return Outcome::Draw;
    }
    return Outcome::Ongoing;
}

} // namespace chess
