#include "CapturedTray.hpp"

#include "GameController.hpp"
#include "PieceRenderer.hpp"

#include "chess/Rules.hpp"

#include <QPainter>
#include <QSvgRenderer>

#include <algorithm>
#include <array>

namespace cines {
namespace {

constexpr int kIconSize = 20;
constexpr int kIconOverlap = 6;
constexpr int kTrayHeight = 26;
constexpr int kLeadGap = 8;

// The values everybody uses. The king has none: it is never captured, so its
// value would never be added to anything.
constexpr int valueOf(chess::PieceType type)
{
    switch (type) {
    case chess::PieceType::Pawn:
        return 1;
    case chess::PieceType::Knight:
    case chess::PieceType::Bishop:
        return 3;
    case chess::PieceType::Rook:
        return 5;
    case chess::PieceType::Queen:
        return 9;
    case chess::PieceType::King:
    case chess::PieceType::None:
        break;
    }
    return 0;
}

// Heaviest first, which is how a player would read a list of captures.
constexpr std::array<chess::PieceType, 5> kByDescendingValue{chess::PieceType::Queen, chess::PieceType::Rook,
    chess::PieceType::Bishop, chess::PieceType::Knight, chess::PieceType::Pawn};

int valueOf(const chess::Material& material)
{
    int total = 0;
    for (const chess::PieceType type : kByDescendingValue) {
        total += material.of(type) * valueOf(type);
    }
    return total;
}

} // namespace

CapturedTray::CapturedTray(
    GameController& controller, PieceRenderer& renderer, chess::Color side, QWidget* parent)
    : QWidget(parent)
    , controller_(controller)
    , renderer_(renderer)
    , side_(side)
{
    setMinimumHeight(kTrayHeight);
    recalculate();
}

QSize CapturedTray::sizeHint() const
{
    return {kIconSize * 8, kTrayHeight};
}

int CapturedTray::materialLead() const
{
    return lead_;
}

void CapturedTray::recalculate()
{
    captured_.clear();

    const chess::Color victim = chess::opposite(side_);

    // Three passes over the board, not one per piece type. What this side has
    // taken is what the other has lost since the game began, which is why a
    // custom starting position needs no special case.
    const chess::Material started = chess::countMaterial(controller_.startPosition().board, victim);
    const chess::Material remains = chess::countMaterial(controller_.position().board, victim);
    const chess::Material mine = chess::countMaterial(controller_.position().board, side_);

    for (const chess::PieceType type : kByDescendingValue) {
        const int lost = started.of(type) - remains.of(type);
        for (int i = 0; i < lost; ++i) {
            captured_.append(type);
        }
    }

    lead_ = std::max(0, valueOf(mine) - valueOf(remains));
}

void CapturedTray::refresh()
{
    recalculate();
    update();
}

void CapturedTray::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const chess::Color victim = chess::opposite(side_);
    const int top = (height() - kIconSize) / 2;
    int x = 0;

    for (const chess::PieceType type : captured_) {
        QSvgRenderer* svg = renderer_.rendererFor(chess::Piece{type, victim});
        if (svg != nullptr) {
            svg->render(&painter, QRectF(x, top, kIconSize, kIconSize));
        }
        // Overlapping keeps a long row of pawns from pushing the panel wide.
        x += kIconSize - kIconOverlap;
    }

    if (lead_ <= 0) {
        return;
    }

    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(QRect(x + kLeadGap, 0, width() - x - kLeadGap, height()),
        Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("+%1").arg(lead_));
}

} // namespace cines
