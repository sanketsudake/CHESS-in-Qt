#include "GameController.hpp"

#include "chess/Fen.hpp"
#include "chess/MoveGenerator.hpp"
#include "chess/Rules.hpp"

#include <QCoreApplication>

namespace cines {
namespace {

QString describeColor(chess::Color color)
{
    return color == chess::Color::White ? QCoreApplication::translate("GameController", "White")
                                        : QCoreApplication::translate("GameController", "Black");
}

} // namespace

GameController::GameController(QObject* parent)
    : QObject(parent)
{
}

void GameController::newGame()
{
    game_.reset();
    pendingPromotion_.reset();
    refreshAfterPositionChange();
}

bool GameController::setPositionFromFen(const QString& text)
{
    chess::fen::ParseResult result = chess::fen::parse(text.trimmed().toStdString());
    if (const auto* failure = std::get_if<chess::fen::ParseError>(&result)) {
        lastPositionError_ = QString::fromStdString(failure->message);
        return false;
    }

    const auto& position = std::get<chess::Position>(result);

    // Parsing proves the record is well formed, not that the board could ever
    // have occurred. Everything downstream is written for positions that can
    // occur -- move generation in particular sizes its list for the 218 moves
    // a real position can offer -- so an impossible board is refused here
    // rather than played from.
    const chess::PositionError legality = chess::validatePosition(position);
    if (legality != chess::PositionError::None) {
        lastPositionError_ = QString::fromUtf8(
            chess::describe(legality).data(), static_cast<qsizetype>(chess::describe(legality).size()));
        return false;
    }

    lastPositionError_.clear();
    game_.reset(position);
    pendingPromotion_.reset();
    refreshAfterPositionChange();
    return true;
}

void GameController::selectSquare(chess::Square square)
{
    // Picking up a piece is only meaningful for the side to move, and only
    // when it has somewhere to go. Anything else is a click on the board.
    if (!chess::isValid(square) || game_.isOver()) {
        clearSelection();
        return;
    }

    const chess::Piece piece = position().board.pieceAt(square);
    if (piece.isEmpty() || piece.color != sideToMove()) {
        clearSelection();
        return;
    }

    setSelection(square);
}

void GameController::clearSelection()
{
    if (!chess::isValid(selected_) && legalTargets_.isEmpty()) {
        return;
    }
    selected_ = chess::Square::None;
    legalTargets_.clear();
    emit selectionChanged(selected_, legalTargets_);
}

void GameController::setSelection(chess::Square square)
{
    QList<chess::Square> targets;

    for (const chess::Move& move : game_.legalMoves()) {
        if (move.from != square) {
            continue;
        }
        // The four promotions of one pawn push share a destination, and the
        // board only needs the square once.
        if (!targets.contains(move.to)) {
            targets.append(move.to);
        }
    }

    // A piece with nowhere to go -- a fully pinned knight, say -- is not
    // selected at all. Highlighting it would invite the player to drag it
    // around only for every drop to be refused.
    if (targets.isEmpty()) {
        clearSelection();
        return;
    }

    selected_ = square;
    legalTargets_ = targets;
    emit selectionChanged(selected_, legalTargets_);
}

bool GameController::moveTo(chess::Square square)
{
    if (!chess::isValid(selected_) || !chess::isValid(square)) {
        return false;
    }

    const chess::Square from = selected_;
    const chess::MoveList legal = game_.legalMoves();

    // A promotion is four moves sharing from and to. Ask which one rather than
    // silently choosing a queen, and hold the move until the answer arrives.
    if (legal.find(from, square, chess::PieceType::Queen).has_value()) {
        pendingPromotion_ = chess::Move{from, square, chess::PieceType::Queen, chess::MoveKind::Quiet};
        emit promotionRequested(from, square, sideToMove());
        return true;
    }

    if (!game_.playFrom(from, square)) {
        emit illegalMoveAttempted(from, square);
        return false;
    }

    const chess::PlayedMove& played = game_.history().back();
    emit moveMade(played.move, QString::fromStdString(played.san));
    refreshAfterPositionChange();
    return true;
}

void GameController::finishPromotion(chess::PieceType piece)
{
    if (!pendingPromotion_) {
        return;
    }

    const chess::Move pending = *pendingPromotion_;
    pendingPromotion_.reset();

    // PieceType::None means the player dismissed the dialog. The pawn stays
    // where it was and the selection is dropped.
    if (piece == chess::PieceType::None) {
        clearSelection();
        return;
    }

    if (!game_.playFrom(pending.from, pending.to, piece)) {
        emit illegalMoveAttempted(pending.from, pending.to);
        clearSelection();
        return;
    }

    const chess::PlayedMove& played = game_.history().back();
    emit moveMade(played.move, QString::fromStdString(played.san));
    refreshAfterPositionChange();
}

void GameController::undo()
{
    // Read the move before undoing: afterwards lastMove names the move before
    // it, not the one being taken back.
    const chess::PlayedMove* undone = game_.lastMove();
    const std::optional<chess::Move> move = undone != nullptr ? std::optional{undone->move} : std::nullopt;

    if (!game_.undo()) {
        return;
    }

    pendingPromotion_.reset();
    if (move) {
        emit moveUndone(*move);
    }
    refreshAfterPositionChange();
}

void GameController::redo()
{
    if (!game_.redo()) {
        return;
    }

    const chess::PlayedMove* replayed = game_.lastMove();
    if (replayed != nullptr) {
        emit moveMade(replayed->move, QString::fromStdString(replayed->san));
    }
    refreshAfterPositionChange();
}

void GameController::goToPly(std::size_t ply)
{
    if (!game_.goToPly(ply)) {
        return;
    }
    pendingPromotion_.reset();
    refreshAfterPositionChange();
}

void GameController::refreshAfterPositionChange()
{
    // Any change of position invalidates the selection, because the piece that
    // was picked up has either moved or is no longer the side to move's.
    selected_ = chess::Square::None;
    legalTargets_.clear();

    emit selectionChanged(selected_, legalTargets_);
    emit positionChanged();

    if (game_.isOver()) {
        emit gameOver(game_.outcome(), game_.terminalReason());
    }
}

bool GameController::isInCheck(chess::Color color) const
{
    return chess::isInCheck(position(), color);
}

QString GameController::statusText() const
{
    const chess::TerminalReason reason = game_.terminalReason();

    switch (reason) {
    case chess::TerminalReason::Checkmate:
        return tr("Checkmate — %1 wins").arg(describeColor(chess::opposite(sideToMove())));
    case chess::TerminalReason::Stalemate:
        return tr("Stalemate — draw");
    case chess::TerminalReason::FiftyMoveRule:
        return tr("Draw — fifty moves without a capture or a pawn move");
    case chess::TerminalReason::ThreefoldRepetition:
        return tr("Draw — the same position three times");
    case chess::TerminalReason::InsufficientMaterial:
        return tr("Draw — neither side can force mate");
    case chess::TerminalReason::None:
        break;
    }

    if (isInCheck(sideToMove())) {
        return tr("%1 to move — check").arg(describeColor(sideToMove()));
    }
    return tr("%1 to move").arg(describeColor(sideToMove()));
}

QString GameController::fen() const
{
    return QString::fromStdString(game_.fen());
}

} // namespace cines
