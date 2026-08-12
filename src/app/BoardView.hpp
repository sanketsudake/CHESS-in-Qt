#pragma once

#include "ChessMetaTypes.hpp"
#include "chess/Types.hpp"

#include <QGraphicsView>

namespace cines {

class BoardScene;
class GameController;

// Turns mouse events into squares and hands them to the controller.
//
// The view owns no game state. It knows where the board is on screen and
// nothing about what a legal move is, which is why it can support click-then-
// click and drag-and-drop through the same two controller calls.
class BoardView : public QGraphicsView {
    Q_OBJECT

public:
    BoardView(GameController& controller, BoardScene& scene, QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    // Keeps the board square and fully visible at any window size. This is the
    // whole reason the scene works in its own coordinate space.
    void resizeEvent(QResizeEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

public slots:
    // Abandons a drag in progress and puts the piece back. The board calls
    // this when it rebuilds -- a flip or a theme change deletes the item being
    // dragged, and a view that still believed the drag was live would play a
    // move to wherever the button happened to come up.
    void cancelDrag();

private:
    [[nodiscard]] chess::Square squareUnder(const QPoint& viewPosition) const;
    void fitBoard();

    GameController& controller_;
    BoardScene& boardScene_;

    // Where the press landed, so a release on the same square reads as a click
    // rather than as a zero-length drag.
    chess::Square pressedSquare_ = chess::Square::None;

    // The press position itself, in view coordinates. The drag threshold is
    // measured from here: measuring from the square's centre instead would
    // mean any press away from dead centre already exceeded the threshold.
    QPoint pressedAt_;

    bool dragging_ = false;
};

} // namespace cines
