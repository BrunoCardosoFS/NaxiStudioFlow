#include "./PlaylistListModel.h"

#include <QFileInfo>
#include <QMediaPlayer>
#include <QMetaObject>
#include <QTimer>
#include <QUrl>

#include <cmath>

PlaylistListModel::PlaylistListModel(QObject *parent)
    : QAbstractListModel(parent) {
  loadMockData();
}

PlaylistListModel::~PlaylistListModel() {
  cancelAllDurationProbes();
  if (m_probePlayer) {
    m_probePlayer->disconnect(this);
  }
}

int PlaylistListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_items.size();
}

const PlaylistItem *PlaylistListModel::itemAt(int row) const {
  if (row >= 0 && row < m_items.size()) {
    return &m_items.at(row);
  }
  return nullptr;
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
  case BlockTitleRole:
    return item.blockTitle;
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
  case StartPointRole:
    return item.startPoint;
  case EndPointRole:
    return item.endPoint;
  case FadeInEndPointRole:
    return item.fadeInEndPoint;
  case FadeOutStartPointRole:
    return item.fadeOutStartPoint;
  case StartVolumeRole:
    return item.startVolume;
  case EndVolumeRole:
    return item.endVolume;
  case VolumeUpStartPointRole:
    return item.volumeUpStartPoint;
  case VolumeUpEndPointRole:
    return item.volumeUpEndPoint;
  case VolumeDownStartPointRole:
    return item.volumeDownStartPoint;
  case VolumeDownEndPointRole:
    return item.volumeDownEndPoint;
  case BlockTypeRole:
    return static_cast<int>(item.blockType);
  case TypeRole:
    return static_cast<int>(item.type);
  case MediaTypeRole:
    return static_cast<int>(item.mediaType);
  case EstimatedTimeRole:
    return item.estimatedTime;
  case TotalDurationRole:
    return item.totalDurationMs;
  case ItemsCountRole:
    return item.itemsCount;
  case MarkerTextRole:
    return item.markerText;
  default:
    return QVariant();
  }
}

Qt::ItemFlags PlaylistListModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return Qt::ItemIsDropEnabled;
  }

  if (index.row() < 0 || index.row() >= m_items.size()) {
    return Qt::NoItemFlags;
  }

  const PlaylistItem &item = m_items.at(index.row());

  if (item.type != ItemType::Media) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  // Items at or above the playing/finished lock row are locked.
  int lockedRow = lastLockedRow();
  if (lockedRow >= 0 && index.row() <= lockedRow) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

Qt::DropActions PlaylistListModel::supportedDropActions() const {
  return Qt::MoveAction | Qt::CopyAction;
}

Qt::DropActions PlaylistListModel::supportedDragActions() const {
  return Qt::MoveAction;
}

int PlaylistListModel::rowForUuid(const QUuid &uuid) const {
  if (uuid.isNull()) {
    return -1;
  }
  return m_uuidRowMap.value(uuid, -1);
}

int PlaylistListModel::findNextMediaRow(int afterRow) const {
  const int start = qMax(0, afterRow + 1);
  for (int i = start; i < m_items.size(); ++i) {
    if (m_items.at(i).type == ItemType::Media) {
      return i;
    }
  }
  return -1;
}

int PlaylistListModel::findNextPlayableRow() const {
  for (int i = 0; i < m_items.size(); ++i) {
    const PlaylistItem &item = m_items.at(i);
    if (item.type == ItemType::Media &&
        item.status == PlaylistItemStatus::Next) {
      return i;
    }
  }
  for (int i = 0; i < m_items.size(); ++i) {
    const PlaylistItem &item = m_items.at(i);
    if (item.type == ItemType::Media &&
        item.status == PlaylistItemStatus::Default) {
      return i;
    }
  }
  return -1;
}

int PlaylistListModel::blockMarkerRowForRow(int row) const {
  if (m_items.isEmpty() || row < 0) {
    return -1;
  }
  for (int i = qMin(row, m_items.size() - 1); i >= 0; --i) {
    if (m_items.at(i).type == ItemType::BlockMarker) {
      return i;
    }
  }
  return -1;
}

int PlaylistListModel::currentPlayingRow() const {
  // Cached; refreshed by recalculateBlockMetadata().
  return m_playingRow;
}

int PlaylistListModel::lastLockedRow() const {
  int locked = m_playingRow;
  for (int i = m_items.size() - 1; i >= 0; --i) {
    const PlaylistItem &it = m_items.at(i);
    if (it.type == ItemType::Media &&
        (it.status == PlaylistItemStatus::Playing ||
         it.status == PlaylistItemStatus::Finished)) {
      locked = qMax(locked, i);
      break;
    }
  }
  return locked;
}

