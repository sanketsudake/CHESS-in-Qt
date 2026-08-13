#include "chess/Types.hpp"

#include <gtest/gtest.h>

namespace chess {
namespace {

TEST(Square, IndexesFromA1ToH8)
{
    EXPECT_EQ(index(Square::A1), 0);
    EXPECT_EQ(index(Square::H1), 7);
    EXPECT_EQ(index(Square::A2), 8);
    EXPECT_EQ(index(Square::E4), 28);
    EXPECT_EQ(index(Square::H8), 63);
}

TEST(Square, FileAndRankRoundTrip)
{
    for (int i = 0; i < kSquareCount; ++i) {
        const auto square = static_cast<Square>(i);
        EXPECT_EQ(makeSquare(fileOf(square), rankOf(square)), square);
    }
}

TEST(Square, MakeSquareRejectsCoordinatesOffTheBoard)
{
    EXPECT_EQ(makeSquare(-1, 0), Square::None);
    EXPECT_EQ(makeSquare(0, -1), Square::None);
    EXPECT_EQ(makeSquare(8, 0), Square::None);
    EXPECT_EQ(makeSquare(0, 8), Square::None);
    EXPECT_EQ(makeSquare(4, 3), Square::E4);
}

TEST(Square, NamesRoundTrip)
{
    for (int i = 0; i < kSquareCount; ++i) {
        const auto square = static_cast<Square>(i);
        const auto parsed = squareFromString(toString(square));
        ASSERT_TRUE(parsed.has_value()) << toString(square);
        EXPECT_EQ(*parsed, square);
    }
    EXPECT_EQ(toString(Square::E4), "e4");
    EXPECT_EQ(toString(Square::None), "-");
}

TEST(Square, RejectsMalformedNames)
{
    EXPECT_FALSE(squareFromString("").has_value());
    EXPECT_FALSE(squareFromString("-").has_value());
    EXPECT_FALSE(squareFromString("e").has_value());
    EXPECT_FALSE(squareFromString("e44").has_value());
    EXPECT_FALSE(squareFromString("i4").has_value());
    EXPECT_FALSE(squareFromString("e9").has_value());
    EXPECT_FALSE(squareFromString("E4").has_value());
}

TEST(Piece, EmptySquaresCompareEqualRegardlessOfColour)
{
    EXPECT_EQ((Piece{}), (Piece{PieceType::None, Color::Black}));
    EXPECT_TRUE(Piece{}.isEmpty());
    EXPECT_FALSE((Piece{PieceType::Pawn, Color::White}).isEmpty());
}

TEST(Piece, ColourIsPartOfIdentityForRealPieces)
{
    EXPECT_NE((Piece{PieceType::Pawn, Color::White}), (Piece{PieceType::Pawn, Color::Black}));
    EXPECT_EQ((Piece{PieceType::Pawn, Color::White}), (Piece{PieceType::Pawn, Color::White}));
}

TEST(Piece, FenLettersRoundTrip)
{
    for (const auto type : {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook,
             PieceType::Queen, PieceType::King}) {
        for (const auto color : {Color::White, Color::Black}) {
            const Piece piece{type, color};
            const auto parsed = pieceFromChar(toChar(piece));
            ASSERT_TRUE(parsed.has_value());
            EXPECT_EQ(*parsed, piece);
        }
    }
    EXPECT_EQ(toChar(Piece{PieceType::Knight, Color::White}), 'N');
    EXPECT_EQ(toChar(Piece{PieceType::Knight, Color::Black}), 'n');
    EXPECT_EQ(toChar(PieceType::None), '\0');
}

TEST(Piece, RejectsUnknownLetters)
{
    EXPECT_FALSE(pieceFromChar('x').has_value());
    EXPECT_FALSE(pieceFromChar('1').has_value());
    EXPECT_FALSE(pieceFromChar('-').has_value());
}

TEST(Color, OppositeIsAnInvolution)
{
    EXPECT_EQ(opposite(Color::White), Color::Black);
    EXPECT_EQ(opposite(opposite(Color::White)), Color::White);
}

TEST(Pawns, StartPromotionRanksAndDirectionMatchColour)
{
    EXPECT_EQ(pawnStartRank(Color::White), Rank::R2);
    EXPECT_EQ(pawnStartRank(Color::Black), Rank::R7);
    EXPECT_EQ(promotionRank(Color::White), Rank::R8);
    EXPECT_EQ(promotionRank(Color::Black), Rank::R1);
    EXPECT_EQ(pawnPush(Color::White), 1);
    EXPECT_EQ(pawnPush(Color::Black), -1);
}

} // namespace
} // namespace chess
