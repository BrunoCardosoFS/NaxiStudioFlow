#include <QAction>
#include <QByteArray>
#include <QDataStream>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QHash>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSet>
#include <QSignalSpy>
#include <QStyleOptionViewItem>
#include <QTemporaryDir>
#include <QtTest>

#include <array>
#include <memory>

#include "core/cartwall/CartItem.h"
#include "core/cartwall/CartWallTableModel.h"
#include "core/catalog/CatalogItem.h"
#include "core/catalog/CatalogListModel.h"
#include "core/catalog/CatalogRepository.h"
#include "core/catalog/CatalogScanner.h"
#include "core/catalog/FolderItem.h"
#include "core/catalog/FolderListModel.h"
#include "core/models.h"
#include "core/playlist/PlaylistListModel.h"
#include "core/playlist/PlaylistManager.h"
#include "core/playlist/player/PlayerManager.h"
#include "ui/widgets/cartwall/CartWallView.h"
#include "ui/widgets/catalog/CatalogItemDelegate.h"
#include "ui/widgets/catalog/CatalogWidget.h"
#include "ui/widgets/catalog/FolderItemDelegate.h"
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

    const qint64 realDur =
        m_registry ? m_registry->durationByPath.value(local, metadata.duration)
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

PlaylistItem makePauseCommand(const QString &title = QStringLiteral("Pausar Playlist")) {
  PlaylistItem item;
  item.uuid = QUuid::createUuid();
  item.type = ItemType::Command;
  item.mediaType = MediaType::Command;
  item.commandType = CommandType::Pause;
  item.commandId = QStringLiteral("pause");
  item.title = title;
  return item;
}

bool writeJsonFile(const QString &filePath, const QByteArray &json) {
  QFile f(filePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  f.write(json);
  f.close();
  return true;
}
} // namespace

class TestCommands : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void testCatalogRepositoryAndFolderListModelLoadCommands();
  void testCatalogWidgetVirtualCommandFoldersSearchAndMenus();
  void testCatalogDragDropToPlaylistAndCartWallRejection();
  void testPlaylistModelCommandMetadataCalculationsAndLocking();
  void testPlaylistManagerPauseFlowAndPreloads();
  void testPlaylistManagerPlayResumeStopManualNextAndMultiplePauses();
  void testPlaylistManagerPauseAtEndAndBlockBoundary();
  void testPlaylistManagerDynamicCommandInsertRemoveAndErrorSkip();
  void testDelegatesRenderCommandStates();
};

void TestCommands::initTestCase() {
  qRegisterMetaType<PlaylistStatus>("PlaylistStatus");
  qRegisterMetaType<PlayerStatus>("PlayerStatus");
  qRegisterMetaType<CommandType>("CommandType");
}

