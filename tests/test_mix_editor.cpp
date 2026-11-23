#include <QAction>
#include <QDataStream>
#include <QFile>
#include <QHash>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSet>
#include <QSignalSpy>
#include <QSlider>
#include <QTemporaryDir>
#include <QTimeEdit>
#include <QtTest>

#include <array>
#include <cmath>

#include "core/audio/WaveformExtractor.h"
#include "core/catalog/CatalogItem.h"
#include "core/catalog/CatalogListModel.h"
#include "core/models.h"
#include "core/playlist/PlaylistListModel.h"
#include "core/playlist/PlaylistManager.h"
#include "core/playlist/player/PlayerManager.h"
#include "ui/widgets/catalog/CatalogWidget.h"
#include "ui/widgets/editor/MixEditor.h"
#include "ui/widgets/editor/WaveformWidget.h"
#include "ui/widgets/playlist/PlaylistView.h"

namespace {
constexpr double kTwoPi = 6.28318530717958647692;

class TestablePlayerManager : public PlayerManager {
public:
  explicit TestablePlayerManager(QObject *parent = nullptr)
      : PlayerManager(false, parent) {}

  void load(const QUrl &source, const PlayerMetadata &newMetadata,
            bool autoPlay = false) override {
    setStatus(PlayerStatus::Loading);
    m_source = source;
    metadata = newMetadata;
    m_pendingPlay = autoPlay;
    normalizeMetadata(metadata.duration);
    m_pos = metadata.startPoint;
    applyVolume(0.0f);
    if (!deferLoad) {
      finishLoad();
    }
  }

  void finishLoad() {
    normalizeMetadata(metadata.duration);
    qint64 initial = (m_pendingSeekPos >= 0) ? m_pendingSeekPos : metadata.startPoint;
    if (metadata.endPoint > 0) {
      initial = qBound<qint64>(metadata.startPoint, initial, metadata.endPoint);
      if (initial >= metadata.endPoint) {
        initial = metadata.startPoint;
      }
    }
    m_pos = initial;
    applyVolumeForPosition(m_pos);
    setStatus(PlayerStatus::Loaded);
    if (m_pendingPlay) {
      const qint64 resumePos = m_pendingSeekPos;
      m_pendingPlay = false;
      m_pendingSeekPos = -1;
      if (resumePos >= 0) {
        playFrom(resumePos);
      } else {
        play();
      }
    } else {
      m_pendingSeekPos = -1;
    }
  }

  void play() override {
    if (status() == PlayerStatus::Loading) {
      m_pendingPlay = true;
      return;
    }
    if (status() == PlayerStatus::Playing || status() == PlayerStatus::Idle ||
        status() == PlayerStatus::Error) {
      return;
    }
    if (status() == PlayerStatus::Loaded || status() == PlayerStatus::End) {
      m_pos = metadata.startPoint;
    }
    applyVolumeForPosition(m_pos);
    setStatus(PlayerStatus::Playing);
  }

  void pause() override {
    if (status() == PlayerStatus::Playing) {
      setStatus(PlayerStatus::Paused);
    }
  }

  void stop() override {
    m_pendingPlay = false;
    m_pendingSeekPos = -1;
    applyVolume(0.0f);
    m_pos = metadata.endPoint;
    setStatus(PlayerStatus::End);
  }

  void seek(qint64 positionMs) override {
    const qint64 dur = duration();
    qint64 clamped = qMax<qint64>(0, positionMs);
    if (dur > 0) {
      clamped = qBound<qint64>(0, clamped, dur);
    }
    if (status() == PlayerStatus::Loading) {
      m_pendingSeekPos = clamped;
      return;
    }
    m_pos = clamped;
    applyVolumeForPosition(m_pos);
    emit positionChanged(m_pos);
  }

