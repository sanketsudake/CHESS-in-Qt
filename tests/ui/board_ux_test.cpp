#include "BoardGeometry.hpp"
#include "BoardScene.hpp"
#include "GameController.hpp"
#include "Theme.hpp"

#include <QGraphicsSimpleTextItem>
#include <QGraphicsSvgItem>
#include <QSignalSpy>

#include <gtest/gtest.h>

namespace cines {
namespace {

using chess::Square;

// Collects the coordinate labels, which are the only simple-text items on the
// board.
QList<QGraphicsSimpleTextItem*> coordinateLabels(const BoardScene& scene)
{
    QList<QGraphicsSimpleTextItem*> labels;
    for (QGraphicsItem* item : scene.items()) {
        if (auto* text = dynamic_cast<QGraphicsSimpleTextItem*>(item)) {
            labels.append(text);
        }
    }
    return labels;
}

QStringList labelTexts(const BoardScene& scene)
{
    QStringList texts;
    for (QGraphicsSimpleTextItem* label : coordinateLabels(scene)) {
        texts.append(label->text());
    }
    texts.sort();
    return texts;
}

TEST(Theme, FollowsTheColourSchemeAndTreatsUnknownAsLight)
{
    EXPECT_EQ(Theme::forScheme(Qt::ColorScheme::Dark).lightSquare, Theme::dark().lightSquare);
    EXPECT_EQ(Theme::forScheme(Qt::ColorScheme::Light).lightSquare, Theme::light().lightSquare);

    // A platform with no preference is not a reason to guess dark.
    EXPECT_EQ(Theme::forScheme(Qt::ColorScheme::Unknown).lightSquare, Theme::light().lightSquare);
}

TEST(Theme, AnExplicitChoiceIgnoresTheDesktop)
{
    EXPECT_EQ(themeFor(ThemeChoice::Light).darkSquare, Theme::light().darkSquare);
    EXPECT_EQ(themeFor(ThemeChoice::Dark).darkSquare, Theme::dark().darkSquare);
}

TEST(Theme, LightAndDarkAreActuallyDifferent)
{
    EXPECT_NE(Theme::light().lightSquare, Theme::dark().lightSquare);
    EXPECT_NE(Theme::light().darkSquare, Theme::dark().darkSquare);
    EXPECT_NE(Theme::light().check, Theme::dark().check);
}

TEST(BoardCoordinates, EightFilesAndEightRanksAreDrawn)
{
    GameController controller;
    BoardScene scene(controller);

    EXPECT_EQ(coordinateLabels(scene).size(), 16);
    EXPECT_EQ(labelTexts(scene),
        (QStringList{QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4"),
            QStringLiteral("5"), QStringLiteral("6"), QStringLiteral("7"), QStringLiteral("8"),
            QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d"),
            QStringLiteral("e"), QStringLiteral("f"), QStringLiteral("g"), QStringLiteral("h")}));
}

// The labels live inside the edge squares rather than in a margin, so the
// board is still exactly eight squares wide.
TEST(BoardCoordinates, StayInsideTheBoard)
{
    GameController controller;
    BoardScene scene(controller);

    for (QGraphicsSimpleTextItem* label : coordinateLabels(scene)) {
        const QRectF bounds = label->mapRectToScene(label->boundingRect());
        EXPECT_TRUE(geometry::boardRect().contains(bounds)) << label->text().toStdString();
    }
    EXPECT_EQ(scene.sceneRect(), geometry::boardRect());
}

TEST(BoardCoordinates, FollowTheBoardWhenItIsFlipped)
{
    GameController controller;
    BoardScene scene(controller);

    QList<QPointF> before;
    for (QGraphicsSimpleTextItem* label : coordinateLabels(scene)) {
        before.append(label->pos());
    }

    scene.setFlipped(true);

    QList<QPointF> after;
    for (QGraphicsSimpleTextItem* label : coordinateLabels(scene)) {
        after.append(label->pos());
    }

    EXPECT_NE(before, after);
    EXPECT_EQ(labelTexts(scene).size(), 16) << "the same labels, in new places";
}

TEST(BoardCoordinates, ChangeColourWithTheTheme)
{
    GameController controller;
    BoardScene scene(controller);
    scene.setTheme(Theme::light());

    const QColor lightThemeColour = coordinateLabels(scene).first()->brush().color();

    scene.setTheme(Theme::dark());
    const QColor darkThemeColour = coordinateLabels(scene).first()->brush().color();

    EXPECT_NE(lightThemeColour, darkThemeColour);
}

// Animation is cosmetic: the rebuild has already put every piece where the
// rules say it is, and the animation only changes where it is drawn on the way
// there. With animation off, the piece is simply there.
TEST(BoardAnimation, WithAnimationOffThePieceIsAtItsDestinationImmediately)
{
    GameController controller;
    BoardScene scene(controller);
    scene.setAnimationEnabled(false);

    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));
    scene.noteMovePlayed(controller.lastMove()->move);
    scene.rebuildPieces();