bool PlaylistListModel::isDropRowBetweenMarkers(int row) const {
  if (row <= 0 || row >= m_items.size()) {
    return false;
  }

  const PlaylistItem &prev = m_items.at(row - 1);
  const PlaylistItem &next = m_items.at(row);

  bool prevValid =
      (prev.type == ItemType::StartMarker || prev.type == ItemType::Media);
  bool nextValid =
      (next.type == ItemType::EndMarker || next.type == ItemType::Media);

  return prevValid && nextValid;
}

bool PlaylistListModel::canRemoveRow(int row) const {
  if (row < 0 || row >= m_items.size()) {
    return false;
  }
  if (m_items.at(row).type != ItemType::Media) {
    return false;
  }
  const int lockedRow = lastLockedRow();
  if (lockedRow >= 0 && row <= lockedRow) {
    return false;
  }
  return true;
}

void PlaylistListModel::addItem(const PlaylistItem &item) {
  PlaylistItem copy = item;
  if (copy.uuid.isNull()) {
    copy.uuid = QUuid::createUuid();
  }
  if (copy.type == ItemType::Media && copy.durationMs > 0) {
    if (copy.endPoint <= 0) {
      copy.endPoint = copy.durationMs;
    }
    if (copy.mixEnd <= copy.startPoint) {
      copy.mixEnd = copy.endPoint;
    }
    if (copy.fadeOutStartPoint <= 0) {
      copy.fadeOutStartPoint = copy.endPoint;
    }
  }

  beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
  m_items.append(copy);
  rebuildUuidMap();
  endInsertRows();

  recalculateBlockMetadata();

  if (copy.type == ItemType::Media && copy.durationMs <= 0 &&
      !copy.path.isEmpty()) {
    enqueueDurationProbe(copy.uuid);
  }
}

void PlaylistListModel::insertItem(int row, const PlaylistItem &item) {
  const int lockedRow = lastLockedRow();
  if (lockedRow >= 0 && row <= lockedRow) {
    return;
  }

  PlaylistItem copy = item;
  if (copy.uuid.isNull()) {
    copy.uuid = QUuid::createUuid();
  }
  if (copy.type == ItemType::Media && !isDropRowBetweenMarkers(row)) {
    return;
  }
  if (copy.type == ItemType::Media && copy.durationMs > 0) {
    if (copy.endPoint <= 0) {
      copy.endPoint = copy.durationMs;
    }
    if (copy.mixEnd <= copy.startPoint) {
      copy.mixEnd = copy.endPoint;
    }
    if (copy.fadeOutStartPoint <= 0) {
      copy.fadeOutStartPoint = copy.endPoint;
    }
  }

  const int targetRow = qBound(0, row, m_items.size());
  beginInsertRows(QModelIndex(), targetRow, targetRow);
  m_items.insert(targetRow, copy);
  rebuildUuidMap();
  endInsertRows();

  recalculateBlockMetadata();

  if (copy.type == ItemType::Media && copy.durationMs <= 0 &&
      !copy.path.isEmpty()) {
    enqueueDurationProbe(copy.uuid);
  }
}

bool PlaylistListModel::insertCatalogItem(int row, const QString &title,
                                          const QString &path,
                                          MediaType mediaType,
                                          qint64 durationMs) {
  const int lockedRow = lastLockedRow();
  if (lockedRow >= 0 && row <= lockedRow) {
    return false;
  }
  if (!isDropRowBetweenMarkers(row)) {
    return false;
  }

  PlaylistItem item;
  item.uuid = QUuid::createUuid();
  item.type = ItemType::Media;
  item.mediaType = mediaType;
  item.title = title.isEmpty() ? QFileInfo(path).fileName() : title;
  item.path = path;
  item.durationMs = qMax<qint64>(0, durationMs);
  item.status = PlaylistItemStatus::Default;

  insertItem(row, item);
  return true;
}

bool PlaylistListModel::removeItem(int row) {
  if (!canRemoveRow(row)) {
    return false;
  }

  const QUuid removedUuid = m_items.at(row).uuid;
  m_pendingDurationProbes.removeAll(removedUuid);
  if (m_activeProbeUuid == removedUuid) {
    m_activeProbeUuid = QUuid();
    if (m_probeTimeoutTimer) {
      m_probeTimeoutTimer->stop();
    }
    if (m_probePlayer) {
      m_probePlayer->setSource(QUrl());
    }
    QMetaObject::invokeMethod(this,
                              &PlaylistListModel::processNextDurationProbe,
                              Qt::QueuedConnection);
  }

  beginRemoveRows(QModelIndex(), row, row);
  m_items.removeAt(row);
  rebuildUuidMap();
  endRemoveRows();

  recalculateBlockMetadata();
  return true;
}

