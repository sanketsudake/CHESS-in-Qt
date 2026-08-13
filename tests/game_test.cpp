#include "chess/Fen.hpp"
#include "chess/Game.hpp"
#include "chess/Pgn.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

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

void playAll(Game& game, const std::vector<std::string>& moves)
{
    for (const std::string& move : moves) {
        ASSERT_TRUE(game.playSan(move)) << "could not play " << move << " in " << game.fen();
    }
}

TEST(Game, StartsFromTheUsualPosition)
{
    const Game game;
    EXPECT_EQ(game.position(), Position::starting());
    EXPECT_EQ(game.currentPly(), 0U);
    EXPECT_TRUE(game.history().empty());
    EXPECT_FALSE(game.canUndo());
    EXPECT_FALSE(game.canRedo());
    EXPECT_EQ(game.lastMove(), nullptr);
}

TEST(Game, PlayingRecordsTheMoveAndItsNotation)
{
    Game game;
    ASSERT_TRUE(game.playSan("e4"));

    ASSERT_EQ(game.history().size(), 1U);
    EXPECT_EQ(game.history()[0].san, "e4");
    EXPECT_EQ(game.history()[0].move.toUci(), "e2e4");
    EXPECT_EQ(game.sideToMove(), Color::Black);
    ASSERT_NE(game.lastMove(), nullptr);
    EXPECT_EQ(game.lastMove()->move.from, Square::E2);
}

TEST(Game, RejectsAnIllegalMoveWithoutChangingAnything)
{
    Game game;
    EXPECT_FALSE(game.playSan("e5"));
    EXPECT_FALSE(game.playUci("e2e5"));
    EXPECT_FALSE(game.playFrom(Square::E2, Square::E5));
    EXPECT_EQ(game.position(), Position::starting());
    EXPECT_TRUE(game.history().empty());
}

