#include "Flow.h"
#include "ui_Flow.h"

#include <QJsonArray>
#include <QJsonObject>

#include "core/cartwall/CartWallTableModel.h"
#include "core/playlist/PlaylistManager.h"
#include "core/storage/AppPaths.h"
#include "core/storage/DataBootstrap.h"
#include "core/storage/MediaPathResolver.h"
#include "ui/widgets/cartwall/CartWallItemDelegate.h"
#include "ui/widgets/cartwall/CartWallView.h"
#include "ui/widgets/editor/MixEditor.h"
#include "ui/widgets/playlist/PlaylistItemDelegate.h"
#include "ui/widgets/playlist/PlaylistView.h"

#include <QGridLayout>

#include <QDateTime>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTimer>
#include <QTranslator>

#include <QDebug>

#include <QLocale>

Flow::Flow(QWidget *parent) : QMainWindow(parent), ui(new Ui::Flow) {
  ui->setupUi(this);
  QString version = QStringLiteral(APP_VERSION);
  this->setWindowTitle("NaxStudio Flow " + version);

  this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::North);
  this->ui->DockTop->setTitleBarWidget(new QWidget());
  // this->ui->DockTop->setTitleBarWidget(nullptr);

  m_dataDir = AppPaths::resolveDataDir(this->settings);
  DataBootstrap::initialize(m_dataDir);

  this->catalogWidget = new CatalogWidget(this);
  this->catalogWidget->setDatabasePath(m_dataDir);
  this->ui->DocCatalogWidget->layout()->addWidget(this->catalogWidget);

  // this->showFullScreen();
  // this->showMaximized();

  this->updateClock();
  QTimer *clockTimer = new QTimer(this);
  connect(clockTimer, &QTimer::timeout, this, &Flow::updateClock);
  clockTimer->start(1000);

  this->ui->selectProfile->hide();

  this->playlistManager = new PlaylistManager(this);
  this->playlistManager->setMediaPathResolver(
      [this](const QString &path) { return resolveMediaPath(path); });
  this->playlistDelegate = new PlaylistItemDelegate(this);
  this->ui->playlistView->setModel(this->playlistManager->model());
  this->ui->playlistView->setItemDelegate(this->playlistDelegate);

  connect(this->ui->btnPlay, &QPushButton::clicked, this->playlistManager,
          &PlaylistManager::play);
  connect(this->ui->btnPause, &QPushButton::clicked, this->playlistManager,
          &PlaylistManager::pause);
  connect(this->ui->btnStop, &QPushButton::clicked, this->playlistManager,
          &PlaylistManager::stop);
  connect(this->ui->btnNext, &QPushButton::clicked, this->playlistManager,
          &PlaylistManager::next);

  connect(this->playlistManager, &PlaylistManager::statusChanged, this,
          &Flow::updateTransportButtons);
  connect(this->playlistManager, &PlaylistManager::currentItemChanged, this,
          [this](int row, const QUuid &) {
            const QModelIndex idx =
                this->playlistManager->model()->index(row, 0);
            if (idx.isValid()) {
              this->ui->playlistView->scrollTo(
                  idx, QAbstractItemView::EnsureVisible);
            }
          });
  connect(this->ui->playlistView, &PlaylistView::mixEditorRequested, this,
          &Flow::openMixEditorForPlaylistItem);

  this->updateTransportButtons(this->playlistManager->status());

  auto *cartWallModel = new CartWallTableModel(this);
  cartWallModel->setColumnCount(this->ui->cartWallColumns->value());
  cartWallModel->loadMockData();

  auto *cartWallView = new CartWallView(this->ui->DocCartWallWidget);
  cartWallView->setObjectName("cartWallView");
  cartWallView->setModel(cartWallModel);
  cartWallView->setItemDelegate(new CartWallItemDelegate(cartWallView));
  this->ui->DocCartWallWidget->layout()->addWidget(cartWallView);

  connect(this->ui->cartWallColumns, &QSpinBox::valueChanged, cartWallView,
          &CartWallView::setColumnCount);
  connect(cartWallView, &CartWallView::slotTriggered, this,
          [](int index) { qInfo() << "Cart slot triggered:" << index; });

  restoreLayout();
}

