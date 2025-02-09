#include "flow.h"
#include "./ui_flow.h"

#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>

#include <QLocale>

Flow::Flow(QWidget *parent):QMainWindow(parent), ui(new Ui::Flow){
    ui->setupUi(this);
    this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::North);
    this->ui->DockTop->setTitleBarWidget(new QWidget());
    // this->ui->DockTop->setTitleBarWidget(nullptr);


    // this->showFullScreen();
    this->showMaximized();

    this->updateClock();
    QTimer *clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &Flow::updateClock);
    clockTimer->start(1000);

    this->ui->selectProfile->addItem("Manhã Show");
    this->ui->selectProfile->addItem("Band Bom Dia");
    this->ui->selectProfile->addItem("Clube da Band");


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
    QLocale locale(QLocale::Portuguese, QLocale::Brazil);

    this->ui->clock->setText(currentTime.toString("hh:mm:ss"));
    this->ui->date->setText(locale.toString(currentTime, "dddd, dd 'de' MMMM 'de' yyyy"));
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
        } else {
            this->ui->btnFull->setIcon(QIcon(":/images/icons/full-screen.svg"));
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

