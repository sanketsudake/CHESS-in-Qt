#include "MainWindow.hpp"

#include "BoardScene.hpp"
#include "BoardView.hpp"
#include "GameController.hpp"
#include "PieceRenderer.hpp"
#include "PromotionDialog.hpp"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QStyleHints>

#include <array>

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

    // The move hints arrive before the position change, so the scene knows
    // what to animate by the time it rebuilds.
    connect(controller_, &GameController::moveMade, scene_,
        [this](const chess::Move& move, const QString&) { scene_->noteMovePlayed(move); });
    connect(controller_, &GameController::moveUndone, scene_, &BoardScene::noteMoveUndone);

    // Following the desktop means following it as it changes, not only at
    // start-up.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        if (themeChoice_ == ThemeChoice::FollowSystem) {
            applyTheme();
        }
    });

    buildMenus();
    restoreSettings();
    applyTheme();
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

    flipAction_ = viewMenu->addAction(tr("&Flip Board"));
    flipAction_->setShortcut(QKeySequence(Qt::Key_F));
    flipAction_->setCheckable(true);
    connect(flipAction_, &QAction::triggered, this, &MainWindow::toggleFlip);

    viewMenu->addSeparator();

    // Exclusive, because a board has one appearance at a time. Follow System
    // is first and is the default: matching the desktop is what most people
    // want, and the override exists for those who do not.
    auto* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    struct ThemeEntry {
        QString label;
        ThemeChoice choice;
    };
    const std::array<ThemeEntry, 3> entries{{
        {tr("Theme: Follow &System"), ThemeChoice::FollowSystem},
        {tr("Theme: &Light"), ThemeChoice::Light},
        {tr("Theme: &Dark"), ThemeChoice::Dark},
    }};

    for (const ThemeEntry& entry : entries) {
        QAction* action = viewMenu->addAction(entry.label);
        action->setCheckable(true);
        action->setData(static_cast<int>(entry.choice));
        themeGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, entry] { setThemeChoice(entry.choice); });
        if (entry.choice == themeChoice_) {
            action->setChecked(true);
        }
    }

    themeActions_ = themeGroup;

    onPositionChanged();
}

void MainWindow::setThemeChoice(ThemeChoice choice)
{
    themeChoice_ = choice;
    applyTheme();
}

void MainWindow::applyTheme()
{
    scene_->setTheme(themeFor(themeChoice_));
}

void MainWindow::restoreSettings()
{
    const QSettings settings;

    const int stored
        = settings.value(QStringLiteral("view/theme"), static_cast<int>(ThemeChoice::FollowSystem)).toInt();
    // A settings file can hold anything, including a value written by a later
    // version, so an unrecognised choice falls back rather than being cast.
    switch (stored) {
    case static_cast<int>(ThemeChoice::Light):
        themeChoice_ = ThemeChoice::Light;
        break;
    case static_cast<int>(ThemeChoice::Dark):
        themeChoice_ = ThemeChoice::Dark;
        break;
    default:
        themeChoice_ = ThemeChoice::FollowSystem;
        break;
    }

    if (themeActions_ != nullptr) {
        for (QAction* action : themeActions_->actions()) {
            action->setChecked(action->data().toInt() == static_cast<int>(themeChoice_));
        }
    }

    const bool flipped = settings.value(QStringLiteral("view/flipped"), false).toBool();
    scene_->setFlipped(flipped);
    if (flipAction_ != nullptr) {
        flipAction_->setChecked(flipped);
    }

    const QByteArray geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (geometry.isEmpty()) {
        resize(720, 780);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("view/theme"), static_cast<int>(themeChoice_));
    settings.setValue(QStringLiteral("view/flipped"), scene_->isFlipped());
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
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
    if (flipAction_ != nullptr) {
        flipAction_->setChecked(scene_->isFlipped());
    }
}

} // namespace cines
