#include "PlaylistManager.h"

#include <QDebug>
#include <QUrl>

#include <cmath>

namespace {
class ScopedFlag {
public:
  explicit ScopedFlag(bool &flag) : m_flag(flag), m_prev(flag) { m_flag = true; }
  ~ScopedFlag() { m_flag = m_prev; }

private:
  bool &m_flag;
  bool m_prev;
};

bool metadataDiffers(const PlayerMetadata &a, const PlayerMetadata &b) {
  return a.mixStart != b.mixStart || a.mixEnd != b.mixEnd ||
         a.startPoint != b.startPoint || a.endPoint != b.endPoint ||
         a.fadeInEndPoint != b.fadeInEndPoint ||
         a.fadeOutStartPoint != b.fadeOutStartPoint ||
         std::abs(a.startVolume - b.startVolume) > 1e-4f ||
         std::abs(a.endVolume - b.endVolume) > 1e-4f ||
         a.volumeUpStartPoint != b.volumeUpStartPoint ||
         a.volumeUpEndPoint != b.volumeUpEndPoint ||
         a.volumeDownStartPoint != b.volumeDownStartPoint ||
         a.volumeDownEndPoint != b.volumeDownEndPoint ||
         a.duration != b.duration;
}
} // namespace

PlaylistManager::PlaylistManager(QObject *parent)
    : QObject{parent}, m_model(new PlaylistListModel(this)),
      m_timer(new QTimer(this)) {
  initConnections();
}

PlaylistManager::PlaylistManager(
    std::array<PlayerManager *, kPlayerCount> players, QObject *parent)
    : QObject{parent}, m_model(new PlaylistListModel(this)),
      m_timer(new QTimer(this)) {
  for (int i = 0; i < kPlayerCount; ++i) {
    if (players[i] && !players[i]->parent()) {
      players[i]->setParent(this);
    }
    m_slots[i].player = players[i];
  }
  initConnections();
}

PlaylistManager::~PlaylistManager() { stopImmediate(); }

void PlaylistManager::initConnections() {
  for (int i = 0; i < kPlayerCount; ++i) {
    if (!m_slots[i].player) {
      m_slots[i].player = new PlayerManager(this);
    }

    PlayerManager *player = m_slots[i].player;
    connect(player, &PlayerManager::positionChanged, this,
            [this, i](qint64 pos) { onPlayerPositionChanged(i, pos); });
    connect(player, &PlayerManager::statusChanged, this,
            [this, i](PlayerStatus status) {
              onPlayerStatusChanged(i, status);
            });
    connect(player, &PlayerManager::durationLoaded, this,
            [this, i](qint64 dur) { onPlayerDurationLoaded(i, dur); });
  }

  m_timer->setTimerType(Qt::PreciseTimer);
  connect(m_timer, &QTimer::timeout, this, &PlaylistManager::onMonitorTimeout);

  connect(m_model, &QAbstractItemModel::rowsInserted, this,
          &PlaylistManager::onModelStructureChanged);
  connect(m_model, &QAbstractItemModel::rowsRemoved, this,
          &PlaylistManager::onModelStructureChanged);
  connect(m_model, &QAbstractItemModel::rowsMoved, this,
          &PlaylistManager::onModelStructureChanged);
  connect(m_model, &QAbstractItemModel::modelReset, this,
          &PlaylistManager::onModelStructureChanged);
  connect(m_model, &QAbstractItemModel::dataChanged, this,
          &PlaylistManager::onModelDataChanged);
}

PlayerMetadata PlaylistManager::metadataFromItem(const PlaylistItem &item) {
  PlayerMetadata meta;
  meta.duration = qMax<qint64>(0, item.durationMs);
  meta.startPoint = qMax<qint64>(0, item.startPoint);
  meta.endPoint = (item.endPoint > 0) ? item.endPoint : meta.duration;
  meta.mixStart = item.mixStart;
  meta.mixEnd = (item.mixEnd > meta.startPoint) ? item.mixEnd : meta.endPoint;
  meta.fadeInEndPoint = item.fadeInEndPoint;
  meta.fadeOutStartPoint =
      (item.fadeOutStartPoint > 0) ? item.fadeOutStartPoint : meta.endPoint;
  meta.startVolume = item.startVolume;
  meta.endVolume = item.endVolume;
  meta.volumeUpStartPoint = item.volumeUpStartPoint;
  meta.volumeUpEndPoint = item.volumeUpEndPoint;
  meta.volumeDownStartPoint = item.volumeDownStartPoint;
  meta.volumeDownEndPoint = item.volumeDownEndPoint;
  return meta;
}

qint64
PlaylistManager::computeMixTriggerPosition(const PlayerMetadata &currentMeta,
                                           const PlayerMetadata &nextMeta) {
  const qint64 currStart = qMax<qint64>(0, currentMeta.startPoint);
  const qint64 currEnd =
      (currentMeta.endPoint > 0) ? currentMeta.endPoint : currentMeta.duration;
  const qint64 currMixEnd =
      (currentMeta.mixEnd > currStart &&
       (currEnd <= 0 || currentMeta.mixEnd <= currEnd))
          ? currentMeta.mixEnd
          : currEnd;

  const qint64 nextStart = qMax<qint64>(0, nextMeta.startPoint);
  const qint64 nextEnd =
      (nextMeta.endPoint > 0) ? nextMeta.endPoint : nextMeta.duration;
  const qint64 nextMixStart =
      (nextMeta.mixStart > nextStart &&
       (nextEnd <= 0 || nextMeta.mixStart <= nextEnd))
          ? nextMeta.mixStart
          : nextStart;

  const qint64 nextIntro = qMax<qint64>(0, nextMixStart - nextStart);
  const qint64 rawTrigger = currMixEnd - nextIntro;

  if (currEnd > 0) {
    return qBound<qint64>(currStart, rawTrigger, currEnd);
  }
  return qMax<qint64>(currStart, rawTrigger);
}

bool PlaylistManager::shouldPreloadExtended(const PlayerMetadata &currentMeta,
                                            const PlayerMetadata &nextMeta,
                                            const PlayerMetadata &afterNextMeta,
                                            qint64 minPreloadWindowMs) {
  if (nextMeta.endPoint <= 0 && nextMeta.duration <= 0) {
    return false;
  }

  const qint64 triggerCurrToNext =
      computeMixTriggerPosition(currentMeta, nextMeta);
  const qint64 triggerNextToAfter =
      computeMixTriggerPosition(nextMeta, afterNextMeta);

  const qint64 currEnd =
      (currentMeta.endPoint > 0) ? currentMeta.endPoint : currentMeta.duration;
  const qint64 nextStart = qMax<qint64>(0, nextMeta.startPoint);

  const qint64 windowInNextMs = qMax<qint64>(0, triggerNextToAfter - nextStart);
  const qint64 tailOfCurrentMs =
      (currEnd > triggerCurrToNext) ? (currEnd - triggerCurrToNext) : 0;

  const bool tripleOverlap =
      (tailOfCurrentMs > 0) && (windowInNextMs < tailOfCurrentMs);
  const bool shortWindow = (windowInNextMs <= minPreloadWindowMs);
  return tripleOverlap || shortWindow;
}

