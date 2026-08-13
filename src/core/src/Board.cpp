#include "chess/Board.hpp"

namespace chess {

Square Board::kingSquare(Color color) const
{
    for (int i = 0; i < kSquareCount; ++i) {
        const auto square = static_cast<Square>(i);
        const Piece piece = pieceAt(square);
        if (piece.type == PieceType::King && piece.color == color) {
            return square;
        }
    }
    return Square::None;
}

Board Board::startingPosition()
{
    static constexpr std::array<PieceType, kBoardSize> kBackRank{
        PieceType::Rook,
        PieceType::Knight,
        PieceType::Bishop,
        PieceType::Queen,
        PieceType::King,
        PieceType::Bishop,
        PieceType::Knight,
        PieceType::Rook,
    };

    Board board;
    for (int file = 0; file < kBoardSize; ++file) {
        const auto backRankPiece = kBackRank[static_cast<std::size_t>(file)];
        board.setPiece(makeSquare(file, 0), Piece{backRankPiece, Color::White});
        board.setPiece(makeSquare(file, 1), Piece{PieceType::Pawn, Color::White});
        board.setPiece(makeSquare(file, 6), Piece{PieceType::Pawn, Color::Black});
        board.setPiece(makeSquare(file, 7), Piece{backRankPiece, Color::Black});
    }
    return board;
}

} // namespace chess
