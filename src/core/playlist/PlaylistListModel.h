#ifndef PLAYLISTLISTMODEL_H
#define PLAYLISTLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QUuid>
#include <QVector>

#include "core/models.h"

class PlaylistListModel : public QAbstractListModel {
  Q_OBJECT
public:
  explicit PlaylistListModel(QObject *parent = nullptr);

  // QAbstractListModel interface
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  const QVector<PlaylistItem> &items() const { return m_items; }

private:
  QVector<PlaylistItem> m_items;
  QHash<QUuid, int> m_uuidRowMap;
};

#endif // PLAYLISTLISTMODEL_H