  void playFrom(qint64 positionMs) override {
    qint64 target = qMax<qint64>(metadata.startPoint, positionMs);
    if (metadata.endPoint > 0 && target >= metadata.endPoint) {
      target = metadata.startPoint;
    }
    if (status() == PlayerStatus::Loading) {
      m_pendingSeekPos = target;
      m_pendingPlay = true;
      return;
    }
    m_pos = target;
    applyVolumeForPosition(m_pos);
    if (status() != PlayerStatus::Playing) {
      setStatus(PlayerStatus::Playing);
    }
    emit positionChanged(m_pos);
  }

  void setMetadata(const PlayerMetadata &newMetadata) override {
    metadata = newMetadata;
    normalizeMetadata(metadata.duration);
    applyVolumeForPosition(m_pos);
  }

  qint64 position() const override { return m_pos; }
  qint64 duration() const override { return metadata.duration; }

  void advanceTo(qint64 posMs) {
    m_pos = posMs;
    volumeControl(posMs);
  }

  bool deferLoad = false;

private:
  qint64 m_pos = 0;
};

bool writeHalfSilenceHalfSineWav(const QString &filePath, int durationMs = 1000) {
  constexpr quint32 sampleRate = 8000;
  constexpr quint16 channels = 1;
  constexpr quint16 bitsPerSample = 16;
  const quint32 totalSamples =
      static_cast<quint32>((static_cast<quint64>(sampleRate) * durationMs) /
                           1000ULL);
  const quint32 halfSamples = totalSamples / 2;
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

  for (quint32 i = 0; i < totalSamples; ++i) {
    qint16 sample = 0;
    if (i >= halfSamples) {
      const double t = static_cast<double>(i) / static_cast<double>(sampleRate);
      sample = static_cast<qint16>(std::llround(28000.0 * std::sin(kTwoPi * 440.0 * t)));
    }
    out << sample;
  }

  file.close();
  return true;
}

PlaylistItem makeMedia(const QString &title, const QString &path,
                       qint64 durationMs) {
  PlaylistItem item;
  item.uuid = QUuid::createUuid();
  item.type = ItemType::Media;
  item.mediaType = MediaType::Music;
  item.title = title;
  item.path = path;
  item.durationMs = durationMs;
  item.startPoint = 0;
  item.endPoint = durationMs;
  item.mixStart = 0;
  item.mixEnd = durationMs;
  item.fadeInEndPoint = 0;
  item.fadeOutStartPoint = durationMs;
  return item;
}

PlaylistItem makeMarker(ItemType type, BlockType bType, const QString &title = {}) {
  PlaylistItem item;
  item.uuid = QUuid::createUuid();
  item.type = type;
  item.blockType = bType;
  item.mediaType = MediaType::Other;
  item.title = title;
  return item;
}
} // namespace

class TestMixEditor : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void testPlayerVolumeEnvelopeAndFades();
  void testPlayerTriggerFadeOutWithOutroEnvelope();
  void testPlayerSeekAndPlayFrom();
  void testWaveformExtractorMissingAndValidWav();
  void testWaveformExtractorCancel();
  void testPlaylistModelAndManagerLiveMixUpdate();
  void testMixEditorLoadEditAndSaveByUuidAfterShift();
  void testMixEditorRemovedOrFinishedItemDoesNotCrash();
  void testWaveformWidgetCoordinateMappingAndZoom();
  void testPlaylistViewContextMenu();
  void testCatalogWidgetContextMenu();
};

void TestMixEditor::initTestCase() {
  qRegisterMetaType<PlayerStatus>("PlayerStatus");
  qRegisterMetaType<PlayerMetadata>("PlayerMetadata");
  qRegisterMetaType<WaveformSample>("WaveformSample");
  qRegisterMetaType<QVector<WaveformSample>>("QVector<WaveformSample>");
}