void PlaylistListModel::clear() {
  cancelAllDurationProbes();
  beginResetModel();
  m_items.clear();
  m_uuidRowMap.clear();
  m_playingRow = -1;
  endResetModel();
}

void PlaylistListModel::cancelDurationProbes() { cancelAllDurationProbes(); }

void PlaylistListModel::setDurationResolver(DurationResolver resolver) {
  m_durationResolver = std::move(resolver);
}

void PlaylistListModel::setPlayingRow(int row) {
  if (row < 0 || row >= m_items.size() ||
      m_items.at(row).type != ItemType::Media) {
    return;
  }

  QList<int> changedRows;
  for (int i = 0; i < m_items.size(); ++i) {
    PlaylistItem &item = m_items[i];
    if (item.type != ItemType::Media || i == row) {
      continue;
    }
    const PlaylistItemStatus oldStatus = item.status;
    if (item.status == PlaylistItemStatus::Playing) {
      item.status = (i < row) ? PlaylistItemStatus::Finished
                              : PlaylistItemStatus::Default;
      item.playerId = -1;
    } else if (i < row) {
      item.status = PlaylistItemStatus::Finished;
    }
    if (item.status != oldStatus) {
      changedRows.append(i);
    }
  }

  if (m_items[row].status != PlaylistItemStatus::Playing) {
    m_items[row].status = PlaylistItemStatus::Playing;
    changedRows.append(row);
  }

  for (int changedRow : std::as_const(changedRows)) {
    const QModelIndex idx = index(changedRow, 0);
    emit dataChanged(idx, idx, {StatusRole});
  }

  recalculateBlockMetadata();
}

void PlaylistListModel::stopPlayingRow(bool markAsFinished) {
  int targetRow = m_playingRow;
  if (targetRow < 0 || targetRow >= m_items.size()) {
    for (int i = 0; i < m_items.size(); ++i) {
      if (m_items.at(i).type == ItemType::Media &&
          m_items.at(i).status == PlaylistItemStatus::Playing) {
        targetRow = i;
        break;
      }
    }
  }

  if (targetRow < 0 || targetRow >= m_items.size()) {
    return;
  }

  PlaylistItem &item = m_items[targetRow];
  item.playerId = -1;

  QList<int> changedRoles;
  const PlaylistItemStatus newStatus =
      markAsFinished ? PlaylistItemStatus::Finished : PlaylistItemStatus::Next;
  if (item.status != newStatus) {
    item.status = newStatus;
    changedRoles.append(StatusRole);
  }
  if (!markAsFinished && item.positionMs != 0) {
    item.positionMs = 0;
    changedRoles.append(PositionRole);
  }

  if (!changedRoles.isEmpty()) {
    const QModelIndex idx = index(targetRow, 0);
    emit dataChanged(idx, idx, changedRoles);
  }

  recalculateBlockMetadata();
}

void PlaylistListModel::markRowFinished(int row) {
  if (row < 0 || row >= m_items.size() ||
      m_items.at(row).type != ItemType::Media) {
    return;
  }

  m_items[row].playerId = -1;
  if (m_items[row].status != PlaylistItemStatus::Finished) {
    m_items[row].status = PlaylistItemStatus::Finished;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {StatusRole});
  }
  recalculateBlockMetadata();
}

void PlaylistListModel::setItemPosition(int row, qint64 positionMs) {
  if (row < 0 || row >= m_items.size() ||
      m_items.at(row).type != ItemType::Media) {
    return;
  }

  if (m_items.at(row).positionMs == positionMs) {
    return;
  }

  m_items[row].positionMs = positionMs;
  const QModelIndex idx = index(row, 0);
  emit dataChanged(idx, idx, {PositionRole});
}

void PlaylistListModel::setItemPlayerId(int row, int playerId) {
  if (row < 0 || row >= m_items.size()) {
    return;
  }
  m_items[row].playerId = playerId;
}

void PlaylistListModel::updateItemAudioMetadata(int row, qint64 durationMs,
                                                qint64 startPoint,
                                                qint64 endPoint,
                                                qint64 mixStart, qint64 mixEnd,
                                                qint64 fadeInEndPoint,
                                                qint64 fadeOutStartPoint) {
  if (row < 0 || row >= m_items.size() ||
      m_items.at(row).type != ItemType::Media) {
    return;
  }

  const PlaylistItem &item = m_items.at(row);
  updateItemMixMetadata(
      row, durationMs, startPoint, endPoint, mixStart, mixEnd, fadeInEndPoint,
      fadeOutStartPoint, item.startVolume, item.endVolume,
      item.volumeUpStartPoint, item.volumeUpEndPoint, item.volumeDownStartPoint,
      item.volumeDownEndPoint);
}

