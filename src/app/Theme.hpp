#pragma once

#include <QColor>
#include <Qt>

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

    [[nodiscard]] static Theme light();
    [[nodiscard]] static Theme dark();

    // Which theme a colour scheme asks for. Qt::ColorScheme::Unknown means the
    // platform has no opinion, which is treated as light.
    [[nodiscard]] static Theme forScheme(Qt::ColorScheme scheme);
};

// What the player chose in the View menu, which is not the same thing as which
// theme is in use: Follow System resolves to one or the other depending on the
// desktop, and changes with it.
enum class ThemeChoice { FollowSystem, Light, Dark };

// Resolves a choice against the platform's current colour scheme.
[[nodiscard]] Theme themeFor(ThemeChoice choice);

} // namespace cines
