#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// Core value types for the chess rules.
//
// Nothing in namespace chess may include a Qt header. The rules are a plain
// C++ library so that they can be unit tested without a display, and so that
// the user interface can be replaced without touching them.
namespace chess {

enum class Color : std::uint8_t { White, Black };

[[nodiscard]] constexpr Color opposite(Color c) noexcept
{
    return c == Color::White ? Color::Black : Color::White;
}

enum class PieceType : std::uint8_t { None, Pawn, Knight, Bishop, Rook, Queen, King };

// A piece on a square. A default-constructed Piece means "empty square"; the
// colour is meaningless in that case and is never read.
struct Piece {
    PieceType type = PieceType::None;
    Color color = Color::White;

    [[nodiscard]] constexpr bool isEmpty() const noexcept { return type == PieceType::None; }

    friend constexpr bool operator==(const Piece& a, const Piece& b) noexcept
    {
        if (a.type != b.type) {
            return false;
        }
        return a.type == PieceType::None || a.color == b.color;
    }
};

enum class File : std::int8_t { A, B, C, D, E, F, G, H };
enum class Rank : std::int8_t { R1, R2, R3, R4, R5, R6, R7, R8 };

// Squares are indexed A1=0, B1=1 ... H1=7, A2=8 ... H8=63, that is
// index = rank * 8 + file. Rank 1 is White's back rank.
enum class Square : std::int8_t {
    // clang-format off
    None = -1,
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    // clang-format on
};

inline constexpr int kBoardSize = 8;
inline constexpr int kSquareCount = 64;

[[nodiscard]] constexpr bool isValid(Square s) noexcept
{
    return s != Square::None;
}

[[nodiscard]] constexpr int index(Square s) noexcept
{
    return static_cast<int>(s);
}

[[nodiscard]] constexpr File fileOf(Square s) noexcept
{
    return static_cast<File>(index(s) % kBoardSize);
}

[[nodiscard]] constexpr Rank rankOf(Square s) noexcept
{
    return static_cast<Rank>(index(s) / kBoardSize);
}

[[nodiscard]] constexpr Square makeSquare(File f, Rank r) noexcept
{
    return static_cast<Square>((static_cast<int>(r) * kBoardSize) + static_cast<int>(f));
}

// Builds a square from zero-based file and rank, returning Square::None when
// either coordinate falls off the board. Move generation relies on this to
// reject candidate destinations without duplicating bounds checks.
[[nodiscard]] constexpr Square makeSquare(int file, int rank) noexcept
{
    if (file < 0 || file >= kBoardSize || rank < 0 || rank >= kBoardSize) {
        return Square::None;
    }
    return static_cast<Square>((rank * kBoardSize) + file);
}

// Light or dark square. a1 is dark, which fixes the whole board.
//
// This decides how the board is painted and whether two bishops can ever meet,
// so it is stated once here rather than separately in the rules and in the
// drawing code -- the two disagreeing would be a bug nobody would think to
// look for.
[[nodiscard]] constexpr bool isLightSquare(Square s) noexcept
{
    return ((static_cast<int>(fileOf(s)) + static_cast<int>(rankOf(s))) % 2) != 0;
}

// The rank a pawn of this colour starts on, and the rank it promotes on.
[[nodiscard]] constexpr Rank pawnStartRank(Color c) noexcept
{
    return c == Color::White ? Rank::R2 : Rank::R7;
}

[[nodiscard]] constexpr Rank promotionRank(Color c) noexcept
{
    return c == Color::White ? Rank::R8 : Rank::R1;
}

// The direction a pawn of this colour advances, in ranks.
[[nodiscard]] constexpr int pawnPush(Color c) noexcept
{
    return c == Color::White ? 1 : -1;
}

// Algebraic name of a square, for example "e4". Square::None yields "-", which
// is what FEN uses for an absent en passant target.
[[nodiscard]] std::string toString(Square s);

// Parses "a1" through "h8". Returns nullopt for anything else, including "-".
[[nodiscard]] std::optional<Square> squareFromString(std::string_view text);

// The single upper-case letter used for a piece in SAN and FEN: PNBRQK.
// Returns '\0' for PieceType::None.
[[nodiscard]] char toChar(PieceType type);

// Accepts either case; the caller decides what the case meant.
[[nodiscard]] std::optional<PieceType> pieceTypeFromChar(char c);

// FEN piece letter: upper case for White, lower case for Black.
[[nodiscard]] char toChar(Piece piece);

[[nodiscard]] std::optional<Piece> pieceFromChar(char c);

} // namespace chess
