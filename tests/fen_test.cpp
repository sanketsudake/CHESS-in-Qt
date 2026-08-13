#include "chess/Fen.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace chess {
namespace {

// Unwraps a parse that is expected to succeed, failing the test with the
// parser's own message when it does not.
Position mustParse(std::string_view text)
{
    fen::ParseResult result = fen::parse(text);
    if (const auto* failure = std::get_if<fen::ParseError>(&result)) {
        ADD_FAILURE() << "expected '" << text << "' to parse, got: " << failure->message;
        return Position{};
    }
    return std::get<Position>(result);
}

std::string parseErrorFor(std::string_view text)
{
    fen::ParseResult result = fen::parse(text);
    if (const auto* failure = std::get_if<fen::ParseError>(&result)) {
        return failure->message;
    }
    ADD_FAILURE() << "expected '" << text << "' to be rejected, but it parsed";
    return {};
}

// The six positions the Chess Programming Wiki publishes perft results for.
// PR 2 walks these to a known node count; here they only need to survive a
// round trip, which is what makes those later numbers trustworthy.
constexpr std::string_view kStartPosition = fen::kStartingPosition;
constexpr std::string_view kKiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
constexpr std::string_view kPosition3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
constexpr std::string_view kPosition4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
constexpr std::string_view kPosition5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
constexpr std::string_view kPosition6
    = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

TEST(Fen, StartingPositionMatchesTheBoardBuiltInCode)
{
    const Position parsed = mustParse(kStartPosition);
    EXPECT_EQ(parsed, Position::starting());
}

TEST(Fen, SerialisesTheStartingPosition)
{
    EXPECT_EQ(fen::serialise(Position::starting()), kStartPosition);
}

TEST(Fen, RoundTripsTheStandardPerftPositions)
{
    for (const std::string_view text :
        {kStartPosition, kKiwipete, kPosition3, kPosition4, kPosition5, kPosition6}) {
        const Position parsed = mustParse(text);
        EXPECT_EQ(fen::serialise(parsed), text);
        EXPECT_EQ(mustParse(fen::serialise(parsed)), parsed);
    }
}

TEST(Fen, ReadsSideToMove)
{
    EXPECT_EQ(mustParse("8/8/8/8/8/8/8/K6k w - - 0 1").sideToMove, Color::White);
    EXPECT_EQ(mustParse("8/8/8/8/8/8/8/K6k b - - 0 1").sideToMove, Color::Black);
}

TEST(Fen, ReadsEachCastlingRightsSubset)
{
    const auto rightsFor = [](std::string_view field) {
        return mustParse("r3k2r/8/8/8/8/8/8/R3K2R w " + std::string(field) + " - 0 1").castling;
    };

    EXPECT_EQ(rightsFor("-"), CastlingRights{});
    EXPECT_EQ(rightsFor("KQkq"), CastlingRights{CastlingRights::All});

    const CastlingRights whiteOnly = rightsFor("KQ");
    EXPECT_TRUE(whiteOnly.has(CastlingRights::WhiteKingSide));
    EXPECT_TRUE(whiteOnly.has(CastlingRights::WhiteQueenSide));
    EXPECT_FALSE(whiteOnly.has(CastlingRights::BlackKingSide));
    EXPECT_FALSE(whiteOnly.has(CastlingRights::BlackQueenSide));

    const CastlingRights queenSides = rightsFor("Qq");
    EXPECT_FALSE(queenSides.has(CastlingRights::WhiteKingSide));
    EXPECT_TRUE(queenSides.has(CastlingRights::WhiteQueenSide));
    EXPECT_FALSE(queenSides.has(CastlingRights::BlackKingSide));
    EXPECT_TRUE(queenSides.has(CastlingRights::BlackQueenSide));

    const CastlingRights blackKingOnly = rightsFor("k");
    EXPECT_EQ(blackKingOnly.bits(), static_cast<std::uint8_t>(CastlingRights::BlackKingSide));
}

TEST(Fen, SerialisesCastlingRightsInKQkqOrder)
{
    Position position = Position::starting();
    position.castling.remove(CastlingRights::WhiteKingSide);
    position.castling.remove(CastlingRights::BlackQueenSide);
    EXPECT_EQ(fen::serialise(position), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Qk - 0 1");

    position.castling = CastlingRights{};
    EXPECT_EQ(fen::serialise(position), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
}

// A White double push leaves a target on rank 3 with Black to move; a Black
// one leaves it on rank 6 with White to move.
TEST(Fen, ReadsEnPassantTargetsOnBothSides)
{
    const Position afterE4 = mustParse("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(afterE4.enPassantTarget, Square::E3);
    EXPECT_EQ(afterE4.sideToMove, Color::Black);

    const Position afterE5 = mustParse("rnbqkbnr/pppp1ppp/8/4p3/8/8/PPPPPPPP/RNBQKBNR w KQkq e6 0 2");
    EXPECT_EQ(afterE5.enPassantTarget, Square::E6);
    EXPECT_EQ(afterE5.sideToMove, Color::White);
}

TEST(Fen, RejectsAnEnPassantTargetOnTheWrongRankForTheSideToMove)
{
    // e6 with Black to move would mean Black just pushed onto its own side.
    EXPECT_FALSE(parseErrorFor("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq e6 0 1").empty());
    EXPECT_FALSE(parseErrorFor("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e3 0 1").empty());
    EXPECT_FALSE(parseErrorFor("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq e4 0 1").empty());
}

TEST(Fen, ReadsTheMoveCounters)
{
    const Position position = mustParse("8/8/8/8/8/8/8/K6k w - - 37 99");
    EXPECT_EQ(position.halfmoveClock, 37);
    EXPECT_EQ(position.fullmoveNumber, 99);
}

// Many published test positions stop after the en passant field.
TEST(Fen, AcceptsARecordWithoutCounters)
{
    const Position position = mustParse("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    EXPECT_EQ(position.halfmoveClock, 0);
    EXPECT_EQ(position.fullmoveNumber, 1);
    EXPECT_EQ(fen::serialise(position), kPosition3);
}

TEST(Fen, ToleratesRepeatedSpacesBetweenFields)
{
    EXPECT_EQ(
        mustParse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR  w   KQkq  -  0  1"), Position::starting());
}

// A record read from a file, an .epd suite or the clipboard arrives with a
// trailing newline. Splitting on spaces alone would fold it into the last
// field and reject the whole thing.
TEST(Fen, ToleratesTabsAndTrailingNewlines)
{
    EXPECT_EQ(mustParse(std::string(kStartPosition) + "\n"), Position::starting());
    EXPECT_EQ(mustParse(std::string(kStartPosition) + "\r\n"), Position::starting());
    EXPECT_EQ(mustParse("\n  " + std::string(kStartPosition) + "  \n"), Position::starting());
    EXPECT_EQ(
        mustParse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR\tw\tKQkq\t-\t0\t1"), Position::starting());
}

// Five fields means the halfmove clock is present and the fullmove number is
// not, which sits between the two cases the other tests cover.
TEST(Fen, AcceptsFiveFields)
{
    const Position position = mustParse("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 7");
    EXPECT_EQ(position.halfmoveClock, 7);
    EXPECT_EQ(position.fullmoveNumber, 1);
}

TEST(Fen, RejectsCountersTooLargeForTheType)
{
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - - 99999999999 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - - 0 99999999999").empty());
}

// Every other round-trip test uses a position with no en passant target, so
// this is the one that exercises that field of serialise.
TEST(Fen, RoundTripsAPositionWithAnEnPassantTarget)
{
    constexpr std::string_view afterE4 = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
    const Position position = mustParse(afterE4);
    EXPECT_EQ(position.enPassantTarget, Square::E3);
    EXPECT_EQ(fen::serialise(position), afterE4);
    EXPECT_EQ(mustParse(fen::serialise(position)), position);
}

// A Position built by hand can hold an en passant target that no double pawn
// push could have produced. parse refuses such a record, so serialise must not
// write one, or the two would disagree about what a valid position is.
TEST(Fen, WritesNoEnPassantTargetWhenItContradictsTheSideToMove)
{
    Position position = Position::starting();
    position.enPassantTarget = Square::E3; // White to move: only e6 could be right.

    const std::string text = fen::serialise(position);
    EXPECT_EQ(text, kStartPosition);
    EXPECT_TRUE(std::holds_alternative<Position>(fen::parse(text)));
}

TEST(Fen, RejectsTooFewFields)
{
    EXPECT_FALSE(parseErrorFor("").empty());
    EXPECT_FALSE(parseErrorFor("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR").empty());
    EXPECT_FALSE(parseErrorFor("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq").empty());
}

TEST(Fen, RejectsTooManyFields)
{
    EXPECT_FALSE(parseErrorFor("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 extra").empty());
}

TEST(Fen, RejectsTheWrongNumberOfRanks)
{
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8 w - - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/8/8 w - - 0 1").empty());
}

TEST(Fen, RejectsARankThatDoesNotDescribeEightSquares)
{
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/7 w - - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/9 w - - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/8P w - - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("KKKKKKKKK/8/8/8/8/8/8/8 w - - 0 1").empty());
}

TEST(Fen, RejectsUnknownPieceLetters)
{
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/X7 w - - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/07 w - - 0 1").empty());
}

TEST(Fen, RejectsAMalformedSideToMove)
{
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k W - - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k white - - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k x - - 0 1").empty());
}

TEST(Fen, RejectsAMalformedCastlingField)
{
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w KQkqX - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w KK - 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w K- - 0 1").empty());
}

TEST(Fen, RejectsAMalformedEnPassantField)
{
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - e 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - i6 0 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - e66 0 1").empty());
}

TEST(Fen, RejectsMalformedCounters)
{
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - - x 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - - -1 1").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - - 0 0").empty());
    EXPECT_FALSE(parseErrorFor("8/8/8/8/8/8/8/K6k w - - 0 1x").empty());
}

TEST(Fen, ErrorMessagesNameTheProblem)
{
    EXPECT_NE(parseErrorFor("8/8/8/8/8/8/8/X7 w - - 0 1").find('X'), std::string::npos);
    EXPECT_NE(parseErrorFor("8/8/8/8/8/8/8/K6k q - - 0 1").find("side to move"), std::string::npos);
}

} // namespace
} // namespace chess
