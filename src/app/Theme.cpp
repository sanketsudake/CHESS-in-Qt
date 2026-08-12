#include "Theme.hpp"

namespace cines {

Theme Theme::light()
{
    Theme theme;
    // A muted olive pair, close in spirit to the original board but with
    // enough contrast between the two to read at small sizes.
    theme.lightSquare = QColor(0xEB, 0xEC, 0xD0);
    theme.darkSquare = QColor(0x77, 0x96, 0x56);
    theme.selection = QColor(0xF6, 0xF6, 0x69, 0xB4);
    theme.legalTarget = QColor(0x20, 0x20, 0x20, 0x40);
    theme.lastMove = QColor(0xBA, 0xCA, 0x2B, 0x80);
    theme.check = QColor(0xE0, 0x4F, 0x39, 0xB0);
    theme.boardBorder = QColor(0x3D, 0x4A, 0x2C);
    theme.coordinateText = QColor(0x3D, 0x4A, 0x2C);
    return theme;
}

Theme Theme::dark()
{
    Theme theme;
    theme.lightSquare = QColor(0x9E, 0xA3, 0xAB);
    theme.darkSquare = QColor(0x4B, 0x52, 0x5D);
    theme.selection = QColor(0xD7, 0xC5, 0x4A, 0xB4);
    theme.legalTarget = QColor(0xF0, 0xF0, 0xF0, 0x50);
    theme.lastMove = QColor(0x9A, 0x8B, 0x33, 0x90);
    theme.check = QColor(0xD9, 0x53, 0x4F, 0xC0);
    theme.boardBorder = QColor(0x24, 0x27, 0x2B);
    theme.coordinateText = QColor(0xD8, 0xDB, 0xE0);
    return theme;
}

} // namespace cines
