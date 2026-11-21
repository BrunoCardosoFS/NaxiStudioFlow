#include "CartWallTableModel.h"

#include <QVariant>

CartWallTableModel::CartWallTableModel(QObject *parent)
    : QAbstractTableModel(parent) {}

int CartWallTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid() || m_columns <= 0)
    return 0;
  return (m_items.size() + m_columns - 1) / m_columns;
}

int CartWallTableModel::columnCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : m_columns;
}

int CartWallTableModel::linearIndex(const QModelIndex &index) const {
  if (!index.isValid())
    return -1;
  int linear = index.row() * m_columns + index.column();
  return linear < m_items.size() ? linear : -1;
}

QModelIndex CartWallTableModel::indexFromLinear(int linear) const {
  if (linear < 0 || linear >= m_items.size() || m_columns <= 0)
    return QModelIndex();
  return createIndex(linear / m_columns, linear % m_columns);
}

const CartItem *CartWallTableModel::itemAt(int linear) const {
  if (linear < 0 || linear >= m_items.size())
    return nullptr;
  return &m_items.at(linear);
}

int CartWallTableModel::indexOfUuid(const QUuid &uuid) const {
  return m_uuidIndexMap.value(uuid, -1);
}

QVariant CartWallTableModel::data(const QModelIndex &index, int role) const {
  const CartItem *item = itemAt(linearIndex(index));
  if (!item)
    return QVariant();

  switch (role) {
  case Qt::DisplayRole:
  case CartTitleRole:
    return item->title;
  case CartUuidRole:
    return item->uuid;
  case CartStatusRole:
    return static_cast<int>(item->status);
  case CartPathRole:
    return item->path;
  case CartDurationRole:
    return item->durationMs;
  case CartPositionRole:
    return item->positionMs;
  case CartMediaTypeRole:
    return static_cast<int>(item->mediaType);
  default:
    return QVariant();
  }
}

Qt::ItemFlags CartWallTableModel::flags(const QModelIndex &index) const {
  if (linearIndex(index) < 0)
    return Qt::NoItemFlags;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
         Qt::ItemIsDropEnabled;
}

Qt::DropActions CartWallTableModel::supportedDropActions() const {
  return Qt::MoveAction | Qt::CopyAction;
}

Qt::DropActions CartWallTableModel::supportedDragActions() const {
  return Qt::MoveAction;
}

void CartWallTableModel::setColumnCount(int columns) {
  if (columns < 1)
    columns = 1;
  if (columns == m_columns)
    return;
  beginResetModel();
  m_columns = columns;
  endResetModel();
}

void CartWallTableModel::rebuildUuidMap() {
  m_uuidIndexMap.clear();
  for (int i = 0; i < m_items.size(); ++i)
    m_uuidIndexMap.insert(m_items.at(i).uuid, i);
}

void CartWallTableModel::addItem(const CartItem &item) {
  insertItem(m_items.size(), item);
}

void CartWallTableModel::insertItem(int linear, const CartItem &item) {
  linear = qBound(0, linear, static_cast<int>(m_items.size()));
  CartItem copy = item;
  if (copy.uuid.isNull())
    copy.uuid = QUuid::createUuid();

  beginResetModel();
  m_items.insert(linear, copy);
  rebuildUuidMap();
  endResetModel();
}

void CartWallTableModel::removeItem(int linear) {
  if (linear < 0 || linear >= m_items.size())
    return;
  beginResetModel();
  m_items.removeAt(linear);
  rebuildUuidMap();
  endResetModel();
}

void CartWallTableModel::clear() {
  beginResetModel();
  m_items.clear();
  m_uuidIndexMap.clear();
  endResetModel();
}

bool CartWallTableModel::moveItem(int fromLinear, int toLinear) {
  if (fromLinear < 0 || fromLinear >= m_items.size() || toLinear < 0 ||
      toLinear >= m_items.size() || fromLinear == toLinear)
    return false;

  beginResetModel();
  m_items.move(fromLinear, toLinear);
  rebuildUuidMap();
  endResetModel();
  return true;
}

void CartWallTableModel::setItemStatus(int linear, CartItemStatus status) {
  if (linear < 0 || linear >= m_items.size() ||
      m_items[linear].status == status)
    return;
  m_items[linear].status = status;
  const QModelIndex idx = indexFromLinear(linear);
  emit dataChanged(idx, idx, {CartStatusRole});
}

void CartWallTableModel::setItemPosition(int linear, qint64 positionMs) {
  if (linear < 0 || linear >= m_items.size() ||
      m_items[linear].positionMs == positionMs)
    return;
  m_items[linear].positionMs = positionMs;
  const QModelIndex idx = indexFromLinear(linear);
  emit dataChanged(idx, idx, {CartPositionRole});
}

void CartWallTableModel::loadMockData() {
  struct Mock {
    const char *title;
    MediaType type;
    qint64 durationMs;
  };
  static const Mock mocks[] = {
      {"Vinheta Abertura", MediaType::Jingle, 8000},
      {"Prefixo Rádio", MediaType::Jingle, 5000},
      {"Comercial Supermercado", MediaType::Commercial, 30000},
      {"Comercial Farmácia", MediaType::Commercial, 45000},
      {"Efeito Aplausos", MediaType::Other, 6000},
      {"Vinheta Notícias", MediaType::Jingle, 12000},
      {"Comercial Concessionária", MediaType::Commercial, 60000},
      {"Efeito Buzina", MediaType::Other, 3000},
  };

  QVector<CartItem> list;
  for (const Mock &m : mocks) {
    CartItem item;
    item.uuid = QUuid::createUuid();
    item.title = QString::fromUtf8(m.title);
    item.mediaType = m.type;
    item.durationMs = m.durationMs;
    list.append(item);
  }

  // Simulate one slot playing, to validate the progress bar.
  list[1].status = CartItemStatus::Playing;
  list[1].positionMs = list[1].durationMs / 3;

  beginResetModel();
  m_items = list;
  rebuildUuidMap();
  endResetModel();
}
