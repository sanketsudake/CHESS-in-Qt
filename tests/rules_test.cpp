#include "chess/Fen.hpp"
#include "chess/MoveGenerator.hpp"
#include "chess/Rules.hpp"

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

TEST(IsSquareAttacked, PawnsAttackForwardDiagonallyOnly)
{
    const Board board = mustParse("8/8/8/8/8/8/4P3/8 w - - 0 1").board;
    EXPECT_TRUE(isSquareAttacked(board, Square::D3, Color::White));
    EXPECT_TRUE(isSquareAttacked(board, Square::F3, Color::White));
    EXPECT_FALSE(isSquareAttacked(board, Square::E3, Color::White)) << "a pawn does not attack ahead";
    EXPECT_FALSE(isSquareAttacked(board, Square::D1, Color::White)) << "nor behind";
}

TEST(IsSquareAttacked, BlackPawnsAttackTheOtherWay)
{
    const Board board = mustParse("8/4p3/8/8/8/8/8/8 w - - 0 1").board;
    EXPECT_TRUE(isSquareAttacked(board, Square::D6, Color::Black));
    EXPECT_TRUE(isSquareAttacked(board, Square::F6, Color::Black));
    EXPECT_FALSE(isSquareAttacked(board, Square::D8, Color::Black));
}

TEST(IsSquareAttacked, KnightsReachAllEightHops)
{
    const Board board = mustParse("8/8/8/3N4/8/8/8/8 w - - 0 1").board;
    for (const Square square :
        {Square::C7, Square::E7, Square::B6, Square::F6, Square::B4, Square::F4, Square::C3, Square::E3}) {
        EXPECT_TRUE(isSquareAttacked(board, square, Color::White)) << toString(square);
    }
    EXPECT_FALSE(isSquareAttacked(board, Square::D6, Color::White));
}

TEST(IsSquareAttacked, SlidersAreBlockedByTheFirstPieceOnTheRay)
{
    const Board open = mustParse("8/8/8/8/8/8/8/R6k w - - 0 1").board;
    EXPECT_TRUE(isSquareAttacked(open, Square::H1, Color::White));

    const Board blocked = mustParse("8/8/8/8/8/8/8/R3n2k w - - 0 1").board;
    EXPECT_TRUE(isSquareAttacked(blocked, Square::E1, Color::White)) << "the blocker itself is attacked";
    EXPECT_FALSE(isSquareAttacked(blocked, Square::H1, Color::White)) << "but nothing beyond it";
}

TEST(IsSquareAttacked, QueensCoverBothRookAndBishopLines)
{
    const Board board = mustParse("8/8/8/3Q4/8/8/8/8 w - - 0 1").board;
    EXPECT_TRUE(isSquareAttacked(board, Square::D1, Color::White));
    EXPECT_TRUE(isSquareAttacked(board, Square::A5, Color::White));
    EXPECT_TRUE(isSquareAttacked(board, Square::H1, Color::White));
    EXPECT_TRUE(isSquareAttacked(board, Square::A8, Color::White));
    EXPECT_FALSE(isSquareAttacked(board, Square::E3, Color::White));
}

TEST(IsSquareAttacked, IgnoresPiecesOfTheOtherColour)
{
    const Board board = mustParse("8/8/8/3r4/8/8/8/8 w - - 0 1").board;
    EXPECT_TRUE(isSquareAttacked(board, Square::D1, Color::Black));
    EXPECT_FALSE(isSquareAttacked(board, Square::D1, Color::White));
}

TEST(IsInCheck, ReportsTheSideWhoseKingIsAttacked)
{
    const Position position = mustParse("4r3/8/8/8/8/8/8/4K3 w - - 0 1");
    EXPECT_TRUE(isInCheck(position));
    EXPECT_TRUE(isInCheck(position, Color::White));
    EXPECT_FALSE(isInCheck(position, Color::Black));
}

TEST(IsInCheck, APositionWithoutAKingIsNotInCheck)
{
    const Position position = mustParse("4r3/8/8/8/8/8/8/8 w - - 0 1");
    EXPECT_FALSE(isInCheck(position, Color::White));
}

