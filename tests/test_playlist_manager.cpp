#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QHash>
#include <QMimeData>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

#include "core/catalog/CatalogItem.h"
#include "core/catalog/CatalogListModel.h"
#include "core/models.h"
#include "core/playlist/PlaylistListModel.h"
#include "core/playlist/PlaylistManager.h"
#include "core/playlist/player/PlayerManager.h"
#include "ui/widgets/playlist/PlaylistItemDelegate.h"
#include "ui/widgets/playlist/PlaylistView.h"

namespace {
struct FakeAudioRegistry {
  QSet<QString> missingPaths;
  QHash<QString, qint64> durationByPath;
};

class FakePlayerManager : public PlayerManager {
public:
  explicit FakePlayerManager(FakeAudioRegistry *registry,
                             QObject *parent = nullptr)
      : PlayerManager(false, parent), m_registry(registry) {}

  int loadCount = 0;
  int playCount = 0;
  int pauseCount = 0;
  int stopCount = 0;
  int unloadCount = 0;
  int fadeOutCalls = 0;
  qint64 lastFadeOutDuration = -1;
  bool deferLoadCompletion = false;

  void load(const QUrl &source, const PlayerMetadata &newMetadata,
            bool autoPlay = false) override {
    if (m_source == source && status() == PlayerStatus::Loaded &&
        !source.isEmpty()) {
      setMetadata(newMetadata);
      if (autoPlay) {
        play();
      }
      return;
    }

    ++loadCount;
    setStatus(PlayerStatus::Loading);

    const QString local =
        source.isLocalFile() ? source.toLocalFile() : source.toString();
    if (source.isEmpty() ||
        (m_registry && m_registry->missingPaths.contains(local))) {
      m_pendingPlay = false;
      m_source.clear();
      metadata = PlayerMetadata{};
      updateFadeParameters();
      applyVolume(0.0f);
      m_pos = 0;
      setStatus(PlayerStatus::Error);
      return;
    }

    m_source = source;
    metadata = newMetadata;
    m_pendingPlay = autoPlay;
    normalizeMetadata(metadata.duration);
    m_pos = metadata.startPoint;
    applyVolume(0.0f);

    if (!deferLoadCompletion) {
      completeLoad();
    }
  }

  void completeLoad() {
    const QString local =
        m_source.isLocalFile() ? m_source.toLocalFile() : m_source.toString();
    const qint64 realDur = m_registry
                               ? m_registry->durationByPath.value(
                                     local, metadata.duration)
                               : metadata.duration;
    normalizeMetadata(realDur);
    m_pos = metadata.startPoint;
    applyVolume(0.0f);
    setStatus(PlayerStatus::Loaded);

    if (metadata.duration > 0) {
      emit durationLoaded(metadata.duration);
    }

    if (m_pendingPlay) {
      m_pendingPlay = false;
      play();
    }
  }

  void unload() override {
    ++unloadCount;
    m_pendingPlay = false;
    m_source.clear();
    metadata = PlayerMetadata{};
    updateFadeParameters();
    applyVolume(0.0f);
    m_pos = 0;
    setStatus(PlayerStatus::Idle);
  }

  void play() override {
    if (status() == PlayerStatus::Loading) {
      m_pendingPlay = true;
      return;
    }
    if (status() == PlayerStatus::Playing || status() == PlayerStatus::Error ||
        status() == PlayerStatus::Idle) {
      return;
    }

    if (status() == PlayerStatus::Loaded || status() == PlayerStatus::End) {
      m_pos = metadata.startPoint;
    }
    if (metadata.endPoint > 0 && m_pos >= metadata.endPoint) {
      stop();
      return;
    }

    ++playCount;
    setStatus(PlayerStatus::Playing);
    volumeControl(m_pos);
  }

  void pause() override {
    if (status() == PlayerStatus::Loading) {
      m_pendingPlay = false;
      return;
    }
    if (status() != PlayerStatus::Playing) {
      return;
    }
    ++pauseCount;
    setStatus(PlayerStatus::Paused);
  }

  void stop() override {
    ++stopCount;
    m_pendingPlay = false;
    applyVolume(0.0f);
    m_pos = metadata.endPoint;
    setStatus(PlayerStatus::End);
  }

  void triggerFadeOut(qint64 fadeDurationMs) override {
    ++fadeOutCalls;
    lastFadeOutDuration = fadeDurationMs;
    PlayerManager::triggerFadeOut(fadeDurationMs);
  }

  void setMetadata(const PlayerMetadata &newMetadata) override {
    metadata = newMetadata;
    normalizeMetadata(metadata.duration);
    if (status() == PlayerStatus::Loaded) {
      m_pos = metadata.startPoint;
    }
    volumeControl(m_pos);
  }

  qint64 position() const override { return m_pos; }
  qint64 duration() const override { return metadata.duration; }

  void advanceTo(qint64 posMs) {
    m_pos = posMs;
    volumeControl(posMs);
  }

private:
  FakeAudioRegistry *m_registry = nullptr;
  qint64 m_pos = 0;
};

PlaylistItem makeBlockMarker(const QString &timeStr, const QString &title) {
  PlaylistItem item;
  item.uuid = QUuid::createUuid();
  item.type = ItemType::BlockMarker;
  item.blockType = BlockType::Commercial;
  item.mediaType = MediaType::Other;
  item.title = timeStr;
  item.blockTitle = title;
  return item;
}

PlaylistItem makeStartMarker(BlockType bType) {
  PlaylistItem item;
  item.uuid = QUuid::createUuid();
  item.type = ItemType::StartMarker;
  item.blockType = bType;
  item.mediaType = MediaType::Other;
  return item;
}

PlaylistItem makeEndMarker(BlockType bType) {
  PlaylistItem item;
  item.uuid = QUuid::createUuid();
  item.type = ItemType::EndMarker;
  item.blockType = bType;
  item.mediaType = MediaType::Other;
  return item;
}

PlaylistItem makeMediaItem(const QString &title, const QString &path,
                           qint64 durationMs, qint64 startPoint = 0,
                           qint64 endPoint = -1, qint64 mixStart = 0,
                           qint64 mixEnd = -1, qint64 fadeInEnd = 0,
                           qint64 fadeOutStart = -1,
                           MediaType mType = MediaType::Music) {
  PlaylistItem item;
  item.uuid = QUuid::createUuid();
  item.type = ItemType::Media;
  item.mediaType = mType;
  item.title = title;
  item.path = path;
  item.durationMs = durationMs;
  item.startPoint = startPoint;
  item.endPoint = (endPoint >= 0) ? endPoint : durationMs;
  item.mixStart = mixStart;
  item.mixEnd = (mixEnd >= 0) ? mixEnd : item.endPoint;
  item.fadeInEndPoint = fadeInEnd;
  item.fadeOutStartPoint = (fadeOutStart >= 0) ? fadeOutStart : item.endPoint;
  return item;
}

bool writeTestWavFile(const QString &filePath, int durationMs) {
  constexpr quint32 sampleRate = 44100;
  constexpr quint16 channels = 1;
  constexpr quint16 bitsPerSample = 16;
  const quint32 totalSamples =
      static_cast<quint32>((static_cast<quint64>(sampleRate) * durationMs) /
                           1000ULL);
  const quint32 dataBytes = totalSamples * channels * (bitsPerSample / 8);
  const quint32 byteRate = sampleRate * channels * (bitsPerSample / 8);
  const quint16 blockAlign = channels * (bitsPerSample / 8);

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  QDataStream out(&file);
  out.setByteOrder(QDataStream::LittleEndian);

  out.writeRawData("RIFF", 4);
  out << static_cast<quint32>(36 + dataBytes);
  out.writeRawData("WAVE", 4);
  out.writeRawData("fmt ", 4);
  out << static_cast<quint32>(16);
  out << static_cast<quint16>(1);
  out << channels;
  out << sampleRate;
  out << byteRate;
  out << blockAlign;
  out << bitsPerSample;
  out.writeRawData("data", 4);
  out << dataBytes;

  const QByteArray silence(static_cast<int>(dataBytes), '\0');
  if (file.write(silence) != static_cast<qint64>(dataBytes)) {
    return false;
  }
  file.close();
  return true;
}
} // namespace

