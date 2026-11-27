#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

#include <array>
#include <cmath>

#include "core/catalog/CatalogRepository.h"
#include "core/catalog/CatalogScanner.h"
#include "core/catalog/FolderItem.h"
#include "core/catalog/FolderListModel.h"
#include "core/models.h"
#include "core/playlist/PlaylistListModel.h"
#include "core/playlist/PlaylistManager.h"
#include "core/playlist/player/PlayerManager.h"
#include "core/storage/AppPaths.h"
#include "core/storage/DataBootstrap.h"
#include "core/storage/MediaPathResolver.h"
#include "ui/widgets/catalog/CatalogWidget.h"
#include "ui/widgets/editor/MixEditor.h"

namespace {
class FakePlayerManager : public PlayerManager {
public:
  explicit FakePlayerManager(QSet<QString> *existingFiles,
                             QObject *parent = nullptr)
      : PlayerManager(false, parent), m_existingFiles(existingFiles) {}

  QString lastLoadedLocalPath;
  int loadCount = 0;

  void load(const QUrl &source, const PlayerMetadata &newMetadata,
            bool autoPlay = false) override {
    ++loadCount;
    const QString local =
        source.isLocalFile() ? source.toLocalFile() : source.toString();
    lastLoadedLocalPath = local;
    const bool exists =
        !local.isEmpty() &&
        ((m_existingFiles && m_existingFiles->contains(local)) ||
         QFileInfo::exists(local));
    if (!exists) {
      m_pendingPlay = false;
      m_source.clear();
      metadata = PlayerMetadata{};
      setStatus(PlayerStatus::Error);
      return;
    }

    m_source = source;
    metadata = newMetadata;
    normalizeMetadata(metadata.duration > 0 ? metadata.duration : 10000);
    m_pos = metadata.startPoint;
    setStatus(PlayerStatus::Loaded);
    if (autoPlay) {
      play();
    }
  }

  void unload() override {
    m_pendingPlay = false;
    m_source.clear();
    metadata = PlayerMetadata{};
    m_pos = 0;
    setStatus(PlayerStatus::Idle);
  }

  void play() override {
    if (status() == PlayerStatus::Loaded || status() == PlayerStatus::Paused ||
        status() == PlayerStatus::End) {
      setStatus(PlayerStatus::Playing);
    }
  }

  void pause() override {
    if (status() == PlayerStatus::Playing) {
      setStatus(PlayerStatus::Paused);
    }
  }

  void stop() override {
    m_pendingPlay = false;
    m_pos = metadata.endPoint;
    setStatus(PlayerStatus::End);
  }

  qint64 position() const override { return m_pos; }
  qint64 duration() const override { return metadata.duration; }

private:
  QSet<QString> *m_existingFiles = nullptr;
  qint64 m_pos = 0;
};

bool writeFile(const QString &filePath, const QByteArray &content) {
  QFileInfo fi(filePath);
  if (!QDir().mkpath(fi.absolutePath())) {
    return false;
  }
  QFile f(filePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  const qint64 written = f.write(content);
  f.close();
  return written == content.size();
}

bool writeSilentWav(const QString &filePath, int durationMs = 500) {
  QFileInfo fi(filePath);
  if (!QDir().mkpath(fi.absolutePath())) {
    return false;
  }
  constexpr quint32 sampleRate = 8000;
  constexpr quint16 channels = 1;
  constexpr quint16 bitsPerSample = 16;
  const quint32 totalSamples =
      static_cast<quint32>((static_cast<quint64>(sampleRate) * durationMs) /
                           1000ULL);
  const quint32 dataBytes = totalSamples * channels * (bitsPerSample / 8);
  const quint32 byteRate = sampleRate * channels * (bitsPerSample / 8);
  const quint16 blockAlign = channels * (bitsPerSample / 8);

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
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
    out << static_cast<qint16>(0);
  }
  file.close();
  return true;
}
} // namespace

class TestStorageAndMirrors : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void testAppPathsResolutionPrecedenceAndLegacyIgnore();
  void testAppPathsEnsureDataStructureAndBootstrap();
  void testCatalogRepositoryMirrorParsingAndDeduplication();
  void testFolderListModelResolutionRolesAndTooltips();
  void testCatalogScannerLocalAndGlobalMirrorCascade();
  void testMediaPathResolverAndPlaybackMixEditorFallback();
};