void TestCommands::testCatalogRepositoryAndFolderListModelLoadCommands() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString musicDir = dir.filePath(QStringLiteral("musics"));
  QVERIFY(QDir().mkpath(musicDir));

  const QByteArray catalogJson = QByteArray(R"({
    "version": 1,
    "folders": [
      {
        "id": "jingles",
        "title": "Vinhetas",
        "path": ")") + musicDir.toUtf8() + QByteArray(R"(",
        "type": "jingle"
      },
      {
        "id": "musicas",
        "title": "Pop",
        "path": ")") + musicDir.toUtf8() + QByteArray(R"(",
        "type": "MUSIC"
      },
      {
        "id": "comerciais",
        "title": "Comerciais",
        "path": ")") + musicDir.toUtf8() + QByteArray(R"(",
        "type": "commercial"
      },
      {
        "id": "outros",
        "title": "Outros",
        "path": ")") + musicDir.toUtf8() + QByteArray(R"(",
        "type": "other"
      },
      {
        "id": "fallback-missing",
        "title": "Sem Tipo",
        "path": ")") + musicDir.toUtf8() + QByteArray(R"("
      },
      {
        "id": "fallback-invalid",
        "title": "Tipo Invalido",
        "path": ")") + musicDir.toUtf8() + QByteArray(R"(",
        "type": "unknown_type"
      },
      {
        "id": "fallback-numeric",
        "title": "Tipo Numerico",
        "path": ")") + musicDir.toUtf8() + QByteArray(R"(",
        "type": 0
      },
      {
        "id": "cmd-playlist",
        "title": "Playlist",
        "type": "command",
        "items": [
          { "id": "pause", "title": "Pausar Playlist", "command": "pause" },
          { "id": "default-title", "title": "", "command": "PAUSE" },
          { "id": "unknown", "title": "Desconhecido", "command": "obs_scene" }
        ]
      },
      {
        "id": "cmd-extra",
        "title": "Automacao",
        "type": "command",
        "items": []
      }
    ]
  })");

  // MediaType string conversion helpers
  QCOMPARE(mediaTypeFromString(QStringLiteral("jingle")), MediaType::Jingle);
  QCOMPARE(mediaTypeFromString(QStringLiteral("  MUSIC  ")), MediaType::Music);
  QCOMPARE(mediaTypeFromString(QStringLiteral("Commercial")), MediaType::Commercial);
  QCOMPARE(mediaTypeFromString(QStringLiteral("other")), MediaType::Other);
  QCOMPARE(mediaTypeFromString(QStringLiteral("COMMAND")), MediaType::Command);
  QCOMPARE(mediaTypeFromString(QStringLiteral("invalid")), MediaType::Other);
  QCOMPARE(mediaTypeFromString(QString()), MediaType::Other);
  QCOMPARE(mediaTypeToString(MediaType::Jingle), QStringLiteral("jingle"));
  QCOMPARE(mediaTypeToString(MediaType::Music), QStringLiteral("music"));
  QCOMPARE(mediaTypeToString(MediaType::Commercial), QStringLiteral("commercial"));
  QCOMPARE(mediaTypeToString(MediaType::Other), QStringLiteral("other"));
  QCOMPARE(mediaTypeToString(MediaType::Command), QStringLiteral("command"));

  QVERIFY(writeJsonFile(dir.filePath(QStringLiteral("catalog.json")), catalogJson));

  const QVector<FolderItem> loaded = CatalogRepository::loadFolders(dir.path());
  QCOMPARE(loaded.size(), 9);
  QCOMPARE(loaded[0].mediaType(), MediaType::Jingle);
  QVERIFY(!loaded[0].isCommandFolder());
  QCOMPARE(loaded[1].mediaType(), MediaType::Music);
  QCOMPARE(loaded[2].mediaType(), MediaType::Commercial);
  QCOMPARE(loaded[3].mediaType(), MediaType::Other);
  QCOMPARE(loaded[4].mediaType(), MediaType::Other);
  QCOMPARE(loaded[5].mediaType(), MediaType::Other);
  QCOMPARE(loaded[6].mediaType(), MediaType::Other);
  QVERIFY(loaded[7].isCommandFolder());
  QCOMPARE(loaded[7].mediaType(), MediaType::Command);
  QCOMPARE(loaded[7].items.size(), 2);
  QCOMPARE(loaded[7].items[0].itemType, CatalogItemType::Command);
  QCOMPARE(loaded[7].items[0].mediaType, MediaType::Command);
  QCOMPARE(loaded[7].items[0].commandType, CommandType::Pause);
  QCOMPARE(loaded[7].items[0].commandId, QStringLiteral("pause"));
  QCOMPARE(loaded[7].items[0].title, QStringLiteral("Pausar Playlist"));
  QCOMPARE(loaded[7].items[1].title, QStringLiteral("Pausa"));
  QVERIFY(loaded[8].isCommandFolder());
  QCOMPARE(loaded[8].items.size(), 0);

  FolderListModel folderModel;
  folderModel.loadFromDatabase(dir.path());

  // CategoryHeader(Comandos), Playlist, Automacao,
  // CategoryHeader(Vinhetas), Vinhetas,
  // CategoryHeader(Músicas), Pop,
  // CategoryHeader(Comerciais), Comerciais,
  // CategoryHeader(Outros), Outros, Sem Tipo, Tipo Invalido, Tipo Numerico
  QCOMPARE(folderModel.rowCount(), 14);
  QVERIFY(folderModel.itemAt(0)->isHeader());
  QCOMPARE(folderModel.itemAt(0)->title, QStringLiteral("Comandos"));
  QCOMPARE(folderModel.itemAt(1)->id, QStringLiteral("cmd-playlist"));
  QCOMPARE(folderModel.itemAt(2)->id, QStringLiteral("cmd-extra"));
  QVERIFY(folderModel.itemAt(3)->isHeader());
  QCOMPARE(folderModel.itemAt(3)->title, QStringLiteral("Vinhetas"));
  QCOMPARE(folderModel.itemAt(4)->id, QStringLiteral("jingles"));
  QVERIFY(folderModel.itemAt(5)->isHeader());
  QCOMPARE(folderModel.itemAt(5)->title, QStringLiteral("Músicas"));
  QCOMPARE(folderModel.itemAt(6)->id, QStringLiteral("musicas"));
  QVERIFY(folderModel.itemAt(7)->isHeader());
  QCOMPARE(folderModel.itemAt(7)->title, QStringLiteral("Comerciais"));
  QCOMPARE(folderModel.itemAt(8)->id, QStringLiteral("comerciais"));
  QVERIFY(folderModel.itemAt(9)->isHeader());
  QCOMPARE(folderModel.itemAt(9)->title, QStringLiteral("Outros"));
  QCOMPARE(folderModel.itemAt(10)->id, QStringLiteral("outros"));
  QCOMPARE(folderModel.itemAt(11)->id, QStringLiteral("fallback-missing"));
  QCOMPARE(folderModel.itemAt(12)->id, QStringLiteral("fallback-invalid"));
  QCOMPARE(folderModel.itemAt(13)->id, QStringLiteral("fallback-numeric"));

  const QModelIndex cmdIdx = folderModel.indexForId(QStringLiteral("cmd-extra"));
  QVERIFY(cmdIdx.isValid());
  QCOMPARE(cmdIdx.row(), 2);

  // Invalid root structures must safely return empty lists
  QTemporaryDir badDir;
  QVERIFY(badDir.isValid());
  QVERIFY(writeJsonFile(badDir.filePath(QStringLiteral("catalog.json")),
                        R"([{"id":"legacy","title":"Legacy","path":"C:/x","type":"jingle"}])"));
  QVERIFY(CatalogRepository::loadFolders(badDir.path()).isEmpty());

  QVERIFY(writeJsonFile(badDir.filePath(QStringLiteral("catalog.json")),
                        R"({"version":1,"items":[]})"));
  QVERIFY(CatalogRepository::loadFolders(badDir.path()).isEmpty());
}