Flow::~Flow() {
  saveLayout();
  delete ui;
}

void Flow::saveLayout() { this->settings->setValue("layout", saveState()); }

void Flow::restoreLayout() {
  QByteArray layoutData = this->settings->value("layout").toByteArray();
  if (!layoutData.isEmpty()) {
    restoreState(layoutData);
  }
}

void Flow::updateClock() {
  QDateTime currentTime = QDateTime::currentDateTime();
  QLocale locale(QLocale::Portuguese, QLocale::Brazil);

  this->ui->clock->setText(currentTime.toString("hh:mm:ss"));
  this->ui->date->setText(
      locale.toString(currentTime, "dddd, dd 'de' MMMM 'de' yyyy"));
}

void Flow::on_btnFull_clicked() {
  if (this->isFullScreen()) {
    // this->showNormal();
    this->showMaximized();
  } else {
    this->showFullScreen();
  }
}

void Flow::changeEvent(QEvent *event) {
  if (event->type() == QEvent::WindowStateChange) {
    if (isFullScreen()) {
      this->ui->btnFull->setIcon(QIcon(":/images/icons/full-screen-exit.svg"));
    } else {
      this->ui->btnFull->setIcon(QIcon(":/images/icons/full-screen.svg"));
    }
  }
}

void Flow::closeEvent(QCloseEvent *event) {
  if (this->m_mixEditor) {
    this->m_mixEditor->close();
  }
  if (this->playlistManager) {
    this->playlistManager->stopImmediate();
    if (this->playlistManager->model()) {
      this->playlistManager->model()->cancelDurationProbes();
    }
  }
  saveLayout();
  QMainWindow::closeEvent(event);
}

void Flow::openMixEditorForPlaylistItem(const QUuid &itemUuid) {
  if (!this->playlistManager || !this->playlistManager->model() ||
      itemUuid.isNull()) {
    return;
  }

  if (!this->m_mixEditor) {
    this->m_mixEditor = new MixEditor(this);
    this->m_mixEditor->setAttribute(Qt::WA_DeleteOnClose, true);
    this->m_mixEditor->setMediaPathResolver(
        [this](const QString &path) { return resolveMediaPath(path); });
  }

  if (!this->m_mixEditor->loadPlaylistItem(this->playlistManager->model(),
                                           itemUuid)) {
    return;
  }

  this->m_mixEditor->show();
  this->m_mixEditor->raise();
  this->m_mixEditor->activateWindow();
}

void Flow::updateTransportButtons(PlaylistStatus status) {
  switch (status) {
  case PlaylistStatus::Stopped:
    this->ui->btnPlay->setEnabled(true);
    this->ui->btnPause->setEnabled(false);
    this->ui->btnStop->setEnabled(false);
    this->ui->btnNext->setEnabled(true);
    break;
  case PlaylistStatus::Playing:
    this->ui->btnPlay->setEnabled(false);
    this->ui->btnPause->setEnabled(true);
    this->ui->btnStop->setEnabled(true);
    this->ui->btnNext->setEnabled(true);
    break;
  case PlaylistStatus::Paused:
    this->ui->btnPlay->setEnabled(true);
    this->ui->btnPause->setEnabled(false);
    this->ui->btnStop->setEnabled(true);
    this->ui->btnNext->setEnabled(true);
    break;
  }
}

bool Flow::eventFilter(QObject *watched, QEvent *event) {
  return QMainWindow::eventFilter(watched, event);
}

QString Flow::resolveMediaPath(const QString &path) const {
  if (!this->catalogWidget || !this->catalogWidget->folderModel()) {
    return path;
  }
  const MediaFileResolution res = MediaPathResolver::resolveMediaFile(
      path, this->catalogWidget->folderModel()->mediaFolders());
  return res.found ? res.resolvedPath : path;
}
