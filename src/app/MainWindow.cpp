#include "MainWindow.hpp"

#include "BoardScene.hpp"
#include "BoardView.hpp"
#include "GameController.hpp"
#include "PieceRenderer.hpp"
#include "PromotionDialog.hpp"

#include <QApplication>
#include <QClipboard>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

namespace cines {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , controller_(new GameController(this))
    , promotionRenderer_(new PieceRenderer(this))
    , scene_(new BoardScene(*controller_, this))
    , view_(new BoardView(*controller_, *scene_, this))
{
    setWindowTitle(tr("CINES"));
    setCentralWidget(view_);
    statusBar()->showMessage(controller_->statusText());

    connect(controller_, &GameController::positionChanged, this, &MainWindow::onPositionChanged);
    connect(controller_, &GameController::selectionChanged, scene_, &BoardScene::updateSelection);
    connect(controller_, &GameController::gameOver, this, &MainWindow::onGameOver);
    connect(controller_, &GameController::promotionRequested, this, &MainWindow::onPromotionRequested);

    buildMenus();
    resize(720, 780);
}

void MainWindow::buildMenus()
{
    QMenu* gameMenu = menuBar()->addMenu(tr("&Game"));

    QAction* newGame = gameMenu->addAction(tr("&New Game"));
    newGame->setShortcut(QKeySequence::New);
    connect(newGame, &QAction::triggered, controller_, &GameController::newGame);

    gameMenu->addSeparator();

    undoAction_ = gameMenu->addAction(tr("&Undo"));
    undoAction_->setShortcut(QKeySequence::Undo);
    connect(undoAction_, &QAction::triggered, controller_, &GameController::undo);

    redoAction_ = gameMenu->addAction(tr("&Redo"));
    redoAction_->setShortcut(QKeySequence::Redo);
    connect(redoAction_, &QAction::triggered, controller_, &GameController::redo);

    gameMenu->addSeparator();

    QAction* copyFen = gameMenu->addAction(tr("&Copy Position (FEN)"));
    copyFen->setShortcut(QKeySequence::Copy);
    connect(copyFen, &QAction::triggered, this, &MainWindow::copyFenToClipboard);

    QAction* pasteFen = gameMenu->addAction(tr("&Paste Position (FEN)"));
    pasteFen->setShortcut(QKeySequence::Paste);
    connect(pasteFen, &QAction::triggered, this, &MainWindow::pasteFenFromClipboard);

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));

    QAction* flip = viewMenu->addAction(tr("&Flip Board"));
    flip->setShortcut(QKeySequence(Qt::Key_F));
    connect(flip, &QAction::triggered, this, &MainWindow::toggleFlip);

    onPositionChanged();
}

void MainWindow::onPositionChanged()
{
    scene_->rebuildPieces();
    statusBar()->showMessage(controller_->statusText());

    if (undoAction_ != nullptr) {
        undoAction_->setEnabled(controller_->canUndo());
    }
    if (redoAction_ != nullptr) {
        redoAction_->setEnabled(controller_->canRedo());
    }
}

void MainWindow::onGameOver(chess::Outcome outcome, chess::TerminalReason reason)
{
    Q_UNUSED(outcome);
    Q_UNUSED(reason);

    // The status bar already carries the detail; the box is what makes the end
    // of the game impossible to miss.
    QMessageBox::information(this, tr("Game over"), controller_->statusText());
}

void MainWindow::onPromotionRequested(chess::Square from, chess::Square to, chess::Color color)
{
    Q_UNUSED(from);
    Q_UNUSED(to);

    PromotionDialog dialog(color, *promotionRenderer_, this);
    dialog.exec();

    // A dismissed dialog yields PieceType::None, which the controller reads as
    // "abandon the move".
    controller_->finishPromotion(dialog.chosenPiece());
}

void MainWindow::copyFenToClipboard()
{
    QApplication::clipboard()->setText(controller_->fen());
    statusBar()->showMessage(tr("Position copied"), 2000);
}

void MainWindow::pasteFenFromClipboard()
{
    const QString text = QApplication::clipboard()->text();
    if (!controller_->setPositionFromFen(text)) {
        statusBar()->showMessage(tr("Clipboard does not hold a position"), 3000);
    }
}

void MainWindow::toggleFlip()
{
    scene_->setFlipped(!scene_->isFlipped());
}

} // namespace cines