void TestCommands::testCatalogWidgetVirtualCommandFoldersSearchAndMenus() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString jinglesDir = dir.filePath(QStringLiteral("jingles"));
  QVERIFY(QDir().mkpath(jinglesDir));
  QVERIFY(writeJsonFile(jinglesDir + QStringLiteral("/VhtPauseTest.mp3"), QByteArray("stub")));

  const QByteArray catalogJson = QByteArray(R"({
    "version": 1,
    "folders": [
      {
        "id": "vinhetas",
        "title": "Vinhetas",
        "path": ")") + jinglesDir.toUtf8() + QByteArray(R"(",
        "type": "jingle"
      },
      {
        "id": "cmd-playlist",
        "title": "Playlist",
        "type": "command",
        "items": [
          { "id": "pause", "title": "Pausar Playlist", "command": "pause" }
        ]
      },
      {
        "id": "cmd-vazio",
        "title": "Vazia",
        "type": "command",
        "items": []
      }
    ]
  })");
  QVERIFY(writeJsonFile(dir.filePath(QStringLiteral("catalog.json")), catalogJson));

  CatalogWidget widget;
  widget.setDatabasePath(dir.path());

  auto *foldersView = widget.findChild<QListView *>(QStringLiteral("FoldersListView"));
  auto *filesView = widget.findChild<QListView *>(QStringLiteral("FilesListView"));
  auto *searchLine = widget.findChild<QLineEdit *>(QStringLiteral("SearchLine"));
  auto *btnLocal = widget.findChild<QPushButton *>(QStringLiteral("SearchLocal"));
  auto *btnGlobal = widget.findChild<QPushButton *>(QStringLiteral("SearchGlobal"));
  auto *btnClean = widget.findChild<QPushButton *>(QStringLiteral("SearchClean"));
  QVERIFY(foldersView && filesView && searchLine && btnLocal && btnGlobal && btnClean);

  const QModelIndex cmdFolderIdx =
      widget.folderModel()->indexForId(QStringLiteral("cmd-playlist"));
  const QModelIndex emptyCmdFolderIdx =
      widget.folderModel()->indexForId(QStringLiteral("cmd-vazio"));
  QVERIFY(cmdFolderIdx.isValid());
  QVERIFY(emptyCmdFolderIdx.isValid());

  // Activate the virtual command folder -> populates synchronously with 1 command
  foldersView->setCurrentIndex(cmdFolderIdx);
  QCOMPARE(widget.catalogModel()->rowCount(), 1);
  const CatalogItem *cmdItem = widget.catalogModel()->itemAt(0);
  QVERIFY(cmdItem);
  QVERIFY(cmdItem->isCommand());
  QCOMPARE(cmdItem->commandType, CommandType::Pause);
  QCOMPARE(cmdItem->title, QStringLiteral("Pausar Playlist"));

  // Context menu must not open MixEditor for commands; double click must not emit fileDoubleClicked
  QVERIFY(widget.createFileContextMenu(widget.catalogModel()->index(0, 0), &widget) == nullptr);
  QSignalSpy doubleClickSpy(&widget, &CatalogWidget::fileDoubleClicked);
  emit filesView->doubleClicked(widget.catalogModel()->index(0, 0));
  QCOMPARE(doubleClickSpy.count(), 0);

  // Local search inside virtual command folder
  searchLine->setText(QStringLiteral("nao-existe"));
  btnLocal->click();
  QCOMPARE(widget.catalogModel()->rowCount(), 0);

  searchLine->setText(QStringLiteral("pause"));
  btnLocal->click();
  QCOMPARE(widget.catalogModel()->rowCount(), 1);

  btnClean->click();
  QVERIFY(searchLine->text().isEmpty());
  QCOMPARE(widget.catalogModel()->rowCount(), 1);

  // Switching to a second virtual folder that also has path="" must clear/update properly
  foldersView->setCurrentIndex(emptyCmdFolderIdx);
  QCOMPARE(widget.catalogModel()->rowCount(), 0);
  foldersView->setCurrentIndex(cmdFolderIdx);
  QCOMPARE(widget.catalogModel()->rowCount(), 1);

  // Global search returns both matching disk files and virtual command items under headers
  searchLine->setText(QStringLiteral("Pause"));
  QSignalSpy finishSpy(widget.catalogModel(), &QAbstractItemModel::modelReset);
  btnGlobal->click();
  QTRY_VERIFY_WITH_TIMEOUT(finishSpy.count() >= 2, 3000);

  bool sawCommandHeader = false;
  bool sawPauseCommand = false;
  bool sawDiskFile = false;
  for (const CatalogItem &it : widget.catalogModel()->items()) {
    if (it.isHeader() && it.mediaType == MediaType::Command &&
        it.title == QStringLiteral("Playlist")) {
      sawCommandHeader = true;
    } else if (it.isCommand() && it.commandType == CommandType::Pause) {
      sawPauseCommand = true;
    } else if (it.itemType == CatalogItemType::File &&
               it.title == QStringLiteral("VhtPauseTest.mp3")) {
      sawDiskFile = true;
    }
  }
  QVERIFY(sawCommandHeader);
  QVERIFY(sawPauseCommand);
  QVERIFY(sawDiskFile);
}