class TestPlaylistManager : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void testMixTriggerCalculation();
  void testDynamicExtendedPreloadDecision();
  void testThreePlayerRoundRobinAndEarlyMixSync();
  void testTripleOverlapWithExtendedPreload();
  void testBlockTransitionsAndEmptySections();
  void testTransportPlayPauseStopNext();
  void testModelReorderUpdatesPreloads();
  void testProgressBarPositionRoleUpdates();
  void testMissingFileErrorSkipsGracefully();
  void testPlaylistListModelEstimatedTimeWithMixPoints();
  void testInsertCatalogItemAndMarkerBoundaries();
  void testRealAudioDurationProbeFromWavFile();
  void testRemoveFutureItemsAndLockProtection();
  void testPlaylistViewCatalogDropAndDeleteKey();
  void testStopFadeOutAndRemainingAudioClamp();
  void testPlaylistViewRowHeightUpdatesOnStopAndFinish();
  void testTriggerFadeOutDuringActiveFadeOutDoesNotJumpVolume();
  void testConsecutiveDuplicateFileProbeUsesCacheAndDoesNotTimeout();
  void testRecalculateBlockMetadataEmitsExactRoles();
};

void TestPlaylistManager::initTestCase() {
  qRegisterMetaType<PlaylistStatus>("PlaylistStatus");
  qRegisterMetaType<PlayerStatus>("PlayerStatus");
}

void TestPlaylistManager::testMixTriggerCalculation() {
  PlayerMetadata curr;
  curr.duration = 200000;
  curr.startPoint = 1000;
  curr.endPoint = 198000;
  curr.mixEnd = 195000;

  PlayerMetadata next;
  next.duration = 180000;
  next.startPoint = 500;
  next.mixStart = 3500; // intro = 3000ms
  next.endPoint = 180000;

  // Standard: mixEnd_curr - (mixStart_next - startPoint_next) = 195000 - 3000
  QCOMPARE(PlaylistManager::computeMixTriggerPosition(curr, next),
           qint64(192000));

  // Zero intro
  next.mixStart = next.startPoint;
  QCOMPARE(PlaylistManager::computeMixTriggerPosition(curr, next),
           qint64(195000));

  // Intro exceeding current track playable length -> clamped to curr.startPoint
  next.duration = 500000;
  next.endPoint = 500000;
  next.mixStart = 300000;
  QCOMPARE(PlaylistManager::computeMixTriggerPosition(curr, next),
           curr.startPoint);

  // Unset mixEnd (0) -> defaults to endPoint
  curr.mixEnd = 0;
  next.duration = 180000;
  next.endPoint = 180000;
  next.mixStart = 2500; // intro = 2000ms
  QCOMPARE(PlaylistManager::computeMixTriggerPosition(curr, next),
           qint64(196000));
}

void TestPlaylistManager::testDynamicExtendedPreloadDecision() {
  PlayerMetadata song1;
  song1.duration = 200000;
  song1.startPoint = 0;
  song1.endPoint = 200000;
  song1.mixEnd = 195000;

  PlayerMetadata longSong2;
  longSong2.duration = 180000;
  longSong2.startPoint = 0;
  longSong2.endPoint = 180000;
  longSong2.mixStart = 2000;
  longSong2.mixEnd = 175000;

  PlayerMetadata song3;
  song3.duration = 180000;
  song3.startPoint = 0;
  song3.endPoint = 180000;
  song3.mixStart = 3000;

  // Long next track without triple overlap -> false (saves RAM)
  QVERIFY(!PlaylistManager::shouldPreloadExtended(song1, longSong2, song3,
                                                  3000));

  // Short next jingle -> windowInNextMs <= minPreloadWindowMs -> true
  PlayerMetadata shortJingle;
  shortJingle.duration = 4000;
  shortJingle.startPoint = 0;
  shortJingle.endPoint = 4000;
  shortJingle.mixStart = 0;
  shortJingle.mixEnd = 3500; // triggerNextToAfter = 3500 - 3000 = 500ms <= 3000
  QVERIFY(PlaylistManager::shouldPreloadExtended(song1, shortJingle, song3,
                                                 3000));

  // Triple overlap even when windowInNextMs > minPreloadWindowMs
  song1.mixEnd = 191000; // with jingle mixStart=1000 -> trigger=190000 -> tail=10000ms
  PlayerMetadata mediumJingle;
  mediumJingle.duration = 7000;
  mediumJingle.startPoint = 0;
  mediumJingle.endPoint = 7000;
  mediumJingle.mixStart = 1000;
  mediumJingle.mixEnd = 6500;
  song3.mixStart = 1500; // triggerNextToAfter = 6500 - 1500 = 5000ms (>3000ms, <10000ms)
  QVERIFY(PlaylistManager::shouldPreloadExtended(song1, mediumJingle, song3,
                                                 3000));

  // Unknown duration for next track -> false
  PlayerMetadata unknownNext;
  QVERIFY(!PlaylistManager::shouldPreloadExtended(song1, unknownNext, song3,
                                                  3000));
}