PlayerManager *PlaylistManager::playerAt(int index) const {
  if (index < 0 || index >= kPlayerCount) {
    return nullptr;
  }
  return m_slots[index].player;
}

PlaylistManager::SlotRole PlaylistManager::slotRole(int index) const {
  if (index < 0 || index >= kPlayerCount) {
    return SlotRole::Free;
  }
  return m_slots[index].role;
}

QUuid PlaylistManager::slotItemUuid(int index) const {
  if (index < 0 || index >= kPlayerCount) {
    return QUuid();
  }
  return m_slots[index].itemUuid;
}

void PlaylistManager::setMinPreloadWindowMs(qint64 ms) {
  m_minPreloadWindowMs = qMax<qint64>(0, ms);
  if (m_status != PlaylistStatus::Stopped && !m_inAdvance) {
    syncPreloads();
  }
}

void PlaylistManager::setManualNextFadeMs(qint64 ms) {
  m_manualNextFadeMs = qMax<qint64>(0, ms);
}

void PlaylistManager::setStopFadeMs(qint64 ms) {
  m_stopFadeMs = qMax<qint64>(0, ms);
}

void PlaylistManager::setMediaPathResolver(MediaPathResolverFn resolver) {
  m_mediaPathResolver = std::move(resolver);
  if (m_model) {
    m_model->setMediaPathResolver(m_mediaPathResolver);
  }
}

QString PlaylistManager::resolveItemPath(const QString &path) const {
  if (m_mediaPathResolver && !path.isEmpty()) {
    const QString resolved = m_mediaPathResolver(path);
    if (!resolved.isEmpty()) {
      return resolved;
    }
  }
  return path;
}

void PlaylistManager::play() {
  if (m_status == PlaylistStatus::Playing) {
    return;
  }

  if (m_status == PlaylistStatus::Paused) {
    for (int i = 0; i < kPlayerCount; ++i) {
      PlayerSlot &slot = m_slots[i];
      if ((slot.role == SlotRole::Active || slot.role == SlotRole::Tail) &&
          slot.player) {
        slot.player->play();
      }
    }
    m_status = PlaylistStatus::Playing;
    m_timer->start(kMonitorIntervalMs);
    emit statusChanged(m_status);

    syncPreloads();
    if (m_activeSlot >= 0 && m_slots[m_activeSlot].player &&
        m_slots[m_activeSlot].player->status() == PlayerStatus::Playing) {
      checkMixTrigger(m_slots[m_activeSlot].player->position());
    }
    return;
  }

  const int playingRow = m_model->currentPlayingRow();
  const int startRow =
      (playingRow >= 0) ? playingRow : m_model->findNextPlayableRow();
  if (startRow < 0) {
    return;
  }

  startPlaybackFromRow(startRow, false);
}

void PlaylistManager::pause() {
  if (m_status != PlaylistStatus::Playing) {
    return;
  }

  m_timer->stop();
  for (int i = 0; i < kPlayerCount; ++i) {
    PlayerSlot &slot = m_slots[i];
    if (!slot.player) {
      continue;
    }
    const PlayerStatus st = slot.player->status();
    if (st == PlayerStatus::Playing ||
        (slot.role == SlotRole::Active && st == PlayerStatus::Loading)) {
      slot.player->pause();
    }
  }

  m_status = PlaylistStatus::Paused;
  emit statusChanged(m_status);
}

void PlaylistManager::next() {
  if (m_status == PlaylistStatus::Playing) {
    clearPendingOverlay(true);
    const int currRow = activeRow();
    const int nextRow = (currRow >= 0) ? m_model->findNextEntryRow(currRow)
                                       : m_model->findNextPlayableRow();
    if (nextRow >= 0) {
      startPlaybackFromRow(nextRow, true);
    } else {
      finishPlaylist();
    }
    return;
  }

  if (m_status == PlaylistStatus::Paused) {
    clearPendingOverlay(true);
    const int currRow = activeRow();
    const int nextRow = (currRow >= 0) ? m_model->findNextEntryRow(currRow)
                                       : m_model->findNextPlayableRow();
    for (int i = 0; i < kPlayerCount; ++i) {
      if (m_slots[i].role == SlotRole::Tail) {
        freeSlot(i);
      }
    }
    if (nextRow >= 0) {
      startPlaybackFromRow(nextRow, false);
    } else {
      finishPlaylist();
    }
    return;
  }

  const int playingRow = m_model->currentPlayingRow();
  int nextRow = -1;
  if (playingRow >= 0) {
    nextRow = m_model->findNextEntryRow(playingRow);
  } else {
    const int cued = m_model->findNextPlayableRow();
    nextRow = (cued >= 0) ? m_model->findNextEntryRow(cued) : -1;
  }
  if (nextRow >= 0) {
    startPlaybackFromRow(nextRow, false);
  }
}

void PlaylistManager::stop() {
  m_timer->stop();
  clearPendingOverlay(false);
  const PlaylistStatus previousStatus = m_status;
  m_activeSlot = -1;
  m_transitionTriggeredForActive = false;
  m_currentBlockUuid = QUuid();
  resetMixTriggerCache();
  m_status = PlaylistStatus::Stopped;

  for (int i = 0; i < kPlayerCount; ++i) {
    PlayerSlot &slot = m_slots[i];
    if (previousStatus == PlaylistStatus::Playing && m_stopFadeMs > 0 &&
        slot.player && slot.player->status() == PlayerStatus::Playing) {
      if (!slot.itemUuid.isNull()) {
        const int row = m_model->rowForUuid(slot.itemUuid);
        if (row >= 0) {
          const PlaylistItem *it = m_model->itemAt(row);
          if (it && it->playerId == i) {
            ScopedFlag guard(m_updatingModelMetadata);
            m_model->setItemPlayerId(row, -1);
          }
        }
      }
      slot.role = SlotRole::Tail;
      slot.itemUuid = QUuid();
      slot.lastUiPositionMs = -1;
      slot.player->triggerFadeOut(m_stopFadeMs);
      if (slot.role != SlotRole::Free &&
          slot.player->status() != PlayerStatus::Playing) {
        freeSlot(i);
      }
    } else {
      freeSlot(i);
    }
  }

  {
    ScopedFlag guard(m_updatingModelMetadata);
    m_model->stopPlayingRow(true);
  }

  if (previousStatus != PlaylistStatus::Stopped) {
    emit statusChanged(m_status);
  }
}