void TestStorageAndMirrors::initTestCase() {
  qRegisterMetaType<PlaylistStatus>("PlaylistStatus");
  qRegisterMetaType<PlayerStatus>("PlayerStatus");
}

void TestStorageAndMirrors::testAppPathsResolutionPrecedenceAndLegacyIgnore() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString appBinDir = tempDir.filePath(QStringLiteral("Flow/bin"));
  const QString expectedDefaultData =
      QDir::cleanPath(tempDir.filePath(QStringLiteral("Flow/data")));

  // Default resolves to ../data relative to binary dir without writing QSettings
  const QString iniPath = tempDir.filePath(QStringLiteral("settings.ini"));
  QSettings settings(iniPath, QSettings::IniFormat);
  settings.setValue(QStringLiteral("db"),
                    tempDir.filePath(QStringLiteral("legacy_db")));
  settings.sync();

  QCOMPARE(AppPaths::resolveDataDir(&settings, appBinDir, QString()),
           expectedDefaultData);
  QVERIFY(!settings.contains(QString::fromLatin1(AppPaths::kDataDirSettingsKey)));

  // Explicit dataDir in QSettings overrides default ../data (legacy "db" still ignored)
  const QString customSettingsData =
      QDir::cleanPath(tempDir.filePath(QStringLiteral("custom_settings_data")));
  settings.setValue(QString::fromLatin1(AppPaths::kDataDirSettingsKey),
                    customSettingsData);
  settings.sync();
  QCOMPARE(AppPaths::resolveDataDir(&settings, appBinDir, QString()),
           customSettingsData);

  // Environment override has highest precedence over QSettings and default
  const QString envData =
      QDir::cleanPath(tempDir.filePath(QStringLiteral("env_data_dir")));
  QCOMPARE(AppPaths::resolveDataDir(&settings, appBinDir, envData), envData);
}

void TestStorageAndMirrors::testAppPathsEnsureDataStructureAndBootstrap() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // 1. Fresh extracted portable layout: empty data/ directory already exists
  const QString dataDir = tempDir.filePath(QStringLiteral("Flow/data"));
  QVERIFY(QDir().mkpath(dataDir));

  const DataBootstrapResult initRes = DataBootstrap::initialize(dataDir, true);
  QVERIFY2(initRes.ok, qPrintable(initRes.errorMessage));
  QCOMPARE(initRes.createdDirs.size(), 4);
  QCOMPARE(initRes.createdFiles.size(), 1);

  QVERIFY(QFileInfo::exists(dataDir));
  QVERIFY(QFileInfo(AppPaths::blocksDir(dataDir)).isDir());
  QVERIFY(QFileInfo(AppPaths::profilesDir(dataDir)).isDir());
  QVERIFY(QFileInfo(AppPaths::logsDir(dataDir)).isDir());
  QVERIFY(QFileInfo(AppPaths::configDir(dataDir)).isDir());

  const QString catalogFile = AppPaths::catalogFilePath(dataDir);
  QVERIFY(QFileInfo(catalogFile).isFile());

  // Minimum default catalog contains version=1 and Comandos/pause
  const QVector<FolderItem> folders = CatalogRepository::loadFolders(dataDir);
  QCOMPARE(folders.size(), 1);
  QVERIFY(folders[0].isCommandFolder());
  QCOMPARE(folders[0].id, QStringLiteral("comandos-playlist"));
  QCOMPARE(folders[0].items.size(), 1);
  QCOMPARE(folders[0].items[0].commandType, CommandType::Pause);

  // 2. Existing user data (catalog.json + files inside blocks/config) must be preserved
  const QByteArray customCatalog = R"({"version":1,"folders":[{"id":"custom","title":"Custom","path":"C:/custom","type":"music"}]})";
  QVERIFY(writeFile(catalogFile, customCatalog));
  const QString userBlockFile =
      QDir(AppPaths::blocksDir(dataDir)).filePath(QStringLiteral("2026-11-27.json"));
  QVERIFY(writeFile(userBlockFile, QByteArray("{\"block\":1}")));

  const DataBootstrapResult secondRes = DataBootstrap::initialize(dataDir, true);
  QVERIFY(secondRes.ok);
  QVERIFY(secondRes.createdDirs.isEmpty());
  QVERIFY(secondRes.createdFiles.isEmpty());

  const QVector<FolderItem> after = CatalogRepository::loadFolders(dataDir);
  QCOMPARE(after.size(), 1);
  QCOMPARE(after[0].id, QStringLiteral("custom"));
  QVERIFY(QFileInfo::exists(userBlockFile));

  // 3. Partial recovery: removing only logs/ recreates only logs/ without touching catalog.json
  QVERIFY(QDir(AppPaths::logsDir(dataDir)).rmdir(QStringLiteral(".")));
  const DataBootstrapResult partialRes = DataBootstrap::initialize(dataDir, true);
  QVERIFY(partialRes.ok);
  QCOMPARE(partialRes.createdDirs.size(), 1);
  QCOMPARE(partialRes.createdDirs.first(),
           QDir::cleanPath(AppPaths::logsDir(dataDir)));
  QVERIFY(partialRes.createdFiles.isEmpty());
  QCOMPARE(CatalogRepository::loadFolders(dataDir)[0].id,
           QStringLiteral("custom"));
}