void TestPlaylistManager::testThreePlayerRoundRobinAndEarlyMixSync() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("14:00", "Block 1"));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto item1 =
      makeMediaItem("Track 1", "C:/audio/t1.mp3", 20000, 0, 20000, 0, 18000,
                    500, 18000);
  const auto item2 =
      makeMediaItem("Track 2", "C:/audio/t2.mp3", 20000, 0, 20000, 2000, 18000,
                    500, 18000);
  const auto item3 =
      makeMediaItem("Track 3", "C:/audio/t3.mp3", 20000, 0, 20000, 1000, 18000,
                    500, 18000);
  const auto item4 =
      makeMediaItem("Track 4", "C:/audio/t4.mp3", 20000, 0, 20000, 1000, 18000,
                    500, 18000);
  model->addItem(item1);
  model->addItem(item2);
  model->addItem(item3);
  model->addItem(item4);
  model->addItem(makeEndMarker(BlockType::Music));

  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(manager.activePlayerIndex(), 0);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Active);
  QCOMPARE(manager.slotItemUuid(0), item1.uuid);
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(1), item2.uuid);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Loaded);
  // Track 2 is long -> P2 stays Free to save RAM
  QCOMPARE(manager.slotRole(2), PlaylistManager::SlotRole::Free);

  // Just before trigger: mixEnd_1 (18000) - (mixStart_2 (2000) - 0) = 16000ms
  fakes[0]->advanceTo(15999);
  QCOMPARE(manager.activePlayerIndex(), 0);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Loaded);

  // Exact trigger position -> P1 starts Track 2 while P0 continues as Tail
  fakes[0]->advanceTo(16000);
  QCOMPARE(manager.activePlayerIndex(), 1);
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::Active);
  QCOMPARE(manager.slotItemUuid(1), item2.uuid);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Playing);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Tail);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
  QCOMPARE(manager.slotRole(2), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(2), item3.uuid);

  // P0 finishes its fade-out tail at 20000 -> becomes Free
  fakes[0]->advanceTo(20000);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Idle);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Free);

  // Track 2 triggers Track 3 at 18000 - 1000 = 17000ms -> P2 active, P0 preloads Track 4
  fakes[1]->advanceTo(17000);
  QCOMPARE(manager.activePlayerIndex(), 2);
  QCOMPARE(manager.slotRole(2), PlaylistManager::SlotRole::Active);
  QCOMPARE(manager.slotItemUuid(2), item3.uuid);
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::Tail);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(0), item4.uuid);

  // P1 finishes tail, then Track 3 triggers Track 4 on P0 -> full 0 -> 1 -> 2 -> 0 ring
  fakes[1]->advanceTo(20000);
  fakes[2]->advanceTo(17000);
  QCOMPARE(manager.activePlayerIndex(), 0);
  QCOMPARE(manager.slotItemUuid(0), item4.uuid);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
}

void TestPlaylistManager::testTripleOverlapWithExtendedPreload() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("15:00", "Triple Overlap"));
  model->addItem(makeStartMarker(BlockType::Music));
  // Song 1: mixEnd=192000, endPoint=200000 -> tail=8000ms (with jingle mixStart=0)
  const auto song1 =
      makeMediaItem("Song 1", "C:/audio/s1.mp3", 200000, 0, 200000, 0, 192000,
                    500, 192000);
  // Short Jingle: duration=4000ms, mixEnd=3500ms
  const auto jingle =
      makeMediaItem("Jingle", "C:/audio/j.mp3", 4000, 0, 4000, 0, 3500, 0, 3500,
                    MediaType::Jingle);
  // Song 2: mixStart=1500ms -> jingle trigger at 3500-1500 = 2000ms (< 8000ms tail!)
  const auto song2 =
      makeMediaItem("Song 2", "C:/audio/s2.mp3", 180000, 0, 180000, 1500,
                    175000, 500, 175000);
  const auto song3 =
      makeMediaItem("Song 3", "C:/audio/s3.mp3", 180000, 0, 180000, 2000,
                    175000, 500, 175000);

  model->addItem(song1);
  model->addItem(jingle);
  model->addItem(song2);
  model->addItem(song3);
  model->addItem(makeEndMarker(BlockType::Music));

  manager.play();

  // P0 plays Song 1, P1 preloads Jingle, P2 dynamically preloads Song 2
  QCOMPARE(manager.activePlayerIndex(), 0);
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(1), jingle.uuid);
  QCOMPARE(manager.slotRole(2), PlaylistManager::SlotRole::PreloadExt);
  QCOMPARE(manager.slotItemUuid(2), song2.uuid);
  QCOMPARE(fakes[2]->status(), PlayerStatus::Loaded);
  const int p2LoadsBeforeTransition = fakes[2]->loadCount;

  // Song 1 triggers Jingle at 192000ms
  fakes[0]->advanceTo(192000);
  QCOMPARE(manager.activePlayerIndex(), 1);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Tail);
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::Active);
  // P2 seamlessly upgrades from PreloadExt to PreloadNext without reloading!
  QCOMPARE(manager.slotRole(2), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(2), song2.uuid);
  QCOMPARE(fakes[2]->loadCount, p2LoadsBeforeTransition);

  // Jingle reaches 2000ms while Song 1 (P0) is STILL fading out (<200000ms)!
  fakes[0]->advanceTo(194000);
  fakes[1]->advanceTo(2000);

  // ALL THREE players are simultaneously Playing!
  QCOMPARE(manager.activePlayerIndex(), 2);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Playing);
  QCOMPARE(fakes[2]->status(), PlayerStatus::Playing);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Tail);
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::Tail);
  QCOMPARE(manager.slotRole(2), PlaylistManager::SlotRole::Active);

  // Once P0 finishes Song 1 tail at 200000ms, P0 is freed and immediately preloads Song 3!
  fakes[0]->advanceTo(200000);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(0), song3.uuid);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Loaded);
}

