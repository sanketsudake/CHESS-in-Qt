// Renders a position to a PNG without opening a window.
//
// Two uses: producing the screenshot the README carries, and looking at a
// board change during development without needing a display or screen
// recording permission. Because it drives the same BoardScene the application
// does, what it writes is what the application draws.
//
//   board_snapshot out.png [--fen "<fen>"] [--dark] [--flip] [--size 720]

#include "BoardGeometry.hpp"
#include "BoardScene.hpp"
#include "GameController.hpp"
#include "Theme.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QImage>
#include <QPainter>

#include <cstdio>

int main(int argc, char* argv[])
{
    // Offscreen so this runs on a build agent with no display.
    qputenv("QT_QPA_PLATFORM", "offscreen");

    // QApplication rather than QGuiApplication: BoardScene is built from
    // QtWidgets classes, and a graphics effect reaches for the style and
    // palette that only QApplication sets up. With QGuiApplication this
    // crashes as soon as the board draws a king in check.
    QApplication application(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Render a chess position to a PNG"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("output"), QStringLiteral("PNG file to write"));

    const QCommandLineOption fenOption(
        QStringLiteral("fen"), QStringLiteral("Position to draw"), QStringLiteral("fen"));
    const QCommandLineOption darkOption(QStringLiteral("dark"), QStringLiteral("Use the dark theme"));
    const QCommandLineOption flipOption(QStringLiteral("flip"), QStringLiteral("Draw from Black's side"));
    const QCommandLineOption sizeOption(QStringLiteral("size"), QStringLiteral("Edge length in pixels"),
        QStringLiteral("pixels"), QStringLiteral("720"));
    parser.addOptions({fenOption, darkOption, flipOption, sizeOption});
    parser.process(application);

    const QStringList positional = parser.positionalArguments();
    if (positional.isEmpty()) {
        std::fputs("an output path is required\n", stderr);
        return 2;
    }

    cines::GameController controller;
    if (parser.isSet(fenOption) && !controller.setPositionFromFen(parser.value(fenOption))) {
        std::fputs("could not read that position\n", stderr);
        return 2;
    }

    cines::BoardScene scene(controller);

    // Off, so the image is the settled board rather than whatever frame an
    // animation happened to be on. Without this the same command would produce
    // a slightly different picture each run.
    scene.setAnimationEnabled(false);

    scene.setTheme(parser.isSet(darkOption) ? cines::Theme::dark() : cines::Theme::light());
    scene.setFlipped(parser.isSet(flipOption));

    bool sizeIsNumber = false;
    const int edge = parser.value(sizeOption).toInt(&sizeIsNumber);
    if (!sizeIsNumber || edge <= 0) {
        std::fputs("--size wants a positive number of pixels\n", stderr);
        return 2;
    }

    QImage image(edge, edge, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    scene.render(&painter, QRectF(0, 0, edge, edge), cines::geometry::boardRect());
    painter.end();

    if (!image.save(positional.first(), "PNG")) {
        std::fputs("could not write that file\n", stderr);
        return 1;
    }
    return 0;
}
