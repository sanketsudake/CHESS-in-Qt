#include "chess/Fen.hpp"
#include "chess/MoveGenerator.hpp"
#include "chess/San.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace chess {
namespace {

Position mustParse(std::string_view text)
{
    fen::ParseResult result = fen::parse(text);
    if (const auto* failure = std::get_if<fen::ParseError>(&result)) {
        ADD_FAILURE() << "expected '" << text << "' to parse, got: " << failure->message;
        return Position{};
    }
    return std::get<Position>(result);
}

// The SAN spelling of the legal move written in UCI form, so a test can name
// the move unambiguously and assert how it should be written.
std::string sanFor(const Position& position, std::string_view uci)
{
    for (const Move& move : generateLegalMoves(position)) {
        if (move.toUci() == uci) {
            return san::toSan(position, move);
        }
    }
    ADD_FAILURE() << uci << " is not legal in " << fen::serialise(position);
    return {};
}

TEST(San, PlainPawnAndPieceMoves)
{
    const Position start = Position::starting();
    EXPECT_EQ(sanFor(start, "e2e4"), "e4");
    EXPECT_EQ(sanFor(start, "g1f3"), "Nf3");
}

TEST(San, CapturesUseAnX)
{
    const Position position = mustParse("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    EXPECT_EQ(sanFor(position, "e4d5"), "exd5");

    const Position pieceCapture = mustParse("rnbqkbnr/ppp1pppp/8/3p4/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 2");
    EXPECT_EQ(sanFor(pieceCapture, "f3e5"), "Ne5");
}

// A pawn capture names its file whether or not anything is ambiguous, which is
// the one place SAN is not minimal.
TEST(San, PawnCapturesAlwaysNameTheirFile)
{
    const Position position = mustParse("8/8/8/3p4/4P3/8/8/K6k w - - 0 1");
    EXPECT_EQ(sanFor(position, "e4d5"), "exd5");
}

TEST(San, CastlingUsesLetterO)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    EXPECT_EQ(sanFor(position, "e1g1"), "O-O");
    EXPECT_EQ(sanFor(position, "e1c1"), "O-O-O");
}

TEST(San, PromotionNamesThePieceAfterAnEquals)
{
    const Position position = mustParse("8/4P3/8/8/8/8/8/K6k w - - 0 1");
    EXPECT_EQ(sanFor(position, "e7e8q"), "e8=Q");
    EXPECT_EQ(sanFor(position, "e7e8n"), "e8=N");

    const Position capturing = mustParse("3r4/4P3/8/8/8/8/8/K6k w - - 0 1");
    EXPECT_EQ(sanFor(capturing, "e7d8q"), "exd8=Q");
}

// Two knights reach d2 from b1 and f3; they differ in file, so the file alone
// separates them.
TEST(San, DisambiguatesByFileWhenFilesDiffer)
{
    const Position position = mustParse("4k3/8/8/8/8/5N2/8/1N2K3 w - - 0 1");
    EXPECT_EQ(sanFor(position, "b1d2"), "Nbd2");
    EXPECT_EQ(sanFor(position, "f3d2"), "Nfd2");
}

// Two rooks on the same file must be told apart by rank.
TEST(San, DisambiguatesByRankWhenFilesMatch)
{
    const Position position = mustParse("4k3/8/8/8/R7/8/8/R3K3 w - - 0 1");
    EXPECT_EQ(sanFor(position, "a1a3"), "R1a3");
    EXPECT_EQ(sanFor(position, "a4a3"), "R4a3");
}

// Three queens reaching one square, which needs promoted pieces to arise. All
// three forms appear at once here: b2 has a rival on its file and another on
// its rank, so it needs both coordinates; the other two need only one each.
TEST(San, DisambiguatesByFileAndRankWhenNeitherAloneIsEnough)
{
    const Position position = mustParse("k7/8/1Q6/8/8/8/1Q3Q2/7K w - - 0 1");
    EXPECT_EQ(sanFor(position, "b2f6"), "Qb2f6");
    EXPECT_EQ(sanFor(position, "b6f6"), "Q6f6");
    EXPECT_EQ(sanFor(position, "f2f6"), "Qff6");
}

TEST(San, DoesNotDisambiguateWhenTheRivalMoveIsIllegal)
{
    // The knight on f3 is pinned by the rook on e8, so only b1 can reach d2
    // and no disambiguation is needed.
    const Position position = mustParse("4r3/8/8/8/8/4N3/8/1N2K3 w - - 0 1");
    EXPECT_EQ(sanFor(position, "b1d2"), "Nd2");
}

TEST(San, AppendsAPlusForCheck)
{
    const Position position = mustParse("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
    EXPECT_EQ(sanFor(position, "a1a8"), "Ra8+");
}

TEST(San, AppendsAHashForCheckmate)
{
    const Position position = mustParse("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1");
    EXPECT_EQ(sanFor(position, "a1a8"), "Ra8#");
}

TEST(San, ScholarsMateReadsAsExpected)
{
    const Position position = mustParse("r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    EXPECT_EQ(sanFor(position, "f3f7"), "Qxf7#");
}

TEST(SanParse, ReadsWhatToSanWrites)
{
    const Position position
        = mustParse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    for (const Move& move : generateLegalMoves(position)) {
        const std::string text = san::toSan(position, move);
        const auto parsed = san::parse(position, text);
        ASSERT_TRUE(parsed.has_value()) << text;
        EXPECT_EQ(*parsed, move) << text;
    }
}

TEST(SanParse, CheckAndMateSuffixesAreOptional)
{
    const Position position = mustParse("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
    EXPECT_TRUE(san::parse(position, "Ra8").has_value());
    EXPECT_TRUE(san::parse(position, "Ra8+").has_value());
    EXPECT_EQ(san::parse(position, "Ra8"), san::parse(position, "Ra8+"));
}

TEST(SanParse, AnnotationGlyphsAreIgnored)
{
    const Position start = Position::starting();
    EXPECT_EQ(san::parse(start, "e4!?"), san::parse(start, "e4"));
    EXPECT_EQ(san::parse(start, "Nf3!!"), san::parse(start, "Nf3"));
}

TEST(SanParse, CastlingMayBeWrittenWithZeroes)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    EXPECT_EQ(san::parse(position, "0-0"), san::parse(position, "O-O"));
    EXPECT_EQ(san::parse(position, "0-0-0"), san::parse(position, "O-O-O"));
    EXPECT_NE(san::parse(position, "O-O"), san::parse(position, "O-O-O"));
}

TEST(SanParse, EnPassantMayCarryTheTraditionalSuffix)
{
    const Position position = mustParse("rnbqkbnr/pp1ppppp/8/8/2pP4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 3");
    const auto plain = san::parse(position, "cxd3");
    ASSERT_TRUE(plain.has_value());
    EXPECT_EQ(plain->kind, MoveKind::EnPassant);
    EXPECT_EQ(san::parse(position, "cxd3e.p."), plain);
}

TEST(SanParse, RejectsMovesThatAreNotLegalHere)
{
    const Position start = Position::starting();
    EXPECT_FALSE(san::parse(start, "e5").has_value());
    EXPECT_FALSE(san::parse(start, "O-O").has_value());
    EXPECT_FALSE(san::parse(start, "Qh5xf7#").has_value());
    EXPECT_FALSE(san::parse(start, "").has_value());
    EXPECT_FALSE(san::parse(start, "not a move").has_value());
}

// A promotion must name its piece; "e8" alone does not identify a move.
TEST(SanParse, PromotionNeedsItsPiece)
{
    const Position position = mustParse("8/4P3/8/8/8/8/8/K6k w - - 0 1");
    EXPECT_FALSE(san::parse(position, "e8").has_value());
    ASSERT_TRUE(san::parse(position, "e8=R").has_value());
    EXPECT_EQ(san::parse(position, "e8=R")->promotion, PieceType::Rook);
}

} // namespace
} // namespace chess