void PlaylistManager::stopImmediate() {
  m_timer->stop();
  clearPendingOverlay(false);
  unloadAllSlots();
  m_activeSlot = -1;
  m_transitionTriggeredForActive = false;
  m_currentBlockUuid = QUuid();
  resetMixTriggerCache();

  if (m_status != PlaylistStatus::Stopped) {
    m_status = PlaylistStatus::Stopped;
    emit statusChanged(m_status);
  }

  ScopedFlag guard(m_updatingModelMetadata);
  m_model->stopPlayingRow(true);
}

int PlaylistManager::activeRow() const {
  if (m_activeSlot < 0 || m_activeSlot >= kPlayerCount) {
    return -1;
  }
  const QUuid &uuid = m_slots[m_activeSlot].itemUuid;
  if (!uuid.isNull()) {
    return m_model->rowForUuid(uuid);
  }
  return m_model->currentPlayingRow();
}

int PlaylistManager::findSlotForItem(const QUuid &uuid,
                                     const QString &path) const {
  if (uuid.isNull()) {
    return -1;
  }
  for (int i = 0; i < kPlayerCount; ++i) {
    if (m_slots[i].itemUuid == uuid && m_slots[i].loadedPath == path &&
        m_slots[i].role != SlotRole::Free) {
      return i;
    }
  }
  return -1;
}

void PlaylistManager::freeSlot(int slotIndex) {
  if (slotIndex < 0 || slotIndex >= kPlayerCount) {
    return;
  }

  PlayerSlot &slot = m_slots[slotIndex];
  const bool wasTail = (slot.role == SlotRole::Tail);
  const QUuid prevUuid = slot.itemUuid;

  if (!prevUuid.isNull() && prevUuid == m_pendingOverlay.skippedItemUuid) {
    m_pendingOverlay.clear();
    if (m_model) {
      ScopedFlag guard(m_updatingModelMetadata);
      m_model->setPendingOverlayUuid(QUuid());
    }
  }

  slot.role = SlotRole::Free;
  slot.itemUuid = QUuid();
  slot.loadedPath.clear();
  slot.lastUiPositionMs = -1;
  if (slot.player && slot.player->status() != PlayerStatus::Idle) {
    slot.player->unload();
  }

  if (!prevUuid.isNull()) {
    const int row = m_model->rowForUuid(prevUuid);
    if (row >= 0) {
      const PlaylistItem *it = m_model->itemAt(row);
      ScopedFlag guard(m_updatingModelMetadata);
      if (it && it->playerId == slotIndex) {
        m_model->setItemPlayerId(row, -1);
      }
      if (wasTail && it && it->status == PlaylistItemStatus::Mixing) {
        m_model->markRowFinished(row);
      }
    }
  }
}

void PlaylistManager::unloadAllSlots() {
  for (int i = 0; i < kPlayerCount; ++i) {
    freeSlot(i);
  }
}

void PlaylistManager::finishPlaylist() {
  m_timer->stop();
  clearPendingOverlay(true);
  unloadAllSlots();
  m_activeSlot = -1;
  m_transitionTriggeredForActive = false;
  resetMixTriggerCache();

  {
    ScopedFlag guard(m_updatingModelMetadata);
    m_model->stopPlayingRow(true);
  }

  if (!m_currentBlockUuid.isNull()) {
    const int oldBlockRow = m_model->rowForUuid(m_currentBlockUuid);
    m_currentBlockUuid = QUuid();
    emit blockFinished(oldBlockRow);
  }

  if (m_status != PlaylistStatus::Stopped) {
    m_status = PlaylistStatus::Stopped;
    emit statusChanged(m_status);
  }
  emit playlistFinished();
}

void PlaylistManager::emitBlockTransitionIfNeeded(int mediaRow) {
  const int newBlockRow = m_model->blockMarkerRowForRow(mediaRow);
  QUuid newBlockUuid;
  QString blockTime;
  QString blockTitle;

  if (newBlockRow >= 0) {
    const PlaylistItem *bm = m_model->itemAt(newBlockRow);
    if (bm) {
      newBlockUuid = bm->uuid;
      blockTime = bm->title;
      blockTitle = bm->blockTitle;
    }
  }

  if (newBlockUuid != m_currentBlockUuid) {
    if (!m_currentBlockUuid.isNull()) {
      const int oldBlockRow = m_model->rowForUuid(m_currentBlockUuid);
      emit blockFinished(oldBlockRow);
    }
    m_currentBlockUuid = newBlockUuid;
    if (newBlockRow >= 0) {
      emit blockChanged(newBlockRow, blockTime, blockTitle);
    }
  }
}

