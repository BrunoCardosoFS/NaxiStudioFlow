#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <array>

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUuid>

#include "core/models.h"
#include "core/playlist/PlaylistListModel.h"
#include "core/playlist/player/PlayerManager.h"

class PlaylistManager : public QObject {
  Q_OBJECT
public:
  static constexpr int kPlayerCount = 3;
  static constexpr qint64 kDefaultMinPreloadWindowMs = 3000;
  static constexpr qint64 kDefaultManualNextFadeMs = 500;
  static constexpr qint64 kDefaultStopFadeMs = 1500;
  static constexpr int kMonitorIntervalMs = 30;

  enum class SlotRole { Free, Active, Tail, PreloadNext, PreloadExt };

  explicit PlaylistManager(QObject *parent = nullptr);
  explicit PlaylistManager(std::array<PlayerManager *, kPlayerCount> players,
                           QObject *parent = nullptr);
  ~PlaylistManager() override;

  void play();
  void pause();
  void next();
  void stop();
  void stopImmediate();

  PlaylistListModel *model() const { return m_model; }
  PlaylistStatus status() const { return m_status; }

  int activePlayerIndex() const { return m_activeSlot; }
  PlayerManager *playerAt(int index) const;
  SlotRole slotRole(int index) const;
  QUuid slotItemUuid(int index) const;

  void setMinPreloadWindowMs(qint64 ms);
  qint64 minPreloadWindowMs() const { return m_minPreloadWindowMs; }

  void setManualNextFadeMs(qint64 ms);
  qint64 manualNextFadeMs() const { return m_manualNextFadeMs; }

  void setStopFadeMs(qint64 ms);
  qint64 stopFadeMs() const { return m_stopFadeMs; }

  static PlayerMetadata metadataFromItem(const PlaylistItem &item);
  static qint64 computeMixTriggerPosition(const PlayerMetadata &currentMeta,
                                          const PlayerMetadata &nextMeta);
  static bool
  shouldPreloadExtended(const PlayerMetadata &currentMeta,
                        const PlayerMetadata &nextMeta,
                        const PlayerMetadata &afterNextMeta,
                        qint64 minPreloadWindowMs = kDefaultMinPreloadWindowMs);

signals:
  void statusChanged(PlaylistStatus status);
  void currentItemChanged(int row, const QUuid &uuid);
  void blockChanged(int blockRow, const QString &blockTime,
                    const QString &blockTitle);
  void blockFinished(int blockRow);
  void playlistFinished();
  void playbackError(int row, const QString &path);

private:
  struct PlayerSlot {
    PlayerManager *player = nullptr;
    SlotRole role = SlotRole::Free;
    QUuid itemUuid;
    QString loadedPath;
  };

  PlaylistListModel *m_model = nullptr;
  PlaylistStatus m_status = PlaylistStatus::Stopped;
  std::array<PlayerSlot, kPlayerCount> m_slots;
  int m_activeSlot = -1;
  QUuid m_currentBlockUuid;
  bool m_transitionTriggeredForActive = false;
  bool m_inAdvance = false;
  bool m_updatingModelMetadata = false;
  qint64 m_minPreloadWindowMs = kDefaultMinPreloadWindowMs;
  qint64 m_manualNextFadeMs = kDefaultManualNextFadeMs;
  qint64 m_stopFadeMs = kDefaultStopFadeMs;

  QTimer *m_timer = nullptr;

  void initConnections();
  int activeRow() const;
  int findSlotForItem(const QUuid &uuid, const QString &path) const;
  void freeSlot(int slotIndex);
  void unloadAllSlots();
  void finishPlaylist();

  void startPlaybackFromRow(int startRow, bool fadeOutOutgoing);
  void syncPreloads();
  void preloadSlot(int slotIndex, int targetRow, SlotRole role);
  void checkMixTrigger(qint64 pos);
  void emitBlockTransitionIfNeeded(int mediaRow);

private slots:
  void onMonitorTimeout();
  void onPlayerPositionChanged(int slotIndex, qint64 pos);
  void onPlayerStatusChanged(int slotIndex, PlayerStatus newStatus);
  void onPlayerDurationLoaded(int slotIndex, qint64 durationMs);
  void onModelStructureChanged();
  void onModelDataChanged(const QModelIndex &topLeft,
                          const QModelIndex &bottomRight,
                          const QList<int> &roles);
};

#endif // PLAYLISTMANAGER_H