void PlaylistListModel::updateItemMixMetadata(
    int row, qint64 durationMs, qint64 startPoint, qint64 endPoint,
    qint64 mixStart, qint64 mixEnd, qint64 fadeInEndPoint,
    qint64 fadeOutStartPoint, float startVolume, float endVolume,
    qint64 volumeUpStartPoint, qint64 volumeUpEndPoint,
    qint64 volumeDownStartPoint, qint64 volumeDownEndPoint) {
  if (row < 0 || row >= m_items.size() ||
      m_items.at(row).type != ItemType::Media) {
    return;
  }

  PlaylistItem &item = m_items[row];
  QList<int> changedRoles;
  bool affectsSchedule = false;

  if (item.durationMs != durationMs) {
    item.durationMs = durationMs;
    changedRoles.append(DurationRole);
    affectsSchedule = true;
  }
  if (item.startPoint != startPoint) {
    item.startPoint = startPoint;
    changedRoles.append(StartPointRole);
    affectsSchedule = true;
  }
  if (item.endPoint != endPoint) {
    item.endPoint = endPoint;
    changedRoles.append(EndPointRole);
    affectsSchedule = true;
  }
  if (item.mixStart != mixStart) {
    item.mixStart = mixStart;
    changedRoles.append(MixStartRole);
    affectsSchedule = true;
  }
  if (item.mixEnd != mixEnd) {
    item.mixEnd = mixEnd;
    changedRoles.append(MixEndRole);
    affectsSchedule = true;
  }
  if (item.fadeInEndPoint != fadeInEndPoint) {
    item.fadeInEndPoint = fadeInEndPoint;
    changedRoles.append(FadeInEndPointRole);
  }
  if (item.fadeOutStartPoint != fadeOutStartPoint) {
    item.fadeOutStartPoint = fadeOutStartPoint;
    changedRoles.append(FadeOutStartPointRole);
  }
  if (std::abs(item.startVolume - startVolume) > 1e-4f) {
    item.startVolume = startVolume;
    changedRoles.append(StartVolumeRole);
  }
  if (std::abs(item.endVolume - endVolume) > 1e-4f) {
    item.endVolume = endVolume;
    changedRoles.append(EndVolumeRole);
  }
  if (item.volumeUpStartPoint != volumeUpStartPoint) {
    item.volumeUpStartPoint = volumeUpStartPoint;
    changedRoles.append(VolumeUpStartPointRole);
  }
  if (item.volumeUpEndPoint != volumeUpEndPoint) {
    item.volumeUpEndPoint = volumeUpEndPoint;
    changedRoles.append(VolumeUpEndPointRole);
  }
  if (item.volumeDownStartPoint != volumeDownStartPoint) {
    item.volumeDownStartPoint = volumeDownStartPoint;
    changedRoles.append(VolumeDownStartPointRole);
  }
  if (item.volumeDownEndPoint != volumeDownEndPoint) {
    item.volumeDownEndPoint = volumeDownEndPoint;
    changedRoles.append(VolumeDownEndPointRole);
  }

  if (changedRoles.isEmpty()) {
    return;
  }

  const QModelIndex idx = index(row, 0);
  emit dataChanged(idx, idx, changedRoles);

  if (affectsSchedule) {
    recalculateBlockMetadata();
  }
}

bool PlaylistListModel::moveItem(int fromRow, int toRow) {
  if (fromRow < 0 || fromRow >= m_items.size() || toRow < 0 ||
      toRow >= m_items.size()) {
    return false;
  }
  if (fromRow == toRow)
    return false;

  if (m_items.at(fromRow).type != ItemType::Media) {
    return false;
  }

  int lockedRow = lastLockedRow();
  if (lockedRow >= 0 && (fromRow <= lockedRow || toRow <= lockedRow)) {
    return false;
  }

  const int dropRow = (toRow > fromRow) ? (toRow + 1) : toRow;
  if (!isDropRowBetweenMarkers(dropRow)) {
    return false;
  }

  int destination = toRow > fromRow ? toRow + 1 : toRow;
  beginMoveRows(QModelIndex(), fromRow, fromRow, QModelIndex(), destination);
  m_items.move(fromRow, toRow);
  rebuildUuidMap();
  endMoveRows();

  recalculateBlockMetadata();
  return true;
}

