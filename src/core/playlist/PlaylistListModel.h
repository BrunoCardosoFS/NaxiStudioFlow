#ifndef PLAYLISTLISTMODEL_H
#define PLAYLISTLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QTime>
#include <QUuid>
#include <QVector>

#include "core/models.h"

class PlaylistListModel : public QAbstractListModel {
  Q_OBJECT
public:
  explicit PlaylistListModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  Qt::DropActions supportedDropActions() const override;
  Qt::DropActions supportedDragActions() const override;

  const QVector<PlaylistItem> &items() const { return m_items; }
  const PlaylistItem *itemAt(int row) const;

  int currentPlayingRow() const;
  bool isDropRowBetweenMarkers(int row) const;

  void addItem(const PlaylistItem &item);
  void insertItem(int row, const PlaylistItem &item);
  void removeItem(int row);
  void clear();

  bool moveItem(int fromRow, int toRow);
  void recalculateBlockMetadata();
  void loadMockData();

private:
  QVector<PlaylistItem> m_items;
  QHash<QUuid, int> m_uuidRowMap;
  int m_playingRow = -1;

  void rebuildUuidMap();
};

#endif // PLAYLISTLISTMODEL_H
