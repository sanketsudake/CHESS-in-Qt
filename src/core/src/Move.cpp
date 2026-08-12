#include "chess/Move.hpp"

#include <cctype>

namespace chess {

std::string Move::toUci() const
{
    if (!isValid()) {
        return "0000";
    }
    std::string text = toString(from) + toString(to);
    if (isPromotion()) {
        const char letter = toChar(promotion);
        text += static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    }
    return text;
}

std::optional<Move> Move::fromUci(std::string_view text)
{
    if (text.size() != 4 && text.size() != 5) {
        return std::nullopt;
    }

    const auto from = squareFromString(text.substr(0, 2));
    const auto to = squareFromString(text.substr(2, 2));
    if (!from || !to) {
        return std::nullopt;
    }

    Move move;
    move.from = *from;
    move.to = *to;

    if (text.size() == 5) {
        const auto promotion = pieceTypeFromChar(text[4]);
        if (!promotion) {
            return std::nullopt;
        }
        // A pawn cannot promote to a pawn or to a king.
        if (*promotion == PieceType::Pawn || *promotion == PieceType::King) {
            return std::nullopt;
        }
        move.promotion = *promotion;
    }

    return move;
}

} // namespace chess