void PlaylistListModel::recalculateBlockMetadata() {
  if (m_items.isEmpty()) {
    m_playingRow = -1;
    return;
  }

  // Snapshot derived fields so dataChanged is emitted only for changed rows.
  struct Derived {
    QString estimatedTime;
    QString markerText;
    qint64 totalDurationMs;
    int itemsCount;
    PlaylistItemStatus status;
    BlockType blockType;
  };
  QVector<Derived> before;
  before.reserve(m_items.size());
  for (const PlaylistItem &it : std::as_const(m_items)) {
    before.append({it.estimatedTime, it.markerText, it.totalDurationMs,
                   it.itemsCount, it.status, it.blockType});
  }

  m_playingRow = -1;
  for (int i = 0; i < m_items.size(); ++i) {
    const PlaylistItem &it = m_items.at(i);
    if (it.type == ItemType::Media &&
        it.status == PlaylistItemStatus::Playing) {
      m_playingRow = i;
      break;
    }
  }

  QTime accumulatedTime(0, 0, 0);
  BlockType activeBlockType = BlockType::Commercial;

  int commercialCount = 0;
  qint64 commercialDurationMs = 0;

  int musicCount = 0;
  qint64 musicDurationMs = 0;

  int currentBlockMarkerIdx = -1;
  qint64 currentBlockDurationMs = 0;

  for (int i = 0; i < m_items.size(); ++i) {
    PlaylistItem &item = m_items[i];

    if (item.type == ItemType::BlockMarker) {
      if (currentBlockMarkerIdx >= 0) {
        m_items[currentBlockMarkerIdx].totalDurationMs = currentBlockDurationMs;
      }
      currentBlockMarkerIdx = i;
      currentBlockDurationMs = 0;

      QTime parsed = QTime::fromString(item.title.trimmed(), "hh:mm");
      if (parsed.isValid()) {
        accumulatedTime = parsed;
      }
      commercialCount = 0;
      commercialDurationMs = 0;
      musicCount = 0;
      musicDurationMs = 0;
    } else if (item.type == ItemType::StartMarker) {
      activeBlockType = item.blockType;
      if (item.blockType == BlockType::Commercial) {
        item.markerText = QStringLiteral("BLOCO COMERCIAL");
        commercialCount = 0;
        commercialDurationMs = 0;
      } else {
        item.markerText.clear();
        musicCount = 0;
        musicDurationMs = 0;
      }
    } else if (item.type == ItemType::Media) {
      item.blockType = activeBlockType;
      item.estimatedTime = accumulatedTime.toString("hh:mm:ss");

      const qint64 effectiveEnd =
          (item.mixEnd > item.startPoint)
              ? item.mixEnd
              : ((item.endPoint > item.startPoint) ? item.endPoint
                                                   : item.durationMs);
      qint64 effectiveDuration = effectiveEnd - item.startPoint;
      for (int j = i + 1; j < m_items.size(); ++j) {
        if (m_items.at(j).type == ItemType::BlockMarker) {
          break;
        }
        if (m_items.at(j).type == ItemType::Media) {
          const PlaylistItem &nextItem = m_items.at(j);
          if (nextItem.mixStart > nextItem.startPoint) {
            effectiveDuration -= (nextItem.mixStart - nextItem.startPoint);
          }
          break;
        }
      }
      if (effectiveDuration < 0)
        effectiveDuration = 0;

      accumulatedTime =
          accumulatedTime.addMSecs(static_cast<int>(effectiveDuration));

      currentBlockDurationMs += item.durationMs;

      if (activeBlockType == BlockType::Commercial) {
        commercialCount++;
        commercialDurationMs += item.durationMs;
      } else {
        musicCount++;
        musicDurationMs += item.durationMs;
      }
    } else if (item.type == ItemType::EndMarker) {
      if (item.blockType == BlockType::Commercial) {
        item.markerText = QStringLiteral("BLOCO COMERCIAL");
        item.itemsCount = commercialCount;
        item.totalDurationMs = commercialDurationMs;
      } else {
        item.markerText.clear();
        item.itemsCount = musicCount;
        item.totalDurationMs = musicDurationMs;
      }
    }
  }

  if (currentBlockMarkerIdx >= 0) {
    m_items[currentBlockMarkerIdx].totalDurationMs = currentBlockDurationMs;
  }

  const int playingRow = m_playingRow;
  bool foundNext = false;

  for (int i = 0; i < m_items.size(); ++i) {
    PlaylistItem &item = m_items[i];
    if (item.type != ItemType::Media)
      continue;

    if (playingRow >= 0) {
      if (i < playingRow) {
        item.status = PlaylistItemStatus::Finished;
      } else if (i == playingRow) {
        item.status = PlaylistItemStatus::Playing;
      } else if (!foundNext) {
        item.status = PlaylistItemStatus::Next;
        foundNext = true;
      } else {
        item.status = PlaylistItemStatus::Default;
      }
    } else {
      if (item.status == PlaylistItemStatus::Finished) {
        continue;
      }
      if (!foundNext) {
        item.status = PlaylistItemStatus::Next;
        foundNext = true;
      } else {
        item.status = PlaylistItemStatus::Default;
      }
    }
  }

  // Coalesce changed rows into contiguous dataChanged ranges.
  int runStart = -1;
  for (int i = 0; i <= m_items.size(); ++i) {
    bool changed = false;
    if (i < m_items.size()) {
      const PlaylistItem &it = m_items.at(i);
      const Derived &b = before.at(i);
      changed = it.status != b.status || it.blockType != b.blockType ||
                it.totalDurationMs != b.totalDurationMs ||
                it.itemsCount != b.itemsCount ||
                it.estimatedTime != b.estimatedTime ||
                it.markerText != b.markerText;
    }
    if (changed) {
      if (runStart < 0)
        runStart = i;
    } else if (runStart >= 0) {
      emit dataChanged(index(runStart, 0), index(i - 1, 0));
      runStart = -1;
    }
  }
}

