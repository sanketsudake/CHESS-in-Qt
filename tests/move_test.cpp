#include "chess/Move.hpp"

#include <gtest/gtest.h>

namespace chess {
namespace {

TEST(Move, UciRoundTripForAPlainMove)
{
    const Move move{Square::E2, Square::E4, PieceType::None, MoveKind::DoublePawnPush};
    EXPECT_EQ(move.toUci(), "e2e4");

    const auto parsed = Move::fromUci("e2e4");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(*parsed, move);
}

TEST(Move, UciPromotionLetterIsLowerCase)
{
    const Move move{Square::E7, Square::E8, PieceType::Queen, MoveKind::Quiet};
    EXPECT_EQ(move.toUci(), "e7e8q");

    const auto parsed = Move::fromUci("e7e8q");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->promotion, PieceType::Queen);
}

TEST(Move, UciAcceptsUpperCasePromotionLetters)
{
    const auto parsed = Move::fromUci("a7a8N");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->promotion, PieceType::Knight);
}

TEST(Move, UciRejectsPromotionToPawnOrKing)
{
    EXPECT_FALSE(Move::fromUci("e7e8p").has_value());
    EXPECT_FALSE(Move::fromUci("e7e8k").has_value());
}

TEST(Move, UciRejectsMalformedInput)
{
    EXPECT_FALSE(Move::fromUci("").has_value());
    EXPECT_FALSE(Move::fromUci("e2e").has_value());
    EXPECT_FALSE(Move::fromUci("e2e4qq").has_value());
    EXPECT_FALSE(Move::fromUci("i2e4").has_value());
    EXPECT_FALSE(Move::fromUci("e2e9").has_value());
    EXPECT_FALSE(Move::fromUci("e2e4x").has_value());
}

TEST(Move, InvalidMoveSerialisesToTheNullMove)
{
    EXPECT_EQ(Move{}.toUci(), "0000");
    EXPECT_FALSE(Move{}.isValid());
}

// Kind is derived from the position, so it is deliberately not part of move
// identity. This is what lets the user interface look a click up in a
// generated list without knowing how to classify it.
TEST(Move, IdentityIgnoresKind)
{
    const Move quiet{Square::E1, Square::G1, PieceType::None, MoveKind::Quiet};
    const Move castle{Square::E1, Square::G1, PieceType::None, MoveKind::CastleKingSide};
    EXPECT_EQ(quiet, castle);
}

TEST(Move, IdentityIncludesThePromotionPiece)
{
    const Move toQueen{Square::E7, Square::E8, PieceType::Queen, MoveKind::Quiet};
    const Move toKnight{Square::E7, Square::E8, PieceType::Knight, MoveKind::Quiet};
    EXPECT_NE(toQueen, toKnight);
}

TEST(Move, ClassifiersMatchTheKind)
{
    EXPECT_TRUE((Move{Square::E1, Square::G1, PieceType::None, MoveKind::CastleKingSide}).isCastle());
    EXPECT_TRUE((Move{Square::E1, Square::C1, PieceType::None, MoveKind::CastleQueenSide}).isCastle());
    EXPECT_FALSE((Move{Square::E2, Square::E4, PieceType::None, MoveKind::DoublePawnPush}).isCastle());

    EXPECT_TRUE((Move{Square::D5, Square::E6, PieceType::None, MoveKind::EnPassant}).isCapture());
    EXPECT_TRUE((Move{Square::D5, Square::E6, PieceType::None, MoveKind::Capture}).isCapture());
    EXPECT_FALSE((Move{Square::D5, Square::D6, PieceType::None, MoveKind::Quiet}).isCapture());

    EXPECT_TRUE((Move{Square::E7, Square::E8, PieceType::Queen, MoveKind::Quiet}).isPromotion());
    EXPECT_FALSE((Move{Square::E7, Square::E8, PieceType::None, MoveKind::Quiet}).isPromotion());
}

} // namespace
} // namespace chess
