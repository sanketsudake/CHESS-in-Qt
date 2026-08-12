#include "chess/Fen.hpp"

#include <array>
#include <charconv>
#include <string>
#include <utility>
#include <vector>

namespace chess::fen {
namespace {

ParseError error(std::string message)
{
    return ParseError{std::move(message)};
}

std::vector<std::string_view> splitOnWhitespace(std::string_view text)
{
    std::vector<std::string_view> fields;
    std::size_t pos = 0;
    while (pos < text.size()) {
        const std::size_t start = text.find_first_not_of(" \t", pos);
        if (start == std::string_view::npos) {
            break;
        }
        std::size_t end = text.find_first_of(" \t", start);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        fields.push_back(text.substr(start, end - start));
        pos = end;
    }
    return fields;
}

// Field 1: eight rank descriptions separated by '/', written from rank 8 down
// to rank 1, each a mix of piece letters and digits that must total eight
// squares.
std::optional<ParseError> parsePlacement(std::string_view field, Board& board)
{
    board.clear();

    int rank = kBoardSize - 1;
    int file = 0;

    for (const char c : field) {
        if (c == '/') {
            if (file != kBoardSize) {
                return error("rank " + std::to_string(rank + 1) + " describes " + std::to_string(file)
                    + " squares, expected 8");
            }
            if (rank == 0) {
                return error("placement has more than 8 ranks");
            }
            --rank;
            file = 0;
            continue;
        }

        if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > kBoardSize) {
                return error("rank " + std::to_string(rank + 1) + " overflows past the h file");
            }
            continue;
        }

        const auto piece = pieceFromChar(c);
        if (!piece) {
            return error(std::string("unknown piece letter '") + c + "'");
        }
        if (file >= kBoardSize) {
            return error("rank " + std::to_string(rank + 1) + " overflows past the h file");
        }
        board.setPiece(makeSquare(file, rank), *piece);
        ++file;
    }

    if (rank != 0) {
        return error("placement has " + std::to_string(kBoardSize - rank) + " ranks, expected 8");
    }
    if (file != kBoardSize) {
        return error("rank 1 describes " + std::to_string(file) + " squares, expected 8");
    }
    return std::nullopt;
}

std::optional<ParseError> parseCastling(std::string_view field, CastlingRights& rights)
{
    rights = CastlingRights{};
    if (field == "-") {
        return std::nullopt;
    }

    for (const char c : field) {
        CastlingRights::Flag flag = CastlingRights::None;
        switch (c) {
        case 'K':
            flag = CastlingRights::WhiteKingSide;
            break;
        case 'Q':
            flag = CastlingRights::WhiteQueenSide;
            break;
        case 'k':
            flag = CastlingRights::BlackKingSide;
            break;
        case 'q':
            flag = CastlingRights::BlackQueenSide;
            break;
        default:
            return error(std::string("unknown castling letter '") + c + "'");
        }
        if (rights.has(flag)) {
            return error(std::string("castling letter '") + c + "' repeated");
        }
        rights.add(flag);
    }
    return std::nullopt;
}

// A double pawn push by White leaves a target on rank 3 with Black to move,
// and vice versa. Anything else is a corrupt record rather than a position we
// should try to interpret.
std::optional<ParseError> parseEnPassant(std::string_view field, Color sideToMove, Square& target)
{
    target = Square::None;
    if (field == "-") {
        return std::nullopt;
    }

    const auto square = squareFromString(field);
    if (!square) {
        return error("malformed en passant target '" + std::string(field) + "'");
    }

    const Rank expected = sideToMove == Color::White ? Rank::R6 : Rank::R3;
    if (rankOf(*square) != expected) {
        return error("en passant target " + std::string(field) + " is not on rank "
            + std::to_string(static_cast<int>(expected) + 1) + ", which is where a "
            + (sideToMove == Color::White ? "Black" : "White") + " double pawn push would leave it");
    }

    target = *square;
    return std::nullopt;
}

std::optional<ParseError> parseCounter(std::string_view field, const char* name, int minimum, int& out)
{
    int value = 0;
    const char* first = field.data();
    const char* last = field.data() + field.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last) {
        return error(std::string(name) + " '" + std::string(field) + "' is not a number");
    }
    if (value < minimum) {
        return error(std::string(name) + " must be at least " + std::to_string(minimum));
    }
    out = value;
    return std::nullopt;
}

} // namespace

ParseResult parse(std::string_view text)
{
    const std::vector<std::string_view> fields = splitOnWhitespace(text);
    if (fields.size() < 4) {
        return error("FEN needs at least 4 fields, found " + std::to_string(fields.size()));
    }
    if (fields.size() > 6) {
        return error("FEN has " + std::to_string(fields.size()) + " fields, expected at most 6");
    }

    Position position;

    if (auto failure = parsePlacement(fields[0], position.board)) {
        return *failure;
    }

    if (fields[1] == "w") {
        position.sideToMove = Color::White;
    } else if (fields[1] == "b") {
        position.sideToMove = Color::Black;
    } else {
        return error("side to move must be 'w' or 'b', found '" + std::string(fields[1]) + "'");
    }

    if (auto failure = parseCastling(fields[2], position.castling)) {
        return *failure;
    }

    if (auto failure = parseEnPassant(fields[3], position.sideToMove, position.enPassantTarget)) {
        return *failure;
    }

    // Many published test positions stop after the en passant field.
    position.halfmoveClock = 0;
    position.fullmoveNumber = 1;

    if (fields.size() >= 5) {
        if (auto failure = parseCounter(fields[4], "halfmove clock", 0, position.halfmoveClock)) {
            return *failure;
        }
    }
    if (fields.size() == 6) {
        if (auto failure = parseCounter(fields[5], "fullmove number", 1, position.fullmoveNumber)) {
            return *failure;
        }
    }

    return position;
}

std::string serialise(const Position& position)
{
    std::string text;

    for (int rank = kBoardSize - 1; rank >= 0; --rank) {
        int emptyRun = 0;
        for (int file = 0; file < kBoardSize; ++file) {
            const Piece piece = position.board.pieceAt(makeSquare(file, rank));
            if (piece.isEmpty()) {
                ++emptyRun;
                continue;
            }
            if (emptyRun > 0) {
                text += static_cast<char>('0' + emptyRun);
                emptyRun = 0;
            }
            text += toChar(piece);
        }
        if (emptyRun > 0) {
            text += static_cast<char>('0' + emptyRun);
        }
        if (rank > 0) {
            text += '/';
        }
    }

    text += position.sideToMove == Color::White ? " w " : " b ";

    if (position.castling == CastlingRights{}) {
        text += '-';
    } else {
        static constexpr std::array<std::pair<CastlingRights::Flag, char>, 4> kOrder{{
            {CastlingRights::WhiteKingSide, 'K'},
            {CastlingRights::WhiteQueenSide, 'Q'},
            {CastlingRights::BlackKingSide, 'k'},
            {CastlingRights::BlackQueenSide, 'q'},
        }};
        for (const auto& [flag, letter] : kOrder) {
            if (position.castling.has(flag)) {
                text += letter;
            }
        }
    }

    text += ' ';
    text += toString(position.enPassantTarget);
    text += ' ';
    text += std::to_string(position.halfmoveClock);
    text += ' ';
    text += std::to_string(position.fullmoveNumber);

    return text;
}

} // namespace chess::fen
