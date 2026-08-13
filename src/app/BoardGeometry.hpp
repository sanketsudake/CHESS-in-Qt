#pragma once

#include "chess/Types.hpp"

#include <QPointF>
#include <QRectF>

namespace cines::geometry {

// The board is laid out in its own fixed coordinate space and the view scales
// that space to whatever size the window happens to be. Nothing in the drawing
// code deals in pixels, which is what the 2012 version did -- it placed every
// tile with setGeometry at a hard-coded offset inside a fixed 1370x700 window,
// so the board could not be resized at all.
inline constexpr qreal kSquareSize = 100.0;
inline constexpr qreal kBoardSize = kSquareSize * chess::kBoardSize;

// Pieces are drawn a little smaller than their square so they do not touch the
// edges of the highlight drawn behind them.
inline constexpr qreal kPieceScale = 0.84;

[[nodiscard]] inline QRectF boardRect()
{
    return {0.0, 0.0, kBoardSize, kBoardSize};
}

// Where a square sits on screen. Rank 1 is at the bottom when the board is not
// flipped, so the rank axis is inverted relative to scene coordinates, whose
// origin is top left.
[[nodiscard]] inline QRectF squareRect(chess::Square square, bool flipped)
{
    const int file = static_cast<int>(chess::fileOf(square));
    const int rank = static_cast<int>(chess::rankOf(square));

    const int column = flipped ? (chess::kBoardSize - 1 - file) : file;
    const int row = flipped ? rank : (chess::kBoardSize - 1 - rank);

    return {column * kSquareSize, row * kSquareSize, kSquareSize, kSquareSize};
}

[[nodiscard]] inline QPointF squareCentre(chess::Square square, bool flipped)
{
    return squareRect(square, flipped).center();
}

// The square under a point in scene coordinates, or Square::None when the
// point is off the board. Clicks outside the board reach here whenever the
// window is not exactly square.
[[nodiscard]] inline chess::Square squareAt(const QPointF& scenePoint, bool flipped)
{
    if (!boardRect().contains(scenePoint)) {
        return chess::Square::None;
    }

    const auto column = static_cast<int>(scenePoint.x() / kSquareSize);
    const auto row = static_cast<int>(scenePoint.y() / kSquareSize);

    const int file = flipped ? (chess::kBoardSize - 1 - column) : column;
    const int rank = flipped ? row : (chess::kBoardSize - 1 - row);

    return chess::makeSquare(file, rank);
}

} // namespace cines::geometry