void PlaylistManager::startPlaybackFromRow(int startRow, bool fadeOutOutgoing) {
  ScopedFlag advanceGuard(m_inAdvance);

  const int previousActiveSlot = m_activeSlot;
  if (previousActiveSlot >= 0 && previousActiveSlot < kPlayerCount) {
    PlayerSlot &outgoing = m_slots[previousActiveSlot];
    if (outgoing.player) {
      if (fadeOutOutgoing && outgoing.player->status() == PlayerStatus::Playing) {
        outgoing.player->triggerFadeOut(m_manualNextFadeMs);
      }
      if (outgoing.player->status() == PlayerStatus::Playing) {
        outgoing.role = SlotRole::Tail;
      } else {
        freeSlot(previousActiveSlot);
      }
    } else {
      freeSlot(previousActiveSlot);
    }
  }

  auto outgoingMixingRowForMedia = [&]() -> int {
    if (previousActiveSlot >= 0 && previousActiveSlot < kPlayerCount &&
        m_slots[previousActiveSlot].role == SlotRole::Tail &&
        !m_slots[previousActiveSlot].itemUuid.isNull()) {
      return m_model->rowForUuid(m_slots[previousActiveSlot].itemUuid);
    }
    return -1;
  };

  auto selectTargetSlotForMedia = [&](const PlaylistItem *mediaItem) -> int {
    int slotIdx = -1;
    if (mediaItem && mediaItem->type == ItemType::Media) {
      const QString resolvedPath = resolveItemPath(mediaItem->path);
      const int preloadedSlot =
          findSlotForItem(mediaItem->uuid, resolvedPath);
      if (preloadedSlot >= 0 && m_slots[preloadedSlot].player &&
          (m_slots[preloadedSlot].role == SlotRole::PreloadNext ||
           m_slots[preloadedSlot].role == SlotRole::PreloadExt)) {
        const PlayerStatus st = m_slots[preloadedSlot].player->status();
        if (st == PlayerStatus::Loaded || st == PlayerStatus::Loading) {
          slotIdx = preloadedSlot;
        }
      }
    }
    if (slotIdx < 0 && previousActiveSlot >= 0) {
      const int candidate = (previousActiveSlot + 1) % kPlayerCount;
      if (m_slots[candidate].role != SlotRole::PendingOverlay &&
          m_slots[candidate].role != SlotRole::Tail) {
        slotIdx = candidate;
      }
    }
    if (slotIdx < 0) {
      for (int i = 0; i < kPlayerCount; ++i) {
        if (m_slots[i].role == SlotRole::Free) {
          slotIdx = i;
          break;
        }
      }
    }
    if (slotIdx < 0) {
      slotIdx = (previousActiveSlot >= 0)
                    ? (previousActiveSlot + 1) % kPlayerCount
                    : 0;
    }
    return slotIdx;
  };

  int candidateRow = startRow;
  bool started = false;

  while (candidateRow >= 0) {
    const PlaylistItem *item = m_model->itemAt(candidateRow);
    if (!item || !item->isEntry()) {
      candidateRow = -1;
      break;
    }

    if (item->type == ItemType::Command) {
      if (item->commandType == CommandType::Pause) {
        m_timer->stop();
        clearPendingOverlay(false);
        const PlaylistStatus previousStatus = m_status;
        m_activeSlot = -1;
        m_transitionTriggeredForActive = false;
        m_currentBlockUuid = QUuid();
        resetMixTriggerCache();
        m_status = PlaylistStatus::Stopped;

        for (int i = 0; i < kPlayerCount; ++i) {
          PlayerSlot &slot = m_slots[i];
          if (slot.role == SlotRole::Tail && slot.player &&
              slot.player->status() == PlayerStatus::Playing) {
            if (!slot.itemUuid.isNull()) {
              const int row = m_model->rowForUuid(slot.itemUuid);
              if (row >= 0) {
                const PlaylistItem *it = m_model->itemAt(row);
                if (it && it->playerId == i) {
                  ScopedFlag guard(m_updatingModelMetadata);
                  m_model->setItemPlayerId(row, -1);
                }
              }
            }
            slot.itemUuid = QUuid();
            slot.lastUiPositionMs = -1;
          } else {
            freeSlot(i);
          }
        }

        {
          ScopedFlag modelGuard(m_updatingModelMetadata);
          m_model->finishThroughRow(candidateRow);
        }

        emit commandExecuted(candidateRow, item->uuid, item->commandType);
        if (previousStatus != PlaylistStatus::Stopped) {
          emit statusChanged(m_status);
        }
        return;
      }

      qWarning() << "Skipping unsupported command at row" << candidateRow << ":"
                 << item->commandId;
      {
        ScopedFlag modelGuard(m_updatingModelMetadata);
        m_model->markRowFinished(candidateRow);
      }
      candidateRow = m_model->findNextEntryRow(candidateRow);
      continue;
    }

    if (m_status != PlaylistStatus::Playing) {
      m_status = PlaylistStatus::Playing;
      emit statusChanged(m_status);
    }

    const QString resolvedPath = resolveItemPath(item->path);
    const int targetSlot = selectTargetSlotForMedia(item);
    PlayerSlot &slot = m_slots[targetSlot];
    const bool alreadyPreloaded =
        (slot.itemUuid == item->uuid && slot.loadedPath == resolvedPath &&
         slot.role != SlotRole::Free && slot.player &&
         (slot.player->status() == PlayerStatus::Loaded ||
          slot.player->status() == PlayerStatus::Loading));

    if (!alreadyPreloaded && slot.role != SlotRole::Free) {
      freeSlot(targetSlot);
    }

    slot.role = SlotRole::Active;
    slot.itemUuid = item->uuid;
    slot.loadedPath = resolvedPath;
    slot.lastUiPositionMs = -1;
    m_activeSlot = targetSlot;
    m_transitionTriggeredForActive = false;

    int outgoingMixingRow = outgoingMixingRowForMedia();
    {
      ScopedFlag modelGuard(m_updatingModelMetadata);
      m_model->setPlayingRow(candidateRow, outgoingMixingRow);
      m_model->setItemPlayerId(candidateRow, targetSlot);
    }

    emitBlockTransitionIfNeeded(candidateRow);
    emit currentItemChanged(candidateRow, item->uuid);

    if (alreadyPreloaded) {
      const PlayerMetadata desiredMeta = metadataFromItem(*item);
      if (slot.player->status() == PlayerStatus::Loaded &&
          metadataDiffers(slot.player->currentMetadata(), desiredMeta)) {
        slot.player->setMetadata(desiredMeta);
      }
      slot.player->play();
    } else {
      slot.player->load(QUrl::fromLocalFile(resolvedPath),
                        metadataFromItem(*item), true);
    }

    if (slot.player->status() == PlayerStatus::Error) {
      qWarning() << "Playback failed for item at row" << candidateRow << ":"
                 << resolvedPath;
      emit playbackError(candidateRow, resolvedPath);
      {
        ScopedFlag modelGuard(m_updatingModelMetadata);
        m_model->markRowFinished(candidateRow);
      }
      freeSlot(targetSlot);
      m_activeSlot = -1;
      candidateRow = m_model->findNextEntryRow(candidateRow);
      continue;
    }

    started = true;
    break;
  }

  if (!started) {
    finishPlaylist();
    return;
  }

  m_timer->start(kMonitorIntervalMs);
  syncPreloads();

  PlayerManager *activePlayer = m_slots[m_activeSlot].player;
  if (activePlayer && activePlayer->status() == PlayerStatus::Playing) {
    const qint64 pos = activePlayer->position();
    m_model->setItemPosition(candidateRow, pos);
    m_slots[m_activeSlot].lastUiPositionMs = pos;
    checkMixTrigger(pos);
  }
}

void PlaylistManager::preloadSlot(int slotIndex, int targetRow, SlotRole role) {
  if (slotIndex < 0 || slotIndex >= kPlayerCount) {
    return;
  }

  PlayerSlot &slot = m_slots[slotIndex];
  if (slot.role == SlotRole::Tail || slot.role == SlotRole::Active ||
      slot.role == SlotRole::PendingOverlay) {
    return;
  }

  if (targetRow < 0) {
    if (slot.role != SlotRole::Free) {
      freeSlot(slotIndex);
    }
    return;
  }

  const PlaylistItem *item = m_model->itemAt(targetRow);
  if (!item || item->type != ItemType::Media) {
    if (slot.role != SlotRole::Free) {
      freeSlot(slotIndex);
    }
    return;
  }

  const QString resolvedPath = resolveItemPath(item->path);
  if (slot.role != SlotRole::Free && slot.itemUuid == item->uuid &&
      slot.loadedPath == resolvedPath) {
    slot.role = role;
    {
      ScopedFlag guard(m_updatingModelMetadata);
      m_model->setItemPlayerId(targetRow, slotIndex);
    }
    if (slot.player && slot.player->status() == PlayerStatus::Loaded) {
      const PlayerMetadata desiredMeta = metadataFromItem(*item);
      if (metadataDiffers(slot.player->currentMetadata(), desiredMeta)) {
        slot.player->setMetadata(desiredMeta);
      }
    }
    return;
  }

  if (slot.role != SlotRole::Free) {
    freeSlot(slotIndex);
  }

  slot.role = role;
  slot.itemUuid = item->uuid;
  slot.loadedPath = resolvedPath;
  {
    ScopedFlag guard(m_updatingModelMetadata);
    m_model->setItemPlayerId(targetRow, slotIndex);
  }

  if (slot.player) {
    slot.player->load(QUrl::fromLocalFile(resolvedPath),
                      metadataFromItem(*item), false);
  }
}

