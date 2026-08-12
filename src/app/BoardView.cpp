#include "BoardView.hpp"

#include "BoardGeometry.hpp"
#include "BoardScene.hpp"
#include "GameController.hpp"

#include <QMouseEvent>
#include <QResizeEvent>

namespace cines {
namespace {

// Below this the press is a click, not a drag. Without it a click with a
// twitchy hand would pick the piece up and put it straight back.
constexpr int kDragThresholdPixels = 4;

} // namespace

BoardView::BoardView(GameController& controller, BoardScene& scene, QWidget* parent)
    : QGraphicsView(&scene, parent)
    , controller_(controller)
    , boardScene_(scene)
{
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setTransformationAnchor(QGraphicsView::NoAnchor);

    // The board grows and shrinks with the window rather than scrolling, so a
    // drag must not be interpreted as a rubber band selection.
    setDragMode(QGraphicsView::NoDrag);
}

QSize BoardView::sizeHint() const
{
    return {560, 560};
}

void BoardView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    fitBoard();
}

void BoardView::fitBoard()
{
    fitInView(geometry::boardRect(), Qt::KeepAspectRatio);
}

chess::Square BoardView::squareUnder(const QPoint& viewPosition) const
{
    return geometry::squareAt(mapToScene(viewPosition), boardScene_.isFlipped());
}

void BoardView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    const chess::Square square = squareUnder(event->pos());
    pressedSquare_ = square;
    dragging_ = false;

    // Pressing a legal target while a piece is selected completes the move,
    // which is the click-then-click path.
    if (chess::isValid(controller_.selectedSquare()) && controller_.legalTargets().contains(square)) {
        controller_.moveTo(square);
        pressedSquare_ = chess::Square::None;
        return;
    }

    controller_.selectSquare(square);
    event->accept();
}

void BoardView::mouseMoveEvent(QMouseEvent* event)
{
    if (!chess::isValid(pressedSquare_) || pressedSquare_ != controller_.selectedSquare()) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    if (!dragging_) {
        const QPointF pressedCentre = geometry::squareCentre(pressedSquare_, boardScene_.isFlipped());
        const QPointF here = mapToScene(event->pos());
        // Compare in scene units so the threshold means the same thing at any
        // window size.
        const qreal scale = transform().m11();
        const qreal movedInPixels = QLineF(pressedCentre, here).length() * (scale > 0.0 ? scale : 1.0);
        if (movedInPixels < kDragThresholdPixels) {
            return;
        }
        boardScene_.liftPiece(pressedSquare_);
        dragging_ = true;
    }

    boardScene_.moveLiftedPieceTo(mapToScene(event->pos()));
    event->accept();
}

void BoardView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !dragging_) {
        pressedSquare_ = chess::Square::None;
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }

    dragging_ = false;
    const chess::Square target = squareUnder(event->pos());

    // moveTo rebuilds the pieces when it succeeds, which removes the lifted
    // item. Dropping it first would leave the scene briefly inconsistent, so
    // the drop only happens when the move did not go through.
    if (!chess::isValid(target) || !controller_.moveTo(target)) {
        boardScene_.dropLiftedPiece();
    } else if (boardScene_.hasLiftedPiece()) {
        // A promotion is pending: the board still shows the old position, so
        // the pawn goes back until the choice is made.
        boardScene_.dropLiftedPiece();
    }

    pressedSquare_ = chess::Square::None;
    event->accept();
}

} // namespace cines
