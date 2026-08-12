#pragma once

#include "ChessMetaTypes.hpp"
#include "chess/Types.hpp"

#include <QDialog>

namespace cines {

class PieceRenderer;

// Asks which piece a pawn becomes.
//
// The choice is genuinely the player's: promoting to a knight is the right
// move often enough that choosing a queen automatically would be a bug rather
// than a convenience. Dismissing the dialog abandons the move and puts the
// pawn back.
class PromotionDialog : public QDialog {
    Q_OBJECT

public:
    PromotionDialog(chess::Color color, const PieceRenderer& renderer, QWidget* parent = nullptr);

    // PieceType::None when the dialog was dismissed.
    [[nodiscard]] chess::PieceType chosenPiece() const { return chosen_; }

private:
    void choose(chess::PieceType piece);

    chess::PieceType chosen_ = chess::PieceType::None;
};

} // namespace cines