void PlaylistManager::syncPreloads() {
  if (m_status == PlaylistStatus::Stopped || m_activeSlot < 0) {
    resetMixTriggerCache();
    return;
  }

  const int currRow = activeRow();
  if (currRow < 0) {
    resetMixTriggerCache();
    return;
  }

  const auto isPreloadable = [](SlotRole r) {
    return r == SlotRole::Free || r == SlotRole::PreloadNext ||
           r == SlotRole::PreloadExt;
  };

  const int nextEntryRow = m_model->findNextEntryRow(currRow);
  const int nextSlotIdx = (m_activeSlot + 1) % kPlayerCount;
  const int extSlotIdx = (m_activeSlot + 2) % kPlayerCount;

  const int actualNextSlot =
      isPreloadable(m_slots[nextSlotIdx].role)
          ? nextSlotIdx
          : (isPreloadable(m_slots[extSlotIdx].role) ? extSlotIdx : nextSlotIdx);

  const PlaylistItem *nextEntryItem =
      (nextEntryRow >= 0) ? m_model->itemAt(nextEntryRow) : nullptr;
  if (nextEntryItem && nextEntryItem->type == ItemType::Command) {
    preloadSlot(actualNextSlot, -1, SlotRole::PreloadNext);
    if (actualNextSlot != extSlotIdx) {
      preloadSlot(extSlotIdx, -1, SlotRole::PreloadExt);
    }
    refreshMixTriggerCache(currRow, nextEntryRow);
    return;
  }

  const int nextRow = nextEntryRow;
  const int afterNextEntryRow =
      (nextRow >= 0) ? m_model->findNextEntryRow(nextRow) : -1;
  const PlaylistItem *afterEntryItem =
      (afterNextEntryRow >= 0) ? m_model->itemAt(afterNextEntryRow) : nullptr;
  const int afterNextRow =
      (afterEntryItem && afterEntryItem->type == ItemType::Media)
          ? afterNextEntryRow
          : -1;

  preloadSlot(actualNextSlot, nextRow, SlotRole::PreloadNext);

  if (nextRow < 0 || afterNextRow < 0) {
    if (actualNextSlot != extSlotIdx) {
      preloadSlot(extSlotIdx, -1, SlotRole::PreloadExt);
    }
    refreshMixTriggerCache(currRow, nextRow);
    return;
  }

  const PlaylistItem *currItem = m_model->itemAt(currRow);
  const PlaylistItem *nextItem = m_model->itemAt(nextRow);
  const PlaylistItem *afterNextItem = m_model->itemAt(afterNextRow);
  if (!currItem || !nextItem || !afterNextItem) {
    if (actualNextSlot != extSlotIdx) {
      preloadSlot(extSlotIdx, -1, SlotRole::PreloadExt);
    }
    refreshMixTriggerCache(currRow, nextRow);
    return;
  }

  PlayerMetadata currMeta = metadataFromItem(*currItem);
  if (m_slots[m_activeSlot].player &&
      (m_slots[m_activeSlot].player->status() == PlayerStatus::Playing ||
       m_slots[m_activeSlot].player->status() == PlayerStatus::Paused ||
       m_slots[m_activeSlot].player->status() == PlayerStatus::Loaded)) {
    currMeta = m_slots[m_activeSlot].player->currentMetadata();
  }

  PlayerMetadata nextMeta = metadataFromItem(*nextItem);
  if (m_slots[actualNextSlot].role == SlotRole::PreloadNext &&
      m_slots[actualNextSlot].itemUuid == nextItem->uuid &&
      m_slots[actualNextSlot].player &&
      m_slots[actualNextSlot].player->status() == PlayerStatus::Loaded) {
    nextMeta = m_slots[actualNextSlot].player->currentMetadata();
  }

  const PlayerMetadata afterNextMeta = metadataFromItem(*afterNextItem);
  if (actualNextSlot != extSlotIdx) {
    if (shouldPreloadExtended(currMeta, nextMeta, afterNextMeta,
                              m_minPreloadWindowMs)) {
      preloadSlot(extSlotIdx, afterNextRow, SlotRole::PreloadExt);
    } else {
      preloadSlot(extSlotIdx, -1, SlotRole::PreloadExt);
    }
  }

  refreshMixTriggerCache(currRow, nextRow);
}

void PlaylistManager::resetMixTriggerCache() {
  m_cachedActiveRow = -1;
  m_cachedNextRow = -1;
  m_cachedTriggerPos = -1;
  m_cachedEarlyAfterRow = -1;
  m_cachedEarlyAfterTriggerPos = -1;
}