void TestPlaylistManager::testBlockTransitionsAndEmptySections() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();

  // Block 1: Commercial (1 item) + Music (1 item)
  model->addItem(makeBlockMarker("14:00", "Morning Show"));
  model->addItem(makeStartMarker(BlockType::Commercial));
  const auto comm1 =
      makeMediaItem("Spot 1", "C:/audio/c1.mp3", 10000, 0, 10000, 0, 10000, 0,
                    10000, MediaType::Commercial);
  model->addItem(comm1);
  model->addItem(makeEndMarker(BlockType::Commercial));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto mus1 =
      makeMediaItem("Music 1", "C:/audio/m1.mp3", 10000, 0, 10000, 0, 10000);
  model->addItem(mus1);
  model->addItem(makeEndMarker(BlockType::Music));

  // Block 2: Empty Commercial section + Music (1 item)
  model->addItem(makeBlockMarker("14:30", "Afternoon Show"));
  model->addItem(makeStartMarker(BlockType::Commercial));
  model->addItem(makeEndMarker(BlockType::Commercial));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto mus2 =
      makeMediaItem("Music 2", "C:/audio/m2.mp3", 10000, 0, 10000, 0, 10000);
  model->addItem(mus2);
  model->addItem(makeEndMarker(BlockType::Music));

  QSignalSpy blockChangedSpy(&manager, &PlaylistManager::blockChanged);
  QSignalSpy blockFinishedSpy(&manager, &PlaylistManager::blockFinished);
  QSignalSpy playlistFinishedSpy(&manager, &PlaylistManager::playlistFinished);

  manager.play();
  QCOMPARE(blockChangedSpy.count(), 1);
  QCOMPARE(blockChangedSpy.at(0).at(0).toInt(), 0);
  QCOMPARE(blockChangedSpy.at(0).at(1).toString(), QString("14:00"));
  QCOMPARE(blockChangedSpy.at(0).at(2).toString(), QString("Morning Show"));
  QCOMPARE(manager.slotItemUuid(manager.activePlayerIndex()), comm1.uuid);

  // Finish Spot 1 -> transitions across EndMarker/StartMarker to Music 1 in same block
  fakes[manager.activePlayerIndex()]->advanceTo(10000);
  QCOMPARE(manager.slotItemUuid(manager.activePlayerIndex()), mus1.uuid);
  QCOMPARE(blockChangedSpy.count(), 1);
  QCOMPARE(blockFinishedSpy.count(), 0);

  // Finish Music 1 -> skips EndMarker, BlockMarker, empty Commercial section -> Music 2
  fakes[manager.activePlayerIndex()]->advanceTo(10000);
  QCOMPARE(manager.slotItemUuid(manager.activePlayerIndex()), mus2.uuid);
  QCOMPARE(blockFinishedSpy.count(), 1);
  QCOMPARE(blockFinishedSpy.at(0).at(0).toInt(), 0);
  QCOMPARE(blockChangedSpy.count(), 2);
  QCOMPARE(blockChangedSpy.at(1).at(0).toInt(), 7);
  QCOMPARE(blockChangedSpy.at(1).at(1).toString(), QString("14:30"));
  QCOMPARE(blockChangedSpy.at(1).at(2).toString(), QString("Afternoon Show"));

  // Finish Music 2 -> finishes Block 2 and whole playlist
  fakes[manager.activePlayerIndex()]->advanceTo(10000);
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(blockFinishedSpy.count(), 2);
  QCOMPARE(blockFinishedSpy.at(1).at(0).toInt(), 7);
  QCOMPARE(playlistFinishedSpy.count(), 1);

  // All played media items must remain Finished (not reset to Next/Default)
  QCOMPARE(model->itemAt(model->rowForUuid(comm1.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(mus1.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(mus2.uuid))->status,
           PlaylistItemStatus::Finished);
}

void TestPlaylistManager::testTransportPlayPauseStopNext() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});
  manager.setManualNextFadeMs(500);
  manager.setStopFadeMs(1500);

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("10:00", "Show"));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto t1 =
      makeMediaItem("T1", "C:/audio/1.mp3", 10000, 0, 10000, 0, 8000);
  const auto t2 =
      makeMediaItem("T2", "C:/audio/2.mp3", 10000, 0, 10000, 0, 8000);
  const auto t3 =
      makeMediaItem("T3", "C:/audio/3.mp3", 10000, 0, 10000, 0, 8000);
  const auto t4 =
      makeMediaItem("T4", "C:/audio/4.mp3", 10000, 0, 10000, 0, 8000);
  const auto t5 =
      makeMediaItem("T5", "C:/audio/5.mp3", 10000, 0, 10000, 0, 8000);
  model->addItem(t1);
  model->addItem(t2);
  model->addItem(t3);
  model->addItem(t4);
  model->addItem(t5);
  model->addItem(makeEndMarker(BlockType::Music));

  QSignalSpy statusSpy(&manager, &PlaylistManager::statusChanged);

  // Play starts T1 on P0
  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(manager.activePlayerIndex(), 0);
  QCOMPARE(statusSpy.last().at(0).value<PlaylistStatus>(),
           PlaylistStatus::Playing);

  // Advance T1 into overlap with T2 so both P0 (Tail) and P1 (Active) play
  fakes[0]->advanceTo(8000);
  QCOMPARE(manager.activePlayerIndex(), 1);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Playing);

  // Pause must pause BOTH Active (P1) and overlapping Tail (P0)
  manager.pause();
  QCOMPARE(manager.status(), PlaylistStatus::Paused);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Paused);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Paused);

  // Play resumes BOTH P0 and P1
  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Playing);

  // Finish P0 tail cleanly, then trigger Next while Playing -> fade-out on P1, P2 active on T3
  fakes[0]->advanceTo(10000);
  fakes[1]->advanceTo(2000);
  manager.next();
  QCOMPARE(manager.activePlayerIndex(), 2);
  QCOMPARE(manager.slotItemUuid(2), t3.uuid);
  QCOMPARE(fakes[2]->status(), PlayerStatus::Playing);
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::Tail);
  QCOMPARE(fakes[1]->fadeOutCalls, 1);
  QCOMPARE(fakes[1]->lastFadeOutDuration, qint64(500));
  QCOMPARE(fakes[1]->status(), PlayerStatus::Playing);

  // Pause while T3 plays, then Next while Paused -> stops paused player and plays T4 immediately
  manager.pause();
  QCOMPARE(fakes[2]->status(), PlayerStatus::Paused);
  manager.next();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(manager.slotItemUuid(manager.activePlayerIndex()), t4.uuid);
  QCOMPARE(fakes[manager.activePlayerIndex()]->status(), PlayerStatus::Playing);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Loaded);
  QCOMPARE(manager.slotItemUuid(1), t5.uuid);
  QCOMPARE(fakes[2]->status(), PlayerStatus::Idle);

  // Advance T4 partway, then Stop -> fades out P0 as Tail, marks T4 Finished, promotes T5 to Next
  fakes[manager.activePlayerIndex()]->advanceTo(3500);
  QCOMPARE(model->itemAt(model->rowForUuid(t4.uuid))->positionMs, qint64(3500));

  manager.stop();
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(manager.activePlayerIndex(), -1);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Tail);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
  QCOMPARE(fakes[0]->fadeOutCalls, 1);
  QCOMPARE(fakes[0]->lastFadeOutDuration, qint64(1500));
  QCOMPARE(fakes[0]->currentMetadata().fadeOutStartPoint, qint64(3500));
  QCOMPARE(fakes[0]->currentMetadata().endPoint, qint64(5000));
  for (int i = 1; i < 3; ++i) {
    QCOMPARE(fakes[i]->status(), PlayerStatus::Idle);
    QCOMPARE(manager.slotRole(i), PlaylistManager::SlotRole::Free);
  }
  QCOMPARE(model->itemAt(model->rowForUuid(t1.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(t2.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(t3.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(t4.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(t5.uuid))->status,
           PlaylistItemStatus::Next);

  // Pressing Play while P0 is still fading out starts T5 cleanly on a Free slot (P1)
  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(manager.slotItemUuid(manager.activePlayerIndex()), t5.uuid);
  QCOMPARE(manager.activePlayerIndex(), 1);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);

  // Finish P0 stop fade-out tail -> P0 becomes Free
  fakes[0]->advanceTo(5000);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Idle);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Free);
}