void TestCommands::testCatalogDragDropToPlaylistAndCartWallRejection() {
  CatalogListModel catalogModel;
  CatalogItem cmd;
  cmd.itemType = CatalogItemType::Command;
  cmd.mediaType = MediaType::Command;
  cmd.commandType = CommandType::Pause;
  cmd.commandId = QStringLiteral("pause");
  cmd.title = QStringLiteral("Pausar Playlist");
  catalogModel.setItems({cmd});

  std::unique_ptr<QMimeData> mime(
      catalogModel.mimeData({catalogModel.index(0, 0)}));
  QVERIFY(mime);
  QVERIFY(mime->hasFormat(QStringLiteral("application/x-catalog-item")));

  // Playlist setup with Commercial and Music sections
  PlaylistListModel playlistModel;
  playlistModel.clear();
  playlistModel.addItem(makeBlockMarker(QStringLiteral("10:00"), QStringLiteral("Bloco")));
  playlistModel.addItem(makeStartMarker(BlockType::Commercial));
  playlistModel.addItem(makeEndMarker(BlockType::Commercial));
  playlistModel.addItem(makeStartMarker(BlockType::Music));
  playlistModel.addItem(makeMediaItem(QStringLiteral("M1"), QStringLiteral("C:/m1.mp3"), 10000));
  playlistModel.addItem(makeEndMarker(BlockType::Music));

  PlaylistView playlistView;
  playlistView.setModel(&playlistModel);
  playlistView.resize(500, 500);
  playlistView.show();

  const QRect commEndRect = playlistView.visualRect(playlistModel.index(2, 0));
  const QPoint dropPoint(commEndRect.center().x(), commEndRect.top() + 2);

  QDragEnterEvent enterEv(dropPoint, Qt::CopyAction, mime.get(),
                          Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(playlistView.viewport(), &enterEv);
  QVERIFY(enterEv.isAccepted());

  QDragMoveEvent moveEv(dropPoint, Qt::CopyAction, mime.get(),
                        Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(playlistView.viewport(), &moveEv);
  QVERIFY(moveEv.isAccepted());

  QDropEvent dropEv(dropPoint, Qt::CopyAction, mime.get(),
                    Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(playlistView.viewport(), &dropEv);
  QVERIFY(dropEv.isAccepted());

  QCOMPARE(playlistModel.itemAt(2)->type, ItemType::Command);
  QCOMPARE(playlistModel.itemAt(2)->commandType, CommandType::Pause);
  QCOMPARE(playlistModel.itemAt(2)->mediaType, MediaType::Command);
  QCOMPARE(playlistModel.itemAt(2)->blockType, BlockType::Commercial);
  QCOMPARE(playlistModel.itemAt(2)->title, QStringLiteral("Pausar Playlist"));

  // Cannot drop outside markers (row 0 = BlockMarker)
  const QRect bmRect = playlistView.visualRect(playlistModel.index(0, 0));
  QDropEvent invalidDrop(QPoint(bmRect.center().x(), bmRect.top() + 1),
                         Qt::CopyAction, mime.get(), Qt::LeftButton,
                         Qt::NoModifier);
  QApplication::sendEvent(playlistView.viewport(), &invalidDrop);
  QVERIFY(!invalidDrop.isAccepted());

  // Context menu on Command only offers "Remover" (no MixEditor action)
  std::unique_ptr<QMenu> menu(
      playlistView.createContextMenu(playlistModel.index(2, 0), &playlistView));
  QVERIFY(menu);
  QCOMPARE(menu->actions().size(), 1);
  QCOMPARE(menu->actions().first()->text(), QStringLiteral("Remover"));

  // Move command to Musical block (row 5 -> after M1 at row 5 after adjustment)
  QVERIFY(playlistModel.moveItem(2, 5));
  QCOMPARE(playlistModel.itemAt(5)->type, ItemType::Command);
  QCOMPARE(playlistModel.itemAt(5)->blockType, BlockType::Music);

  // Delete key removes unlocked command
  playlistView.setCurrentIndex(playlistModel.index(5, 0));
  QKeyEvent delEv(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
  QApplication::sendEvent(&playlistView, &delEv);
  QCOMPARE(playlistModel.rowCount(), 6);

  // CartWall must reject Command drags
  CartWallTableModel cartModel;
  cartModel.setColumnCount(2);
  CartWallView cartView;
  cartView.setModel(&cartModel);
  cartView.resize(300, 200);
  cartView.show();

  QDragEnterEvent cartEnter(QPoint(20, 20), Qt::CopyAction, mime.get(),
                            Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(cartView.viewport(), &cartEnter);
  QVERIFY(!cartEnter.isAccepted());

  QDropEvent cartDrop(QPoint(20, 20), Qt::CopyAction, mime.get(),
                      Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(cartView.viewport(), &cartDrop);
  QVERIFY(!cartDrop.isAccepted());
  QCOMPARE(cartModel.itemCount(), 0);
}

void TestCommands::testPlaylistModelCommandMetadataCalculationsAndLocking() {
  PlaylistListModel model;
  model.clear();
  model.addItem(makeBlockMarker(QStringLiteral("14:00"), QStringLiteral("Show")));
  model.addItem(makeStartMarker(BlockType::Music));
  // M1: dur=10000, endPoint=10000, mixEnd=8000 (must NOT cut at mixEnd when followed by Pause!)
  const auto m1 = makeMediaItem(QStringLiteral("M1"), QStringLiteral("C:/m1.mp3"),
                                10000, 0, 10000, 0, 8000);
  const auto pauseCmd = makePauseCommand();
  // M2: mixStart=2000 (must NOT overlap with M1 across Pause!)
  const auto m2 = makeMediaItem(QStringLiteral("M2"), QStringLiteral("C:/m2.mp3"),
                                15000, 0, 15000, 2000, 15000);
  model.addItem(m1);
  model.addItem(pauseCmd);
  model.addItem(m2);
  model.addItem(makeEndMarker(BlockType::Music));

  QCOMPARE(model.itemAt(2)->estimatedTime, QStringLiteral("14:00:00"));
  QCOMPARE(model.itemAt(3)->estimatedTime, QStringLiteral("14:00:10"));
  QCOMPARE(model.itemAt(4)->estimatedTime, QStringLiteral("14:00:10"));
  QCOMPARE(model.itemAt(5)->itemsCount, 3);
  QCOMPARE(model.itemAt(5)->totalDurationMs, qint64(25000));
  QCOMPARE(model.itemAt(0)->totalDurationMs, qint64(25000));

  // Set playing row to M1 (row 2), then finishThroughRow(3) -> M1 & Pause Finished, M2 Next
  model.setPlayingRow(2);
  model.finishThroughRow(3);
  QCOMPARE(model.itemAt(2)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model.itemAt(3)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model.itemAt(4)->status, PlaylistItemStatus::Next);
  QCOMPARE(model.currentPlayingRow(), -1);
  QCOMPARE(model.lastLockedRow(), 3);
  QVERIFY(!model.canRemoveRow(3));
  QVERIFY(!model.insertCommandItem(3, QStringLiteral("Blocked"), CommandType::Pause));
  QVERIFY(model.insertCommandItem(4, QStringLiteral("Allowed"), CommandType::Pause));
}

void TestCommands::testPlaylistManagerPauseFlowAndPreloads() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {new FakePlayerManager(&registry),
                                              new FakePlayerManager(&registry),
                                              new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker(QStringLiteral("14:00"), QStringLiteral("Block")));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto m1 = makeMediaItem(QStringLiteral("M1"), QStringLiteral("C:/m1.mp3"),
                                20000, 0, 20000, 0, 16000);
  const auto pauseCmd = makePauseCommand();
  const auto m2 = makeMediaItem(QStringLiteral("M2"), QStringLiteral("C:/m2.mp3"),
                                20000, 0, 20000, 3000, 18000);
  model->addItem(m1);
  model->addItem(pauseCmd);
  model->addItem(m2);
  model->addItem(makeEndMarker(BlockType::Music));

  QSignalSpy statusSpy(&manager, &PlaylistManager::statusChanged);
  QSignalSpy cmdSpy(&manager, &PlaylistManager::commandExecuted);

  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(manager.activePlayerIndex(), 0);
  // Pause is next -> no preload / no mix trigger across Pause
  QCOMPARE(manager.slotRole(1), PlaylistManager::SlotRole::Free);

  // Passing M1's mixEnd (16000) must NOT mix-trigger M2 across Pause!
  fakes[0]->advanceTo(16000);
  QCOMPARE(manager.activePlayerIndex(), 0);
  QCOMPARE(fakes[1]->status(), PlayerStatus::Idle);

  fakes[0]->advanceTo(19999);
  QCOMPARE(manager.activePlayerIndex(), 0);

  // At M1 endPoint (20000), playback hits Pause -> acts like Stop and marks Pause Finished
  fakes[0]->advanceTo(20000);
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(manager.activePlayerIndex(), -1);
  QCOMPARE(cmdSpy.count(), 1);
  QCOMPARE(cmdSpy.at(0).at(0).toInt(), 3);
  QCOMPARE(cmdSpy.at(0).at(1).toUuid(), pauseCmd.uuid);
  QCOMPARE(cmdSpy.at(0).at(2).value<CommandType>(), CommandType::Pause);
  QCOMPARE(statusSpy.last().at(0).value<PlaylistStatus>(), PlaylistStatus::Stopped);

  QCOMPARE(model->itemAt(2)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(3)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(4)->status, PlaylistItemStatus::Next);
  for (int i = 0; i < 3; ++i) {
    QCOMPARE(manager.slotRole(i), PlaylistManager::SlotRole::Free);
    QCOMPARE(fakes[i]->status(), PlayerStatus::Idle);
  }

  // Pressing Play resumes immediately with M2
  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(manager.activePlayerIndex(), 0);
  QCOMPARE(manager.slotItemUuid(0), m2.uuid);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
  QCOMPARE(model->itemAt(3)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(4)->status, PlaylistItemStatus::Playing);
}

void TestCommands::testPlaylistManagerPlayResumeStopManualNextAndMultiplePauses() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {new FakePlayerManager(&registry),
                                              new FakePlayerManager(&registry),
                                              new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});
  manager.setManualNextFadeMs(500);

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker(QStringLiteral("15:00"), QStringLiteral("Block")));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto m1 = makeMediaItem(QStringLiteral("M1"), QStringLiteral("C:/m1.mp3"), 10000);
  const auto p1 = makePauseCommand(QStringLiteral("Pause 1"));
  const auto p2 = makePauseCommand(QStringLiteral("Pause 2"));
  const auto m2 = makeMediaItem(QStringLiteral("M2"), QStringLiteral("C:/m2.mp3"), 10000);
  model->addItem(m1);
  model->addItem(p1);
  model->addItem(p2);
  model->addItem(m2);
  model->addItem(makeEndMarker(BlockType::Music));

  manager.play();
  fakes[0]->advanceTo(3000);

  // Manual Next while M1 plays into p1 -> M1 fades out as detached Tail, playlist stops, p1 Finished, p2 Next
  manager.next();
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(manager.activePlayerIndex(), -1);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Tail);
  QCOMPARE(fakes[0]->fadeOutCalls, 1);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Playing);
  QCOMPARE(model->itemAt(2)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(3)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(4)->status, PlaylistItemStatus::Next);

  // Play() executes consecutive pause p2 -> marks p2 Finished, stays Stopped, promotes m2 to Next
  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(model->itemAt(4)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(5)->status, PlaylistItemStatus::Next);

  // Tail finishes cleanly
  fakes[0]->advanceTo(3500);
  QCOMPARE(fakes[0]->status(), PlayerStatus::Idle);
  QCOMPARE(manager.slotRole(0), PlaylistManager::SlotRole::Free);

  // Play() starts M2
  manager.play();
  QCOMPARE(manager.status(), PlaylistStatus::Playing);
  QCOMPARE(manager.slotItemUuid(manager.activePlayerIndex()), m2.uuid);
}