void PlaylistManager::refreshMixTriggerCache(int knownActiveRow,
                                             int knownNextRow) {
  m_cachedActiveRow = (knownActiveRow >= 0) ? knownActiveRow : activeRow();
  m_cachedNextRow = -1;
  m_cachedTriggerPos = -1;
  m_cachedEarlyAfterRow = -1;
  m_cachedEarlyAfterTriggerPos = -1;

  if (m_status == PlaylistStatus::Stopped || m_activeSlot < 0 ||
      m_cachedActiveRow < 0) {
    return;
  }

  PlayerSlot &active = m_slots[m_activeSlot];
  if (!active.player) {
    return;
  }

  const PlayerMetadata &currMeta = active.player->currentMetadata();
  if (currMeta.endPoint <= 0 && currMeta.duration <= 0) {
    return;
  }

  const int nextRow = (knownNextRow != -2)
                          ? knownNextRow
                          : m_model->findNextEntryRow(m_cachedActiveRow);
  if (nextRow < 0) {
    return;
  }

  const PlaylistItem *nextItem = m_model->itemAt(nextRow);
  if (!nextItem || nextItem->type != ItemType::Media) {
    return;
  }

  auto loadedMetaForRow = [this](const PlaylistItem *item,
                                 const PlayerMetadata &fallback) {
    for (int i = 0; i < kPlayerCount; ++i) {
      if ((m_slots[i].role == SlotRole::PreloadNext ||
           m_slots[i].role == SlotRole::PreloadExt) &&
          m_slots[i].itemUuid == item->uuid && m_slots[i].player &&
          m_slots[i].player->status() == PlayerStatus::Loaded) {
        return m_slots[i].player->currentMetadata();
      }
    }
    return fallback;
  };

  const PlayerMetadata nextMeta =
      loadedMetaForRow(nextItem, metadataFromItem(*nextItem));

  m_cachedNextRow = nextRow;
  m_cachedTriggerPos = computeMixTriggerPosition(currMeta, nextMeta);

  const int afterNextRow = m_model->findNextEntryRow(nextRow);
  if (afterNextRow >= 0) {
    const PlaylistItem *afterItem = m_model->itemAt(afterNextRow);
    if (afterItem && afterItem->type == ItemType::Media) {
      const PlayerMetadata afterMeta =
          loadedMetaForRow(afterItem, metadataFromItem(*afterItem));

      const qint64 nextStart = qMax<qint64>(0, nextMeta.startPoint);
      const qint64 nextEnd =
          (nextMeta.endPoint > 0) ? nextMeta.endPoint : nextMeta.duration;
      const qint64 nextMixEnd =
          (nextMeta.mixEnd > nextStart &&
           (nextEnd <= 0 || nextMeta.mixEnd <= nextEnd))
              ? nextMeta.mixEnd
              : nextEnd;
      const qint64 nextPlayToMixEnd = qMax<qint64>(0, nextMixEnd - nextStart);

      const qint64 afterStart = qMax<qint64>(0, afterMeta.startPoint);
      const qint64 afterEnd =
          (afterMeta.endPoint > 0) ? afterMeta.endPoint : afterMeta.duration;
      const qint64 afterMixStart =
          (afterMeta.mixStart > afterStart &&
           (afterEnd <= 0 || afterMeta.mixStart <= afterEnd))
              ? afterMeta.mixStart
              : afterStart;
      const qint64 afterIntro = qMax<qint64>(0, afterMixStart - afterStart);

      if (nextPlayToMixEnd > 0 && afterIntro > nextPlayToMixEnd) {
        const qint64 earlyLeadMs = afterIntro - nextPlayToMixEnd;
        const qint64 currStart = qMax<qint64>(0, currMeta.startPoint);
        const qint64 currEnd =
            (currMeta.endPoint > 0) ? currMeta.endPoint : currMeta.duration;
        const qint64 rawEarlyTrigger = m_cachedTriggerPos - earlyLeadMs;
        m_cachedEarlyAfterRow = afterNextRow;
        m_cachedEarlyAfterTriggerPos =
            (currEnd > 0) ? qBound<qint64>(currStart, rawEarlyTrigger, currEnd)
                          : qMax<qint64>(currStart, rawEarlyTrigger);
      }
    }
  }
}

void PlaylistManager::checkMixTrigger(qint64 pos) {
  if (m_status != PlaylistStatus::Playing || m_activeSlot < 0 ||
      m_transitionTriggeredForActive || m_inAdvance) {
    return;
  }

  if (m_cachedNextRow < 0 || m_cachedTriggerPos < 0) {
    return;
  }

  PlayerSlot &active = m_slots[m_activeSlot];
  if (!active.player || active.player->status() != PlayerStatus::Playing) {
    return;
  }

  if (m_cachedEarlyAfterRow >= 0 && m_cachedEarlyAfterTriggerPos >= 0 &&
      pos >= m_cachedEarlyAfterTriggerPos && !m_pendingOverlay.active()) {
    const PlaylistItem *skippedItem = m_model->itemAt(m_cachedNextRow);
    if (skippedItem && skippedItem->type == ItemType::Media) {
      m_pendingOverlay.skippedItemUuid = skippedItem->uuid;
      m_pendingOverlay.sourceItemUuid = active.itemUuid;
      m_pendingOverlay.sourceSlot = m_activeSlot;
      m_pendingOverlay.triggerPosMs = m_cachedTriggerPos;

      const int skippedSlot = findSlotForItem(
          skippedItem->uuid, resolveItemPath(skippedItem->path));
      if (skippedSlot >= 0) {
        m_slots[skippedSlot].role = SlotRole::PendingOverlay;
      }

      {
        ScopedFlag guard(m_updatingModelMetadata);
        m_model->setPendingOverlayUuid(m_pendingOverlay.skippedItemUuid);
      }

      m_transitionTriggeredForActive = true;
      startPlaybackFromRow(m_cachedEarlyAfterRow, false);
      return;
    }
  }

  if (pos >= m_cachedTriggerPos) {
    m_transitionTriggeredForActive = true;
    startPlaybackFromRow(m_cachedNextRow, false);
  }
}

void PlaylistManager::clearPendingOverlay(bool markFinished) {
  if (!m_pendingOverlay.active()) {
    return;
  }

  const QUuid skippedUuid = m_pendingOverlay.skippedItemUuid;
  m_pendingOverlay.clear();

  for (int i = 0; i < kPlayerCount; ++i) {
    if (m_slots[i].role == SlotRole::PendingOverlay &&
        m_slots[i].itemUuid == skippedUuid) {
      freeSlot(i);
      break;
    }
  }

  if (m_model) {
    ScopedFlag guard(m_updatingModelMetadata);
    m_model->setPendingOverlayUuid(QUuid());
    if (markFinished && !skippedUuid.isNull()) {
      const int row = m_model->rowForUuid(skippedUuid);
      if (row >= 0) {
        m_model->markRowFinished(row);
      }
    }
  }
}