void TestPlaylistManager::testModelReorderUpdatesPreloads() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("12:00", "Dynamic Edit"));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto t1 =
      makeMediaItem("T1", "C:/audio/1.mp3", 100000, 0, 100000, 0, 95000);
  const auto shortJingle =
      makeMediaItem("Jingle", "C:/audio/j.mp3", 2000, 0, 2000, 0, 1500, 0,
                    1500, MediaType::Jingle);
  const auto longSong =
      makeMediaItem("Long Song", "C:/audio/long.mp3", 180000, 0, 180000, 0,
                    175000);
  const auto t4 =
      makeMediaItem("T4", "C:/audio/4.mp3", 180000, 0, 180000, 0, 175000);

  model->addItem(t1);          // row 2
  model->addItem(shortJingle); // row 3
  model->addItem(longSong);    // row 4
  model->addItem(t4);          // row 5
  model->addItem(makeEndMarker(BlockType::Music));

  manager.play();
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(1), shortJingle.uuid);
  // Short jingle at i+1 triggers PreloadExt on P2 for longSong
  QCOMPARE(manager.slotRole(2), PlaylistManager::SlotRole::PreloadExt);
  QCOMPARE(manager.slotItemUuid(2), longSong.uuid);

  // Move Long Song (row 4) above Short Jingle (row 3)
  QVERIFY(model->moveItem(4, 3));
  // P1 must immediately reload Long Song, and P2 must unload to save RAM!
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(1), longSong.uuid);
  QCOMPARE(manager.slotRole(2), PlaylistManager::SlotRole::Free);
  QCOMPARE(fakes[2]->status(), PlayerStatus::Idle);
}

void TestPlaylistManager::testProgressBarPositionRoleUpdates() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("09:00", "Progress"));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto t1 =
      makeMediaItem("Track", "C:/audio/1.mp3", 30000, 0, 30000, 0, 30000);
  model->addItem(t1);
  model->addItem(makeEndMarker(BlockType::Music));

  manager.play();
  const int row = model->rowForUuid(t1.uuid);

  QSignalSpy dataSpy(model, &QAbstractItemModel::dataChanged);
  fakes[0]->advanceTo(4321);

  QCOMPARE(model->data(model->index(row, 0), PositionRole).toLongLong(),
           qint64(4321));
  QVERIFY(dataSpy.count() >= 1);
  const auto roles = dataSpy.last().at(2).value<QList<int>>();
  QCOMPARE(roles, QList<int>{PositionRole});
}

void TestPlaylistManager::testMissingFileErrorSkipsGracefully() {
  FakeAudioRegistry registry;
  registry.missingPaths.insert("C:/audio/missing1.mp3");
  registry.missingPaths.insert("C:/audio/missing2.mp3");

  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("08:00", "Recovery"));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto bad1 = makeMediaItem("Bad 1", "C:/audio/missing1.mp3", 10000);
  const auto bad2 = makeMediaItem("Bad 2", "C:/audio/missing2.mp3", 10000);
  const auto good =
      makeMediaItem("Good", "C:/audio/good.mp3", 15000, 0, 15000, 0, 15000);
  model->addItem(bad1);
  model->addItem(bad2);
  model->addItem(good);
  model->addItem(makeEndMarker(BlockType::Music));

  QSignalSpy errSpy(&manager, &PlaylistManager::playbackError);
  manager.play();

  QCOMPARE(errSpy.count(), 2);
  QCOMPARE(errSpy.at(0).at(1).toString(), QString("C:/audio/missing1.mp3"));
  QCOMPARE(errSpy.at(1).at(1).toString(), QString("C:/audio/missing2.mp3"));
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(manager.slotItemUuid(manager.activePlayerIndex()), good.uuid);
  QCOMPARE(model->itemAt(model->rowForUuid(bad1.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(bad2.uuid))->status,
           PlaylistItemStatus::Finished);

  // Now test playlist with only missing tracks -> cleanly stops + playlistFinished
  model->clear();
  model->addItem(makeBlockMarker("08:30", "All Broken"));
  model->addItem(makeStartMarker(BlockType::Music));
  model->addItem(makeMediaItem("Bad Only", "C:/audio/missing1.mp3", 10000));
  model->addItem(makeEndMarker(BlockType::Music));

  QSignalSpy finishedSpy(&manager, &PlaylistManager::playlistFinished);
  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(finishedSpy.count(), 1);
}

void TestPlaylistManager::testPlaylistListModelEstimatedTimeWithMixPoints() {
  PlaylistListModel model;
  model.clear();

  model.addItem(makeBlockMarker("14:00", "Mix Schedule"));
  model.addItem(makeStartMarker(BlockType::Music));
  // Track 1: duration 200s, start=0, mixEnd=195s
  const auto t1 =
      makeMediaItem("T1", "C:/1.mp3", 200000, 0, 200000, 0, 195000);
  // Track 2: duration 180s, start=0, mixStart=2s, mixEnd=176s
  // Starts at 14:00:00 + (195000 - 2000) = 14:00:00 + 193s = 14:03:13
  const auto t2 =
      makeMediaItem("T2", "C:/2.mp3", 180000, 0, 180000, 2000, 176000);
  // Track 3: duration 100s, start=1000, mixStart=4000 (intro=3s)
  // Starts at 14:03:13 + (176000 - 0 - 3000) = 14:03:13 + 173s = 14:06:06
  const auto t3 =
      makeMediaItem("T3", "C:/3.mp3", 100000, 1000, 100000, 4000, 100000);

  model.addItem(t1);
  model.addItem(t2);
  model.addItem(t3);
  model.addItem(makeEndMarker(BlockType::Music));

  QCOMPARE(model.itemAt(model.rowForUuid(t1.uuid))->estimatedTime,
           QString("14:00:00"));
  QCOMPARE(model.itemAt(model.rowForUuid(t2.uuid))->estimatedTime,
           QString("14:03:13"));
  QCOMPARE(model.itemAt(model.rowForUuid(t3.uuid))->estimatedTime,
           QString("14:06:06"));
}

void TestPlaylistManager::testInsertCatalogItemAndMarkerBoundaries() {
  PlaylistListModel model;
  model.clear();

  // Row 0: BlockMarker("14:00"), Row 1: Start(Comm), Row 2: End(Comm),
  // Row 3: Start(Music), Row 4: Media(M1), Row 5: End(Music)
  model.addItem(makeBlockMarker("14:00", "Afternoon"));
  model.addItem(makeStartMarker(BlockType::Commercial));
  model.addItem(makeEndMarker(BlockType::Commercial));
  model.addItem(makeStartMarker(BlockType::Music));
  const auto m1 =
      makeMediaItem("M1", "C:/audio/m1.mp3", 10000, 0, 10000, 0, 10000);
  model.addItem(m1);
  model.addItem(makeEndMarker(BlockType::Music));

  // Invalid boundaries: outside markers or at block/section boundaries
  QVERIFY(!model.insertCatalogItem(0, "Invalid", "C:/a.mp3", MediaType::Music));
  QVERIFY(!model.insertCatalogItem(1, "Invalid", "C:/a.mp3", MediaType::Music));
  QVERIFY(!model.insertCatalogItem(3, "Invalid", "C:/a.mp3", MediaType::Music));
  QVERIFY(!model.insertCatalogItem(model.rowCount(), "Invalid", "C:/a.mp3",
                                   MediaType::Music));

  // Valid insertion into empty commercial section (row 2 = EndMarker Commercial)
  QVERIFY(model.insertCatalogItem(2, "Spot", "C:/audio/spot.mp3",
                                  MediaType::Commercial, 15000));
  QCOMPARE(model.itemAt(2)->title, QString("Spot"));
  QCOMPARE(model.itemAt(2)->blockType, BlockType::Commercial);
  QCOMPARE(model.itemAt(2)->mediaType, MediaType::Commercial);
  QCOMPARE(model.itemAt(3)->itemsCount, 1);
  QCOMPARE(model.itemAt(3)->totalDurationMs, qint64(15000));

  // Valid insertion into music section (before M1 at row 5)
  QVERIFY(model.insertCatalogItem(5, "Intro Jingle", "C:/audio/jingle.mp3",
                                  MediaType::Jingle, 5000));
  QCOMPARE(model.itemAt(5)->blockType, BlockType::Music);
  QCOMPARE(model.itemAt(5)->estimatedTime, QString("14:00:15"));
  QCOMPARE(model.itemAt(6)->estimatedTime, QString("14:00:20"));

  // Lock row at M1 (row 6) as Playing -> cannot insert at or before row 6
  model.setPlayingRow(6);
  QCOMPARE(model.lastLockedRow(), 6);
  QVERIFY(!model.insertCatalogItem(2, "Locked Comm", "C:/audio/c.mp3",
                                   MediaType::Commercial, 5000));
  QVERIFY(!model.insertCatalogItem(6, "Before Playing", "C:/audio/m0.mp3",
                                   MediaType::Music, 5000));
  // Inserting right after Playing (row 7) succeeds and becomes Next
  QVERIFY(model.insertCatalogItem(7, "After Playing", "C:/audio/m2.mp3",
                                  MediaType::Music, 8000));
  QCOMPARE(model.itemAt(7)->status, PlaylistItemStatus::Next);
}