void TestCommands::testPlaylistManagerPauseAtEndAndBlockBoundary() {
  FakeAudioRegistry registry;
  std::array<FakePlayerManager *, 3> fakes = {new FakePlayerManager(&registry),
                                              new FakePlayerManager(&registry),
                                              new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker(QStringLiteral("16:00"), QStringLiteral("B1")));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto m1 = makeMediaItem(QStringLiteral("M1"), QStringLiteral("C:/m1.mp3"), 5000);
  model->addItem(m1);
  model->addItem(makeEndMarker(BlockType::Music));

  model->addItem(makeBlockMarker(QStringLiteral("16:30"), QStringLiteral("B2")));
  model->addItem(makeStartMarker(BlockType::Commercial));
  const auto pauseEnd = makePauseCommand(QStringLiteral("Pause B2"));
  model->addItem(pauseEnd);
  model->addItem(makeEndMarker(BlockType::Commercial));

  QSignalSpy blockChangedSpy(&manager, &PlaylistManager::blockChanged);
  QSignalSpy cmdSpy(&manager, &PlaylistManager::commandExecuted);

  manager.play();
  QCOMPARE(blockChangedSpy.count(), 1);
  fakes[0]->advanceTo(5000);

  // Hit pauseEnd at end of playlist -> Stopped, pauseEnd Finished, no next playable row
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(cmdSpy.count(), 1);
  QCOMPARE(model->itemAt(6)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model->findNextPlayableRow(), -1);
}

