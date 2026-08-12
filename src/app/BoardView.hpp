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

private:
    [[nodiscard]] chess::Square squareUnder(const QPoint& viewPosition) const;
    void fitBoard();

    GameController& controller_;
    BoardScene& boardScene_;

    // Where the press landed, so a release on the same square reads as a click
    // rather than as a zero-length drag.
    chess::Square pressedSquare_ = chess::Square::None;
    bool dragging_ = false;
};

} // namespace cines