void TestStorageAndMirrors::testCatalogRepositoryMirrorParsingAndDeduplication() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString primaryDir = tempDir.filePath(QStringLiteral("primary"));
  const QString mirror1Dir = tempDir.filePath(QStringLiteral("mirror1"));
  const QString mirror2Dir = tempDir.filePath(QStringLiteral("mirror2"));

  const QByteArray json = QStringLiteral(R"({
    "version": 1,
    "folders": [
      {
        "id": "single-mirror",
        "title": "Single",
        "path": "%1",
        "mirrorPath": " %2 ",
        "type": "music"
      },
      {
        "id": "array-and-dedup",
        "title": "Combined",
        "path": "%1/",
        "mirrorPath": "%2",
        "mirrorPaths": ["", "   ", "%1", "%2/", "%3"],
        "type": "jingle"
      }
    ]
  })")
                              .arg(primaryDir, mirror1Dir, mirror2Dir)
                              .toUtf8();

  QVERIFY(writeFile(AppPaths::catalogFilePath(tempDir.path()), json));
  const QVector<FolderItem> loaded = CatalogRepository::loadFolders(tempDir.path());
  QCOMPARE(loaded.size(), 2);

  QCOMPARE(loaded[0].mirrorPaths.size(), 1);
  QCOMPARE(loaded[0].mirrorPaths[0], MediaPathResolver::normalizePath(mirror1Dir));

  QCOMPARE(loaded[1].mirrorPaths.size(), 2);
  QCOMPARE(loaded[1].mirrorPaths[0], MediaPathResolver::normalizePath(mirror1Dir));
  QCOMPARE(loaded[1].mirrorPaths[1], MediaPathResolver::normalizePath(mirror2Dir));
}

