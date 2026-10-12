#ifndef CARTWALLTABLEMODEL_H
#define CARTWALLTABLEMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QUuid>
#include <QVector>

#include "core/cartwall/CartItem.h"

// Table model backed by a linear list. Cell (row, col) maps to the linear
// index row * columnCount + col. Cells past the end of the list are not valid
// items (no flags, no data), so no empty slots are ever shown.
class CartWallTableModel : public QAbstractTableModel {
  Q_OBJECT
public:
  explicit CartWallTableModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  Qt::DropActions supportedDropActions() const override;
  Qt::DropActions supportedDragActions() const override;

  void setColumnCount(int columns);

  // Linear <-> grid mapping
  int linearIndex(const QModelIndex &index) const;
  QModelIndex indexFromLinear(int linear) const;

  int itemCount() const { return m_items.size(); }
  const QVector<CartItem> &items() const { return m_items; }
  const CartItem *itemAt(int linear) const;
  int indexOfUuid(const QUuid &uuid) const;

  void addItem(const CartItem &item);
  void insertItem(int linear, const CartItem &item);
  void removeItem(int linear);
  void clear();
  bool moveItem(int fromLinear, int toLinear);

  void setItemStatus(int linear, CartItemStatus status);
  void setItemPosition(int linear, qint64 positionMs);

  void loadMockData();

private:
  void rebuildUuidMap();

  QVector<CartItem> m_items;
  QHash<QUuid, int> m_uuidIndexMap;
  int m_columns = 4;
};

#endif // CARTWALLTABLEMODEL_H
