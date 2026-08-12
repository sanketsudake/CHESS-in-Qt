#include "chess/San.hpp"

#include "chess/MoveGenerator.hpp"
#include "chess/Rules.hpp"

#include <cctype>

namespace chess::san {
namespace {

char fileLetter(Square square)
{
    return static_cast<char>('a' + static_cast<int>(fileOf(square)));
}

char rankDigit(Square square)
{
    return static_cast<char>('1' + static_cast<int>(rankOf(square)));
}

// What must be written between the piece letter and the destination so the
// move is unambiguous: nothing, the file, the rank, or both.
//
// Both is needed only when three or more of the same piece can reach the
// square, which happens with promoted pieces. Handling it costs one extra
// branch and saves a wrong record later.
std::string disambiguation(const Position& position, const Move& move, PieceType type)
{
    bool anyRival = false;
    bool rivalOnSameFile = false;
    bool rivalOnSameRank = false;

    for (const Move& other : generateLegalMoves(position)) {
        if (other.from == move.from || other.to != move.to) {
            continue;
        }
        if (position.board.pieceAt(other.from).type != type) {
            continue;
        }
        anyRival = true;
        rivalOnSameFile = rivalOnSameFile || fileOf(other.from) == fileOf(move.from);
        rivalOnSameRank = rivalOnSameRank || rankOf(other.from) == rankOf(move.from);
    }

    std::string hint;
    if (!anyRival) {
        return hint;
    }

    // The file alone when no rival shares it, the rank alone when no rival
    // shares that, and both when rivals share each.
    if (rivalOnSameFile && rivalOnSameRank) {
        hint += fileLetter(move.from);
        hint += rankDigit(move.from);
    } else if (rivalOnSameFile) {
        hint += rankDigit(move.from);
    } else {
        hint += fileLetter(move.from);
    }
    return hint;
}

// Strips what does not identify the move: check and mate marks, annotation
// glyphs such as "!?", the optional "e.p." after an en passant capture, and
// any stray whitespace. Zeroes in castling become the letter O.
std::string normalise(std::string_view text)
{
    std::string cleaned;
    cleaned.reserve(text.size());

    for (const char c : text) {
        switch (c) {
        case '+':
        case '#':
        case '!':
        case '?':
        case ' ':
        case '\t':
        case '.':
            continue;
        case '0':
            cleaned += 'O';
            continue;
        default:
            cleaned += c;
        }
    }

    // "exd6e.p." has lost its dots by now and reads "exd6ep".
    if (cleaned.size() > 2 && cleaned.ends_with("ep")) {
        cleaned.erase(cleaned.size() - 2);
    }
    return cleaned;
}

} // namespace

std::string toSan(const Position& position, const Move& move)
{
    std::string text;

    if (move.kind == MoveKind::CastleKingSide) {
        text = "O-O";
    } else if (move.kind == MoveKind::CastleQueenSide) {
        text = "O-O-O";
    } else {
        const Piece piece = position.board.pieceAt(move.from);

        if (piece.type == PieceType::Pawn) {
            // A pawn capture always names its starting file, ambiguous or not.
            if (move.isCapture()) {
                text += fileLetter(move.from);
                text += 'x';
            }
            text += toString(move.to);
            if (move.isPromotion()) {
                text += '=';
                text += toChar(move.promotion);
            }
        } else {
            text += toChar(piece.type);
            text += disambiguation(position, move, piece.type);
            if (move.isCapture()) {
                text += 'x';
            }
            text += toString(move.to);
        }
    }

    // Playing the move is the only way to be sure about check and mate, and it
    // costs one move generation on a path that is not hot.
    const Position after = applyMove(position, move);
    if (isInCheck(after)) {
        text += generateLegalMoves(after).empty() ? '#' : '+';
    }

    return text;
}

std::optional<Move> parse(const Position& position, std::string_view text)
{
    const std::string wanted = normalise(text);
    if (wanted.empty()) {
        return std::nullopt;
    }

    for (const Move& move : generateLegalMoves(position)) {
        if (normalise(toSan(position, move)) == wanted) {
            return move;
        }
    }
    return std::nullopt;
}

} // namespace chess::san