void TestPlaylistManager::testRealAudioDurationProbeFromWavFile() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString wavPath = tempDir.filePath("probe_sample.wav");
  QVERIFY(writeTestWavFile(wavPath, 2400));

  PlaylistListModel model;
  model.clear();

  model.addItem(makeBlockMarker("16:00", "Probe Test"));
  model.addItem(makeStartMarker(BlockType::Music));
  const auto trailing = makeMediaItem("Trailing", "C:/audio/trailing.mp3",
                                      10000, 0, 10000, 0, 10000);
  model.addItem(trailing);
  model.addItem(makeEndMarker(BlockType::Music));

  QSignalSpy durationSpy(&model, &PlaylistListModel::itemDurationLoaded);

  QVERIFY(model.insertCatalogItem(2, "Real Wav", wavPath, MediaType::Music));
  QCOMPARE(model.itemAt(2)->durationMs, qint64(0));

  QTRY_COMPARE_WITH_TIMEOUT(durationSpy.count(), 1, 5000);

  const qint64 loadedDuration = durationSpy.at(0).at(2).toLongLong();
  QVERIFY2(qAbs(loadedDuration - 2400) <= 50,
           qPrintable(QString("Unexpected WAV duration: %1").arg(loadedDuration)));

  const PlaylistItem *probedItem = model.itemAt(2);
  QVERIFY(probedItem != nullptr);
  QCOMPARE(probedItem->durationMs, loadedDuration);
  QCOMPARE(probedItem->endPoint, loadedDuration);
  QCOMPARE(probedItem->mixEnd, loadedDuration);
  QCOMPARE(probedItem->fadeOutStartPoint, loadedDuration);
  QCOMPARE(probedItem->estimatedTime, QString("16:00:00"));

  const PlaylistItem *afterItem = model.itemAt(3);
  QVERIFY(afterItem != nullptr);
  QCOMPARE(afterItem->estimatedTime,
           QTime(16, 0, 0).addMSecs(static_cast<int>(loadedDuration)).toString("hh:mm:ss"));

  // EndMarker (row 4) and BlockMarker (row 0) totals updated
  QCOMPARE(model.itemAt(4)->itemsCount, 2);
  QCOMPARE(model.itemAt(4)->totalDurationMs, loadedDuration + 10000);
  QCOMPARE(model.itemAt(0)->totalDurationMs, loadedDuration + 10000);
}

void TestPlaylistManager::testRemoveFutureItemsAndLockProtection() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("10:00", "Removal Test"));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto t1 =
      makeMediaItem("T1", "C:/audio/1.mp3", 10000, 0, 10000, 0, 9000);
  const auto t2 =
      makeMediaItem("T2", "C:/audio/2.mp3", 15000, 0, 15000, 0, 14000);
  const auto t3 =
      makeMediaItem("T3", "C:/audio/3.mp3", 20000, 0, 20000, 0, 19000);
  const auto t4 =
      makeMediaItem("T4", "C:/audio/4.mp3", 12000, 0, 12000, 0, 11000);
  model->addItem(t1); // row 2
  model->addItem(t2); // row 3
  model->addItem(t3); // row 4
  model->addItem(t4); // row 5
  model->addItem(makeEndMarker(BlockType::Music)); // row 6

  // Markers cannot be removed
  QVERIFY(!model->canRemoveRow(0));
  QVERIFY(!model->canRemoveRow(1));
  QVERIFY(!model->canRemoveRow(6));
  QVERIFY(!model->removeItem(0));

  manager.play();
  QCOMPARE(manager.slotItemUuid(1), t2.uuid);

  // Playing item (t1 at row 2) is locked
  QVERIFY(!model->canRemoveRow(2));
  QVERIFY(!model->removeItem(2));

  // Future Next item (t2 at row 3) CAN be removed; t3 upgrades to Next & PreloadNext
  QVERIFY(model->canRemoveRow(3));
  QVERIFY(model->removeItem(3));
  QCOMPARE(model->rowForUuid(t2.uuid), -1);
  const int t3Row = model->rowForUuid(t3.uuid);
  QCOMPARE(t3Row, 3);
  QCOMPARE(model->itemAt(t3Row)->status, PlaylistItemStatus::Next);
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::PreloadNext);
  QCOMPARE(manager.slotItemUuid(1), t3.uuid);
  QCOMPARE(model->itemAt(5)->itemsCount, 3);
  // Effective total with mixEnd points: T1(9000) + T3(19000) + T4(11000) = 39000
  QCOMPARE(model->itemAt(5)->totalDurationMs, qint64(39000));

  // Advance T1 to finish -> T3 plays, T1 is Finished
  fakes[0]->advanceTo(9000);
  QCOMPARE(manager.slotItemUuid(manager.activePlayerIndex()), t3.uuid);

  // Stop playlist -> T1 and T3 remain Finished (locked), T4 advances to Next (unlocked)
  manager.stop();
  QCOMPARE(model->itemAt(model->rowForUuid(t1.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(t3.uuid))->status,
           PlaylistItemStatus::Finished);
  QVERIFY(!model->canRemoveRow(model->rowForUuid(t1.uuid)));
  QVERIFY(!model->removeItem(model->rowForUuid(t1.uuid)));
  QVERIFY(!model->canRemoveRow(model->rowForUuid(t3.uuid)));
  QVERIFY(!model->removeItem(model->rowForUuid(t3.uuid)));

  const int stoppedT4Row = model->rowForUuid(t4.uuid);
  QCOMPARE(model->itemAt(stoppedT4Row)->status, PlaylistItemStatus::Next);
  QVERIFY(model->canRemoveRow(stoppedT4Row));
  QVERIFY(model->removeItem(stoppedT4Row));
  QCOMPARE(model->itemAt(4)->itemsCount, 2);
}

