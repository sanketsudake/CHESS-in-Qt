#pragma once

#include "ChessMetaTypes.hpp"

#include <QList>
#include <QWidget>

namespace cines {

class GameController;
class PieceRenderer;

// The pieces one side has taken, and by how much it is ahead.
//
// Worked out by comparing the current board against the position the game
// started from, rather than by counting captures as they happen. That means
// undo, redo and jumping to a ply all show the right thing without any
// bookkeeping to keep in step -- the tray is a function of the board, so it
// cannot disagree with it.
class CapturedTray : public QWidget {
    Q_OBJECT

public:
    // `side` is whose captures are shown: the pieces this colour has taken
    // from the other.
    CapturedTray(
        GameController& controller, PieceRenderer& renderer, chess::Color side, QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

    // Points ahead, or zero when the material is level or this side is behind.
    // Only the leader shows a number, which is the convention every chess site
    // follows and avoids showing "+3" and "-3" at once.
    [[nodiscard]] int materialLead() const;

public slots:
    void refresh();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // Heaviest first, so the row reads the way a player would list them.
    void recalculate();

    GameController& controller_;
    PieceRenderer& renderer_;
    chess::Color side_;

    QList<chess::PieceType> captured_;
    int lead_ = 0;
};

} // namespace cines
