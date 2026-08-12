#pragma once

#include "ChessMetaTypes.hpp"

#include <QAbstractTableModel>

#include <cstddef>

namespace cines {

class GameController;

// The game's moves as a table: move number, White's move, Black's reply.
//
// A model rather than a list of strings the window keeps in step, because the
// same history is already the controller's and duplicating it is how the two
// drift apart. Rows are computed from the history on demand; nothing is
// stored here.
class MoveListModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { NumberColumn = 0, WhiteColumn = 1, BlackColumn = 2, ColumnCount = 3 };

    explicit MoveListModel(GameController& controller, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // The ply an index refers to, counting from 1, or 0 when the cell holds no
    // move -- a move number, or Black's half of an unfinished pair.
    [[nodiscard]] std::size_t plyAt(const QModelIndex& index) const;

    // Where the viewer currently stands, so the view can put the selection
    // there. Invalid at the start of the game, when no move has been played.
    [[nodiscard]] QModelIndex indexOfCurrentPly() const;

public slots:
    void refresh();

private:
    GameController& controller_;
};

} // namespace cines
