#include "MoveListModel.hpp"

#include "GameController.hpp"

#include <QBrush>
#include <QFont>

namespace cines {
namespace {

// A row is one move pair, so ply 1 and 2 share row 0.
std::size_t rowForPly(std::size_t ply)
{
    return (ply - 1) / 2;
}

} // namespace

MoveListModel::MoveListModel(GameController& controller, QObject* parent)
    : QAbstractTableModel(parent)
    , controller_(controller)
{
}

int MoveListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    // A game of an odd number of plies still needs the row its last move sits
    // in, so this rounds up.
    return static_cast<int>((controller_.history().size() + 1) / 2);
}

int MoveListModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

std::size_t MoveListModel::plyAt(const QModelIndex& index) const
{
    if (!index.isValid() || index.column() == NumberColumn) {
        return 0;
    }

    const auto row = static_cast<std::size_t>(index.row());
    const std::size_t ply = (row * 2) + (index.column() == WhiteColumn ? 1 : 2);

    // Black's half of the final pair is empty when White has just moved.
    return ply <= controller_.history().size() ? ply : 0;
}

QVariant MoveListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.column() == NumberColumn) {
        if (role == Qt::DisplayRole) {
            // Numbering follows the position the game started from, so a game
            // set up mid-play does not restart at one.
            const int firstNumber = controller_.startPosition().fullmoveNumber;
            return QStringLiteral("%1.").arg(firstNumber + index.row());
        }
        if (role == Qt::TextAlignmentRole) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return {};
    }

    const std::size_t ply = plyAt(index);
    if (ply == 0) {
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
        return QString::fromStdString(controller_.history()[ply - 1].san);
    case Qt::FontRole: {
        // The move the board is showing is emboldened, which is what makes
        // rewinding through the game legible.
        QFont font;
        font.setBold(ply == controller_.currentPly());
        return font;
    }
    case Qt::ToolTipRole:
        return QString::fromStdString(controller_.history()[ply - 1].move.toUci());
    default:
        return {};
    }
}

QVariant MoveListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case NumberColumn:
        return QStringLiteral("#");
    case WhiteColumn:
        return tr("White");
    case BlackColumn:
        return tr("Black");
    default:
        return {};
    }
}

QModelIndex MoveListModel::indexOfCurrentPly() const
{
    const std::size_t ply = controller_.currentPly();
    if (ply == 0) {
        return {};
    }

    const int column = (ply % 2) == 1 ? WhiteColumn : BlackColumn;
    return index(static_cast<int>(rowForPly(ply)), column);
}

void MoveListModel::refresh()
{
    // The whole table is rebuilt because a move can also truncate the history,
    // and the table is at most a few hundred rows.
    beginResetModel();
    endResetModel();
}

} // namespace cines