void TestStorageAndMirrors::testFolderListModelResolutionRolesAndTooltips() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString primaryOk = tempDir.filePath(QStringLiteral("primary_ok"));
  const QString primaryMissing = tempDir.filePath(QStringLiteral("primary_missing"));
  const QString mirrorOk = tempDir.filePath(QStringLiteral("mirror_ok"));
  const QString mirrorMissing = tempDir.filePath(QStringLiteral("mirror_missing"));
  QVERIFY(QDir().mkpath(primaryOk));
  QVERIFY(QDir().mkpath(mirrorOk));

  const QByteArray json = QStringLiteral(R"({
    "version": 1,
    "folders": [
      {
        "id": "f-primary",
        "title": "Primary Folder",
        "path": "%1",
        "mirrorPath": "%3",
        "type": "music"
      },
      {
        "id": "f-mirror",
        "title": "Mirrored Folder",
        "path": "%2",
        "mirrorPaths": ["%4", "%3"],
        "type": "jingle"
      },
      {
        "id": "f-offline",
        "title": "Offline Folder",
        "path": "%2",
        "mirrorPath": "%4",
        "type": "commercial"
      }
    ]
  })")
                              .arg(primaryOk, primaryMissing, mirrorOk, mirrorMissing)
                              .toUtf8();
  QVERIFY(writeFile(AppPaths::catalogFilePath(tempDir.path()), json));

  FolderListModel model;
  model.loadFromDatabase(tempDir.path());

  const QModelIndex primaryIdx = model.indexForId(QStringLiteral("f-primary"));
  const QModelIndex mirrorIdx = model.indexForId(QStringLiteral("f-mirror"));
  const QModelIndex offlineIdx = model.indexForId(QStringLiteral("f-offline"));
  QVERIFY(primaryIdx.isValid() && mirrorIdx.isValid() && offlineIdx.isValid());

  QCOMPARE(primaryIdx.data(FolderActivePathRole).toString(),
           MediaPathResolver::normalizePath(primaryOk));
  QCOMPARE(primaryIdx.data(FolderUsingMirrorRole).toBool(), false);
  QCOMPARE(primaryIdx.data(FolderAccessibleRole).toBool(), true);
  QVERIFY(primaryIdx.data(Qt::ToolTipRole).toString().contains(
      MediaPathResolver::normalizePath(primaryOk)));

  QCOMPARE(mirrorIdx.data(FolderActivePathRole).toString(),
           MediaPathResolver::normalizePath(mirrorOk));
  QCOMPARE(mirrorIdx.data(FolderUsingMirrorRole).toBool(), true);
  QCOMPARE(mirrorIdx.data(FolderAccessibleRole).toBool(), true);
  QVERIFY(mirrorIdx.data(Qt::ToolTipRole).toString().contains(
      QStringLiteral("Espelho local")));

  QCOMPARE(offlineIdx.data(FolderActivePathRole).toString(), QString());
  QCOMPARE(offlineIdx.data(FolderUsingMirrorRole).toBool(), false);
  QCOMPARE(offlineIdx.data(FolderAccessibleRole).toBool(), false);
  QVERIFY(offlineIdx.data(Qt::ToolTipRole).toString().contains(
      QStringLiteral("Caminho indisponível")));

  QCOMPARE(model.mediaFolders().size(), 3);
}