void TestMixEditor::testPlayerVolumeEnvelopeAndFades() {
  PlayerMetadata meta;
  meta.duration = 20000;
  meta.startPoint = 1000;
  meta.fadeInEndPoint = 3000;   // 2s equal-power fade-in
  meta.fadeOutStartPoint = 17000;
  meta.endPoint = 19000;        // 2s equal-power fade-out

  // Default envelope (startVolume = 1.0, endVolume = 1.0)
  QCOMPARE(PlayerManager::computeVolumeFactor(meta, 500), 0.0f);
  QCOMPARE(PlayerManager::computeVolumeFactor(meta, 1000), 0.0f);
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 2000) -
               std::sin(0.5f * 1.5707963267948966f)) < 0.005f);
  QCOMPARE(PlayerManager::computeVolumeFactor(meta, 5000), 1.0f);
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 18000) -
               std::sin(0.5f * 1.5707963267948966f)) < 0.005f);
  QCOMPARE(PlayerManager::computeVolumeFactor(meta, 19000), 0.0f);

  // Intro ducking (0.4) + ramp-up [5000..7000]
  meta.startVolume = 0.4f;
  meta.volumeUpStartPoint = 5000;
  meta.volumeUpEndPoint = 7000;
  QCOMPARE(PlayerManager::computeVolumeFactor(meta, 1000), 0.0f);
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 3000) - 0.4f) < 0.005f);
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 4000) - 0.4f) < 0.005f);
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 6000) - 0.7f) < 0.005f);
  QCOMPARE(PlayerManager::computeVolumeFactor(meta, 7000), 1.0f);
  QCOMPARE(PlayerManager::computeVolumeFactor(meta, 10000), 1.0f);

  // Outro ramp-down [13000..15000] + ducking (0.25) -> fade-out [17000..19000]
  meta.endVolume = 0.25f;
  meta.volumeDownStartPoint = 13000;
  meta.volumeDownEndPoint = 15000;
  QCOMPARE(PlayerManager::computeVolumeFactor(meta, 12500), 1.0f);
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 14000) - 0.625f) <
          0.005f);
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 16000) - 0.25f) < 0.005f);
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 17000) - 0.25f) < 0.005f);
  const float expectedMidFadeOut =
      std::sin(0.5f * 1.5707963267948966f) * 0.25f;
  QVERIFY(qAbs(PlayerManager::computeVolumeFactor(meta, 18000) -
               expectedMidFadeOut) < 0.005f);

  // Overlapping fade-in & fade-out on short audio preserves std::min behavior
  PlayerMetadata shortMeta;
  shortMeta.duration = 2000;
  shortMeta.startPoint = 0;
  shortMeta.endPoint = 2000;
  shortMeta.fadeInEndPoint = 1500;
  shortMeta.fadeOutStartPoint = 500;
  const float midVal = PlayerManager::computeVolumeFactor(shortMeta, 1000);
  const float expectedShortMid =
      std::sin((1000.0f / 1500.0f) * 1.5707963267948966f);
  QVERIFY(qAbs(midVal - expectedShortMid) < 0.005f);

  // Live PlayerManager applies the same curve
  TestablePlayerManager player;
  player.load(QUrl::fromLocalFile(QStringLiteral("dummy.mp3")), meta, true);
  player.advanceTo(4000);
  QVERIFY(qAbs(player.currentVolume() - 0.4f) < 0.01f);
  player.advanceTo(10000);
  QVERIFY(qAbs(player.currentVolume() - 1.0f) < 0.01f);
  player.advanceTo(16000);
  QVERIFY(qAbs(player.currentVolume() - 0.25f) < 0.01f);
}