void PlaylistManager::triggerPendingOverlay() {
  if (!m_pendingOverlay.active()) {
    return;
  }

  const QUuid skippedUuid = m_pendingOverlay.skippedItemUuid;
  m_pendingOverlay.clear();

  if (!m_model || skippedUuid.isNull()) {
    return;
  }

  const int row = m_model->rowForUuid(skippedUuid);
  const PlaylistItem *item = m_model->itemAt(row);
  if (!item || item->type != ItemType::Media) {
    ScopedFlag guard(m_updatingModelMetadata);
    m_model->setPendingOverlayUuid(QUuid());
    return;
  }

  int targetSlot = -1;
  for (int i = 0; i < kPlayerCount; ++i) {
    if (m_slots[i].itemUuid == skippedUuid &&
        m_slots[i].role != SlotRole::Free) {
      targetSlot = i;
      break;
    }
  }
  if (targetSlot < 0) {
    for (int i = 0; i < kPlayerCount; ++i) {
      if (m_slots[i].role == SlotRole::Free) {
        targetSlot = i;
        break;
      }
    }
  }

  if (targetSlot < 0) {
    ScopedFlag guard(m_updatingModelMetadata);
    m_model->setPendingOverlayUuid(QUuid());
    m_model->markRowFinished(row);
    return;
  }

  PlayerSlot &slot = m_slots[targetSlot];
  const QString resolvedPath = resolveItemPath(item->path);
  const bool alreadyLoaded =
      (slot.itemUuid == item->uuid && slot.loadedPath == resolvedPath &&
       slot.player &&
       (slot.player->status() == PlayerStatus::Loaded ||
        slot.player->status() == PlayerStatus::Loading));

  slot.role = SlotRole::Tail;
  slot.itemUuid = item->uuid;
  slot.loadedPath = resolvedPath;
  slot.lastUiPositionMs = -1;

  {
    ScopedFlag guard(m_updatingModelMetadata);
    m_model->setPendingOverlayUuid(QUuid());
    m_model->setItemPlayerId(row, targetSlot);
    m_model->markRowMixing(row);
  }

  if (slot.player) {
    if (alreadyLoaded) {
      const PlayerMetadata desiredMeta = metadataFromItem(*item);
      if (slot.player->status() == PlayerStatus::Loaded &&
          metadataDiffers(slot.player->currentMetadata(), desiredMeta)) {
        slot.player->setMetadata(desiredMeta);
      }
      slot.player->play();
    } else {
      slot.player->load(QUrl::fromLocalFile(resolvedPath),
                        metadataFromItem(*item), true);
    }

    if (slot.player->status() == PlayerStatus::Error) {
      emit playbackError(row, resolvedPath);
      freeSlot(targetSlot);
      return;
    }

    if (slot.player->status() == PlayerStatus::Playing) {
      updateSlotUiPosition(targetSlot, row, slot.player->position(), true);
    }
  }
}

void PlaylistManager::updateSlotUiPosition(int slotIndex, int row, qint64 pos,
                                           bool force) {
  if (slotIndex < 0 || slotIndex >= kPlayerCount || row < 0) {
    return;
  }
  PlayerSlot &slot = m_slots[slotIndex];
  if (pos == slot.lastUiPositionMs) {
    return;
  }
  if (!force && slot.lastUiPositionMs >= 0 &&
      std::abs(pos - slot.lastUiPositionMs) < kUiPositionMinDeltaMs) {
    return;
  }
  slot.lastUiPositionMs = pos;
  m_model->setItemPosition(row, pos);
}

void PlaylistManager::onMonitorTimeout() {
  if (m_status != PlaylistStatus::Playing || m_inAdvance) {
    return;
  }

  for (int i = 0; i < kPlayerCount; ++i) {
    if (i == m_activeSlot) {
      continue;
    }
    const PlayerSlot &slot = m_slots[i];
    if (slot.role == SlotRole::Tail && !slot.itemUuid.isNull() && slot.player &&
        slot.player->status() == PlayerStatus::Playing) {
      const qint64 tailPos = slot.player->position();
      const int tailRow = m_model->rowForUuid(slot.itemUuid);
      updateSlotUiPosition(i, tailRow, tailPos);
      if (m_pendingOverlay.active() && i == m_pendingOverlay.sourceSlot &&
          slot.itemUuid == m_pendingOverlay.sourceItemUuid &&
          tailPos >= m_pendingOverlay.triggerPosMs) {
        triggerPendingOverlay();
      }
    }
  }

  if (m_activeSlot < 0) {
    return;
  }

  PlayerManager *activePlayer = m_slots[m_activeSlot].player;
  if (!activePlayer || activePlayer->status() != PlayerStatus::Playing) {
    return;
  }

  const qint64 pos = activePlayer->position();
  const int row = (m_cachedActiveRow >= 0) ? m_cachedActiveRow : activeRow();
  updateSlotUiPosition(m_activeSlot, row, pos);
  checkMixTrigger(pos);
}

void PlaylistManager::onPlayerPositionChanged(int slotIndex, qint64 pos) {
  if (m_status != PlaylistStatus::Playing || m_inAdvance || slotIndex < 0 ||
      slotIndex >= kPlayerCount) {
    return;
  }

  if (slotIndex == m_activeSlot) {
    const int row = (m_cachedActiveRow >= 0) ? m_cachedActiveRow : activeRow();
    updateSlotUiPosition(slotIndex, row, pos);
    checkMixTrigger(pos);
    return;
  }

  const PlayerSlot &slot = m_slots[slotIndex];
  if (slot.role == SlotRole::Tail && !slot.itemUuid.isNull()) {
    const int tailRow = m_model->rowForUuid(slot.itemUuid);
    updateSlotUiPosition(slotIndex, tailRow, pos);
    if (m_pendingOverlay.active() && slotIndex == m_pendingOverlay.sourceSlot &&
        slot.itemUuid == m_pendingOverlay.sourceItemUuid &&
        pos >= m_pendingOverlay.triggerPosMs) {
      triggerPendingOverlay();
    }
  }
}

void PlaylistManager::onPlayerStatusChanged(int slotIndex,
                                            PlayerStatus newStatus) {
  if (m_inAdvance || slotIndex < 0 || slotIndex >= kPlayerCount) {
    return;
  }

  PlayerSlot &slot = m_slots[slotIndex];
  if (slot.role == SlotRole::Free) {
    return;
  }

  if (slot.role == SlotRole::Tail) {
    if (newStatus == PlayerStatus::End || newStatus == PlayerStatus::Error) {
      if (m_pendingOverlay.active() &&
          slotIndex == m_pendingOverlay.sourceSlot) {
        triggerPendingOverlay();
      }
      freeSlot(slotIndex);
      if (m_status != PlaylistStatus::Stopped) {
        syncPreloads();
      }
    }
    return;
  }

  if (slot.role == SlotRole::PendingOverlay) {
    return;
  }

  if (slot.role == SlotRole::PreloadNext || slot.role == SlotRole::PreloadExt) {
    if (newStatus == PlayerStatus::Loaded && m_status != PlaylistStatus::Stopped) {
      syncPreloads();
      if (m_status == PlaylistStatus::Playing && m_activeSlot >= 0 &&
          m_slots[m_activeSlot].player &&
          m_slots[m_activeSlot].player->status() == PlayerStatus::Playing) {
        checkMixTrigger(m_slots[m_activeSlot].player->position());
      }
    }
    return;
  }

  // Active slot
  if (newStatus == PlayerStatus::Loaded) {
    if (m_status != PlaylistStatus::Stopped) {
      syncPreloads();
    }
    return;
  }

  if (newStatus == PlayerStatus::End) {
    if (m_status != PlaylistStatus::Playing) {
      return;
    }
    const int currRow = activeRow();
    const int nextRow = (currRow >= 0) ? m_model->findNextEntryRow(currRow)
                                       : m_model->findNextPlayableRow();
    freeSlot(slotIndex);
    m_activeSlot = -1;
    m_transitionTriggeredForActive = false;

    if (nextRow >= 0) {
      startPlaybackFromRow(nextRow, false);
    } else {
      finishPlaylist();
    }
    return;
  }

  if (newStatus == PlayerStatus::Error) {
    if (m_status != PlaylistStatus::Playing) {
      return;
    }
    const int currRow = activeRow();
    const QString failedPath = slot.loadedPath;
    const int nextRow = (currRow >= 0) ? m_model->findNextEntryRow(currRow)
                                       : m_model->findNextPlayableRow();

    if (currRow >= 0) {
      qWarning() << "Playback error for item at row" << currRow << ":"
                 << failedPath;
      emit playbackError(currRow, failedPath);
      ScopedFlag guard(m_updatingModelMetadata);
      m_model->markRowFinished(currRow);
    }

    freeSlot(slotIndex);
    m_activeSlot = -1;
    m_transitionTriggeredForActive = false;

    if (nextRow >= 0) {
      startPlaybackFromRow(nextRow, false);
    } else {
      finishPlaylist();
    }
  }
}