void TestPlaylistManager::testPlaylistViewCatalogDropAndDeleteKey() {
  PlaylistListModel model;
  model.clear();
  model.setDurationResolver([](const QString &) -> qint64 { return 12000; });

  model.addItem(makeBlockMarker("18:00", "View Drop"));
  model.addItem(makeStartMarker(BlockType::Music));
  const auto t1 =
      makeMediaItem("Playing Song", "C:/audio/playing.mp3", 20000, 0, 20000);
  const auto t2 =
      makeMediaItem("Future Song", "C:/audio/future.mp3", 18000, 0, 18000);
  model.addItem(t1); // row 2
  model.addItem(t2); // row 3
  model.addItem(makeEndMarker(BlockType::Music)); // row 4
  model.setPlayingRow(2);

  PlaylistView view;
  view.setModel(&model);
  view.resize(400, 400);
  view.doItemsLayout();

  CatalogListModel catalogModel;
  CatalogItem catItem;
  catItem.itemType = CatalogItemType::File;
  catItem.title = QStringLiteral("Catalog Track.mp3");
  catItem.path = QStringLiteral("C:/audio/catalog_track.mp3");
  catItem.mediaType = MediaType::Jingle;
  catalogModel.setItems({catItem});

  std::unique_ptr<QMimeData> mime(
      catalogModel.mimeData({catalogModel.index(0, 0)}));
  QVERIFY(mime != nullptr);
  QVERIFY(mime->hasFormat(QStringLiteral("application/x-catalog-item")));

  // Attempt drop onto locked Playing item top half (targetRow = 2) -> rejected
  const QRect playingRect = view.visualRect(model.index(2, 0));
  QVERIFY(playingRect.isValid());
  const QPoint lockedPos(playingRect.center().x(), playingRect.top() + 2);
  QDragEnterEvent lockedEnter(lockedPos, Qt::CopyAction, mime.get(),
                              Qt::LeftButton, Qt::NoModifier);
  QCoreApplication::sendEvent(view.viewport(), &lockedEnter);
  QDragMoveEvent lockedMove(lockedPos, Qt::CopyAction, mime.get(),
                            Qt::LeftButton, Qt::NoModifier);
  QCoreApplication::sendEvent(view.viewport(), &lockedMove);
  QVERIFY(!lockedMove.isAccepted());

  // Drop onto top half of EndMarker (row 4) -> inserts at row 4
  const QRect endRect = view.visualRect(model.index(4, 0));
  QVERIFY(endRect.isValid());
  const QPoint validPos(endRect.center().x(), endRect.top() + 2);

  QDragEnterEvent enterEvt(validPos, Qt::CopyAction, mime.get(),
                           Qt::LeftButton, Qt::NoModifier);
  QCoreApplication::sendEvent(view.viewport(), &enterEvt);
  QVERIFY(enterEvt.isAccepted());

  QDragMoveEvent moveEvt(validPos, Qt::CopyAction, mime.get(), Qt::LeftButton,
                         Qt::NoModifier);
  QCoreApplication::sendEvent(view.viewport(), &moveEvt);
  QVERIFY(moveEvt.isAccepted());

  QDropEvent dropEvt(validPos, Qt::CopyAction, mime.get(), Qt::LeftButton,
                     Qt::NoModifier);
  QCoreApplication::sendEvent(view.viewport(), &dropEvt);
  QVERIFY(dropEvt.isAccepted());

  QCOMPARE(model.rowCount(), 6);
  QCOMPARE(model.itemAt(4)->title, QString("Catalog Track.mp3"));
  QCOMPARE(model.itemAt(4)->mediaType, MediaType::Jingle);
  QCOMPARE(model.itemAt(4)->blockType, BlockType::Music);
  QCOMPARE(model.itemAt(4)->durationMs, qint64(12000));

  // Delete key on selected dropped item removes it
  QCOMPARE(view.currentIndex().row(), 4);
  QTest::keyClick(&view, Qt::Key_Delete);
  QCOMPARE(model.rowCount(), 5);

  // Delete key on locked Playing item (row 2) does NOT remove it
  view.setCurrentIndex(model.index(2, 0));
  QTest::keyClick(&view, Qt::Key_Delete);
  QCOMPARE(model.rowCount(), 5);
  QCOMPARE(model.itemAt(2)->uuid, t1.uuid);
}

void TestPlaylistManager::testStopFadeOutAndRemainingAudioClamp() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});
  manager.setStopFadeMs(1500);

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("20:00", "Stop Fade"));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto track1 =
      makeMediaItem("Track 1", "C:/audio/s1.mp3", 10000, 0, 10000, 0, 9000);
  const auto track2 =
      makeMediaItem("Track 2", "C:/audio/s2.mp3", 10000, 0, 10000, 0, 10000);
  const auto track3 =
      makeMediaItem("Track 3", "C:/audio/s3.mp3", 10000, 0, 10000, 0, 9000);
  model->addItem(track1);
  model->addItem(track2);
  model->addItem(track3);
  model->addItem(makeEndMarker(BlockType::Music));

  // Case 1: Remaining audio (7000ms) > stopFadeMs (1500ms)
  manager.play();
  QCOMPARE(manager.activePlayerIndex(), 0);
  fakes[0]->advanceTo(3000);

  manager.stop();
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(model->itemAt(model->rowForUuid(track1.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(track2.uuid))->status,
           PlaylistItemStatus::Next);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Tail);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
  QCOMPARE(fakes[0]->currentMetadata().fadeOutStartPoint, qint64(3000));
  QCOMPARE(fakes[0]->currentMetadata().endPoint, qint64(4500));

  fakes[0]->advanceTo(4500);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Idle);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Free);

  // Case 2: Remaining audio (800ms) < stopFadeMs (1500ms) -> clamp endPoint to audio end (10000ms)
  manager.play();
  const int track2Slot = manager.activePlayerIndex();
  QCOMPARE(manager.slotItemUuid(track2Slot), track2.uuid);
  fakes[track2Slot]->advanceTo(9200);

  manager.stop();
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(model->itemAt(model->rowForUuid(track2.uuid))->status,
           PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(model->rowForUuid(track3.uuid))->status,
           PlaylistItemStatus::Next);
  QCOMPARE(manager.slotRole(track2Slot), PlaylistManager::SlotRole::Tail);
  QCOMPARE(fakes[track2Slot]->status(), PlayerStatus::Playing);
  QCOMPARE(fakes[track2Slot]->currentMetadata().fadeOutStartPoint,
           qint64(9200));
  QCOMPARE(fakes[track2Slot]->currentMetadata().endPoint, qint64(10000));

  fakes[track2Slot]->advanceTo(10000);
  QCOMPARE(fakes[track2Slot]->status(), PlayerStatus::Idle);
  QCOMPARE(manager.slotRole(track2Slot), PlaylistManager::SlotRole::Free);
}

