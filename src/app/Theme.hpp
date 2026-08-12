#pragma once

#include <QColor>

namespace cines {

// Every colour the board draws, in one place.
//
// The 2012 code wrote colours as inline stylesheet strings at each use --
// "background-color: rgb(120, 120, 90)" in one function, "orange" in another --
// so changing how the board looked meant finding every literal. Naming them
// once is what makes a second theme possible at all.
struct Theme {
    QColor lightSquare;
    QColor darkSquare;

    // The square the player has picked up.
    QColor selection;

    // Where the selected piece may go. Empty squares get a dot in this colour,
    // occupied ones a ring, so a capture reads differently from a quiet move.
    QColor legalTarget;

    // From and to of the move just played, so the board answers "what just
    // happened" without the player having to read the move list.
    QColor lastMove;

    // Drawn behind a king that is attacked.
    QColor check;

    QColor boardBorder;
    QColor coordinateText;

    [[nodiscard]] static Theme light();
    [[nodiscard]] static Theme dark();
};

} // namespace cines