void TestMixEditor::testPlayerTriggerFadeOutWithOutroEnvelope() {
  PlayerMetadata meta;
  meta.duration = 20000;
  meta.startPoint = 0;
  meta.endPoint = 20000;
  meta.fadeOutStartPoint = 18000;
  meta.endVolume = 0.2f;
  meta.volumeDownStartPoint = 14000;
  meta.volumeDownEndPoint = 16000;

  // Triggering during plateau resets outro ducking so volume doesn't jump to 0.2
  TestablePlayerManager p1;
  p1.load(QUrl::fromLocalFile(QStringLiteral("a.mp3")), meta, true);
  p1.advanceTo(8000);
  QVERIFY(qAbs(p1.currentVolume() - 1.0f) < 0.01f);
  p1.triggerFadeOut(1000);
  QVERIFY(qAbs(p1.currentVolume() - 1.0f) < 0.01f);
  p1.advanceTo(8500);
  QVERIFY(p1.currentVolume() > 0.65f && p1.currentVolume() < 0.8f);

  // Triggering while inside outro hold preserves 0.2 level and fades smoothly
  TestablePlayerManager p2;
  p2.load(QUrl::fromLocalFile(QStringLiteral("b.mp3")), meta, true);
  p2.advanceTo(16500);
  QVERIFY(qAbs(p2.currentVolume() - 0.2f) < 0.01f);
  p2.triggerFadeOut(1000);
  QVERIFY(qAbs(p2.currentVolume() - 0.2f) < 0.01f);
  p2.advanceTo(17000);
  QVERIFY(p2.currentVolume() < 0.16f && p2.currentVolume() > 0.12f);
}

void TestMixEditor::testPlayerSeekAndPlayFrom() {
  PlayerMetadata meta;
  meta.duration = 10000;
  meta.startPoint = 1000;
  meta.endPoint = 9000;

  TestablePlayerManager player;
  player.deferLoad = true;
  player.load(QUrl::fromLocalFile(QStringLiteral("c.mp3")), meta, false);
  player.playFrom(4500);
  player.finishLoad();

  QCOMPARE(player.status(), PlayerStatus::Playing);
  QCOMPARE(player.position(), qint64(4500));

  player.seek(2500);
  QCOMPARE(player.position(), qint64(2500));

  // Clamping below startPoint or >= endPoint resets to startPoint
  player.playFrom(200);
  QCOMPARE(player.position(), qint64(1000));
  player.playFrom(9500);
  QCOMPARE(player.position(), qint64(1000));
}

void TestMixEditor::testWaveformExtractorMissingAndValidWav() {
  WaveformExtractor extractor;

  QSignalSpy errorSpy(&extractor, &WaveformExtractor::errorOccurred);
  extractor.extract(QStringLiteral("Z:/nonexistent_path/does_not_exist.wav"));
  QCOMPARE(errorSpy.count(), 1);
  QVERIFY(!extractor.isDecoding());

  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString wavPath = tempDir.filePath(QStringLiteral("test_half_sine.wav"));
  QVERIFY(writeHalfSilenceHalfSineWav(wavPath, 1000));

  QSignalSpy finishedSpy(&extractor, &WaveformExtractor::finished);
  QSignalSpy wavErrSpy(&extractor, &WaveformExtractor::errorOccurred);

  extractor.extract(wavPath);
  QVERIFY(finishedSpy.wait(10000));
  QCOMPARE(wavErrSpy.count(), 0);
  QCOMPARE(finishedSpy.count(), 1);

  const auto args = finishedSpy.takeFirst();
  const auto samples = args.at(0).value<QVector<WaveformSample>>();
  const qint64 durMs = args.at(1).toLongLong();

  QVERIFY2(durMs >= 900 && durMs <= 1100,
           qPrintable(QStringLiteral("Unexpected decoded duration: %1").arg(durMs)));
  QVERIFY(samples.size() >= 80);

  // First 40% (silent half) should have negligible peaks; last 40% (sine) > 0.5
  float silentMax = 0.0f;
  for (int i = 0; i < samples.size() * 2 / 5; ++i) {
    silentMax = std::max(silentMax, samples.at(i).peak);
  }
  float toneMax = 0.0f;
  for (int i = samples.size() * 3 / 5; i < samples.size(); ++i) {
    toneMax = std::max(toneMax, samples.at(i).peak);
  }
  QVERIFY2(silentMax < 0.05f,
           qPrintable(QStringLiteral("Silent peak too high: %1").arg(silentMax)));
  QVERIFY2(toneMax > 0.5f,
           qPrintable(QStringLiteral("Sine peak too low: %1").arg(toneMax)));
}

