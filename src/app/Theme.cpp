#include "Theme.hpp"

#include <QGuiApplication>
#include <QStyleHints>

namespace cines {

Theme Theme::light()
{
    Theme theme;
    // Maple and walnut: a warm wooden board to sit the turned ivory and ebony
    // pieces on. The two woods are far enough apart in value to read at a
    // glance, and the squares carry a gentle gradient (see BoardScene) so they
    // catch the same light the pieces do.
    theme.lightSquare = QColor(0xE9, 0xD2, 0xA4);
    theme.darkSquare = QColor(0xA5, 0x78, 0x4A);
    // A warm honey highlight for the picked-up square, an amber for the last
    // move, and a soft brown dot for a legal target -- all in the board's own
    // family so nothing on it looks borrowed from another set.
    theme.selection = QColor(0xF2, 0xD9, 0x6A, 0xC0);
    theme.legalTarget = QColor(0x35, 0x24, 0x12, 0x4D);
    theme.lastMove = QColor(0xD8, 0xB4, 0x54, 0x9C);
    theme.check = QColor(0xD2, 0x46, 0x3A, 0xC0);
    return theme;
}

Theme Theme::dark()
{
    Theme theme;
    // Slate: a cool grey board for a dark desktop, the same turned pieces on
    // stone instead of wood.
    theme.lightSquare = QColor(0xB4, 0xB9, 0xC3);
    theme.darkSquare = QColor(0x5E, 0x66, 0x73);
    theme.selection = QColor(0xD8, 0xC6, 0x54, 0xC0);
    theme.legalTarget = QColor(0xF2, 0xF2, 0xF2, 0x59);
    theme.lastMove = QColor(0x9A, 0x8C, 0x46, 0x9C);
    theme.check = QColor(0xD8, 0x50, 0x48, 0xC0);
    return theme;
}

Theme Theme::forScheme(Qt::ColorScheme scheme)
{
    // Unknown means the platform has no preference to report, which is not a
    // reason to guess dark.
    return scheme == Qt::ColorScheme::Dark ? dark() : light();
}

Theme themeFor(ThemeChoice choice)
{
    switch (choice) {
    case ThemeChoice::Light:
        return Theme::light();
    case ThemeChoice::Dark:
        return Theme::dark();
    case ThemeChoice::FollowSystem:
        break;
    }

    // In a test or a tool there may be no application object yet.
    if (QGuiApplication::instance() == nullptr) {
        return Theme::light();
    }
    return Theme::forScheme(QGuiApplication::styleHints()->colorScheme());
}

} // namespace cines
