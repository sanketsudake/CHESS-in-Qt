#include "BoardGeometry.hpp"
#include "BoardScene.hpp"
#include "GameController.hpp"
#include "Theme.hpp"

#include <QGraphicsSvgItem>

#include <gtest/gtest.h>

namespace cines {
namespace {

using chess::Square;

TEST(BoardGeometry, EverySquareRoundTripsThroughItsCentre)
{
    for (const bool flipped : {false, true}) {
        for (int i = 0; i < chess::kSquareCount; ++i) {
            const auto square = static_cast<Square>(i);
            const QPointF centre = geometry::squareCentre(square, flipped);
            EXPECT_EQ(geometry::squareAt(centre, flipped), square)
                << chess::toString(square) << (flipped ? " flipped" : "");
        }
    }
}

// Rank 1 is at the bottom of the screen when the board is not flipped, and the
// scene's origin is top left, so the rank axis has to be inverted. Getting
// this backwards would put White on top.
TEST(BoardGeometry, WhitesBackRankIsAtTheBottomUntilTheBoardIsFlipped)
{
    EXPECT_GT(geometry::squareCentre(Square::A1, false).y(), geometry::squareCentre(Square::A8, false).y());

    EXPECT_LT(geometry::squareCentre(Square::A1, true).y(), geometry::squareCentre(Square::A8, true).y());
}

TEST(BoardGeometry, FilesRunLeftToRightUntilTheBoardIsFlipped)
{
    EXPECT_LT(geometry::squareCentre(Square::A1, false).x(), geometry::squareCentre(Square::H1, false).x());

    EXPECT_GT(geometry::squareCentre(Square::A1, true).x(), geometry::squareCentre(Square::H1, true).x());
}

TEST(BoardGeometry, PointsOffTheBoardBelongToNoSquare)
{
    EXPECT_EQ(geometry::squareAt(QPointF(-1.0, 10.0), false), Square::None);
    EXPECT_EQ(geometry::squareAt(QPointF(10.0, -1.0), false), Square::None);
    EXPECT_EQ(geometry::squareAt(QPointF(geometry::kBoardSize + 1.0, 10.0), false), Square::None);
    EXPECT_EQ(geometry::squareAt(QPointF(10.0, geometry::kBoardSize + 1.0), false), Square::None);
}

TEST(BoardScene, DrawsOnePieceItemPerOccupiedSquare)
{
    GameController controller;
    BoardScene scene(controller);

    int drawn = 0;
    for (int i = 0; i < chess::kSquareCount; ++i) {
        const auto square = static_cast<Square>(i);
        const bool occupied = !controller.position().board.isEmpty(square);
        const bool hasItem = scene.pieceItemAt(square) != nullptr;
        EXPECT_EQ(occupied, hasItem) << chess::toString(square);
        drawn += hasItem ? 1 : 0;
    }
    EXPECT_EQ(drawn, 32);
}

TEST(BoardScene, RedrawsAfterAMove)
{
    GameController controller;
    BoardScene scene(controller);

    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));
    scene.rebuildPieces();

    EXPECT_EQ(scene.pieceItemAt(Square::E2), nullptr);
    EXPECT_NE(scene.pieceItemAt(Square::E4), nullptr);
}

TEST(BoardScene, TheSceneIsExactlyTheBoard)
{
    GameController controller;
    BoardScene scene(controller);
    EXPECT_EQ(scene.sceneRect(), geometry::boardRect());
}

TEST(BoardScene, LiftingAPieceRaisesItAndDroppingPutsItBack)
{
    GameController controller;
    BoardScene scene(controller);

    QGraphicsSvgItem* pawn = scene.pieceItemAt(Square::E2);
    ASSERT_NE(pawn, nullptr);
    const QPointF resting = pawn->pos();

    scene.liftPiece(Square::E2);
    EXPECT_TRUE(scene.hasLiftedPiece());

    scene.moveLiftedPieceTo(QPointF(400.0, 120.0));
    EXPECT_NE(pawn->pos(), resting);

    scene.dropLiftedPiece();
    EXPECT_FALSE(scene.hasLiftedPiece());
    EXPECT_EQ(pawn->pos(), resting) << "a cancelled drag returns the piece to its square";
}

TEST(BoardScene, LiftingAnEmptySquareDoesNothing)
{
    GameController controller;
    BoardScene scene(controller);

    scene.liftPiece(Square::E4);
    EXPECT_FALSE(scene.hasLiftedPiece());

    scene.liftPiece(Square::None);
    EXPECT_FALSE(scene.hasLiftedPiece());
}

TEST(BoardScene, FlippingMovesThePiecesWithoutChangingThePosition)
{
    GameController controller;
    BoardScene scene(controller);

    QGraphicsSvgItem* before = scene.pieceItemAt(Square::A1);
    ASSERT_NE(before, nullptr);
    const QPointF unflipped = before->pos();

    scene.setFlipped(true);
    EXPECT_TRUE(scene.isFlipped());

    QGraphicsSvgItem* after = scene.pieceItemAt(Square::A1);
    ASSERT_NE(after, nullptr) << "a1 still holds a rook whichever way the board faces";
    EXPECT_NE(after->pos(), unflipped);
    EXPECT_EQ(controller.position(), chess::Position::starting());
}

TEST(BoardScene, SelectingAPieceMarksItsTargets)
{
    GameController controller;
    BoardScene scene(controller);
    const qsizetype quiet = scene.items().size();

    controller.selectSquare(Square::E2);
    scene.updateSelection(controller.selectedSquare(), controller.legalTargets());

    // One highlight for the square plus a mark for each of the two targets.
    EXPECT_EQ(scene.items().size(), quiet + 3);

    scene.updateSelection(Square::None, {});
    EXPECT_EQ(scene.items().size(), quiet);
}

TEST(BoardScene, ChangingThemeKeepsThePiecesOnTheBoard)
{
    GameController controller;
    BoardScene scene(controller);

    scene.setTheme(Theme::dark());

    EXPECT_EQ(scene.theme().lightSquare, Theme::dark().lightSquare);
    EXPECT_NE(scene.pieceItemAt(Square::E1), nullptr);
}

} // namespace
} // namespace cines