void TestMixEditor::testWaveformExtractorCancel() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString wavPath = tempDir.filePath(QStringLiteral("cancel_test.wav"));
  QVERIFY(writeHalfSilenceHalfSineWav(wavPath, 2000));

  WaveformExtractor extractor;
  QSignalSpy finishedSpy(&extractor, &WaveformExtractor::finished);
  extractor.extract(wavPath);
  extractor.cancel();
  QVERIFY(!extractor.isDecoding());
  QTest::qWait(150);
  QCOMPARE(finishedSpy.count(), 0);
}

void TestMixEditor::testPlaylistModelAndManagerLiveMixUpdate() {
  std::array<TestablePlayerManager *, 3> players = {
      new TestablePlayerManager(), new TestablePlayerManager(),
      new TestablePlayerManager()};
  PlaylistManager manager({players[0], players[1], players[2]});
  PlaylistListModel *model = manager.model();
  model->clear();

  model->addItem(makeMarker(ItemType::BlockMarker, BlockType::Music, QStringLiteral("15:00")));
  model->addItem(makeMarker(ItemType::StartMarker, BlockType::Music));
  PlaylistItem item1 = makeMedia(QStringLiteral("Song 1"), QStringLiteral("s1.mp3"), 20000);
  PlaylistItem item2 = makeMedia(QStringLiteral("Song 2"), QStringLiteral("s2.mp3"), 20000);
  model->addItem(item1);
  model->addItem(item2);
  model->addItem(makeMarker(ItemType::EndMarker, BlockType::Music));

  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);

  const int row1 = model->rowForUuid(item1.uuid);
  const int row2 = model->rowForUuid(item2.uuid);
  QCOMPARE(model->itemAt(row2)->estimatedTime, QStringLiteral("15:00:20"));

  // Update active track's mixEnd & intro envelope, and next track's start/mixStart
  model->updateItemMixMetadata(row1, 20000, 1000, 19000, 0, 16000, 2000, 18000,
                               0.35f, 0.25f, 4000, 6000, 12000, 14000);
  model->updateItemMixMetadata(row2, 20000, 500, 19500, 2500, 18000, 1500, 17500,
                               0.5f, 1.0f, 3000, 5000, 0, 0);

  // Active player received live envelope update
  const int activeIdx = manager.activePlayerIndex();
  QVERIFY(activeIdx >= 0);
  const PlayerMetadata &activeMeta = players[activeIdx]->currentMetadata();
  QCOMPARE(activeMeta.mixEnd, qint64(16000));
  QVERIFY(qAbs(activeMeta.startVolume - 0.35f) < 0.001f);
  QCOMPARE(activeMeta.volumeDownStartPoint, qint64(12000));

  // Preloaded next player received live update too
  const int nextSlot = (activeIdx + 1) % 3;
  const PlayerMetadata &nextMeta = players[nextSlot]->currentMetadata();
  QCOMPARE(nextMeta.mixStart, qint64(2500));
  QVERIFY(qAbs(nextMeta.startVolume - 0.5f) < 0.001f);

  // Estimated time of item2 recalculated: (mixEnd 16000 - startPoint 1000) - (nextMixStart 2500 - nextStart 500) = 13000ms
  QCOMPARE(model->itemAt(row2)->estimatedTime, QStringLiteral("15:00:13"));
}