void PlaylistManager::onPlayerDurationLoaded(int slotIndex, qint64 durationMs) {
  if (slotIndex < 0 || slotIndex >= kPlayerCount || durationMs <= 0) {
    return;
  }

  PlayerSlot &slot = m_slots[slotIndex];
  if (slot.role == SlotRole::Free || slot.itemUuid.isNull() || !slot.player) {
    return;
  }

  if (!slot.loadedPath.isEmpty()) {
    m_model->cacheDuration(slot.loadedPath, durationMs);
  }

  const int row = m_model->rowForUuid(slot.itemUuid);
  if (row >= 0) {
    const PlaylistItem *item = m_model->itemAt(row);
    if (item && (item->durationMs <= 0 || item->durationMs != durationMs)) {
      const PlayerMetadata &meta = slot.player->currentMetadata();
      ScopedFlag guard(m_updatingModelMetadata);
      m_model->updateItemMixMetadata(
          row, meta.duration, meta.startPoint, meta.endPoint, meta.mixStart,
          meta.mixEnd, meta.fadeInEndPoint, meta.fadeOutStartPoint,
          meta.startVolume, meta.endVolume, meta.volumeUpStartPoint,
          meta.volumeUpEndPoint, meta.volumeDownStartPoint,
          meta.volumeDownEndPoint);
    }
  }

  // If the player is still Loading, statusChanged(Loaded) will immediately
  // trigger syncPreloads() and checkMixTrigger() right after this signal.
  if (m_status != PlaylistStatus::Stopped && !m_inAdvance &&
      slot.player->status() != PlayerStatus::Loading) {
    syncPreloads();
    if (m_status == PlaylistStatus::Playing && m_activeSlot >= 0 &&
        m_slots[m_activeSlot].player &&
        m_slots[m_activeSlot].player->status() == PlayerStatus::Playing) {
      checkMixTrigger(m_slots[m_activeSlot].player->position());
    }
  }
}

void PlaylistManager::onModelStructureChanged() {
  if (m_updatingModelMetadata || m_inAdvance) {
    return;
  }

  if (m_status == PlaylistStatus::Stopped) {
    for (int i = 0; i < kPlayerCount; ++i) {
      if (m_slots[i].role != SlotRole::Free &&
          m_slots[i].role != SlotRole::Tail) {
        freeSlot(i);
      }
    }
    m_activeSlot = -1;
    m_transitionTriggeredForActive = false;
    m_currentBlockUuid = QUuid();
    resetMixTriggerCache();
    return;
  }

  if (activeRow() < 0) {
    stop();
    return;
  }

  syncPreloads();
  if (m_status == PlaylistStatus::Playing && m_activeSlot >= 0 &&
      m_slots[m_activeSlot].player &&
      m_slots[m_activeSlot].player->status() == PlayerStatus::Playing) {
    checkMixTrigger(m_slots[m_activeSlot].player->position());
  }
}

void PlaylistManager::onModelDataChanged(const QModelIndex &topLeft,
                                         const QModelIndex &bottomRight,
                                         const QList<int> &roles) {
  if (m_updatingModelMetadata || m_inAdvance ||
      m_status == PlaylistStatus::Stopped) {
    return;
  }

  if (roles.size() == 1 && roles.constFirst() == PositionRole) {
    return;
  }

  bool metadataEdited = roles.isEmpty();
  if (!metadataEdited) {
    for (int role : roles) {
      if (role == DurationRole || role == MixStartRole || role == MixEndRole ||
          role == StartPointRole || role == EndPointRole ||
          role == FadeInEndPointRole || role == FadeOutStartPointRole ||
          role == StartVolumeRole || role == EndVolumeRole ||
          role == VolumeUpStartPointRole || role == VolumeUpEndPointRole ||
          role == VolumeDownStartPointRole || role == VolumeDownEndPointRole) {
        metadataEdited = true;
        break;
      }
    }
  }

  if (!metadataEdited) {
    return;
  }

  const int firstRow = topLeft.isValid() ? topLeft.row() : 0;
  const int lastRow =
      bottomRight.isValid() ? bottomRight.row() : (m_model->rowCount() - 1);

  const int currRow = activeRow();
  const int nextRow = (currRow >= 0) ? m_model->findNextMediaRow(currRow) : -1;
  const int afterNextRow =
      (nextRow >= 0) ? m_model->findNextMediaRow(nextRow) : -1;

  const auto inRange = [firstRow, lastRow](int r) {
    return r >= firstRow && r <= lastRow;
  };
  if (!inRange(currRow) && !inRange(nextRow) && !inRange(afterNextRow)) {
    return;
  }

  for (int i = 0; i < kPlayerCount; ++i) {
    PlayerSlot &slot = m_slots[i];
    if ((slot.role == SlotRole::Active || slot.role == SlotRole::PreloadNext ||
         slot.role == SlotRole::PreloadExt) &&
        !slot.itemUuid.isNull() && slot.player) {
      const int row = m_model->rowForUuid(slot.itemUuid);
      if (row >= firstRow && row <= lastRow) {
        const PlaylistItem *item = m_model->itemAt(row);
        if (item) {
          const PlayerMetadata desiredMeta = metadataFromItem(*item);
          if (metadataDiffers(slot.player->currentMetadata(), desiredMeta)) {
            slot.player->setMetadata(desiredMeta);
          }
        }
      }
    }
  }

  syncPreloads();
  if (m_status == PlaylistStatus::Playing && m_activeSlot >= 0 &&
      m_slots[m_activeSlot].player &&
      m_slots[m_activeSlot].player->status() == PlayerStatus::Playing) {
    checkMixTrigger(m_slots[m_activeSlot].player->position());
  }
}