void PlaylistListModel::rebuildUuidMap() {
  m_uuidRowMap.clear();
  for (int i = 0; i < m_items.size(); ++i) {
    m_uuidRowMap.insert(m_items.at(i).uuid, i);
  }
}

void PlaylistListModel::enqueueDurationProbe(const QUuid &uuid) {
  const int row = rowForUuid(uuid);
  const PlaylistItem *item = itemAt(row);
  if (!item || item->type != ItemType::Media || item->durationMs > 0 ||
      item->path.isEmpty()) {
    return;
  }

  if (m_durationResolver) {
    const qint64 resolved = m_durationResolver(item->path);
    if (resolved > 0) {
      applyProbedDuration(uuid, resolved);
    }
    return;
  }

  if (!QFileInfo::exists(item->path)) {
    return;
  }

  if (!m_pendingDurationProbes.contains(uuid) && m_activeProbeUuid != uuid) {
    m_pendingDurationProbes.append(uuid);
  }
  processNextDurationProbe();
}

void PlaylistListModel::processNextDurationProbe() {
  if (!m_activeProbeUuid.isNull()) {
    return;
  }

  while (!m_pendingDurationProbes.isEmpty()) {
    const QUuid uuid = m_pendingDurationProbes.takeFirst();
    const int row = rowForUuid(uuid);
    if (row < 0) {
      continue;
    }

    const PlaylistItem *it = itemAt(row);
    if (!it || it->type != ItemType::Media || it->durationMs > 0 ||
        it->path.isEmpty() || !QFileInfo::exists(it->path)) {
      continue;
    }

    if (!m_probePlayer) {
      m_probePlayer = new QMediaPlayer(this);
      m_probeTimeoutTimer = new QTimer(this);
      m_probeTimeoutTimer->setSingleShot(true);

      connect(m_probePlayer, &QMediaPlayer::durationChanged, this,
              [this](qint64 duration) {
                if (!m_activeProbeUuid.isNull() && duration > 0) {
                  finalizeActiveDurationProbe(duration);
                }
              });

      connect(m_probePlayer, &QMediaPlayer::mediaStatusChanged, this,
              [this](QMediaPlayer::MediaStatus status) {
                if (m_activeProbeUuid.isNull()) {
                  return;
                }
                if (status == QMediaPlayer::LoadedMedia ||
                    status == QMediaPlayer::BufferedMedia) {
                  const qint64 dur = m_probePlayer->duration();
                  if (dur > 0) {
                    finalizeActiveDurationProbe(dur);
                  }
                } else if (status == QMediaPlayer::InvalidMedia) {
                  finalizeActiveDurationProbe(0);
                }
              });

      connect(m_probePlayer, &QMediaPlayer::errorOccurred, this,
              [this](QMediaPlayer::Error error, const QString &errorString) {
                Q_UNUSED(error);
                Q_UNUSED(errorString);
                if (!m_activeProbeUuid.isNull()) {
                  finalizeActiveDurationProbe(0);
                }
              });

      connect(m_probeTimeoutTimer, &QTimer::timeout, this, [this]() {
        if (!m_activeProbeUuid.isNull()) {
          finalizeActiveDurationProbe(0);
        }
      });
    }

    m_activeProbeUuid = uuid;
    m_probeTimeoutTimer->start(5000);
    m_probePlayer->setSource(QUrl::fromLocalFile(it->path));
    return;
  }

  if (m_probePlayer && !m_probePlayer->source().isEmpty()) {
    m_probePlayer->setSource(QUrl());
  }
}

