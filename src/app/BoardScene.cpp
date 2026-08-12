#include "BoardScene.hpp"

#include "BoardGeometry.hpp"
#include "GameController.hpp"
#include "PieceRenderer.hpp"

#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsOpacityEffect>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSvgItem>
#include <QPen>
#include <QPropertyAnimation>
#include <QSvgRenderer>

#include <array>

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

constexpr qreal kCoordinateZ = 4.0;

constexpr qreal kTargetDotDiameter = geometry::kSquareSize * 0.30;
constexpr qreal kCaptureRingWidth = geometry::kSquareSize * 0.08;

// Long enough to read as movement, short enough that a fast player never waits
// for it. The piece is already on its destination as far as the rules are
// concerned; this only animates where it is drawn.
constexpr int kMoveAnimationMs = 160;

// Two soft beats rather than a strobe. A steady colour would be missed on a
// glance, and a fast flash is unpleasant to sit next to.
constexpr int kCheckPulseMs = 900;

constexpr qreal kCoordinateFontSize = geometry::kSquareSize * 0.17;
constexpr qreal kCoordinateInset = geometry::kSquareSize * 0.06;

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
    buildCoordinates();
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

// Coordinates are drawn inside the edge squares rather than in a margin, so
// the board stays a plain 8x8 and nothing has to be reserved around it. Each
// label takes the colour of the opposite square so it reads against its own.
void BoardScene::buildCoordinates()
{
    QFont font;
    font.setPointSizeF(kCoordinateFontSize);
    font.setBold(true);

    for (int i = 0; i < chess::kBoardSize; ++i) {
        for (int axis = 0; axis < 2; ++axis) {
            auto* label = addSimpleText(QString(), font);
            label->setZValue(kCoordinateZ);
            coordinateItems_.append(label);
        }
    }
    updateCoordinates();
}

void BoardScene::updateCoordinates()
{
    int index = 0;
    for (int i = 0; i < chess::kBoardSize; ++i) {
        // File letters along the bottom edge, rank digits along the left edge.
        const chess::Square fileSquare = chess::makeSquare(i, flipped_ ? chess::kBoardSize - 1 : 0);
        const chess::Square rankSquare = chess::makeSquare(flipped_ ? chess::kBoardSize - 1 : 0, i);

        struct Label {
            chess::Square square;
            QString text;
            bool alignToBottomRight;
        };

        const std::array<Label, 2> labels{{
            {fileSquare, QString(QChar('a' + i)), true},
            {rankSquare, QString::number(i + 1), false},
        }};

        for (const Label& label : labels) {
            QGraphicsSimpleTextItem* item = coordinateItems_.at(index++);
            item->setText(label.text);
            item->setBrush(isLightSquare(label.square) ? theme_.darkSquare : theme_.lightSquare);

            const QRectF square = geometry::squareRect(label.square, flipped_);
            const QRectF bounds = item->boundingRect();
            if (label.alignToBottomRight) {
                item->setPos(square.right() - bounds.width() - kCoordinateInset,
                    square.bottom() - bounds.height() - kCoordinateInset);
            } else {
                item->setPos(square.left() + kCoordinateInset, square.top() + kCoordinateInset);
            }
        }
    }
}

void BoardScene::setTheme(const Theme& theme)
{
    theme_ = theme;
    updateSquareColours();
    updateCoordinates();
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
    updateCoordinates();
    rebuildPieces();
    updateSelection(controller_.selectedSquare(), controller_.legalTargets());
}

void BoardScene::noteMovePlayed(const chess::Move& move)
{
    pendingAnimation_ = move;
    pendingAnimationIsUndo_ = false;
}

void BoardScene::noteMoveUndone(const chess::Move& move)
{
    pendingAnimation_ = move;
    pendingAnimationIsUndo_ = true;
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
    // A lifted piece belongs to a drag whose item is about to be deleted. Tell
    // whoever was dragging, or they will keep believing the drag is live and
    // play a move when the button eventually comes up.
    const bool wasDragging = liftedItem_ != nullptr;
    liftedItem_ = nullptr;
    liftedFrom_ = chess::Square::None;
    if (wasDragging) {
        emit draggedPieceInvalidated();
    }

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
    runPendingAnimation();
}

// Animation is purely cosmetic: the rebuild above has already put every piece
// where the rules say it is. This puts the moved piece back where it came from
// and lets it travel, so nothing can end up drawn in the wrong place even if
// an animation is interrupted.
void BoardScene::slideItemFrom(QGraphicsItem* item, const QPointF& offset)
{
    auto* object = dynamic_cast<QGraphicsObject*>(item);
    if (object == nullptr) {
        return;
    }

    const QPointF target = object->pos();
    const QPointF start = target + offset;
    object->setPos(start);

    auto* animation = new QPropertyAnimation(object, "pos", this);
    animation->setDuration(kMoveAnimationMs);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(start);
    animation->setEndValue(target);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void BoardScene::runPendingAnimation()
{
    const std::optional<chess::Move> move = pendingAnimation_;
    const bool undoing = pendingAnimationIsUndo_;
    pendingAnimation_.reset();

    if (!move || !animationEnabled_) {
        return;
    }

    // Undoing walks the same move backwards, so the two squares swap roles.
    const chess::Square from = undoing ? move->to : move->from;
    const chess::Square to = undoing ? move->from : move->to;

    QGraphicsSvgItem* item = pieceItemAt(to);
    if (item == nullptr) {
        return;
    }

    // The offset between two square centres is exactly the offset between the
    // item positions, whatever the piece's own size and scale happen to be.
    const QPointF offset = geometry::squareCentre(from, flipped_) - geometry::squareCentre(to, flipped_);
    slideItemFrom(item, offset);

    if (!move->isCastle()) {
        return;
    }

    // The rook travels with the king, or the castle looks like the king
    // teleporting past a stationary rook.
    const int backRank = static_cast<int>(chess::rankOf(move->from));
    const bool kingSide = move->kind == chess::MoveKind::CastleKingSide;
    chess::Square rookFrom = chess::makeSquare(kingSide ? 7 : 0, backRank);
    chess::Square rookTo = chess::makeSquare(kingSide ? 5 : 3, backRank);
    if (undoing) {
        std::swap(rookFrom, rookTo);
    }

    if (QGraphicsSvgItem* rook = pieceItemAt(rookTo)) {
        slideItemFrom(
            rook, geometry::squareCentre(rookFrom, flipped_) - geometry::squareCentre(rookTo, flipped_));
    }
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

        if (!animationEnabled_) {
            continue;
        }

        // Two soft beats and then it settles. A steady colour is missed on a
        // glance and a fast flash is unpleasant to sit next to.
        //
        // The effect belongs to the item, so when the highlight is cleared the
        // effect goes with it and the animation stops on a destroyed target
        // rather than writing to freed memory.
        // Created unparented; setGraphicsEffect takes ownership. A
        // QGraphicsRectItem is not a QObject and so cannot be the parent.
        auto* fade = new QGraphicsOpacityEffect;
        highlight->setGraphicsEffect(fade);

        auto* pulse = new QPropertyAnimation(fade, "opacity", this);
        pulse->setDuration(kCheckPulseMs);
        pulse->setKeyValueAt(0.0, 0.30);
        pulse->setKeyValueAt(0.25, 1.00);
        pulse->setKeyValueAt(0.50, 0.45);
        pulse->setKeyValueAt(0.75, 1.00);
        pulse->setKeyValueAt(1.0, 0.85);
        pulse->setEasingCurve(QEasingCurve::InOutSine);
        pulse->start(QAbstractAnimation::DeleteWhenStopped);
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
