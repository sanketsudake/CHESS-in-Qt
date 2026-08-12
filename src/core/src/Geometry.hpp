#pragma once

#include "chess/Types.hpp"

#include <array>
#include <span>

// Board geometry shared by move generation and attack detection.
//
// This table is what replaces the 2012 validation.cpp, where the same ray walk
// was written out eight times for the queen, four more for the rook and four
// more for the bishop. Describing the directions as data instead of as code
// means a bug in the walk can only exist in one place.
namespace chess::geometry {

struct Offset {
    int file;
    int rank;
};

inline constexpr std::array<Offset, 4> kOrthogonal{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

inline constexpr std::array<Offset, 4> kDiagonal{{{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};

inline constexpr std::array<Offset, 8> kAllDirections{
    {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}}};

inline constexpr std::array<Offset, 8> kKnightHops{
    {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};

// How a piece travels: which directions, and whether it slides along them or
// takes exactly one step. Pawns are absent because they are the one piece
// whose captures and non-captures differ, so they get their own generator.
struct Movement {
    std::span<const Offset> directions;
    bool sliding;
};

[[nodiscard]] constexpr Movement movementFor(PieceType type)
{
    switch (type) {
    case PieceType::Rook:
        return {kOrthogonal, true};
    case PieceType::Bishop:
        return {kDiagonal, true};
    case PieceType::Queen:
        return {kAllDirections, true};
    case PieceType::King:
        return {kAllDirections, false};
    case PieceType::Knight:
        return {kKnightHops, false};
    case PieceType::Pawn:
    case PieceType::None:
        break;
    }
    return {{}, false};
}

// Steps off a square, yielding Square::None when the step leaves the board.
// Guarding against an invalid input matters: fileOf(Square::None) is -1, and
// adding an offset to that could otherwise land back on a real square.
[[nodiscard]] constexpr Square offsetFrom(Square square, Offset offset)
{
    if (!isValid(square)) {
        return Square::None;
    }
    return makeSquare(
        static_cast<int>(fileOf(square)) + offset.file, static_cast<int>(rankOf(square)) + offset.rank);
}

// Light or dark square, which is what decides whether two bishops can ever
// meet and so whether a bishop endgame is a dead draw.
[[nodiscard]] constexpr bool isLightSquare(Square square)
{
    return ((static_cast<int>(fileOf(square)) + static_cast<int>(rankOf(square))) % 2) != 0;
}

} // namespace chess::geometry