void PlaylistListModel::finalizeActiveDurationProbe(qint64 durationMs) {
  if (m_activeProbeUuid.isNull()) {
    return;
  }

  const QUuid finishedUuid = m_activeProbeUuid;
  m_activeProbeUuid = QUuid();
  if (m_probeTimeoutTimer) {
    m_probeTimeoutTimer->stop();
  }

  if (durationMs > 0) {
    applyProbedDuration(finishedUuid, durationMs);
  }

  QMetaObject::invokeMethod(this, &PlaylistListModel::processNextDurationProbe,
                            Qt::QueuedConnection);
}

void PlaylistListModel::cancelAllDurationProbes() {
  m_pendingDurationProbes.clear();
  m_activeProbeUuid = QUuid();
  if (m_probeTimeoutTimer) {
    m_probeTimeoutTimer->stop();
  }
  if (m_probePlayer) {
    m_probePlayer->setSource(QUrl());
  }
}

void PlaylistListModel::applyProbedDuration(const QUuid &uuid,
                                            qint64 durationMs) {
  const int row = rowForUuid(uuid);
  const PlaylistItem *item = itemAt(row);
  if (item && item->type == ItemType::Media && item->durationMs <= 0 &&
      durationMs > 0) {
    const qint64 endPt = (item->endPoint > 0 && item->endPoint <= durationMs)
                             ? item->endPoint
                             : durationMs;
    const qint64 startPt = qBound<qint64>(0, item->startPoint, endPt);
    const qint64 mixSt = qBound<qint64>(startPt, item->mixStart, endPt);
    const qint64 mixEd = (item->mixEnd > startPt && item->mixEnd <= endPt)
                             ? item->mixEnd
                             : endPt;
    const qint64 fadeInEnd =
        qBound<qint64>(startPt, item->fadeInEndPoint, endPt);
    const qint64 fadeOutStart =
        (item->fadeOutStartPoint > 0)
            ? qBound<qint64>(startPt, item->fadeOutStartPoint, endPt)
            : endPt;

    updateItemAudioMetadata(row, durationMs, startPt, endPt, mixSt, mixEd,
                            fadeInEnd, fadeOutStart);
    emit itemDurationLoaded(row, uuid, durationMs);
  }
}

