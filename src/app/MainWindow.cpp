#include "MainWindow.hpp"

#include "BoardScene.hpp"
#include "BoardView.hpp"
#include "CapturedTray.hpp"
#include "GameController.hpp"
#include "MoveListModel.hpp"
#include "PieceRenderer.hpp"
#include "PromotionDialog.hpp"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QStyleHints>
#include <QTableView>
#include <QVBoxLayout>

#include <array>

namespace cines {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , controller_(new GameController(this))
    , scene_(new BoardScene(*controller_, this))
    , view_(new BoardView(*controller_, *scene_, this))
{
    setWindowTitle(tr("CINES"));

    // The board keeps whatever space is left after the panel and stays square
    // inside it; the panel is the part that gives way when the window narrows.
    auto* central = new QWidget(this);
    auto* layout = new QHBoxLayout(central);
    layout->addWidget(view_, 1);
    layout->addWidget(buildSidePanel(), 0);
    setCentralWidget(central);

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

    // Without this a refused move just snaps back in silence, and a player who
    // has missed a pin repeats the same drag with no idea why it will not go.
    connect(controller_, &GameController::illegalMoveAttempted, this,
        [this](chess::Square from, chess::Square to) {
            statusBar()->showMessage(GameController::describeIllegalMove(from, to), 2500);
        });

    // Following the desktop means following it as it changes, not only at
    // start-up.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        if (themeChoice_ == ThemeChoice::FollowSystem) {
            applyTheme();
        }
    });

    // Settings are read before the menus are built, so each action is created
    // already showing the right state instead of being created wrong and
    // corrected a moment later.
    restoreSettings();
    buildMenus();
    applyTheme();
    onPositionChanged();
}

QWidget* MainWindow::buildSidePanel()
{
    auto* panel = new QWidget(this);
    panel->setFixedWidth(240);

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(8, 0, 0, 0);

    blackTray_ = new CapturedTray(*controller_, scene_->renderer(), chess::Color::Black, panel);
    whiteTray_ = new CapturedTray(*controller_, scene_->renderer(), chess::Color::White, panel);

    moveListModel_ = new MoveListModel(*controller_, this);
    moveListView_ = new QTableView(panel);
    moveListView_->setModel(moveListModel_);
    moveListView_->setSelectionBehavior(QAbstractItemView::SelectItems);
    moveListView_->setSelectionMode(QAbstractItemView::SingleSelection);
    moveListView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    moveListView_->verticalHeader()->setVisible(false);
    moveListView_->setShowGrid(false);
    moveListView_->horizontalHeader()->setSectionResizeMode(
        MoveListModel::NumberColumn, QHeaderView::ResizeToContents);
    moveListView_->horizontalHeader()->setSectionResizeMode(MoveListModel::WhiteColumn, QHeaderView::Stretch);
    moveListView_->horizontalHeader()->setSectionResizeMode(MoveListModel::BlackColumn, QHeaderView::Stretch);

    // Clicking a move rewinds the board to it without discarding anything, so
    // the game can be read backwards and then continued.
    connect(moveListView_, &QTableView::clicked, this, &MainWindow::jumpToMove);

    // The trays sit either side of the move list, each next to the player it
    // describes, with Black on top because that is where Black sits.
    layout->addWidget(blackTray_);
    layout->addWidget(moveListView_, 1);
    layout->addWidget(whiteTray_);

    return panel;
}

void MainWindow::jumpToMove(const QModelIndex& index)
{
    const std::size_t ply = moveListModel_->plyAt(index);
    if (ply == 0) {
        return;
    }
    controller_->goToPly(ply);
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

    QAction* copyPgn = gameMenu->addAction(tr("Copy &Game (PGN)"));
    copyPgn->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    connect(copyPgn, &QAction::triggered, this, &MainWindow::copyPgnToClipboard);

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));

    flipAction_ = viewMenu->addAction(tr("&Flip Board"));
    flipAction_->setShortcut(QKeySequence(Qt::Key_F));
    flipAction_->setCheckable(true);
    flipAction_->setChecked(scene_->isFlipped());
    connect(flipAction_, &QAction::toggled, scene_, &BoardScene::setFlipped);

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
        connect(action, &QAction::triggered, this, [this, choice = entry.choice] { setThemeChoice(choice); });
        if (entry.choice == themeChoice_) {
            action->setChecked(true);
        }
    }

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction* about = helpMenu->addAction(tr("&About CINES"));
    connect(about, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::copyPgnToClipboard()
{
    QApplication::clipboard()->setText(controller_->pgn());
    statusBar()->showMessage(tr("Game copied"), 2000);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About CINES"),
        tr("<h3>CINES %1</h3>"
           "<p>Two player chess.</p>"
           "<p>Free software under the GNU General Public License, version 3 "
           "or later. Originally written in 2012 by Sagar Rakshe, Nisarg Patel, "
           "Sanket Sudake and Nikhil Pachpande.</p>")
            .arg(QCoreApplication::applicationVersion()));
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

    scene_->setFlipped(settings.value(QStringLiteral("view/flipped"), false).toBool());

    // restoreGeometry reports whether the saved bytes were usable -- they may
    // have been written by another version, or name a screen that is no longer
    // attached. Falling back to a default size beats opening off-screen.
    const QByteArray geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (geometry.isEmpty() || !restoreGeometry(geometry)) {
        resize(980, 800);
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

    // The panel is derived from the same position, so it is refreshed here
    // rather than kept in step by its own signal wiring.
    moveListModel_->refresh();
    moveListView_->scrollTo(moveListModel_->indexOfCurrentPly());
    whiteTray_->refresh();
    blackTray_->refresh();

    undoAction_->setEnabled(controller_->canUndo());
    redoAction_->setEnabled(controller_->canRedo());
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

    PromotionDialog dialog(color, scene_->renderer(), this);
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
        // Say what was wrong with it. "Does not hold a position" is unhelpful
        // when the record parsed and was simply not a board that could occur.
        statusBar()->showMessage(
            tr("Cannot use that position: %1").arg(controller_->lastPositionError()), 5000);
    }
}

} // namespace cines
