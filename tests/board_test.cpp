#include "chess/Board.hpp"
#include "chess/Position.hpp"

#include <gtest/gtest.h>

namespace chess {
namespace {

TEST(Board, DefaultIsEmpty)
{
    const Board board;
    for (int i = 0; i < kSquareCount; ++i) {
        EXPECT_TRUE(board.isEmpty(static_cast<Square>(i)));
    }
    EXPECT_EQ(board.kingSquare(Color::White), Square::None);
}

TEST(Board, StartingPositionPlacesWhiteOnRanksOneAndTwo)
{
    const Board board = Board::startingPosition();

    EXPECT_EQ(board.pieceAt(Square::A1), (Piece{PieceType::Rook, Color::White}));
    EXPECT_EQ(board.pieceAt(Square::B1), (Piece{PieceType::Knight, Color::White}));
    EXPECT_EQ(board.pieceAt(Square::C1), (Piece{PieceType::Bishop, Color::White}));
    EXPECT_EQ(board.pieceAt(Square::D1), (Piece{PieceType::Queen, Color::White}));
    EXPECT_EQ(board.pieceAt(Square::E1), (Piece{PieceType::King, Color::White}));
    EXPECT_EQ(board.pieceAt(Square::E2), (Piece{PieceType::Pawn, Color::White}));
}

TEST(Board, StartingPositionPlacesBlackOnRanksSevenAndEight)
{
    const Board board = Board::startingPosition();

    EXPECT_EQ(board.pieceAt(Square::E7), (Piece{PieceType::Pawn, Color::Black}));
    EXPECT_EQ(board.pieceAt(Square::D8), (Piece{PieceType::Queen, Color::Black}));
    EXPECT_EQ(board.pieceAt(Square::E8), (Piece{PieceType::King, Color::Black}));
}

// The 2012 code recorded the white king at row 7 while placing white's pieces
// on rows 0 and 1. Asserting both kings explicitly keeps that class of
// off-by-a-side error out.
TEST(Board, KingSquareFindsEachKingOnItsOwnSide)
{
    const Board board = Board::startingPosition();
    EXPECT_EQ(board.kingSquare(Color::White), Square::E1);
    EXPECT_EQ(board.kingSquare(Color::Black), Square::E8);
}

TEST(Board, StartingPositionLeavesTheMiddleFourRanksEmpty)
{
    const Board board = Board::startingPosition();
    for (int rank = 2; rank <= 5; ++rank) {
        for (int file = 0; file < kBoardSize; ++file) {
            EXPECT_TRUE(board.isEmpty(makeSquare(file, rank))) << "rank " << rank + 1 << " file " << file;
        }
    }
}

TEST(Board, HasPieceOfIsFalseForEmptySquares)
{
    const Board board = Board::startingPosition();
    EXPECT_TRUE(board.hasPieceOf(Square::E2, Color::White));
    EXPECT_FALSE(board.hasPieceOf(Square::E2, Color::Black));
    EXPECT_FALSE(board.hasPieceOf(Square::E4, Color::White));
    EXPECT_FALSE(board.hasPieceOf(Square::E4, Color::Black));
}

TEST(Board, SetAndClearSquare)
{
    Board board;
    board.setPiece(Square::D4, Piece{PieceType::Queen, Color::Black});
    EXPECT_EQ(board.pieceAt(Square::D4), (Piece{PieceType::Queen, Color::Black}));
    board.clearSquare(Square::D4);
    EXPECT_TRUE(board.isEmpty(Square::D4));
}

TEST(CastlingRights, FlagsAreIndependent)
{
    CastlingRights rights{CastlingRights::All};
    EXPECT_TRUE(rights.has(CastlingRights::WhiteKingSide));
    EXPECT_TRUE(rights.has(CastlingRights::BlackQueenSide));

    rights.remove(CastlingRights::WhiteKingSide);
    EXPECT_FALSE(rights.has(CastlingRights::WhiteKingSide));
    EXPECT_TRUE(rights.has(CastlingRights::WhiteQueenSide));
    EXPECT_TRUE(rights.has(CastlingRights::BlackKingSide));
}

TEST(CastlingRights, PerColourAccessors)
{
    EXPECT_EQ(CastlingRights::kingSideFor(Color::White), CastlingRights::WhiteKingSide);
    EXPECT_EQ(CastlingRights::kingSideFor(Color::Black), CastlingRights::BlackKingSide);
    EXPECT_EQ(CastlingRights::queenSideFor(Color::White), CastlingRights::WhiteQueenSide);
    EXPECT_EQ(CastlingRights::queenSideFor(Color::Black), CastlingRights::BlackQueenSide);
}

TEST(Position, StartingPositionHasAllRightsAndNoEnPassant)
{
    const Position position = Position::starting();
    EXPECT_EQ(position.sideToMove, Color::White);
    EXPECT_EQ(position.castling, CastlingRights{CastlingRights::All});
    EXPECT_EQ(position.enPassantTarget, Square::None);
    EXPECT_EQ(position.halfmoveClock, 0);
    EXPECT_EQ(position.fullmoveNumber, 1);
}

// Threefold repetition compares positions, not move numbers. Two positions
// that differ only in the counters are the same position for that rule.
TEST(Position, SameGameStateIgnoresTheCounters)
{
    Position a = Position::starting();
    Position b = Position::starting();
    b.halfmoveClock = 12;
    b.fullmoveNumber = 40;

    EXPECT_TRUE(a.sameGameState(b));
    EXPECT_FALSE(a == b);

    b.sideToMove = Color::Black;
    EXPECT_FALSE(a.sameGameState(b));
}

} // namespace
} // namespace chess