void TestMixEditor::testMixEditorLoadEditAndSaveByUuidAfterShift() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString wavPath = tempDir.filePath(QStringLiteral("edit_target.wav"));
  QVERIFY(writeHalfSilenceHalfSineWav(wavPath, 1000));

  PlaylistListModel model;
  model.clear();
  model.addItem(makeMarker(ItemType::BlockMarker, BlockType::Music, QStringLiteral("16:00")));
  model.addItem(makeMarker(ItemType::StartMarker, BlockType::Music));
  PlaylistItem target = makeMedia(QStringLiteral("Target Track"), wavPath, 20000);
  target.fadeInEndPoint = 1000;
  target.fadeOutStartPoint = 18000;
  model.addItem(target);
  model.addItem(makeMarker(ItemType::EndMarker, BlockType::Music));

  MixEditor editor;
  QVERIFY(editor.loadPlaylistItem(&model, target.uuid));

  auto *btnSaveGlobal = editor.findChild<QPushButton *>(QStringLiteral("btnSaveGlobal"));
  auto *btnSave = editor.findChild<QPushButton *>(QStringLiteral("btnSave"));
  auto *labelFileName = editor.findChild<QLabel *>(QStringLiteral("labelFileName"));
  auto *timeStart = editor.findChild<QTimeEdit *>(QStringLiteral("timeStartPoint"));
  auto *timeMixEnd = editor.findChild<QTimeEdit *>(QStringLiteral("timeMixEnd"));
  auto *sliderVolStart = editor.findChild<QSlider *>(QStringLiteral("sliderVolStart"));

  QVERIFY(btnSaveGlobal != nullptr);
  QVERIFY(!btnSaveGlobal->isEnabled());
  QVERIFY(btnSaveGlobal->text().contains(QStringLiteral("to do (M11)")));
  QVERIFY(btnSave != nullptr && btnSave->isEnabled());
  QCOMPARE(labelFileName->text(), QStringLiteral("Target Track"));

  // Wait for waveform extraction to complete (wav is 1000ms -> clamps duration to 1000ms)
  QTRY_VERIFY_WITH_TIMEOUT(
      editor.currentMetadata().duration == 1000, 10000);

  timeStart->setTime(QTime(0, 0, 0, 100));
  timeMixEnd->setTime(QTime(0, 0, 0, 900));
  sliderVolStart->setValue(45);

  QCOMPARE(editor.currentMetadata().startPoint, qint64(100));
  QCOMPARE(editor.currentMetadata().mixEnd, qint64(900));
  QVERIFY(qAbs(editor.currentMetadata().startVolume - 0.45f) < 0.01f);
  QCOMPARE(editor.waveformWidget()->metadata().startPoint, qint64(100));

  // Shift row index by inserting an item before target while MixEditor is open
  const int originalRow = model.rowForUuid(target.uuid);
  PlaylistItem beforeItem = makeMedia(QStringLiteral("Inserted Before"), QString(), 15000);
  model.insertItem(originalRow, beforeItem);
  const int shiftedRow = model.rowForUuid(target.uuid);
  QCOMPARE(shiftedRow, originalRow + 1);

  QSignalSpy savedSpy(&editor, &MixEditor::savedLocally);
  btnSave->click();

  QCOMPARE(savedSpy.count(), 1);
  const PlaylistItem *savedItem = model.itemAt(shiftedRow);
  QVERIFY(savedItem != nullptr);
  QCOMPARE(savedItem->startPoint, qint64(100));
  QCOMPARE(savedItem->mixEnd, qint64(900));
  QVERIFY(qAbs(savedItem->startVolume - 0.45f) < 0.01f);
  // Ensure inserted sibling was untouched
  QCOMPARE(model.itemAt(originalRow)->startPoint, qint64(0));
}

