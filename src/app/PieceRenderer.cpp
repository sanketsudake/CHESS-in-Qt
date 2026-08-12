#include "PieceRenderer.hpp"

#include <QString>
#include <QSvgRenderer>

namespace cines {
namespace {

int keyFor(chess::Piece piece)
{
    return (static_cast<int>(piece.type) * 2) + static_cast<int>(piece.color);
}

QString baseNameFor(chess::PieceType type)
{
    switch (type) {
    case chess::PieceType::Pawn:
        return QStringLiteral("pawn");
    case chess::PieceType::Knight:
        return QStringLiteral("knight");
    case chess::PieceType::Bishop:
        return QStringLiteral("bishop");
    case chess::PieceType::Rook:
        return QStringLiteral("rook");
    case chess::PieceType::Queen:
        return QStringLiteral("queen");
    case chess::PieceType::King:
        return QStringLiteral("king");
    case chess::PieceType::None:
        break;
    }
    return {};
}

} // namespace

PieceRenderer::PieceRenderer(QObject* parent)
    : QObject(parent)
{
}

QString PieceRenderer::resourcePathFor(chess::Piece piece)
{
    const QString base = baseNameFor(piece.type);
    if (base.isEmpty()) {
        return {};
    }
    const QString colour
        = piece.color == chess::Color::White ? QStringLiteral("white") : QStringLiteral("black");
    return QStringLiteral(":/pieces/%1_%2.svg").arg(base, colour);
}

QSvgRenderer* PieceRenderer::rendererFor(chess::Piece piece) const
{
    if (piece.isEmpty()) {
        return nullptr;
    }

    const int key = keyFor(piece);
    const auto existing = renderers_.constFind(key);
    if (existing != renderers_.constEnd()) {
        return existing.value();
    }

    auto* renderer = new QSvgRenderer(resourcePathFor(piece), const_cast<PieceRenderer*>(this));
    if (!renderer->isValid()) {
        // A missing or malformed asset. Drop it rather than caching a renderer
        // that would draw nothing on every future lookup.
        delete renderer;
        return nullptr;
    }

    renderers_.insert(key, renderer);
    return renderer;
}

} // namespace cines
