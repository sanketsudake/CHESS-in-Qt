#include "chess/Types.hpp"

namespace chess {

std::string toString(Square s)
{
    if (!isValid(s)) {
        return "-";
    }
    const auto file = static_cast<char>('a' + static_cast<int>(fileOf(s)));
    const auto rank = static_cast<char>('1' + static_cast<int>(rankOf(s)));
    return std::string{file, rank};
}

std::optional<Square> squareFromString(std::string_view text)
{
    if (text.size() != 2) {
        return std::nullopt;
    }
    const int file = text[0] - 'a';
    const int rank = text[1] - '1';
    const Square square = makeSquare(file, rank);
    if (!isValid(square)) {
        return std::nullopt;
    }
    return square;
}

char toChar(PieceType type)
{
    switch (type) {
    case PieceType::Pawn:
        return 'P';
    case PieceType::Knight:
        return 'N';
    case PieceType::Bishop:
        return 'B';
    case PieceType::Rook:
        return 'R';
    case PieceType::Queen:
        return 'Q';
    case PieceType::King:
        return 'K';
    case PieceType::None:
        break;
    }
    return '\0';
}

std::optional<PieceType> pieceTypeFromChar(char c)
{
    switch (c) {
    case 'P':
    case 'p':
        return PieceType::Pawn;
    case 'N':
    case 'n':
        return PieceType::Knight;
    case 'B':
    case 'b':
        return PieceType::Bishop;
    case 'R':
    case 'r':
        return PieceType::Rook;
    case 'Q':
    case 'q':
        return PieceType::Queen;
    case 'K':
    case 'k':
        return PieceType::King;
    default:
        return std::nullopt;
    }
}

char toChar(Piece piece)
{
    const char letter = toChar(piece.type);
    if (letter == '\0') {
        return '\0';
    }
    if (piece.color == Color::Black) {
        return static_cast<char>(letter - 'A' + 'a');
    }
    return letter;
}

std::optional<Piece> pieceFromChar(char c)
{
    const auto type = pieceTypeFromChar(c);
    if (!type) {
        return std::nullopt;
    }
    const Color color = (c >= 'a' && c <= 'z') ? Color::Black : Color::White;
    return Piece{*type, color};
}

} // namespace chess