void TestMixEditor::testMixEditorRemovedOrFinishedItemDoesNotCrash() {
  PlaylistListModel model;
  model.clear();
  model.addItem(makeMarker(ItemType::BlockMarker, BlockType::Music, QStringLiteral("17:00")));
  model.addItem(makeMarker(ItemType::StartMarker, BlockType::Music));
  PlaylistItem item = makeMedia(QStringLiteral("Ephemeral"), QStringLiteral("Z:/missing.mp3"), 12000);
  model.addItem(item);
  model.addItem(makeMarker(ItemType::EndMarker, BlockType::Music));

  MixEditor editor;
  QVERIFY(editor.loadPlaylistItem(&model, item.uuid));

  auto *btnPlay = editor.findChild<QPushButton *>(QStringLiteral("btnPlay"));
  QVERIFY(btnPlay != nullptr);
  QVERIFY(!btnPlay->isEnabled());

  // Remove item and verify saveToPlaylist() fails gracefully without crash
  const int row = model.rowForUuid(item.uuid);
  QVERIFY(model.removeItem(row));
  QVERIFY(!editor.saveToPlaylist());

  // Re-add, mark Finished, verify saveToPlaylist() returns false cleanly
  PlaylistItem finItem = makeMedia(QStringLiteral("Done Track"), QStringLiteral("Z:/missing2.mp3"), 12000);
  model.insertItem(row, finItem);
  QVERIFY(editor.loadPlaylistItem(&model, finItem.uuid));
  model.markRowFinished(model.rowForUuid(finItem.uuid));
  QVERIFY(!editor.saveToPlaylist());

  // Re-add, mark Mixing, verify saveToPlaylist() returns false cleanly
  PlaylistItem mixItem = makeMedia(QStringLiteral("Mixing Track"), QStringLiteral("Z:/missing3.mp3"), 12000);
  model.insertItem(row + 1, mixItem);
  QVERIFY(editor.loadPlaylistItem(&model, mixItem.uuid));
  model.markRowMixing(model.rowForUuid(mixItem.uuid));
  QVERIFY(!editor.saveToPlaylist());
}

void TestMixEditor::testWaveformWidgetCoordinateMappingAndZoom() {
  WaveformWidget widget;
  widget.resize(801, 240);

  PlayerMetadata meta;
  meta.duration = 10000;
  meta.startPoint = 1000;
  meta.endPoint = 9000;
  meta.mixStart = 2000;
  meta.mixEnd = 8000;
  meta.fadeInEndPoint = 1500;
  meta.fadeOutStartPoint = 8500;

  QVector<WaveformSample> samples(1000, WaveformSample{0.5f, 0.3f});
  widget.setWaveformData(samples, 10000, 10);
  widget.setMetadata(meta);
  widget.grab(); // Populate painted marker pill rects & envelope handles.

  QCOMPARE(widget.xForTime(0), 0);
  QCOMPARE(widget.xForTime(10000), 800);
  QCOMPARE(widget.timeForX(400), qint64(5000));

  const int mixStartX = widget.xForTime(2000);
  QCOMPARE(widget.hitTestMarker(QPoint(mixStartX, 120)),
           WaveformWidget::MarkerId::MixStart);

  widget.setZoomFactor(2.0);
  QCOMPARE(widget.zoomFactor(), 2.0);
  QCOMPARE(widget.visibleDurationMs(), qint64(5000));
  widget.resetZoom();
  QCOMPARE(widget.zoomFactor(), 1.0);
  QCOMPARE(widget.scrollOffsetMs(), qint64(0));
}