void TestPlaylistManager::testPlaylistViewRowHeightUpdatesOnStopAndFinish() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {
      new FakePlayerManager(&registry), new FakePlayerManager(&registry),
      new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker("21:00", "Layout Height Test"));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto t1 =
      makeMediaItem("Track 1", "C:/audio/1.mp3", 10000, 0, 10000, 0, 10000);
  const auto t2 =
      makeMediaItem("Track 2", "C:/audio/2.mp3", 10000, 0, 10000, 0, 10000);
  model->addItem(t1); // row 2
  model->addItem(t2); // row 3
  model->addItem(makeEndMarker(BlockType::Music)); // row 4

  PlaylistView view;
  PlaylistItemDelegate delegate;
  view.setModel(model);
  view.setItemDelegate(&delegate);
  view.resize(500, 500);
  view.doItemsLayout();

  // Start Track 1 (row 2): height should be 50px, row 3 right below it
  manager.play();
  QCoreApplication::processEvents();
  const QRect playingRect1 = view.visualRect(model->index(2, 0));
  const QRect nextRectBefore = view.visualRect(model->index(3, 0));
  QCOMPARE(playingRect1.height(), 50);
  QCOMPARE(nextRectBefore.top(), playingRect1.bottom() + 1);

  // Stop Track 1 -> row 2 becomes Finished (32px), row 3 shifts up cleanly with no gap
  manager.stop();
  QCoreApplication::processEvents();
  const QRect finishedRect1 = view.visualRect(model->index(2, 0));
  const QRect nextRectAfter = view.visualRect(model->index(3, 0));
  QCOMPARE(finishedRect1.height(), 32);
  QCOMPARE(nextRectAfter.top(), finishedRect1.bottom() + 1);

  // Play last track (Track 2 at row 3) then Stop -> row 3 shrinks from 50px to 32px and EndMarker (row 4) shifts up
  manager.play();
  QCoreApplication::processEvents();
  const QRect playingRect2 = view.visualRect(model->index(3, 0));
  const QRect endBefore = view.visualRect(model->index(4, 0));
  QCOMPARE(playingRect2.height(), 50);
  QCOMPARE(endBefore.top(), playingRect2.bottom() + 1);

  manager.stop();
  QCoreApplication::processEvents();
  const QRect finishedRect2 = view.visualRect(model->index(3, 0));
  const QRect endAfter = view.visualRect(model->index(4, 0));
  QCOMPARE(finishedRect2.height(), 32);
  QCOMPARE(endAfter.top(), finishedRect2.bottom() + 1);
}

void TestPlaylistManager::testTriggerFadeOutDuringActiveFadeOutDoesNotJumpVolume() {
  FakeAudioRegistry registry;
  FakePlayerManager player(&registry);

  PlayerMetadata meta;
  meta.duration = 10000;
  meta.startPoint = 0;
  meta.endPoint = 10000;
  meta.fadeInEndPoint = 0;
  meta.fadeOutStartPoint = 6000; // 4s fade-out

  player.load(QUrl::fromLocalFile("C:/audio/fade.mp3"), meta, true);
  QCOMPARE(player.status(), PlayerStatus::Playing);

  // Mid-fade-out at 8000ms (progress = 0.5 -> sin(pi/4) ~= 0.7071)
  player.advanceTo(8000);
  const float volBefore = player.currentVolume();
  QVERIFY(volBefore > 0.65f && volBefore < 0.75f);

  // Shorten fade-out to 500ms (ends at 8500ms): must NOT jump back to 1.0!
  player.triggerFadeOut(500);
  const float volAtTrigger = player.currentVolume();
  QVERIFY2(volAtTrigger <= volBefore + 0.003f,
           qPrintable(QString("Volume jumped from %1 to %2")
                          .arg(volBefore)
                          .arg(volAtTrigger)));
  QCOMPARE(player.currentMetadata().endPoint, qint64(8500));

  // Halfway through shortened fade-out (8250ms): monotonically decreasing
  player.advanceTo(8250);
  const float volMid = player.currentVolume();
  QVERIFY(volMid > 0.0f && volMid < volAtTrigger);

  // Calling triggerFadeOut with a longer duration (2000ms) while only 250ms remain
  // must keep existing shorter fade-out without jumping volume
  player.triggerFadeOut(2000);
  QCOMPARE(player.currentMetadata().endPoint, qint64(8500));
  QVERIFY(player.currentVolume() <= volMid + 0.003f);

  // Reach endPoint -> stops cleanly at 0.0
  player.advanceTo(8500);
  QCOMPARE(player.status(), PlayerStatus::End);
  QCOMPARE(player.currentVolume(), 0.0f);
}

void TestPlaylistManager::testConsecutiveDuplicateFileProbeUsesCacheAndDoesNotTimeout() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString wavPath = tempDir.filePath("dup_probe.wav");
  QVERIFY(writeTestWavFile(wavPath, 1000));

  PlaylistListModel model;
  model.clear();
  model.addItem(makeBlockMarker("22:00", "Dup Probe"));
  model.addItem(makeStartMarker(BlockType::Music));
  model.addItem(makeEndMarker(BlockType::Music));

  // Drop the exact same file twice back-to-back before the first probe finishes
  QVERIFY(model.insertCatalogItem(2, "Dup 1", wavPath, MediaType::Jingle, 0));
  QVERIFY(model.insertCatalogItem(3, "Dup 2", wavPath, MediaType::Jingle, 0));

  // Both must resolve quickly (well under the 5000ms probe timeout)
  QTRY_VERIFY_WITH_TIMEOUT(
      model.itemAt(2)->durationMs >= 800 && model.itemAt(3)->durationMs >= 800,
      4000);

  // A third drop of the same file must resolve synchronously from cache!
  QVERIFY(model.insertCatalogItem(4, "Dup 3", wavPath, MediaType::Jingle, 0));
  QVERIFY(model.itemAt(4)->durationMs >= 800);
}

void TestPlaylistManager::testRecalculateBlockMetadataEmitsExactRoles() {
  PlaylistListModel model;
  model.clear();
  model.addItem(makeBlockMarker("23:00", "Roles Test"));
  model.addItem(makeStartMarker(BlockType::Music));
  model.addItem(makeMediaItem("A", "C:/audio/a.mp3", 10000));
  model.addItem(makeMediaItem("B", "C:/audio/b.mp3", 10000));
  model.addItem(makeEndMarker(BlockType::Music));

  QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);

  // Updating duration of row 2 shifts estimatedTime of row 3 & totalDuration of EndMarker/BlockMarker,
  // but does NOT change any item's StatusRole!
  model.updateItemAudioMetadata(2, 20000, 0, 20000, 0, 20000, 0, 20000);

  QVERIFY(dataSpy.count() >= 1);
  for (int i = 0; i < dataSpy.count(); ++i) {
    const QList<int> roles = dataSpy.at(i).at(2).value<QList<int>>();
    QVERIFY2(!roles.isEmpty(),
             "dataChanged must specify exact roles instead of empty list");
    QVERIFY2(!roles.contains(StatusRole),
             "StatusRole should not be emitted when only schedule/duration changed");
  }
}

QTEST_MAIN(TestPlaylistManager)
#include "test_playlist_manager.moc"
