#include "Flow.h"
#include "ui_Flow.h"

#include "core/catalog/FoldersList.h"

#include <QJsonArray>
#include <QJsonObject>

#include "ui/widgets/cartwall/CartWallArea.h"

#include <QGridLayout>

#include <QDateTime>
#include <QMessageBox>
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

  if (!this->settings->contains("db")) {
    this->settings->setValue("db",
                             QCoreApplication::applicationDirPath() + "/../db");
  }

  this->catalogModel = new CatalogListModel(this);
  this->catalogDelegate = new CatalogItemDelegate(this);
  this->ui->FilesListView->setModel(this->catalogModel);
  this->ui->FilesListView->setItemDelegate(this->catalogDelegate);
  this->ui->FilesListView->setUniformItemSizes(true);

  connect(this->filesList, &FilesList::finish, this, &Flow::loadFiles);

  this->loadFolders();

  // this->showFullScreen();
  // this->showMaximized();

  this->updateClock();
  QTimer *clockTimer = new QTimer(this);
  connect(clockTimer, &QTimer::timeout, this, &Flow::updateClock);
  clockTimer->start(1000);

  this->ui->selectProfile->hide();

  QSpacerItem *spacer =
      new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  this->ui->PlaylistContent->layout()->addItem(spacer);

  CartWallArea *cartWallArea =
      new CartWallArea(this); // Consumindo muita memória >90MB
  this->ui->DocCartWallScrollWidget->layout()->addWidget(cartWallArea);
  // this->ui->DocCartWallScroll->setWidget(cartWallArea);

  restoreLayout();
}

Flow::~Flow() {
  saveLayout();
  delete ui;
}

void Flow::loadFolders() {
  QJsonArray folders = getFolders(this->settings->value("db").toString());

  foreach (QJsonValue jsonValue, folders) {
    QJsonObject jsonObject = jsonValue.toObject();

    QString title = jsonObject.value("title").toString();
    QString folderPath = jsonObject.value("path").toString();
    int type = jsonObject.value("type").toInt();

    QPushButton *item = new QPushButton(this->ui->FoldersListContent);
    item->setText(title);
    item->setToolTip(title);
    item->setProperty("id", jsonObject.value("id").toString());

    item->setIconSize(QSize(20, 20));
    item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    item->setCursor(Qt::PointingHandCursor);

    connect(item, &QPushButton::clicked, this, [this, folderPath, type]() {
      this->openFolder = folderPath;
      this->openFolderType = type;
      this->filesList->scanLocal(this->openFolder, this->ui->SearchLine->text(), this->openFolderType);
    });

    if (type == 0) {
      item->setProperty("type", "jingle");
      item->setIcon(QIcon(":/images/catalog/jingle.svg"));
      this->ui->JinglesFolders->layout()->addWidget(item);
    } else if (type == 1) {
      item->setProperty("type", "music");
      item->setIcon(QIcon(":/images/catalog/music.svg"));
      this->ui->MusicFolders->layout()->addWidget(item);
    } else if (type == 2) {
      item->setProperty("type", "commercial");
      item->setIcon(QIcon(":/images/catalog/commercial.svg"));
      this->ui->CommercialFolders->layout()->addWidget(item);
    } else {
      item->setProperty("type", "other");
      item->setIcon(QIcon(":/images/catalog/other.svg"));
      this->ui->OtherFolders->layout()->addWidget(item);
    }
  }
}

void Flow::loadFiles(const QVector<CatalogItem> &list, const QString &pathFolder, bool isGlobal) {
  if (!isGlobal) {
    this->openFolder = pathFolder;
  }
  this->catalogModel->setItems(list);
}

QList<CatalogFolderTarget> Flow::getFolderTargets() const {
  QList<CatalogFolderTarget> targets;
  QJsonArray folders = getFolders(this->settings->value("db").toString());
  for (const QJsonValue &jsonValue : folders) {
    QJsonObject jsonObject = jsonValue.toObject();
    CatalogFolderTarget target;
    target.title = jsonObject.value("title").toString();
    target.path = jsonObject.value("path").toString();
    target.type = jsonObject.value("type").toInt();
    if (!target.path.isEmpty()) {
      targets.append(target);
    }
  }
  return targets;
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
  // QMessageBox msgBox;
  // msgBox.setWindowTitle("Fechar o NaxStudio Flow");
  // msgBox.setText("Tem certeza que deseja fechar?");
  // msgBox.setIcon(QMessageBox::Question);
  // msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

  // int res = msgBox.exec();

  // if (res == QMessageBox::Yes) {
  //     event->accept();
  // } else {
  //     event->ignore();
  // }
}

void Flow::on_SearchLocal_clicked() {
  if (!this->openFolder.isEmpty()) {
    this->filesList->scanLocal(this->openFolder, this->ui->SearchLine->text(), this->openFolderType);
  }
}

void Flow::on_SearchGlobal_clicked() {
  QList<CatalogFolderTarget> targets = this->getFolderTargets();
  this->filesList->scanGlobal(targets, this->ui->SearchLine->text());
}

void Flow::on_SearchClean_clicked() {
  this->ui->SearchLine->clear();
  if (!this->openFolder.isEmpty()) {
    this->filesList->scanLocal(this->openFolder, "", this->openFolderType);
  } else {
    this->catalogModel->clear();
  }
}

void Flow::on_SearchLine_returnPressed() {
  if (!this->openFolder.isEmpty()) {
    this->on_SearchLocal_clicked();
  } else {
    this->on_SearchGlobal_clicked();
  }
}

void Flow::on_btnPlay_clicked() {}

void Flow::on_btnPause_clicked() {}

void Flow::on_btnStop_clicked() {}
