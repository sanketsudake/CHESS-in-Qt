#include "BoardScene.hpp"

#include "BoardGeometry.hpp"
#include "GameController.hpp"
#include "PieceRenderer.hpp"

#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QPen>
#include <QSvgRenderer>

namespace cines {
namespace {

// Stacking order. Highlights sit above the squares and below the pieces, so a
// piece is never obscured by the marks that describe it.
constexpr qreal kSquareZ = 0.0;
constexpr qreal kLastMoveZ = 1.0;
constexpr qreal kCheckZ = 2.0;
constexpr qreal kSelectionZ = 3.0;
constexpr qreal kPieceZ = 10.0;
constexpr qreal kTargetMarkZ = 11.0;
constexpr qreal kLiftedPieceZ = 20.0;

constexpr qreal kTargetDotDiameter = geometry::kSquareSize * 0.30;
constexpr qreal kCaptureRingWidth = geometry::kSquareSize * 0.08;

void clearItems(QGraphicsScene& scene, QList<QGraphicsItem*>& items)
{
    for (QGraphicsItem* item : items) {
        scene.removeItem(item);
        delete item;
    }
    items.clear();
}

} // namespace

BoardScene::BoardScene(GameController& controller, QObject* parent)
    : QGraphicsScene(parent)
    , controller_(controller)
    , renderer_(new PieceRenderer(this))
    , theme_(Theme::light())
{
    setSceneRect(geometry::boardRect());
    buildSquares();
    rebuildPieces();
}

bool BoardScene::isLightSquare(chess::Square square)
{
    return ((static_cast<int>(chess::fileOf(square)) + static_cast<int>(chess::rankOf(square))) % 2) != 0;
}

void BoardScene::buildSquares()
{
    for (int i = 0; i < chess::kSquareCount; ++i) {
        const auto square = static_cast<chess::Square>(i);
        auto* item = addRect(geometry::squareRect(square, flipped_), Qt::NoPen);
        item->setZValue(kSquareZ);
        squareItems_.insert(i, item);
    }
    updateSquareColours();
}

void BoardScene::updateSquareColours()
{
    for (auto it = squareItems_.constBegin(); it != squareItems_.constEnd(); ++it) {
        const auto square = static_cast<chess::Square>(it.key());
        it.value()->setRect(geometry::squareRect(square, flipped_));
        it.value()->setBrush(isLightSquare(square) ? theme_.lightSquare : theme_.darkSquare);
    }
}

void BoardScene::setTheme(const Theme& theme)
{
    theme_ = theme;
    updateSquareColours();
    rebuildPieces();
    updateSelection(controller_.selectedSquare(), controller_.legalTargets());
}

void BoardScene::setFlipped(bool flipped)
{
    if (flipped_ == flipped) {
        return;
    }
    flipped_ = flipped;
    updateSquareColours();
    rebuildPieces();
    updateSelection(controller_.selectedSquare(), controller_.legalTargets());
}

QGraphicsSvgItem* BoardScene::pieceItemAt(chess::Square square) const
{
    if (!chess::isValid(square)) {
        return nullptr;
    }
    return pieceItems_.value(chess::index(square), nullptr);
}

void BoardScene::rebuildPieces()
{
    // A lifted piece belongs to a drag that the new position has invalidated.
    liftedItem_ = nullptr;
    liftedFrom_ = chess::Square::None;

    for (QGraphicsSvgItem* item : pieceItems_) {
        removeItem(item);
        delete item;
    }
    pieceItems_.clear();

    const chess::Board& board = controller_.position().board;

    for (int i = 0; i < chess::kSquareCount; ++i) {
        const auto square = static_cast<chess::Square>(i);
        const chess::Piece piece = board.pieceAt(square);
        QSvgRenderer* svg = renderer_->rendererFor(piece);
        if (svg == nullptr) {
            continue;
        }

        auto* item = new QGraphicsSvgItem;
        item->setSharedRenderer(svg);
        item->setZValue(kPieceZ);

        // The SVGs are not all authored at the same size, so each is scaled to
        // the square rather than assumed to fit it.
        const QRectF bounds = item->boundingRect();
        const qreal longestSide = std::max(bounds.width(), bounds.height());
        if (longestSide > 0.0) {
            item->setScale(geometry::kSquareSize * geometry::kPieceScale / longestSide);
        }

        const QRectF scaled = item->mapRectToScene(item->boundingRect());
        const QPointF centre = geometry::squareCentre(square, flipped_);
        item->setPos(centre.x() - (scaled.width() / 2.0), centre.y() - (scaled.height() / 2.0));

        addItem(item);
        pieceItems_.insert(i, item);
    }

    updateLastMoveHighlight();
    updateCheckHighlight();
}

void BoardScene::updateSelection(chess::Square selected, const QList<chess::Square>& targets)
{
    clearItems(*this, selectionItems_);

    if (chess::isValid(selected)) {
        auto* highlight = addRect(geometry::squareRect(selected, flipped_), Qt::NoPen, theme_.selection);
        highlight->setZValue(kSelectionZ);
        selectionItems_.append(highlight);
    }

    const chess::Board& board = controller_.position().board;

    for (const chess::Square target : targets) {
        const QRectF square = geometry::squareRect(target, flipped_);

        if (board.isEmpty(target)) {
            // A dot for a quiet move.
            const QRectF dot(square.center().x() - (kTargetDotDiameter / 2.0),
                square.center().y() - (kTargetDotDiameter / 2.0), kTargetDotDiameter, kTargetDotDiameter);
            auto* mark = addEllipse(dot, Qt::NoPen, theme_.legalTarget);
            mark->setZValue(kTargetMarkZ);
            selectionItems_.append(mark);
            continue;
        }

        // A ring for a capture, so the piece underneath stays visible.
        QPen ring(theme_.legalTarget);
        ring.setWidthF(kCaptureRingWidth);
        auto* mark = addEllipse(square.adjusted(kCaptureRingWidth / 2.0, kCaptureRingWidth / 2.0,
                                    -kCaptureRingWidth / 2.0, -kCaptureRingWidth / 2.0),
            ring, Qt::NoBrush);
        mark->setZValue(kTargetMarkZ);
        selectionItems_.append(mark);
    }
}

void BoardScene::updateLastMoveHighlight()
{
    clearItems(*this, lastMoveItems_);

    const chess::PlayedMove* played = controller_.lastMove();
    if (played == nullptr) {
        return;
    }

    for (const chess::Square square : {played->move.from, played->move.to}) {
        auto* highlight = addRect(geometry::squareRect(square, flipped_), Qt::NoPen, theme_.lastMove);
        highlight->setZValue(kLastMoveZ);
        lastMoveItems_.append(highlight);
    }
}

void BoardScene::updateCheckHighlight()
{
    clearItems(*this, checkItems_);

    for (const chess::Color color : {chess::Color::White, chess::Color::Black}) {
        if (!controller_.isInCheck(color)) {
            continue;
        }
        const chess::Square king = controller_.position().board.kingSquare(color);
        if (!chess::isValid(king)) {
            continue;
        }
        auto* highlight = addRect(geometry::squareRect(king, flipped_), Qt::NoPen, theme_.check);
        highlight->setZValue(kCheckZ);
        checkItems_.append(highlight);
    }
}

void BoardScene::liftPiece(chess::Square square)
{
    QGraphicsSvgItem* item = pieceItemAt(square);
    if (item == nullptr) {
        return;
    }
    liftedItem_ = item;
    liftedFrom_ = square;
    item->setZValue(kLiftedPieceZ);
    item->setOpacity(0.85);
}

void BoardScene::moveLiftedPieceTo(const QPointF& scenePosition)
{
    if (liftedItem_ == nullptr) {
        return;
    }
    const QRectF bounds = liftedItem_->mapRectToScene(liftedItem_->boundingRect());
    liftedItem_->setPos(
        scenePosition.x() - (bounds.width() / 2.0), scenePosition.y() - (bounds.height() / 2.0));
}

void BoardScene::dropLiftedPiece()
{
    if (liftedItem_ == nullptr) {
        return;
    }

    // Snap back to where it came from. When the drop was a legal move the
    // controller has already emitted positionChanged and the whole set of
    // pieces has been rebuilt, so this only runs for a cancelled drag.
    const QRectF bounds = liftedItem_->mapRectToScene(liftedItem_->boundingRect());
    const QPointF centre = geometry::squareCentre(liftedFrom_, flipped_);
    liftedItem_->setPos(centre.x() - (bounds.width() / 2.0), centre.y() - (bounds.height() / 2.0));
    liftedItem_->setZValue(kPieceZ);
    liftedItem_->setOpacity(1.0);

    liftedItem_ = nullptr;
    liftedFrom_ = chess::Square::None;
}

} // namespace cines
