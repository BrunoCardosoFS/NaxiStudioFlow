#include "Flow.h"
#include "ui_Flow.h"

#include <QJsonArray>
#include <QJsonObject>

#include "ui/widgets/cartwall/CartWallArea.h"

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

  if (!this->settings->contains("db")) {
    this->settings->setValue("db",
                             QCoreApplication::applicationDirPath() + "/../db");
  }

  this->catalogWidget = new CatalogWidget(this);
  this->catalogWidget->setDatabasePath(this->settings->value("db").toString());
  this->ui->DocCatalogWidget->layout()->addWidget(this->catalogWidget);

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

void Flow::on_btnPlay_clicked() {}

void Flow::on_btnPause_clicked() {}

void Flow::on_btnStop_clicked() {}

bool Flow::eventFilter(QObject *watched, QEvent *event) {
  return QMainWindow::eventFilter(watched, event);
}
