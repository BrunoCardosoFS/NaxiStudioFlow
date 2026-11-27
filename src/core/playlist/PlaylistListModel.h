#ifndef PLAYLISTLISTMODEL_H
#define PLAYLISTLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QTime>
#include <QUuid>
#include <QVector>

#include <functional>

#include "core/models.h"

class QMediaPlayer;
class QTimer;

class PlaylistListModel : public QAbstractListModel {
  Q_OBJECT
public:
  using DurationResolver = std::function<qint64(const QString &path)>;
  using MediaPathResolverFn = std::function<QString(const QString &path)>;

  explicit PlaylistListModel(QObject *parent = nullptr);
  ~PlaylistListModel() override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  Qt::DropActions supportedDropActions() const override;
  Qt::DropActions supportedDragActions() const override;

  const QVector<PlaylistItem> &items() const { return m_items; }
  const PlaylistItem *itemAt(int row) const;

  int rowForUuid(const QUuid &uuid) const;
  int findNextMediaRow(int afterRow) const;
  int findNextEntryRow(int afterRow) const;
  int findNextPlayableRow() const;
  int blockMarkerRowForRow(int row) const;

  int currentPlayingRow() const;
  int lastLockedRow() const;
  bool isDropRowBetweenMarkers(int row) const;
  bool canRemoveRow(int row) const;

  void addItem(const PlaylistItem &item);
  void insertItem(int row, const PlaylistItem &item);
  bool insertCatalogItem(int row, const QString &title, const QString &path,
                         MediaType mediaType, qint64 durationMs = 0);
  bool insertCommandItem(int row, const QString &title, CommandType commandType,
                         const QString &commandId = QString());
  bool removeItem(int row);
  void clear();
  void cancelDurationProbes();

  void setDurationResolver(DurationResolver resolver);
  void setMediaPathResolver(MediaPathResolverFn resolver);
  void cacheDuration(const QString &path, qint64 durationMs);

  void setPlayingRow(int row, int mixingOutgoingRow = -1);
  void stopPlayingRow(bool markAsFinished);
  void finishThroughRow(int row);
  void markRowMixing(int row);
  void markRowFinished(int row);
  void setPendingOverlayUuid(const QUuid &uuid);
  void setItemPosition(int row, qint64 positionMs);
  void setItemPlayerId(int row, int playerId);
  void updateItemAudioMetadata(int row, qint64 durationMs, qint64 startPoint,
                               qint64 endPoint, qint64 mixStart, qint64 mixEnd,
                               qint64 fadeInEndPoint, qint64 fadeOutStartPoint);
  void updateItemMixMetadata(int row, qint64 durationMs, qint64 startPoint,
                             qint64 endPoint, qint64 mixStart, qint64 mixEnd,
                             qint64 fadeInEndPoint, qint64 fadeOutStartPoint,
                             float startVolume, float endVolume,
                             qint64 volumeUpStartPoint,
                             qint64 volumeUpEndPoint,
                             qint64 volumeDownStartPoint,
                             qint64 volumeDownEndPoint);

  bool moveItem(int fromRow, int toRow);
  void recalculateBlockMetadata();
  void loadMockData();

signals:
  void itemDurationLoaded(int row, const QUuid &uuid, qint64 durationMs);

private:
  static constexpr int kMaxDurationCacheEntries = 4096;

  QVector<PlaylistItem> m_items;
  QHash<QUuid, int> m_uuidRowMap;
  QHash<QString, qint64> m_durationCache;
  int m_playingRow = -1;
  int m_lastLockedRow = -1;

  QMediaPlayer *m_probePlayer = nullptr;
  QTimer *m_probeTimeoutTimer = nullptr;
  QList<QUuid> m_pendingDurationProbes;
  QUuid m_activeProbeUuid;
  QUuid m_pendingOverlayUuid;
  DurationResolver m_durationResolver;
  MediaPathResolverFn m_mediaPathResolver;

  QString resolveMediaPath(const QString &path) const;
  void rebuildUuidMap();
  void updateUuidMapFrom(int startRow);
  void recalculateBlockMetadataInternal(int overridePlayingRow,
                                        int mixingOutgoingRow = -1);
  void enqueueDurationProbe(const QUuid &uuid);
  void processNextDurationProbe();
  void finalizeActiveDurationProbe(qint64 durationMs);
  void cancelAllDurationProbes();
  void applyProbedDuration(const QUuid &uuid, qint64 durationMs);
};

#endif // PLAYLISTLISTMODEL_H
