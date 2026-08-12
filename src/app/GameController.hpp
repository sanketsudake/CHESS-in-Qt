#pragma once

#include "ChessMetaTypes.hpp"
#include "chess/Game.hpp"
#include "chess/Rules.hpp"
#include "chess/Types.hpp"

#include <QList>
#include <QObject>
#include <QString>

#include <optional>

namespace cines {

// The only place the user interface and the rules meet.
//
// Widgets know about clicks and squares; chess::Game knows about moves and
// legality. Everything that translates between the two lives here, so no
// widget ever reasons about castling, en passant or whose turn it is. That
// separation is the direct answer to the 2012 Tile, which was board square,
// piece, painter and rule engine at once.
//
// Widgets observe through signals rather than reading the Game, which keeps
// them from depending on more of it than they use.
class GameController : public QObject {
    Q_OBJECT

public:
    explicit GameController(QObject* parent = nullptr);

    [[nodiscard]] const chess::Position& position() const { return game_.position(); }
    [[nodiscard]] chess::Color sideToMove() const { return game_.sideToMove(); }
    [[nodiscard]] const std::vector<chess::PlayedMove>& history() const { return game_.history(); }
    [[nodiscard]] std::size_t currentPly() const { return game_.currentPly(); }

    // The square the player has picked up, or Square::None.
    [[nodiscard]] chess::Square selectedSquare() const { return selected_; }

    // Where the selected piece may legally go. Empty when nothing is selected.
    [[nodiscard]] const QList<chess::Square>& legalTargets() const { return legalTargets_; }

    // The move that produced the current position, for highlighting where the
    // last piece came from. Null at the start of a game.
    [[nodiscard]] const chess::PlayedMove* lastMove() const { return game_.lastMove(); }

    [[nodiscard]] bool isGameOver() const { return game_.isOver(); }
    [[nodiscard]] bool canUndo() const { return game_.canUndo(); }
    [[nodiscard]] bool canRedo() const { return game_.canRedo(); }

    // True when that side's king is currently attacked, which the board draws
    // differently.
    [[nodiscard]] bool isInCheck(chess::Color color) const;

    // A sentence for the status bar: whose turn it is, or how the game ended.
    [[nodiscard]] QString statusText() const;

    [[nodiscard]] QString fen() const;

public slots:
    void newGame();

    // Sets up a position directly, for "paste FEN". Does nothing and returns
    // false when the text is not a position.
    bool setPositionFromFen(const QString& text);

    // A click, or the start of a drag. Selecting an empty square, an enemy
    // piece, or a piece with no legal move clears the selection instead.
    void selectSquare(chess::Square square);
    void clearSelection();

    // A click on a second square, or a drop. Returns false when the move is
    // not legal, which the board answers by putting the piece back.
    //
    // A pawn arriving on the far rank does not move yet: the controller emits
    // promotionRequested and waits for finishPromotion.
    bool moveTo(chess::Square square);

    // Answers promotionRequested. Passing PieceType::None abandons the move,
    // which is what closing the dialog means.
    void finishPromotion(chess::PieceType piece);

    void undo();
    void redo();

    // Rewinds the board to a point in the history without discarding it, which
    // is what clicking an entry in the move list does.
    void goToPly(std::size_t ply);

signals:
    // The board should redraw from scratch: a move, an undo, a new game.
    void positionChanged();

    void moveMade(const chess::Move& move, const QString& san);

    // The move that was just taken back. The board plays it in reverse, which
    // is what makes undo readable rather than a sudden change of position.
    void moveUndone(const chess::Move& move);

    void selectionChanged(chess::Square selected, const QList<chess::Square>& targets);

    // A pawn reached the far rank and the player must choose a piece. The
    // controller holds the move until finishPromotion answers.
    void promotionRequested(chess::Square from, chess::Square to, chess::Color color);

    void gameOver(chess::Outcome outcome, chess::TerminalReason reason);

    // For the status bar and for a board that wants to shake the square. Not
    // an error: clicking the wrong square is ordinary.
    void illegalMoveAttempted(chess::Square from, chess::Square to);

private:
    void refreshAfterPositionChange();
    void setSelection(chess::Square square);

    chess::Game game_;
    chess::Square selected_ = chess::Square::None;
    QList<chess::Square> legalTargets_;

    // Set between promotionRequested and finishPromotion. While it holds a
    // value the board is showing a position the player has already left.
    std::optional<chess::Move> pendingPromotion_;
};

} // namespace cines