void PlaylistListModel::loadMockData() {
  cancelAllDurationProbes();
  m_items.clear();

  auto createItem = [](ItemType type, BlockType bType, MediaType mType,
                       const QString &title, const QString &blockTitle,
                       const QString &path, qint64 durationMs) {
    PlaylistItem item;
    item.uuid = QUuid::createUuid();
    item.type = type;
    item.blockType = bType;
    item.mediaType = mType;
    item.title = title;
    item.blockTitle = blockTitle;
    item.path = path;
    item.durationMs = durationMs;
    item.status = PlaylistItemStatus::Default;

    if (durationMs > 0) {
      item.startPoint = 0;
      item.endPoint = durationMs;
      item.mixStart = 0;
      item.mixEnd = (durationMs > 2000) ? (durationMs - 1000) : durationMs;
      item.fadeInEndPoint =
          (mType == MediaType::Music) ? qMin<qint64>(1000, durationMs) : 0;
      item.fadeOutStartPoint =
          (durationMs > 2000) ? (durationMs - 1500) : durationMs;
    }

    return item;
  };

  // Bloco 14:00
  m_items.append(createItem(ItemType::BlockMarker, BlockType::Commercial,
                            MediaType::Other, QStringLiteral("14:00"),
                            QStringLiteral("Flow Music"), QString(), 0));
  m_items.append(createItem(ItemType::StartMarker, BlockType::Commercial,
                            MediaType::Other, QString(), QString(), QString(),
                            0));
  m_items.append(createItem(
      ItemType::Media, BlockType::Commercial, MediaType::Jingle,
      QStringLiteral("A RADIO QUE VOCE AJUDOU A CONSTRUIR.mp3"), QString(),
      QStringLiteral("D:/MEDIA/VINHETAS/Jacobina FM/Passagem/"
                     "A RADIO QUE VOCE AJUDOU A CONSTRUIR.mp3"),
      13000));
  m_items.append(createItem(
      ItemType::Media, BlockType::Commercial, MediaType::Commercial,
      QStringLiteral("Armazem Paraiba - Aniversario.mp3"), QString(),
      QStringLiteral("D:/MEDIA/COMERCIAIS/Armazem Paraiba - Aniversario.mp3"),
      29000));
  m_items.append(
      createItem(ItemType::Media, BlockType::Commercial, MediaType::Commercial,
                 QStringLiteral("Italac.mp3"), QString(),
                 QStringLiteral("D:/MEDIA/COMERCIAIS/Italac.mp3"), 15000));
  m_items.append(createItem(ItemType::EndMarker, BlockType::Commercial,
                            MediaType::Other, QString(), QString(), QString(),
                            0));

  m_items.append(createItem(ItemType::StartMarker, BlockType::Music,
                            MediaType::Other, QString(), QString(), QString(),
                            0));
  m_items.append(createItem(
      ItemType::Media, BlockType::Music, MediaType::Music,
      QStringLiteral(
          "Calvin Harris & Dua Lipa - One Kiss (Official Video).mp3"),
      QString(),
      QStringLiteral(
          "D:/MEDIA/MÚSICAS/Dance/"
          "Calvin Harris & Dua Lipa - One Kiss (Official Video).mp3"),
      223000));
  m_items.append(createItem(
      ItemType::Media, BlockType::Music, MediaType::Jingle,
      QStringLiteral("00 PONTE-179.mp3"), QString(),
      QStringLiteral("D:/MEDIA/VINHETAS/Jacobina FM/Ponte/00 PONTE-179.mp3"),
      6000));
  m_items.append(createItem(
      ItemType::Media, BlockType::Music, MediaType::Music,
      QStringLiteral("Dua Lipa & Elton John - Cold Heart (PNAU Remix).mp3"),
      QString(),
      QStringLiteral("D:/MEDIA/MÚSICAS/Dance/"
                     "Dua Lipa & Elton John - Cold Heart (PNAU Remix).mp3"),
      254000));
  m_items.append(
      createItem(ItemType::Media, BlockType::Music, MediaType::Jingle,
                 QStringLiteral("Aqui a musica não para.mp3"), QString(),
                 QStringLiteral("D:/MEDIA/VINHETAS/Jacobina FM/Passagem/"
                                "Aqui a musica não para.mp3"),
                 5000));
  m_items.append(createItem(
      ItemType::Media, BlockType::Music, MediaType::Music,
      QStringLiteral("David Guetta & Bebe Rexha - I'm Good (Blue).mp3"),
      QString(),
      QStringLiteral("D:/MEDIA/MÚSICAS/Dance/"
                     "David Guetta & Bebe Rexha - I'm Good (Blue).mp3"),
      177000));
  m_items.append(createItem(ItemType::EndMarker, BlockType::Music,
                            MediaType::Other, QString(), QString(), QString(),
                            0));

  // Bloco 14:30
  m_items.append(createItem(ItemType::BlockMarker, BlockType::Commercial,
                            MediaType::Other, QStringLiteral("14:30"),
                            QStringLiteral("Flow Music"), QString(), 0));
  m_items.append(createItem(ItemType::StartMarker, BlockType::Commercial,
                            MediaType::Other, QString(), QString(), QString(),
                            0));
  m_items.append(createItem(
      ItemType::Media, BlockType::Commercial, MediaType::Commercial,
      QStringLiteral("POSTO AVENIDA.mp3"), QString(),
      QStringLiteral("D:/MEDIA/COMERCIAIS/POSTO AVENIDA.mp3"), 34000));
  m_items.append(createItem(
      ItemType::Media, BlockType::Commercial, MediaType::Commercial,
      QStringLiteral("Newnet - 20-12-2010.mp3"), QString(),
      QStringLiteral("D:/MEDIA/COMERCIAIS/Newnet - 20-12-2010.mp3"), 34000));
  m_items.append(createItem(ItemType::EndMarker, BlockType::Commercial,
                            MediaType::Other, QString(), QString(), QString(),
                            0));

  m_items.append(createItem(ItemType::StartMarker, BlockType::Music,
                            MediaType::Other, QString(), QString(), QString(),
                            0));
  m_items.append(createItem(
      ItemType::Media, BlockType::Music, MediaType::Jingle,
      QStringLiteral("VHT Antigo ID 1.mp3"), QString(),
      QStringLiteral("D:/MEDIA/VINHETAS/MIX/VHT Antigo ID 1.mp3"), 10000));
  m_items.append(createItem(
      ItemType::Media, BlockType::Music, MediaType::Music,
      QStringLiteral("Bruno Mars, ROSÉ - APT.mp3"), QString(),
      QStringLiteral("D:/MEDIA/MÚSICAS/Dance/Bruno Mars, ROSÉ - APT.mp3"),
      169000));
  m_items.append(createItem(
      ItemType::Media, BlockType::Music, MediaType::Music,
      QStringLiteral(
          "Ariana Grande & Lady Gaga - Rain On Me (Official Music Video).mp3"),
      QString(),
      QStringLiteral(
          "D:/MEDIA/MÚSICAS/Dance/"
          "Ariana Grande & Lady Gaga - Rain On Me (Official Music Video).mp3"),
      188000));
  m_items.append(createItem(ItemType::EndMarker, BlockType::Music,
                            MediaType::Other, QString(), QString(), QString(),
                            0));

  rebuildUuidMap();
  recalculateBlockMetadata();
}