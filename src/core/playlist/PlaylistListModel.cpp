#include "./PlaylistListModel.h"

PlaylistListModel::PlaylistListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int PlaylistListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_items.size();
}

QVariant PlaylistListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
    return QVariant();
  }

  const PlaylistItem &item = m_items.at(index.row());

  switch (role) {
  case Qt::DisplayRole:
  case Qt::ToolTipRole:
  case TitleRole:
    return item.title;
  case UuidRole:
    return item.uuid;
  case StatusRole:
    return static_cast<int>(item.status);
  case PathRole:
    return item.path;
  case DurationRole:
    return item.durationMs;
  case PositionRole:
    return item.positionMs;
  case MixStartRole:
    return item.mixStart;
  case MixEndRole:
    return item.mixEnd;
  case BlockTypeRole:
    return static_cast<int>(item.blockType);
  case TypeRole:
    return static_cast<int>(item.type);
  case MediaTypeRole:
    return static_cast<int>(item.mediaType);
  default:
    return QVariant();
  }
}