void TestMixEditor::testPlaylistViewContextMenu() {
  PlaylistListModel model;
  model.clear();
  model.addItem(makeMarker(ItemType::BlockMarker, BlockType::Music, QStringLiteral("18:00")));
  model.addItem(makeMarker(ItemType::StartMarker, BlockType::Music));
  PlaylistItem finItem = makeMedia(QStringLiteral("Finished"), QString(), 10000);
  PlaylistItem mixItem = makeMedia(QStringLiteral("Mixing"), QString(), 10000);
  PlaylistItem playingItem = makeMedia(QStringLiteral("Playing"), QString(), 10000);
  PlaylistItem futureItem = makeMedia(QStringLiteral("Future"), QString(), 10000);
  model.addItem(finItem);
  model.addItem(mixItem);
  model.addItem(playingItem);
  model.addItem(futureItem);
  model.addItem(makeMarker(ItemType::EndMarker, BlockType::Music));
  model.setPlayingRow(model.rowForUuid(playingItem.uuid),
                      model.rowForUuid(mixItem.uuid));

  PlaylistView view;
  view.setModel(&model);

  // Marker returns null menu
  QVERIFY(view.createContextMenu(model.index(0, 0)) == nullptr);

  // Finished item -> MixEditor action disabled
  std::unique_ptr<QMenu> finMenu(
      view.createContextMenu(model.index(model.rowForUuid(finItem.uuid), 0)));
  QVERIFY(finMenu != nullptr);
  QVERIFY(!finMenu->actions().isEmpty());
  QVERIFY(!finMenu->actions().first()->isEnabled());

  // Mixing item -> MixEditor action and Remover both disabled
  std::unique_ptr<QMenu> mixMenu(
      view.createContextMenu(model.index(model.rowForUuid(mixItem.uuid), 0)));
  QVERIFY(mixMenu != nullptr);
  QVERIFY(!mixMenu->actions().first()->isEnabled());
  QVERIFY(!mixMenu->actions().last()->isEnabled());

  // Playing item -> MixEditor enabled, Remover disabled
  std::unique_ptr<QMenu> playMenu(
      view.createContextMenu(model.index(model.rowForUuid(playingItem.uuid), 0)));
  QVERIFY(playMenu != nullptr);
  QVERIFY(playMenu->actions().first()->isEnabled());
  QVERIFY(!playMenu->actions().last()->isEnabled());

  // Future item -> both enabled, clicking MixEditor emits signal with uuid
  std::unique_ptr<QMenu> futMenu(
      view.createContextMenu(model.index(model.rowForUuid(futureItem.uuid), 0)));
  QVERIFY(futMenu != nullptr);
  QVERIFY(futMenu->actions().first()->isEnabled());
  QVERIFY(futMenu->actions().last()->isEnabled());

  QSignalSpy reqSpy(&view, &PlaylistView::mixEditorRequested);
  futMenu->actions().first()->trigger();
  QCOMPARE(reqSpy.count(), 1);
  QCOMPARE(reqSpy.takeFirst().at(0).toUuid(), futureItem.uuid);
}

void TestMixEditor::testCatalogWidgetContextMenu() {
  CatalogWidget catalog;
  CatalogListModel *catModel = catalog.catalogModel();

  CatalogItem header;
  header.itemType = CatalogItemType::FolderHeader;
  header.title = QStringLiteral("Pop");

  CatalogItem fileItem;
  fileItem.itemType = CatalogItemType::File;
  fileItem.title = QStringLiteral("Track.mp3");
  fileItem.path = QStringLiteral("D:/Audio/Track.mp3");
  fileItem.mediaType = MediaType::Music;

  CatalogItem loading;
  loading.itemType = CatalogItemType::Loading;
  loading.title = QStringLiteral("Carregando...");

  catModel->setItems({header, fileItem, loading});

  QVERIFY(catalog.createFileContextMenu(catModel->index(0, 0)) == nullptr);
  QVERIFY(catalog.createFileContextMenu(catModel->index(2, 0)) == nullptr);

  std::unique_ptr<QMenu> fileMenu(
      catalog.createFileContextMenu(catModel->index(1, 0)));
  QVERIFY(fileMenu != nullptr);
  QCOMPARE(fileMenu->actions().size(), 1);
  QAction *act = fileMenu->actions().first();
  QVERIFY(!act->isEnabled());
  QVERIFY(act->text().contains(QStringLiteral("to do (M11)")));
}

QTEST_MAIN(TestMixEditor)
#include "test_mix_editor.moc"
