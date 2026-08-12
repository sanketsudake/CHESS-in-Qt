#pragma once

#include "ChessMetaTypes.hpp"
#include "Theme.hpp"
#include "chess/Types.hpp"

#include <QGraphicsScene>
#include <QHash>
#include <QList>

#include <optional>

class QGraphicsRectItem;
class QGraphicsSimpleTextItem;
class QGraphicsSvgItem;

namespace cines {

class GameController;
class PieceRenderer;

// Draws the board. It reads nothing from chess::Game -- everything it shows
// arrives through GameController's signals, so the rules could be replaced
// without touching this file, and this file could be replaced without touching
// the rules.
//
// Squares, highlights and pieces are separate layers of graphics items rather
// than one repainted widget, which is what lets a piece animate across the
// board in the next step without redrawing anything underneath it.
class BoardScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit BoardScene(GameController& controller, QObject* parent = nullptr);

    void setTheme(const Theme& theme);
    [[nodiscard]] const Theme& theme() const { return theme_; }

    // Black at the bottom. The geometry helpers take this as an argument, so
    // nothing else in the drawing code needs to know which way round it is.
    void setFlipped(bool flipped);
    [[nodiscard]] bool isFlipped() const { return flipped_; }

    // The piece currently drawn on a square, or null. The view uses this to
    // pick a piece up for dragging.
    [[nodiscard]] QGraphicsSvgItem* pieceItemAt(chess::Square square) const;

    // Lifts a piece out of the square grid so it can follow the cursor, and
    // puts it back afterwards. While a piece is lifted it draws above every
    // other item.
    void liftPiece(chess::Square square);
    void dropLiftedPiece();
    void moveLiftedPieceTo(const QPointF& scenePosition);
    [[nodiscard]] bool hasLiftedPiece() const { return liftedItem_ != nullptr; }

    // Animation is a courtesy, not a correctness feature. Tests turn it off so
    // they can assert final positions without waiting on the event loop.
    void setAnimationEnabled(bool enabled) { animationEnabled_ = enabled; }
    [[nodiscard]] bool isAnimationEnabled() const { return animationEnabled_; }

signals:
    // A rebuild has deleted every piece item, including any the view had
    // picked up. Whoever is dragging must let go.
    void draggedPieceInvalidated();

public slots:
    void rebuildPieces();
    void updateSelection(chess::Square selected, const QList<chess::Square>& targets);

    // Remembers a move so the next rebuild can glide the piece into place
    // instead of teleporting it. The controller emits the move before it emits
    // the position change, so the hint always arrives first.
    void noteMovePlayed(const chess::Move& move);
    void noteMoveUndone(const chess::Move& move);

private:
    void buildSquares();
    void buildCoordinates();
    void updateSquareColours();
    void updateCoordinates();
    void updateLastMoveHighlight();
    void updateCheckHighlight();
    void runPendingAnimation();
    void slideItemFrom(QGraphicsItem* item, const QPointF& offset);
    [[nodiscard]] static bool isLightSquare(chess::Square square);

    GameController& controller_;
    PieceRenderer* renderer_;
    Theme theme_;
    bool flipped_ = false;

    QHash<int, QGraphicsRectItem*> squareItems_;
    QHash<int, QGraphicsSvgItem*> pieceItems_;
    QList<QGraphicsSimpleTextItem*> coordinateItems_;

    // Highlight layers, kept separately so each can be cleared without
    // disturbing the others.
    QList<QGraphicsItem*> selectionItems_;
    QList<QGraphicsItem*> lastMoveItems_;
    QList<QGraphicsItem*> checkItems_;

    QGraphicsSvgItem* liftedItem_ = nullptr;
    chess::Square liftedFrom_ = chess::Square::None;

    // The move to animate on the next rebuild, and which way round to play it.
    std::optional<chess::Move> pendingAnimation_;
    bool pendingAnimationIsUndo_ = false;
    bool animationEnabled_ = true;
};

} // namespace cines