void TestCommands::testPlaylistManagerDynamicCommandInsertRemoveAndErrorSkip() {
  FakeAudioRegistry registry;
  registry.missingPaths.insert(QStringLiteral("C:/missing.mp3"));
  std::array<FakePlayerManager *, 3> fakes = {new FakePlayerManager(&registry),
                                              new FakePlayerManager(&registry),
                                              new FakePlayerManager(&registry)};
  PlaylistManager manager({fakes[0], fakes[1], fakes[2]});

  auto *model = manager.model();
  model->clear();
  model->addItem(makeBlockMarker(QStringLiteral("17:00"), QStringLiteral("Dynamic")));
  model->addItem(makeStartMarker(BlockType::Music));
  const auto m1 = makeMediaItem(QStringLiteral("M1"), QStringLiteral("C:/m1.mp3"),
                                20000, 0, 20000, 0, 18000);
  const auto m2 = makeMediaItem(QStringLiteral("M2"), QStringLiteral("C:/m2.mp3"),
                                20000, 0, 20000, 2000, 18000);
  model->addItem(m1);
  model->addItem(m2);
  model->addItem(makeEndMarker(BlockType::Music));

  manager.play();
  // Mix trigger before inserting Pause = 18000 - 2000 = 16000
  fakes[0]->advanceTo(15000);
  QCOMPARE(manager.activePlayerIndex(), 0);

  // Dynamically insert Pause between M1 (row 2) and M2 (row 3)
  QVERIFY(model->insertCommandItem(3, QStringLiteral("Pausar"), CommandType::Pause));
  fakes[0]->advanceTo(16000);
  // Mix trigger is suppressed because Pause is next!
  QCOMPARE(manager.activePlayerIndex(), 0);

  // Remove Pause dynamically -> mix trigger is restored immediately!
  QVERIFY(model->removeItem(3));
  fakes[0]->advanceTo(16000);
  QCOMPARE(manager.activePlayerIndex(), 1);
  QCOMPARE(manager.slotItemUuid(1), m2.uuid);

  // Missing media followed by Pause stops cleanly at Pause instead of playing M_ok
  manager.stopImmediate();
  model->clear();
  model->addItem(makeBlockMarker(QStringLiteral("18:00"), QStringLiteral("Error")));
  model->addItem(makeStartMarker(BlockType::Music));
  model->addItem(makeMediaItem(QStringLiteral("Missing"), QStringLiteral("C:/missing.mp3"), 5000));
  const auto pAfterErr = makePauseCommand();
  const auto mOk = makeMediaItem(QStringLiteral("OK"), QStringLiteral("C:/ok.mp3"), 5000);
  model->addItem(pAfterErr);
  model->addItem(mOk);
  model->addItem(makeEndMarker(BlockType::Music));

  QSignalSpy errSpy(&manager, &PlaylistManager::playbackError);
  QSignalSpy cmdSpy(&manager, &PlaylistManager::commandExecuted);
  manager.play();
  QCOMPARE(errSpy.count(), 1);
  QCOMPARE(cmdSpy.count(), 1);
  QCOMPARE(manager.status(), PlaylistStatus::Stopped);
  QCOMPARE(model->itemAt(3)->status, PlaylistItemStatus::Finished);
  QCOMPARE(model->itemAt(4)->status, PlaylistItemStatus::Next);
}

