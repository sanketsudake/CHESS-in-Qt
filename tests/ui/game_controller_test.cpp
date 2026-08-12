#include "GameController.hpp"

#include "chess/Fen.hpp"

#include <QSignalSpy>

#include <gtest/gtest.h>

namespace cines {
namespace {

using chess::Color;
using chess::PieceType;
using chess::Square;

TEST(GameController, StartsFromTheUsualPositionWithNothingSelected)
{
    GameController controller;

    EXPECT_EQ(controller.position(), chess::Position::starting());
    EXPECT_EQ(controller.selectedSquare(), Square::None);
    EXPECT_TRUE(controller.legalTargets().isEmpty());
    EXPECT_EQ(controller.lastMove(), nullptr);
    EXPECT_FALSE(controller.canUndo());
    EXPECT_FALSE(controller.isGameOver());
}

TEST(GameController, SelectingAPieceReportsWhereItMayGo)
{
    GameController controller;
    QSignalSpy selectionSpy(&controller, &GameController::selectionChanged);

    controller.selectSquare(Square::E2);

    EXPECT_EQ(controller.selectedSquare(), Square::E2);
    EXPECT_EQ(controller.legalTargets().size(), 2);
    EXPECT_TRUE(controller.legalTargets().contains(Square::E3));
    EXPECT_TRUE(controller.legalTargets().contains(Square::E4));
    EXPECT_EQ(selectionSpy.count(), 1);
}

// Clicking an empty square, or an enemy piece, is an ordinary thing to do and
// simply clears the selection rather than being an error.
TEST(GameController, SelectingSomethingUnplayableClearsTheSelection)
{
    GameController controller;

    controller.selectSquare(Square::E2);
    ASSERT_EQ(controller.selectedSquare(), Square::E2);

    controller.selectSquare(Square::E4);
    EXPECT_EQ(controller.selectedSquare(), Square::None);

    controller.selectSquare(Square::E7); // Black's pawn, and it is White to move.
    EXPECT_EQ(controller.selectedSquare(), Square::None);

    controller.selectSquare(Square::None);
    EXPECT_EQ(controller.selectedSquare(), Square::None);
}

// The four promotions of one pawn share a destination, and the board only
// needs to draw the square once.
TEST(GameController, PromotionTargetsAreNotRepeated)
{
    GameController controller;
    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("8/4P3/8/8/8/8/8/K6k w - - 0 1")));

    controller.selectSquare(Square::E7);
    EXPECT_EQ(controller.legalTargets().size(), 1);
    EXPECT_TRUE(controller.legalTargets().contains(Square::E8));
}

TEST(GameController, PlayingALegalMoveAnnouncesItInNotation)
{
    GameController controller;
    QSignalSpy moveSpy(&controller, &GameController::moveMade);
    QSignalSpy positionSpy(&controller, &GameController::positionChanged);

    controller.selectSquare(Square::E2);
    EXPECT_TRUE(controller.moveTo(Square::E4));

    ASSERT_EQ(moveSpy.count(), 1);
    EXPECT_EQ(moveSpy.at(0).at(1).toString(), QStringLiteral("e4"));
    EXPECT_EQ(positionSpy.count(), 1);
    EXPECT_EQ(controller.sideToMove(), Color::Black);
    EXPECT_EQ(controller.selectedSquare(), Square::None) << "the move clears the selection";
    ASSERT_NE(controller.lastMove(), nullptr);
    EXPECT_EQ(controller.lastMove()->move.to, Square::E4);
}

TEST(GameController, RejectingAnIllegalMoveLeavesThePositionAlone)
{
    GameController controller;
    QSignalSpy illegalSpy(&controller, &GameController::illegalMoveAttempted);
    QSignalSpy positionSpy(&controller, &GameController::positionChanged);

    controller.selectSquare(Square::E2);
    EXPECT_FALSE(controller.moveTo(Square::E5));

    EXPECT_EQ(illegalSpy.count(), 1);
    EXPECT_EQ(positionSpy.count(), 0);
    EXPECT_EQ(controller.position(), chess::Position::starting());
}

TEST(GameController, MovingWithNothingSelectedDoesNothing)
{
    GameController controller;
    EXPECT_FALSE(controller.moveTo(Square::E4));
    EXPECT_EQ(controller.position(), chess::Position::starting());
}

// A pawn reaching the far rank must not move until the player has chosen what
// it becomes, because promoting to a knight is a real move rather than an
// oddity worth guessing at.
TEST(GameController, PromotionWaitsForTheChoiceBeforeMoving)
{
    GameController controller;
    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("8/4P3/8/8/8/8/8/K6k w - - 0 1")));

    QSignalSpy promotionSpy(&controller, &GameController::promotionRequested);
    QSignalSpy moveSpy(&controller, &GameController::moveMade);

    controller.selectSquare(Square::E7);
    EXPECT_TRUE(controller.moveTo(Square::E8));

    ASSERT_EQ(promotionSpy.count(), 1);
    EXPECT_EQ(moveSpy.count(), 0) << "nothing has moved yet";
    EXPECT_EQ(controller.position().board.pieceAt(Square::E7).type, PieceType::Pawn);

    controller.finishPromotion(PieceType::Knight);

    ASSERT_EQ(moveSpy.count(), 1);
    EXPECT_EQ(moveSpy.at(0).at(1).toString(), QStringLiteral("e8=N"));
    EXPECT_EQ(
        controller.position().board.pieceAt(Square::E8), (chess::Piece{PieceType::Knight, Color::White}));
}

