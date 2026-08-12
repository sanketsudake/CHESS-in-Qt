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

    connect(&scene, &BoardScene::draggedPieceInvalidated, this, &BoardView::cancelDrag);
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
    pressedAt_ = event->pos();

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

    if (!isDragging()) {
        // Measured from where the button actually went down, in view pixels.
        // Measuring from the square's centre would mean any press away from
        // dead centre was already past the threshold, so a plain click would
        // pick the piece up and put it straight back.
        if ((event->pos() - pressedAt_).manhattanLength() < kDragThresholdPixels) {
            return;
        }
        boardScene_.liftPiece(pressedSquare_);
    }

    boardScene_.moveLiftedPieceTo(mapToScene(event->pos()));
    event->accept();
}

bool BoardView::isDragging() const
{
    return boardScene_.hasLiftedPiece();
}

void BoardView::cancelDrag()
{
    boardScene_.dropLiftedPiece();
    pressedSquare_ = chess::Square::None;
}

void BoardView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        // A right or middle button coming up during a drag ends it. Leaving
        // the drag live would let the eventual left release play a move to
        // wherever the cursor had wandered.
        cancelDrag();
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }

    if (!isDragging()) {
        pressedSquare_ = chess::Square::None;
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }

    const chess::Square target = squareUnder(event->pos());

    // The move goes first: a successful one rebuilds the pieces and takes the
    // lifted item with it, and dropping beforehand would leave the scene
    // briefly showing a piece on two squares. Afterwards the drop is a no-op
    // unless something is still lifted -- a refused move, or a promotion
    // waiting on the dialog.
    if (chess::isValid(target)) {
        controller_.moveTo(target);
    }
    boardScene_.dropLiftedPiece();

    pressedSquare_ = chess::Square::None;
    event->accept();
}

} // namespace cines
