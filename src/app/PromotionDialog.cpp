#include "PromotionDialog.hpp"

#include "PieceRenderer.hpp"

#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QSvgRenderer>
#include <QVBoxLayout>

namespace cines {
namespace {

constexpr int kButtonSize = 72;
constexpr int kIconSize = 60;

// The SVGs are the same artwork the board uses, rasterised once at the size
// the button needs.
QIcon iconFor(const PieceRenderer& renderer, chess::Piece piece)
{
    QSvgRenderer* svg = renderer.rendererFor(piece);
    if (svg == nullptr) {
        return {};
    }

    QPixmap pixmap(kIconSize, kIconSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    svg->render(&painter, QRectF(0, 0, kIconSize, kIconSize));
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
        button->setIcon(iconFor(renderer, chess::Piece{choice.type, color}));
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