// A click on a board knows two squares and nothing about castling. playFrom is
// what turns that into a move carrying the right kind.
TEST(Game, PlayFromResolvesTheMoveKindForTheCaller)
{
    Game game(mustParse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"));
    ASSERT_TRUE(game.playFrom(Square::E1, Square::G1));

    EXPECT_EQ(game.history()[0].san, "O-O");
    EXPECT_EQ(game.position().board.pieceAt(Square::F1), (Piece{PieceType::Rook, Color::White}));
}

TEST(Game, PlayFromNeedsThePromotionPieceNamed)
{
    Game game(mustParse("8/4P3/8/8/8/8/8/K6k w - - 0 1"));
    EXPECT_FALSE(game.playFrom(Square::E7, Square::E8));
    ASSERT_TRUE(game.playFrom(Square::E7, Square::E8, PieceType::Knight));
    EXPECT_EQ(game.position().board.pieceAt(Square::E8), (Piece{PieceType::Knight, Color::White}));
}

TEST(Game, UndoAndRedoWalkTheHistoryWithoutLosingIt)
{
    Game game;
    playAll(game, {"e4", "e5", "Nf3"});
    ASSERT_EQ(game.currentPly(), 3U);

    ASSERT_TRUE(game.undo());
    EXPECT_EQ(game.currentPly(), 2U);
    EXPECT_EQ(game.sideToMove(), Color::White);
    EXPECT_EQ(game.history().size(), 3U) << "undo rewinds, it does not delete";

    ASSERT_TRUE(game.undo());
    ASSERT_TRUE(game.undo());
    EXPECT_EQ(game.position(), Position::starting());
    EXPECT_FALSE(game.canUndo());
    EXPECT_FALSE(game.undo());

    ASSERT_TRUE(game.redo());
    ASSERT_TRUE(game.redo());
    ASSERT_TRUE(game.redo());
    EXPECT_EQ(game.currentPly(), 3U);
    EXPECT_FALSE(game.canRedo());
    EXPECT_FALSE(game.redo());
}

TEST(Game, PlayingAfterUndoDiscardsTheAbandonedLine)
{
    Game game;
    playAll(game, {"e4", "e5", "Nf3"});
    ASSERT_TRUE(game.undo());

    ASSERT_TRUE(game.playSan("d4"));
    EXPECT_EQ(game.history().size(), 3U);
    EXPECT_EQ(game.history()[2].san, "d4");
    EXPECT_FALSE(game.canRedo()) << "the discarded line is gone";
}

TEST(Game, GoToPlyMovesTheViewerWithoutDiscarding)
{
    Game game;
    playAll(game, {"e4", "e5", "Nf3", "Nc6"});

    ASSERT_TRUE(game.goToPly(1));
    EXPECT_EQ(game.currentPly(), 1U);
    EXPECT_EQ(game.history().size(), 4U);
    EXPECT_TRUE(game.canRedo());

    ASSERT_TRUE(game.goToPly(0));
    EXPECT_EQ(game.position(), Position::starting());

    ASSERT_TRUE(game.goToPly(4));
    EXPECT_FALSE(game.goToPly(5)) << "there is no fifth ply";
}

TEST(Game, ResetClearsEverything)
{
    Game game;
    playAll(game, {"e4", "e5"});
    game.reset();

    EXPECT_EQ(game.position(), Position::starting());
    EXPECT_TRUE(game.history().empty());
    EXPECT_EQ(game.currentPly(), 0U);
    EXPECT_FALSE(game.canRedo());
}

TEST(Game, StartsFromACustomPositionWhenGivenOne)
{
    const Position start = mustParse("4k3/8/8/8/8/8/8/R3K3 w Q - 5 30");
    Game game(start);
    EXPECT_EQ(game.startPosition(), start);
    EXPECT_EQ(game.fen(), fen::serialise(start));
}

TEST(Game, FoolsMateEndsTheGame)
{
    Game game;
    playAll(game, {"f3", "e5", "g4", "Qh4"});

    EXPECT_TRUE(game.isOver());
    EXPECT_EQ(game.terminalReason(), TerminalReason::Checkmate);
    EXPECT_EQ(game.outcome(), Outcome::BlackWins);
    EXPECT_TRUE(game.legalMoves().empty());
}

TEST(Game, StalemateIsADraw)
{
    Game game(mustParse("7k/5Q2/8/8/8/8/8/6K1 w - - 0 1"));
    ASSERT_TRUE(game.playSan("Kg2"));

    EXPECT_EQ(game.terminalReason(), TerminalReason::Stalemate);
    EXPECT_EQ(game.outcome(), Outcome::Draw);
}

// Knights shuffling back and forth reach the same position three times. This
// is the rule the old code had no way to express, because it kept no history.
TEST(Game, ThreefoldRepetitionIsDetected)
{
    Game game;
    EXPECT_EQ(game.repetitionCount(), 1);

    playAll(game, {"Nf3", "Nf6", "Ng1", "Ng8"});
    EXPECT_EQ(game.repetitionCount(), 2);
    EXPECT_FALSE(game.isOver());

    playAll(game, {"Nf3", "Nf6", "Ng1", "Ng8"});
    EXPECT_EQ(game.repetitionCount(), 3);
    EXPECT_EQ(game.terminalReason(), TerminalReason::ThreefoldRepetition);
    EXPECT_EQ(game.outcome(), Outcome::Draw);
}

TEST(Game, RepetitionCountFollowsTheViewerBackwards)
{
    Game game;
    playAll(game, {"Nf3", "Nf6", "Ng1", "Ng8", "Nf3", "Nf6", "Ng1", "Ng8"});
    EXPECT_EQ(game.repetitionCount(), 3);

    // Rewinding to the second occurrence should count two, not three: the
    // third has not been reached yet from where the viewer stands.
    ASSERT_TRUE(game.goToPly(4));
    EXPECT_EQ(game.repetitionCount(), 2);
}

// A double pawn push with no pawn able to answer it leaves an en passant
// square that changes nothing. Two such positions repeat, even though their
// FEN differs.
TEST(Game, RepetitionIgnoresAnEnPassantSquareNobodyCanUse)
{
    Game game;
    playAll(game, {"e4", "Nf6", "Nf3", "Ng8", "Ng1", "Nf6", "Nf3", "Ng8", "Ng1"});

    // The position after 1.e4 recurs; the en passant square recorded then was
    // never usable, so it must not keep the positions apart.
    EXPECT_EQ(game.repetitionCount(), 3);
    EXPECT_EQ(game.terminalReason(), TerminalReason::ThreefoldRepetition);
}

TEST(Game, CheckmateOutranksThreefoldRepetition)
{
    Game game(mustParse("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1"));
    ASSERT_TRUE(game.playSan("Ra8#"));
    EXPECT_EQ(game.terminalReason(), TerminalReason::Checkmate);
}

TEST(Pgn, WritesTheSevenRequiredTagsAndTheMovetext)
{
    Game game;
    playAll(game, {"e4", "e5", "Nf3", "Nc6"});

    const std::string text = pgn::serialise(game, pgn::Tags{"Test", "Nowhere", "2026.08.12", "1", "A", "B"});

    EXPECT_NE(text.find("[Event \"Test\"]"), std::string::npos);
    EXPECT_NE(text.find("[White \"A\"]"), std::string::npos);
    EXPECT_NE(text.find("[Black \"B\"]"), std::string::npos);
    EXPECT_NE(text.find("[Result \"*\"]"), std::string::npos);
    EXPECT_NE(text.find("1. e4 e5 2. Nf3 Nc6 *"), std::string::npos);
    EXPECT_EQ(text.find("[FEN"), std::string::npos) << "no FEN tag for a normal start";
}

TEST(Pgn, RecordsTheResultOfAFinishedGame)
{
    Game game;
    playAll(game, {"f3", "e5", "g4", "Qh4"});

    const std::string text = pgn::serialise(game);
    EXPECT_EQ(pgn::resultToken(game), "0-1");
    EXPECT_NE(text.find("[Result \"0-1\"]"), std::string::npos);
    EXPECT_NE(text.find("Qh4# 0-1"), std::string::npos);
}

// Without SetUp and FEN, the movetext of a game that did not start from the
// usual position cannot be replayed.
TEST(Pgn, CarriesSetUpAndFenForACustomStart)
{
    const Position start = mustParse("4k3/8/8/8/8/8/8/R3K3 w Q - 5 30");
    Game game(start);
    ASSERT_TRUE(game.playSan("Ra8+"));

    const std::string text = pgn::serialise(game);
    EXPECT_NE(text.find("[SetUp \"1\"]"), std::string::npos);
    EXPECT_NE(text.find("[FEN \"" + fen::serialise(start) + "\"]"), std::string::npos);
    EXPECT_NE(text.find("30. Ra8+"), std::string::npos) << "numbering follows the starting position";
}

TEST(Pgn, NumbersABlackFirstMoveWithAnEllipsis)
{
    const Position start = mustParse("4k3/8/8/8/8/8/8/R3K3 b Q - 5 30");
    Game game(start);
    ASSERT_TRUE(game.playSan("Kd8"));

    EXPECT_NE(pgn::serialise(game).find("30... Kd8"), std::string::npos);
}

TEST(Pgn, WrapsMovetextAtEightyColumns)
{
    Game game;
    playAll(game,
        {"Nf3", "Nf6", "Ng1", "Ng8", "Nf3", "Nf6", "Ng1", "Ng8", "Nc3", "Nc6", "Nb1", "Nb8", "Nc3", "Nc6",
            "Nb1", "Nb8"});

    const std::string text = pgn::serialise(game);
    std::size_t lineStart = 0;
    while (lineStart < text.size()) {
        const std::size_t lineEnd = text.find('\n', lineStart);
        const std::size_t length = (lineEnd == std::string::npos ? text.size() : lineEnd) - lineStart;
        EXPECT_LE(length, 80U) << text.substr(lineStart, length);
        if (lineEnd == std::string::npos) {
            break;
        }
        lineStart = lineEnd + 1;
    }
}

} // namespace
} // namespace chess
