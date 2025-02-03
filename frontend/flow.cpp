#include "flow.h"
#include "./ui_flow.h"

#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>

Flow::Flow(QWidget *parent):QMainWindow(parent), ui(new Ui::Flow){
    ui->setupUi(this);
    this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::North);
    this->ui->DockTop->setTitleBarWidget(new QWidget());
    // this->ui->DockTop->setTitleBarWidget(nullptr);

    // this->showFullScreen();
    // this->showMaximized();

    this->updateClock();
    QTimer *clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &Flow::updateClock);
    clockTimer->start(1000);




    this->ui->btnClose->hide();

    this->layout()->setSpacing(0);

    restoreLayout();
}

Flow::~Flow(){
    saveLayout();
    delete ui;
}

void Flow::saveLayout(){
    QSettings settings("NaxiStudio", "PlayerFlow");
    settings.setValue("layout", saveState());
}

void Flow::restoreLayout(){
    QSettings settings("NaxiStudio", "PlayerFlow");

    QByteArray layoutData = settings.value("layout").toByteArray();
    if (!layoutData.isEmpty()) {
        restoreState(layoutData);
    }
}

void Flow::updateClock(){
    QDateTime currentTime = QDateTime::currentDateTime();
    this->ui->clock->setText(currentTime.toString("dd/MM/yyyy  hh:mm:ss"));
}


void Flow::on_btnFull_clicked()
{
    if(this->isFullScreen()){
        // this->showNormal();
        this->showMaximized();
    }else{
        this->showFullScreen();
    }
}

void Flow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isFullScreen()) {
            this->ui->btnFull->setIcon(QIcon(":/images/icons/full-screen-exit.svg"));
            this->ui->btnClose->show();
        } else {
            this->ui->btnFull->setIcon(QIcon(":/images/icons/full-screen.svg"));
            this->ui->btnClose->hide();
        }
    }
}

void Flow::closeEvent(QCloseEvent *event) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Fechar o NaxiStudio Flow");
    msgBox.setText("Tem certeza que deseja fechar?");
    msgBox.setIcon(QMessageBox::Question);

    QPushButton *yesButton = msgBox.addButton("Sim", QMessageBox::YesRole);
    QPushButton *noButton = msgBox.addButton("Não", QMessageBox::NoRole);

    msgBox.exec();

    if (msgBox.clickedButton() == yesButton) {
        event->accept();
    }else{
        event->ignore();
    }
}

void Flow::on_btnClose_clicked()
{
    this->close();
}

