#pragma once

#include "ChessMetaTypes.hpp"
#include "chess/Rules.hpp"
#include "chess/Types.hpp"

#include <QMainWindow>

namespace cines {

class BoardScene;
class BoardView;
class GameController;
class PieceRenderer;

// The window: a board, a status line, and the actions that act on the game.
//
// It holds the controller and wires signals to widgets. It contains no rules
// and no drawing of its own, so the two things most likely to change -- how
// the game is played and how it looks -- are both somewhere else.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onPositionChanged();
    void onGameOver(chess::Outcome outcome, chess::TerminalReason reason);
    void onPromotionRequested(chess::Square from, chess::Square to, chess::Color color);

private:
    void buildMenus();
    void copyFenToClipboard();
    void pasteFenFromClipboard();
    void toggleFlip();

    GameController* controller_;
    PieceRenderer* promotionRenderer_;
    BoardScene* scene_;
    BoardView* view_;

    QAction* undoAction_ = nullptr;
    QAction* redoAction_ = nullptr;
};

} // namespace cines
