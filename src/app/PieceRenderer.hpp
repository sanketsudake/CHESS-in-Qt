#pragma once

#include "chess/Types.hpp"

#include <QHash>
#include <QObject>

class QSvgRenderer;

namespace cines {

// Owns one QSvgRenderer per kind of piece, shared by every item that draws it.
//
// There are twelve pieces and up to thirty-two items showing them, so the
// renderers are shared rather than duplicated. Sharing also keeps the drawing
// resolution-independent: a QGraphicsSvgItem re-renders from the renderer at
// whatever scale the view is using, so the board stays crisp when the window
// grows and on a high density display. Caching rasterised pixmaps instead
// would defeat both.
//
// The renderers are QObjects parented to this one, so Qt destroys them with
// it and no explicit ownership bookkeeping is needed here.
class PieceRenderer : public QObject {
    Q_OBJECT

public:
    explicit PieceRenderer(QObject* parent = nullptr);

    // Null for an empty square, which is not an error: callers ask before they
    // know whether a square holds anything.
    [[nodiscard]] QSvgRenderer* rendererFor(chess::Piece piece);

private:
    [[nodiscard]] static QString resourcePathFor(chess::Piece piece);

    // Keyed by piece type and colour packed into one integer, because QHash
    // needs a hashable key and chess::Piece is a plain struct.
    QHash<int, QSvgRenderer*> renderers_;
};

} // namespace cines