TEST(GameController, DismissingThePromotionDialogAbandonsTheMove)
{
    GameController controller;
    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("8/4P3/8/8/8/8/8/K6k w - - 0 1")));

    controller.selectSquare(Square::E7);
    ASSERT_TRUE(controller.moveTo(Square::E8));

    controller.finishPromotion(PieceType::None);

    EXPECT_EQ(controller.position().board.pieceAt(Square::E7).type, PieceType::Pawn);
    EXPECT_EQ(controller.selectedSquare(), Square::None);
    EXPECT_EQ(controller.sideToMove(), Color::White);
}

TEST(GameController, FinishPromotionWithNothingPendingIsHarmless)
{
    GameController controller;
    QSignalSpy moveSpy(&controller, &GameController::moveMade);

    controller.finishPromotion(PieceType::Queen);

    EXPECT_EQ(moveSpy.count(), 0);
    EXPECT_EQ(controller.position(), chess::Position::starting());
}

TEST(GameController, UndoAndRedoWalkTheGame)
{
    GameController controller;

    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));
    ASSERT_TRUE(controller.canUndo());

    controller.undo();
    EXPECT_EQ(controller.position(), chess::Position::starting());
    EXPECT_TRUE(controller.canRedo());

    controller.redo();
    EXPECT_EQ(controller.sideToMove(), Color::Black);
}

TEST(GameController, AnnouncesTheEndOfTheGame)
{
    GameController controller;
    QSignalSpy overSpy(&controller, &GameController::gameOver);

    // Black to deliver Qh4 mate after 1.f3 e5 2.g4.
    ASSERT_TRUE(controller.setPositionFromFen(
        QStringLiteral("rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq - 0 2")));

    controller.selectSquare(Square::D8);
    ASSERT_TRUE(controller.moveTo(Square::H4));

    ASSERT_EQ(overSpy.count(), 1);
    EXPECT_EQ(overSpy.at(0).at(0).value<chess::Outcome>(), chess::Outcome::BlackWins);
    EXPECT_EQ(overSpy.at(0).at(1).value<chess::TerminalReason>(), chess::TerminalReason::Checkmate);
    EXPECT_TRUE(controller.isGameOver());
}

TEST(GameController, NothingCanBeSelectedOnceTheGameIsOver)
{
    GameController controller;
    ASSERT_TRUE(controller.setPositionFromFen(
        QStringLiteral("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3")));
    ASSERT_TRUE(controller.isGameOver());

    controller.selectSquare(Square::E1);
    EXPECT_EQ(controller.selectedSquare(), Square::None);
}

TEST(GameController, ReportsCheckSoTheBoardCanShowIt)
{
    GameController controller;
    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("4r3/8/8/8/8/8/8/4K3 w - - 0 1")));

    EXPECT_TRUE(controller.isInCheck(Color::White));
    EXPECT_FALSE(controller.isInCheck(Color::Black));
}

TEST(GameController, StatusTextNamesWhoseTurnItIsAndHowItEnded)
{
    GameController controller;
    EXPECT_EQ(controller.statusText(), QStringLiteral("White to move"));

    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("4r3/8/8/8/8/8/8/4K3 w - - 0 1")));
    EXPECT_EQ(controller.statusText(), QStringLiteral("White to move — check"));

    ASSERT_TRUE(controller.setPositionFromFen(
        QStringLiteral("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3")));
    EXPECT_EQ(controller.statusText(), QStringLiteral("Checkmate — Black wins"));

    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1")));
    EXPECT_EQ(controller.statusText(), QStringLiteral("Stalemate — draw"));

    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("4k3/8/8/8/8/8/8/3BK3 w - - 0 1")));
    EXPECT_EQ(controller.statusText(), QStringLiteral("Draw — neither side can force mate"));
}

TEST(GameController, RefusesTextThatIsNotAPosition)
{
    GameController controller;

    EXPECT_FALSE(controller.setPositionFromFen(QStringLiteral("not a position")));
    EXPECT_FALSE(controller.setPositionFromFen(QString()));
    EXPECT_EQ(controller.position(), chess::Position::starting());
}

// Copying a position and pasting it back must give the same board, since that
// is the whole point of the two menu items.
TEST(GameController, FenRoundTripsThroughTheClipboardText)
{
    GameController controller;
    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));

    const QString text = controller.fen();

    GameController other;
    ASSERT_TRUE(other.setPositionFromFen(text));
    EXPECT_EQ(other.position(), controller.position());
}

TEST(GameController, TrimsSurroundingWhitespaceWhenReadingAPosition)
{
    GameController controller;
    EXPECT_TRUE(controller.setPositionFromFen(
        QStringLiteral("  rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n")));
    EXPECT_EQ(controller.position(), chess::Position::starting());
}

TEST(GameController, NewGameClearsTheHistory)
{
    GameController controller;
    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));

    controller.newGame();

    EXPECT_EQ(controller.position(), chess::Position::starting());
    EXPECT_TRUE(controller.history().empty());
    EXPECT_FALSE(controller.canUndo());
}

} // namespace
} // namespace cines
