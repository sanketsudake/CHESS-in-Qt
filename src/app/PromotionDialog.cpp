#include "PromotionDialog.hpp"

#include "PieceRenderer.hpp"

#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QSvgRenderer>
#include <QVBoxLayout>

#include <array>
#include <cmath>

namespace cines {
namespace {

constexpr int kButtonSize = 72;
constexpr int kIconSize = 60;

// The SVGs are the same artwork the board uses, rasterised at the size the
// button needs.
//
// Rasterised at the display's own pixel density rather than at logical size:
// on a high density screen a 60x60 pixmap would be stretched to 120 device
// pixels and look soft next to a board that renders from SVG at any scale.
QIcon iconFor(const PieceRenderer& renderer, chess::Piece piece, qreal pixelRatio)
{
    QSvgRenderer* svg = renderer.rendererFor(piece);
    if (svg == nullptr) {
        return {};
    }

    const auto side = static_cast<int>(std::lround(kIconSize * pixelRatio));
    QPixmap pixmap(side, side);
    pixmap.fill(Qt::transparent);
    {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        svg->render(&painter, QRectF(0, 0, side, side));
    }
    pixmap.setDevicePixelRatio(pixelRatio);
    return QIcon(pixmap);
}

} // namespace

PromotionDialog::PromotionDialog(chess::Color color, const PieceRenderer& renderer, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Promote pawn"));
    setModal(true);

    auto* layout = new QVBoxLayout(this);
    auto* buttons = new QHBoxLayout;

    // Queen first: it is the usual choice, so it is the one under the cursor
    // and the one the keyboard reaches first.
    struct Choice {
        chess::PieceType type;
        QString shortcut;
    };
    const std::array<Choice, 4> choices{{
        {chess::PieceType::Queen, QStringLiteral("Q")},
        {chess::PieceType::Rook, QStringLiteral("R")},
        {chess::PieceType::Bishop, QStringLiteral("B")},
        {chess::PieceType::Knight, QStringLiteral("N")},
    }};

    for (const Choice& choice : choices) {
        auto* button = new QPushButton(this);
        button->setIcon(iconFor(renderer, chess::Piece{choice.type, color}, devicePixelRatioF()));
        button->setIconSize(QSize(kIconSize, kIconSize));
        button->setFixedSize(kButtonSize, kButtonSize);
        button->setToolTip(choice.shortcut);
        button->setShortcut(QKeySequence(choice.shortcut));
        connect(button, &QPushButton::clicked, this, [this, choice] { choose(choice.type); });
        buttons->addWidget(button);
    }

    layout->addLayout(buttons);
}

void PromotionDialog::choose(chess::PieceType piece)
{
    chosen_ = piece;
    accept();
}

} // namespace cines