void TestStorageAndMirrors::testCatalogScannerLocalAndGlobalMirrorCascade() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString primaryOnline = tempDir.filePath(QStringLiteral("primary_online"));
  const QString mirrorFallback = tempDir.filePath(QStringLiteral("mirror_fallback"));
  const QString mirrorSecond = tempDir.filePath(QStringLiteral("mirror_second"));
  const QString missingPrimary = tempDir.filePath(QStringLiteral("missing_primary"));
  const QString missingMirror = tempDir.filePath(QStringLiteral("missing_mirror"));

  QVERIFY(writeFile(primaryOnline + QStringLiteral("/PrimaryTrack.mp3"),
                    QByteArray("p")));
  QVERIFY(writeFile(mirrorFallback + QStringLiteral("/MirrorTrack.mp3"),
                    QByteArray("m")));
  QVERIFY(writeFile(mirrorSecond + QStringLiteral("/SecondMirrorOnly.mp3"),
                    QByteArray("s")));

  const QByteArray json = QStringLiteral(R"({
    "version": 1,
    "folders": [
      {
        "id": "online",
        "title": "Online",
        "path": "%1",
        "mirrorPath": "%2",
        "type": "music"
      },
      {
        "id": "cascade-first-missing-mirror",
        "title": "CascadeMirror",
        "path": "%4",
        "mirrorPaths": ["%5", "%2", "%3"],
        "type": "jingle"
      },
      {
        "id": "mirror-only-no-primary",
        "title": "MirrorOnly",
        "mirrorPath": "%3",
        "type": "commercial"
      },
      {
        "id": "all-offline",
        "title": "AllOffline",
        "path": "%4",
        "mirrorPath": "%5",
        "type": "other"
      }
    ]
  })")
                              .arg(primaryOnline, mirrorFallback, mirrorSecond,
                                   missingPrimary, missingMirror)
                              .toUtf8();
  QVERIFY(writeFile(AppPaths::catalogFilePath(tempDir.path()), json));

  CatalogWidget widget;
  widget.setDatabasePath(tempDir.path());
  auto *foldersView = widget.findChild<QListView *>(QStringLiteral("FoldersListView"));
  auto *btnGlobal = widget.findChild<QPushButton *>(QStringLiteral("SearchGlobal"));
  QVERIFY(foldersView && btnGlobal);

  // Activate CascadeMirror -> must scan mirrorFallback
  const QModelIndex cascadeIdx =
      widget.folderModel()->indexForId(QStringLiteral("cascade-first-missing-mirror"));
  QVERIFY(cascadeIdx.isValid());
  QSignalSpy resetSpy(widget.catalogModel(), &QAbstractItemModel::modelReset);
  foldersView->setCurrentIndex(cascadeIdx);
  QTRY_VERIFY_WITH_TIMEOUT(
      !widget.catalogModel()->isLoading() && widget.catalogModel()->rowCount() == 1,
      3000);
  QCOMPARE(widget.catalogModel()->itemAt(0)->title,
           QStringLiteral("MirrorTrack.mp3"));
  QCOMPARE(widget.currentOpenFolder(),
           MediaPathResolver::normalizePath(mirrorFallback));

  // Activate MirrorOnly (no primary path at all, only mirrorPath)
  const QModelIndex mirrorOnlyIdx =
      widget.folderModel()->indexForId(QStringLiteral("mirror-only-no-primary"));
  QVERIFY(mirrorOnlyIdx.isValid());
  foldersView->setCurrentIndex(mirrorOnlyIdx);
  QTRY_VERIFY_WITH_TIMEOUT(
      !widget.catalogModel()->isLoading() &&
          widget.catalogModel()->rowCount() == 1 &&
          widget.catalogModel()->itemAt(0)->title ==
              QStringLiteral("SecondMirrorOnly.mp3"),
      3000);

  // Activate AllOffline -> must safely resolve empty without crashing
  const QModelIndex offlineIdx =
      widget.folderModel()->indexForId(QStringLiteral("all-offline"));
  QVERIFY(offlineIdx.isValid());
  foldersView->setCurrentIndex(offlineIdx);
  QTRY_VERIFY_WITH_TIMEOUT(
      !widget.catalogModel()->isLoading() && widget.catalogModel()->rowCount() == 0,
      3000);

  // Global search across primary + cascaded mirror + mirror-only
  const int beforeGlobalResets = resetSpy.count();
  btnGlobal->click();
  QTRY_VERIFY_WITH_TIMEOUT(
      resetSpy.count() >= beforeGlobalResets + 2 &&
          !widget.catalogModel()->isLoading(),
      3000);

  QStringList foundTitles;
  for (const CatalogItem &it : widget.catalogModel()->items()) {
    if (it.itemType == CatalogItemType::File) {
      foundTitles.append(it.title);
    }
  }
  QVERIFY(foundTitles.contains(QStringLiteral("PrimaryTrack.mp3")));
  QVERIFY(foundTitles.contains(QStringLiteral("MirrorTrack.mp3")));
  QVERIFY(foundTitles.contains(QStringLiteral("SecondMirrorOnly.mp3")));
}

