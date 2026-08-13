#include "CapturedTray.hpp"
#include "GameController.hpp"
#include "MoveListModel.hpp"
#include "PieceRenderer.hpp"

#include "chess/MoveGenerator.hpp"
#include "chess/San.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace cines {
namespace {

using chess::Color;
using chess::Square;

void playAll(GameController& controller, const std::vector<std::string>& sanMoves);

// Plays a list of moves through the controller by looking each one up in the
// legal move list, so the tests read as a game rather than as square pairs.
void playAll(GameController& controller, const std::vector<std::string>& sanMoves)
{
    for (const std::string& san : sanMoves) {
        bool played = false;
        for (const chess::Move& move : chess::generateLegalMoves(controller.position())) {
            if (chess::san::toSan(controller.position(), move) == san) {
                controller.selectSquare(move.from);
                played = controller.moveTo(move.to);
                break;
            }
        }
        ASSERT_TRUE(played) << "could not play " << san;
    }
}

TEST(MoveListModel, IsEmptyBeforeAnyMove)
{
    GameController controller;
    MoveListModel model(controller);

    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.columnCount(), MoveListModel::ColumnCount);
    EXPECT_FALSE(model.indexOfCurrentPly().isValid());
}

TEST(MoveListModel, PairsWhiteAndBlackOnOneRow)
{
    GameController controller;
    MoveListModel model(controller);
    playAll(controller, {"e4", "e5", "Nf3"});
    model.refresh();

    // Three plies is two rows: the second holds White's move and nothing yet.
    ASSERT_EQ(model.rowCount(), 2);
    EXPECT_EQ(model.data(model.index(0, MoveListModel::NumberColumn)).toString(), QStringLiteral("1."));
    EXPECT_EQ(model.data(model.index(0, MoveListModel::WhiteColumn)).toString(), QStringLiteral("e4"));
    EXPECT_EQ(model.data(model.index(0, MoveListModel::BlackColumn)).toString(), QStringLiteral("e5"));
    EXPECT_EQ(model.data(model.index(1, MoveListModel::WhiteColumn)).toString(), QStringLiteral("Nf3"));
    EXPECT_TRUE(model.data(model.index(1, MoveListModel::BlackColumn)).toString().isEmpty());
}

TEST(MoveListModel, MapsACellBackToItsPly)
{
    GameController controller;
    MoveListModel model(controller);
    playAll(controller, {"e4", "e5", "Nf3"});
    model.refresh();

    EXPECT_EQ(model.plyAt(model.index(0, MoveListModel::WhiteColumn)), 1U);
    EXPECT_EQ(model.plyAt(model.index(0, MoveListModel::BlackColumn)), 2U);
    EXPECT_EQ(model.plyAt(model.index(1, MoveListModel::WhiteColumn)), 3U);

    // The empty half of the last pair, and the move number, are not moves.
    EXPECT_EQ(model.plyAt(model.index(1, MoveListModel::BlackColumn)), 0U);
    EXPECT_EQ(model.plyAt(model.index(0, MoveListModel::NumberColumn)), 0U);
    EXPECT_EQ(model.plyAt(QModelIndex()), 0U);
}

TEST(MoveListModel, PointsAtTheMoveTheBoardIsShowing)
{
    GameController controller;
    MoveListModel model(controller);
    playAll(controller, {"e4", "e5", "Nf3"});
    model.refresh();

    EXPECT_EQ(model.plyAt(model.indexOfCurrentPly()), 3U);

    controller.goToPly(1);
    model.refresh();
    EXPECT_EQ(model.plyAt(model.indexOfCurrentPly()), 1U);

    controller.goToPly(0);
    model.refresh();
    EXPECT_FALSE(model.indexOfCurrentPly().isValid()) << "no move has been played yet";
}

// Numbering follows the position the game started from, so a game set up
// mid-play does not restart at one.
TEST(MoveListModel, NumbersFromTheStartingPosition)
{
    GameController controller;
    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("4k3/8/8/8/8/8/8/R3K3 w Q - 5 30")));

    MoveListModel model(controller);
    playAll(controller, {"Ra8+"});
    model.refresh();

    EXPECT_EQ(model.data(model.index(0, MoveListModel::NumberColumn)).toString(), QStringLiteral("30."));
}

TEST(MoveListModel, HeadersNameTheTwoSides)
{
    GameController controller;
    MoveListModel model(controller);

    EXPECT_EQ(model.headerData(MoveListModel::WhiteColumn, Qt::Horizontal, Qt::DisplayRole).toString(),
        QStringLiteral("White"));
    EXPECT_EQ(model.headerData(MoveListModel::BlackColumn, Qt::Horizontal, Qt::DisplayRole).toString(),
        QStringLiteral("Black"));
    EXPECT_FALSE(model.headerData(0, Qt::Vertical, Qt::DisplayRole).isValid());
}

TEST(CapturedTray, ShowsNothingAtTheStartOfAGame)
{
    GameController controller;
    PieceRenderer renderer;
    CapturedTray white(controller, renderer, Color::White);

    EXPECT_EQ(white.materialLead(), 0);
}

TEST(CapturedTray, CountsTheMaterialLeadAfterACapture)
{
    GameController controller;
    PieceRenderer renderer;
    CapturedTray white(controller, renderer, Color::White);
    CapturedTray black(controller, renderer, Color::Black);

    // 1.e4 d5 2.exd5 wins a pawn for White.
    playAll(controller, {"e4", "d5", "exd5"});
    white.refresh();
    black.refresh();

    EXPECT_EQ(white.materialLead(), 1);
    EXPECT_EQ(black.materialLead(), 0) << "only the side ahead shows a number";
}

// The tray is worked out from the board rather than by counting captures as
// they happen, so rewinding cannot leave it out of step.
TEST(CapturedTray, FollowsTheBoardBackwards)
{
    GameController controller;
    PieceRenderer renderer;
    CapturedTray white(controller, renderer, Color::White);

    playAll(controller, {"e4", "d5", "exd5"});
    white.refresh();
    ASSERT_EQ(white.materialLead(), 1);

    controller.undo();
    white.refresh();
    EXPECT_EQ(white.materialLead(), 0) << "the pawn is back on the board";
}

// Captures are counted against the position the game started from, which is
// what makes a custom starting position work with no special case.
TEST(CapturedTray, CountsAgainstACustomStartingPosition)
{
    GameController controller;
    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("4k3/8/8/3q4/4P3/8/8/4K3 w - - 0 1")));

    PieceRenderer renderer;
    CapturedTray white(controller, renderer, Color::White);
    CapturedTray black(controller, renderer, Color::Black);

    // Black starts a queen up, and nothing has been captured yet.
    EXPECT_EQ(black.materialLead(), 8);
    EXPECT_EQ(white.materialLead(), 0);

    playAll(controller, {"exd5"});
    white.refresh();
    black.refresh();

    EXPECT_EQ(white.materialLead(), 1) << "a pawn against nothing";
    EXPECT_EQ(black.materialLead(), 0);
}

TEST(GameControllerPgn, ProducesAReadableGame)
{
    GameController controller;
    playAll(controller, {"e4", "e5", "Nf3", "Nc6"});

    const QString pgn = controller.pgn();
    EXPECT_TRUE(pgn.contains(QStringLiteral("[Event")));
    EXPECT_TRUE(pgn.contains(QStringLiteral("1. e4 e5 2. Nf3 Nc6")));
}

} // namespace
} // namespace cines