void TestCommands::testDelegatesRenderCommandStates() {
  QPixmap canvas(420, 80);
  canvas.fill(Qt::transparent);

  // FolderItemDelegate
  FolderListModel folderModel;
  {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(writeJsonFile(dir.filePath(QStringLiteral("catalog.json")),
                          R"({"version":1,"folders":[{"id":"cmd","title":"Playlist","type":"command","items":[{"id":"pause","title":"Pausar","command":"pause"}]}]})"));
    folderModel.loadFromDatabase(dir.path());

    FolderItemDelegate folderDelegate;
    QPainter p(&canvas);
    QStyleOptionViewItem opt;
    opt.rect = QRect(0, 0, 240, 34);
    for (int r = 0; r < folderModel.rowCount(); ++r) {
      const QModelIndex idx = folderModel.index(r, 0);
      QVERIFY(folderDelegate.sizeHint(opt, idx).height() > 0);
      folderDelegate.paint(&p, opt, idx);
    }
  }

  // CatalogItemDelegate
  {
    CatalogListModel catModel;
    CatalogItem header;
    header.itemType = CatalogItemType::FolderHeader;
    header.mediaType = MediaType::Command;
    header.title = QStringLiteral("Playlist");
    CatalogItem cmd;
    cmd.itemType = CatalogItemType::Command;
    cmd.mediaType = MediaType::Command;
    cmd.commandType = CommandType::Pause;
    cmd.commandId = QStringLiteral("pause");
    cmd.title = QStringLiteral("Pausar Playlist");
    catModel.setItems({header, cmd});

    CatalogItemDelegate catDelegate;
    QPainter p(&canvas);
    QStyleOptionViewItem opt;
    opt.rect = QRect(0, 0, 280, 35);
    for (int r = 0; r < catModel.rowCount(); ++r) {
      const QModelIndex idx = catModel.index(r, 0);
      QVERIFY(catDelegate.sizeHint(opt, idx).height() > 0);
      catDelegate.paint(&p, opt, idx);
    }
  }

  // PlaylistItemDelegate (Default, Next, Finished in both Commercial and Music; heights match Media 48 / 32)
  {
    PlaylistListModel plistModel;
    plistModel.clear();
    plistModel.addItem(makeBlockMarker(QStringLiteral("12:00"), QStringLiteral("Test")));
    plistModel.addItem(makeStartMarker(BlockType::Commercial));
    plistModel.addItem(makePauseCommand(QStringLiteral("Pause Finished")));
    plistModel.addItem(makePauseCommand(QStringLiteral("Pause Next")));
    plistModel.addItem(makeEndMarker(BlockType::Commercial));
    plistModel.addItem(makeStartMarker(BlockType::Music));
    plistModel.addItem(makePauseCommand(QStringLiteral("Pause Default")));
    plistModel.addItem(makeEndMarker(BlockType::Music));

    plistModel.finishThroughRow(2);

    PlaylistItemDelegate plistDelegate;
    QPainter p(&canvas);
    QStyleOptionViewItem opt;
    QCOMPARE(plistDelegate.sizeHint(opt, plistModel.index(2, 0)).height(), 32);
    QCOMPARE(plistDelegate.sizeHint(opt, plistModel.index(3, 0)).height(), 48);
    QCOMPARE(plistDelegate.sizeHint(opt, plistModel.index(6, 0)).height(), 48);

    for (int r = 0; r < plistModel.rowCount(); ++r) {
      const QModelIndex idx = plistModel.index(r, 0);
      const QSize hint = plistDelegate.sizeHint(opt, idx);
      QVERIFY(hint.height() > 0);
      opt.rect = QRect(0, 0, 400, hint.height());
      if (r == 3) {
        opt.state |= QStyle::State_Selected;
      } else {
        opt.state &= ~QStyle::State_Selected;
      }
      plistDelegate.paint(&p, opt, idx);
    }
  }
}

QTEST_MAIN(TestCommands)
#include "test_commands.moc"
