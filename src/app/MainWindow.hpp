#pragma once

#include "ChessMetaTypes.hpp"
#include "Theme.hpp"
#include "chess/Rules.hpp"
#include "chess/Types.hpp"

#include <QMainWindow>

class QCloseEvent;

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

protected:
    // Settings are written on close rather than on every change, so a session
    // that ends normally remembers how it was left.
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onPositionChanged();
    void onGameOver(chess::Outcome outcome, chess::TerminalReason reason);
    void onPromotionRequested(chess::Square from, chess::Square to, chess::Color color);

private:
    void buildMenus();
    void copyFenToClipboard();
    void pasteFenFromClipboard();
    void toggleFlip();

    // Applies the current choice, resolving Follow System against the desktop.
    void applyTheme();
    void setThemeChoice(ThemeChoice choice);

    void restoreSettings();
    void saveSettings() const;

    GameController* controller_;
    PieceRenderer* promotionRenderer_;
    BoardScene* scene_;
    BoardView* view_;

    QAction* undoAction_ = nullptr;
    QAction* redoAction_ = nullptr;
    QAction* flipAction_ = nullptr;
    QActionGroup* themeActions_ = nullptr;

    ThemeChoice themeChoice_ = ThemeChoice::FollowSystem;
};

} // namespace cines