// The 2012 validation::check() returned 0 unconditionally, so no game ever
// ended. These are the cases it never detected.
TEST(Terminal, FoolsMateIsCheckmate)
{
    const Position position = mustParse("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    EXPECT_TRUE(isInCheck(position));
    EXPECT_TRUE(generateLegalMoves(position).empty());
    EXPECT_EQ(terminalReason(position), TerminalReason::Checkmate);
    EXPECT_EQ(outcomeFor(terminalReason(position), position.sideToMove), Outcome::BlackWins);
}

TEST(Terminal, BackRankMateIsCheckmate)
{
    const Position position = mustParse("6k1/5ppp/8/8/8/8/8/R5K1 b - - 0 1");
    EXPECT_EQ(terminalReason(position), TerminalReason::None) << "not yet -- the rook must reach the rank";

    const Position mated = mustParse("R5k1/5ppp/8/8/8/8/8/6K1 b - - 0 1");
    EXPECT_EQ(terminalReason(mated), TerminalReason::Checkmate);
}

TEST(Terminal, StalemateIsNotCheckmate)
{
    // Black to move, not in check, and with no legal move at all.
    const Position position = mustParse("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    EXPECT_FALSE(isInCheck(position));
    EXPECT_TRUE(generateLegalMoves(position).empty());
    EXPECT_EQ(terminalReason(position), TerminalReason::Stalemate);
    EXPECT_EQ(outcomeFor(terminalReason(position), position.sideToMove), Outcome::Draw);
}

TEST(Terminal, CheckAloneDoesNotEndTheGame)
{
    const Position position = mustParse("4r3/8/8/8/8/8/8/4K3 w - - 0 1");
    EXPECT_TRUE(isInCheck(position));
    EXPECT_EQ(terminalReason(position), TerminalReason::None);
    EXPECT_EQ(outcomeFor(TerminalReason::None, Color::White), Outcome::Ongoing);
}

TEST(Terminal, FiftyMoveRuleAppliesAtOneHundredPlies)
{
    EXPECT_EQ(terminalReason(mustParse("4k3/8/8/4r3/8/8/8/4K3 w - - 99 60")), TerminalReason::None);
    EXPECT_EQ(terminalReason(mustParse("4k3/8/8/4r3/8/8/8/4K3 w - - 100 60")), TerminalReason::FiftyMoveRule);
}

// A mate delivered on the hundredth ply is a mate, not a fifty-move draw.
TEST(Terminal, CheckmateOutranksTheFiftyMoveRule)
{
    const Position position = mustParse("R5k1/5ppp/8/8/8/8/8/6K1 b - - 100 60");
    EXPECT_EQ(terminalReason(position), TerminalReason::Checkmate);
}

TEST(InsufficientMaterial, KingAgainstKing)
{
    EXPECT_TRUE(hasInsufficientMaterial(mustParse("4k3/8/8/8/8/8/8/4K3 w - - 0 1")));
}

TEST(InsufficientMaterial, KingAndOneMinorPieceAgainstKing)
{
    EXPECT_TRUE(hasInsufficientMaterial(mustParse("4k3/8/8/8/8/8/8/3BK3 w - - 0 1")));
    EXPECT_TRUE(hasInsufficientMaterial(mustParse("4k3/8/8/8/8/8/8/3NK3 w - - 0 1")));
    EXPECT_TRUE(hasInsufficientMaterial(mustParse("3bk3/8/8/8/8/8/8/4K3 w - - 0 1")));
}

TEST(InsufficientMaterial, BishopsOnTheSameColourOfSquareCanNeverMeet)
{
    // c1 and f8 are both dark squares.
    EXPECT_TRUE(hasInsufficientMaterial(mustParse("4kb2/8/8/8/8/8/8/2B1K3 w - - 0 1")));

    // c1 is dark, c8 is light.
    EXPECT_FALSE(hasInsufficientMaterial(mustParse("2b1k3/8/8/8/8/8/8/2B1K3 w - - 0 1")));
}

TEST(InsufficientMaterial, AnyPawnRookOrQueenIsEnough)
{
    EXPECT_FALSE(hasInsufficientMaterial(mustParse("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1")));
    EXPECT_FALSE(hasInsufficientMaterial(mustParse("4k3/8/8/8/8/8/8/R3K3 w - - 0 1")));
    EXPECT_FALSE(hasInsufficientMaterial(mustParse("4k3/8/8/8/8/8/8/3QK3 w - - 0 1")));
}

// Two knights cannot force mate, but mate is reachable if the defender helps,
// so the position is not dead and play continues.
TEST(InsufficientMaterial, TwoKnightsAgainstAKingIsStillPlayable)
{
    EXPECT_FALSE(hasInsufficientMaterial(mustParse("4k3/8/8/8/8/8/8/1N1NK3 w - - 0 1")));
}

TEST(InsufficientMaterial, TwoBishopsAgainstAKingIsStillPlayable)
{
    EXPECT_FALSE(hasInsufficientMaterial(mustParse("4k3/8/8/8/8/8/8/2BBK3 w - - 0 1")));
}

TEST(Terminal, InsufficientMaterialIsReportedAsADraw)
{
    const Position position = mustParse("4k3/8/8/8/8/8/8/3BK3 w - - 0 1");
    EXPECT_EQ(terminalReason(position), TerminalReason::InsufficientMaterial);
    EXPECT_EQ(outcomeFor(terminalReason(position), position.sideToMove), Outcome::Draw);
}

TEST(Outcome, CheckmateLosesForWhoeverIsToMove)
{
    EXPECT_EQ(outcomeFor(TerminalReason::Checkmate, Color::White), Outcome::BlackWins);
    EXPECT_EQ(outcomeFor(TerminalReason::Checkmate, Color::Black), Outcome::WhiteWins);
}

// A FEN record can be perfectly well formed and describe a board that could
// never have occurred. Move generation sizes its list for the 218 moves a real
// position can offer, so a board of sixty-three queens used to write past the
// end of that list -- landing on the size field itself and turning the next
// push into a write at an arbitrary offset. Pasting one crashed the
// application.
TEST(MoveGeneration, AnImpossibleBoardCannotOverflowTheMoveList)
{
    const Position absurd = mustParse("BQQQQQQQ/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/BQQQQQQB w - - 0 1");

    const MoveList pseudoLegal = generatePseudoLegalMoves(absurd);
    EXPECT_LE(pseudoLegal.size(), MoveList::kCapacity);

    const MoveList legal = generateLegalMoves(absurd);
    EXPECT_LE(legal.size(), MoveList::kCapacity);

    // The point is that it returns at all rather than corrupting the stack.
    EXPECT_NO_FATAL_FAILURE((void)terminalReason(absurd));
}

TEST(MoveGeneration, ARealPositionIsNowhereNearTheCapacity)
{
    // The most crowded of the standard perft positions, well under the bound.
    const Position kiwipete
        = mustParse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_LT(generateLegalMoves(kiwipete).size(), MoveList::kCapacity / 2);
}

TEST(ValidatePosition, AcceptsPositionsThatCouldOccur)
{
    EXPECT_EQ(validatePosition(Position::starting()), PositionError::None);

    for (const std::string_view text :
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
            "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"}) {
        EXPECT_EQ(validatePosition(mustParse(text)), PositionError::None) << text;
    }
}

TEST(ValidatePosition, RejectsTheBoardThatOverflowedTheMoveList)
{
    EXPECT_EQ(validatePosition(mustParse("BQQQQQQQ/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/BQQQQQQB w - - 0 1")),
        PositionError::MissingKing);

    // Even with a king each it is still impossible, and still rejected.
    EXPECT_NE(validatePosition(mustParse("BQQQkQQQ/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/Q6Q/BQQKQQQB w - - 0 1")),
        PositionError::None);
}

TEST(ValidatePosition, RequiresExactlyOneKingEachSide)
{
    EXPECT_EQ(validatePosition(mustParse("4k3/8/8/8/8/8/8/8 w - - 0 1")), PositionError::MissingKing);
    EXPECT_EQ(validatePosition(mustParse("8/8/8/8/8/8/8/4K3 w - - 0 1")), PositionError::MissingKing);
    EXPECT_EQ(validatePosition(mustParse("4k3/8/8/8/8/8/8/K3K3 w - - 0 1")), PositionError::TooManyKings);
}

TEST(ValidatePosition, RejectsPawnsOnTheBackRanks)
{
    EXPECT_EQ(validatePosition(mustParse("4k3/8/8/8/8/8/8/P3K3 w - - 0 1")), PositionError::PawnOnBackRank);
    EXPECT_EQ(validatePosition(mustParse("p3k3/8/8/8/8/8/8/4K3 w - - 0 1")), PositionError::PawnOnBackRank);
}

TEST(ValidatePosition, RejectsMorePiecesThanASideCouldHave)
{
    EXPECT_EQ(validatePosition(mustParse("4k3/8/8/QQQQQQQQ/QQQQQQQQ/QQQ5/8/4K3 w - - 0 1")),
        PositionError::TooManyPieces);
    EXPECT_EQ(validatePosition(mustParse("4k3/pppppppp/pppppppp/8/8/8/8/4K3 w - - 0 1")),
        PositionError::TooManyPieces);
}

// If the side that just moved left its own king attacked, the move that
// produced this position was itself illegal.
TEST(ValidatePosition, RejectsAPositionWhereTheSideNotToMoveIsInCheck)
{
    EXPECT_EQ(
        validatePosition(mustParse("4rk2/8/8/8/8/8/8/4K3 b - - 0 1")), PositionError::OpponentAlreadyInCheck);

    // The same board with White to move is an ordinary check.
    EXPECT_EQ(validatePosition(mustParse("4rk2/8/8/8/8/8/8/4K3 w - - 0 1")), PositionError::None);
}

TEST(ValidatePosition, EveryErrorHasSomethingToSay)
{
    for (const PositionError error : {PositionError::None, PositionError::MissingKing,
             PositionError::TooManyKings, PositionError::PawnOnBackRank, PositionError::TooManyPieces,
             PositionError::OpponentAlreadyInCheck}) {
        EXPECT_FALSE(describe(error).empty());
    }
}

TEST(Outcome, EveryOtherTerminalReasonIsADraw)
{
    for (const TerminalReason reason : {TerminalReason::Stalemate, TerminalReason::FiftyMoveRule,
             TerminalReason::ThreefoldRepetition, TerminalReason::InsufficientMaterial}) {
        EXPECT_EQ(outcomeFor(reason, Color::White), Outcome::Draw);
        EXPECT_EQ(outcomeFor(reason, Color::Black), Outcome::Draw);
    }
}

} // namespace
} // namespace chess