void TestStorageAndMirrors::testMediaPathResolverAndPlaybackMixEditorFallback() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString primaryRoot = tempDir.filePath(QStringLiteral("net_music"));
  const QString prefixSibling = tempDir.filePath(QStringLiteral("net_music_rock"));
  const QString mirrorRoot = tempDir.filePath(QStringLiteral("local_mirror"));
  const QString mirrorSubFile = mirrorRoot + QStringLiteral("/sub/song.wav");
  const QString primaryOnlyFile = primaryRoot + QStringLiteral("/only_primary.mp3");

  QVERIFY(writeSilentWav(mirrorSubFile, 500));
  QVERIFY(writeFile(primaryOnlyFile, QByteArray("primary-audio")));
  QVERIFY(writeFile(prefixSibling + QStringLiteral("/sub/song.wav"),
                    QByteArray("wrong-sibling")));

  FolderItem folder;
  folder.id = QStringLiteral("pop");
  folder.title = QStringLiteral("Pop");
  folder.path = primaryRoot;
  folder.mirrorPaths = {mirrorRoot};
  folder.type = MediaType::Music;

  const QVector<FolderItem> folders = {folder};

  // Missing primary file with subpath -> resolves to mirrorRoot/sub/song.wav
  const QString missingPrimarySubFile = primaryRoot + QStringLiteral("/sub/song.wav");
  const MediaFileResolution resMirror =
      MediaPathResolver::resolveMediaFile(missingPrimarySubFile, folders);
  QVERIFY(resMirror.found);
  QVERIFY(resMirror.usedMirror);
  QCOMPARE(resMirror.resolvedPath, MediaPathResolver::normalizePath(mirrorSubFile));
  QCOMPARE(resMirror.folderId, QStringLiteral("pop"));

  // Sibling prefix (net_music_rock vs net_music) must NOT falsely match net_music
  const QString missingSiblingFile =
      prefixSibling + QStringLiteral("/sub/only_missing.mp3");
  const MediaFileResolution resSibling =
      MediaPathResolver::resolveMediaFile(missingSiblingFile, folders);
  QVERIFY(!resSibling.found);

  // Reverse fallback: item stored with mirror path when mirror disappears, primary exists
  const QString missingMirrorPrimaryFile =
      mirrorRoot + QStringLiteral("/only_primary.mp3");
  const MediaFileResolution resBackToPrimary =
      MediaPathResolver::resolveMediaFile(missingMirrorPrimaryFile, folders);
  QVERIFY(resBackToPrimary.found);
  QVERIFY(!resBackToPrimary.usedMirror);
  QCOMPARE(resBackToPrimary.resolvedPath,
           MediaPathResolver::normalizePath(primaryOnlyFile));

  // Integrate with PlaylistManager + FakePlayerManager + PlaylistListModel + MixEditor
  QSet<QString> fakeFiles;
  std::array<FakePlayerManager *, 3> fakes = {new FakePlayerManager(&fakeFiles),
                                              new FakePlayerManager(&fakeFiles),
                                              new FakePlayerManager(&fakeFiles)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});
  auto resolverFn = [&folders](const QString &path) {
    const MediaFileResolution res =
        MediaPathResolver::resolveMediaFile(path, folders);
    return res.found ? res.resolvedPath : path;
  };
  manager.setMediaPathResolver(resolverFn);

  auto *model = manager.model();
  model->clear();
  QString probedPath;
  model->setDurationResolver([&probedPath](const QString &resolved) {
    probedPath = resolved;
    return qint64(15000);
  });

  PlaylistItem bm;
  bm.uuid = QUuid::createUuid();
  bm.type = ItemType::BlockMarker;
  bm.blockType = BlockType::Music;
  bm.title = QStringLiteral("10:00");
  model->addItem(bm);

  PlaylistItem sm;
  sm.uuid = QUuid::createUuid();
  sm.type = ItemType::StartMarker;
  sm.blockType = BlockType::Music;
  model->addItem(sm);

  PlaylistItem em;
  em.uuid = QUuid::createUuid();
  em.type = ItemType::EndMarker;
  em.blockType = BlockType::Music;
  model->addItem(em);

  QVERIFY(model->insertCatalogItem(2, QStringLiteral("Song"),
                                   missingPrimarySubFile, MediaType::Music, 0));
  QCOMPARE(probedPath, MediaPathResolver::normalizePath(mirrorSubFile));
  QCOMPARE(model->itemAt(2)->durationMs, qint64(15000));

  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(fakes[0]->lastLoadedLocalPath,
           MediaPathResolver::normalizePath(mirrorSubFile));

  MixEditor editor;
  editor.setMediaPathResolver(resolverFn);
  QVERIFY(editor.loadPlaylistItem(model, model->itemAt(2)->uuid));
  auto *labelFileName = editor.findChild<QLabel *>(QStringLiteral("labelFileName"));
  QVERIFY(labelFileName);
  QCOMPARE(labelFileName->toolTip(),
           MediaPathResolver::normalizePath(mirrorSubFile));
  QTRY_VERIFY_WITH_TIMEOUT(editor.currentMetadata().duration == 500, 10000);
}

QTEST_MAIN(TestStorageAndMirrors)
#include "test_storage_and_mirrors.moc"