    QGraphicsSvgItem* pawn = scene.pieceItemAt(Square::E4);
    ASSERT_NE(pawn, nullptr);

    const QRectF drawn = pawn->mapRectToScene(pawn->boundingRect());
    EXPECT_TRUE(geometry::squareRect(Square::E4, false).contains(drawn.center()));
}

TEST(BoardAnimation, WithAnimationOnThePieceStartsFromWhereItCameFrom)
{
    GameController controller;
    BoardScene scene(controller);
    ASSERT_TRUE(scene.isAnimationEnabled());

    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));
    scene.noteMovePlayed(controller.lastMove()->move);
    scene.rebuildPieces();

    QGraphicsSvgItem* pawn = scene.pieceItemAt(Square::E4);
    ASSERT_NE(pawn, nullptr);

    // The animation has been started but not stepped, so the piece is drawn on
    // the square it left.
    const QRectF drawn = pawn->mapRectToScene(pawn->boundingRect());
    EXPECT_TRUE(geometry::squareRect(Square::E2, false).contains(drawn.center()));
}

TEST(BoardAnimation, AnUndoneMoveTravelsTheOtherWay)
{
    GameController controller;
    BoardScene scene(controller);

    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));
    const chess::Move move = controller.lastMove()->move;

    controller.undo();
    scene.noteMoveUndone(move);
    scene.rebuildPieces();

    QGraphicsSvgItem* pawn = scene.pieceItemAt(Square::E2);
    ASSERT_NE(pawn, nullptr);

    const QRectF drawn = pawn->mapRectToScene(pawn->boundingRect());
    EXPECT_TRUE(geometry::squareRect(Square::E4, false).contains(drawn.center()))
        << "it starts where it was and returns to where it came from";
}

// A castle moves two pieces. Animating only the king would look like it
// teleported past a rook that never moved.
TEST(BoardAnimation, CastlingMovesTheRookAsWell)
{
    GameController controller;
    ASSERT_TRUE(controller.setPositionFromFen(QStringLiteral("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1")));
    BoardScene scene(controller);

    controller.selectSquare(Square::E1);
    ASSERT_TRUE(controller.moveTo(Square::G1));
    scene.noteMovePlayed(controller.lastMove()->move);
    scene.rebuildPieces();

    QGraphicsSvgItem* rook = scene.pieceItemAt(Square::F1);
    ASSERT_NE(rook, nullptr);

    const QRectF drawn = rook->mapRectToScene(rook->boundingRect());
    EXPECT_TRUE(geometry::squareRect(Square::H1, false).contains(drawn.center()))
        << "the rook sets off from the corner it stood in";
}

TEST(BoardAnimation, ARebuildWithNoNotedMoveDoesNotAnimate)
{
    GameController controller;
    BoardScene scene(controller);
    scene.rebuildPieces();

    QGraphicsSvgItem* pawn = scene.pieceItemAt(Square::E2);
    ASSERT_NE(pawn, nullptr);

    const QRectF drawn = pawn->mapRectToScene(pawn->boundingRect());
    EXPECT_TRUE(geometry::squareRect(Square::E2, false).contains(drawn.center()));
}

TEST(GameControllerAnimation, UndoAnnouncesTheMoveItTookBack)
{
    GameController controller;
    QSignalSpy undoneSpy(&controller, &GameController::moveUndone);

    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));

    controller.undo();

    ASSERT_EQ(undoneSpy.count(), 1);
    EXPECT_EQ(undoneSpy.at(0).at(0).value<chess::Move>().to, Square::E4);
}

TEST(GameControllerAnimation, UndoWithNothingToTakeBackSaysNothing)
{
    GameController controller;
    QSignalSpy undoneSpy(&controller, &GameController::moveUndone);

    controller.undo();

    EXPECT_EQ(undoneSpy.count(), 0);
}

// Redo replays a move, so it announces one just as playing it did, and the
// board animates it the same way.
TEST(GameControllerAnimation, RedoAnnouncesTheMoveAgain)
{
    GameController controller;
    controller.selectSquare(Square::E2);
    ASSERT_TRUE(controller.moveTo(Square::E4));
    controller.undo();

    QSignalSpy moveSpy(&controller, &GameController::moveMade);
    controller.redo();

    ASSERT_EQ(moveSpy.count(), 1);
    EXPECT_EQ(moveSpy.at(0).at(0).value<chess::Move>().to, Square::E4);
    EXPECT_EQ(moveSpy.at(0).at(1).toString(), QStringLiteral("e4"));
}

} // namespace
} // namespace cines
