#include "flow.h"
#include "./ui_flow.h"

#include "../backend/catalog/folderslist.h"

#include "./widgets/filewidget.h"

#include <QDateTime>
#include <QTimer>
#include <QMessageBox>

#include <QDebug>

#include <QLocale>

Flow::Flow(QWidget *parent):QMainWindow(parent), ui(new Ui::Flow){
    ui->setupUi(this);
    this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::North);
    this->ui->DockTop->setTitleBarWidget(new QWidget());
    // this->ui->DockTop->setTitleBarWidget(nullptr);

    if(!this->settings->contains("db")){
        this->settings->setValue("db", QCoreApplication::applicationDirPath() + "/../db");
    }

    connect(this->filesList, &FilesList::finish, this, &Flow::loadFiles);

    connect(this, &Flow::getFiles, this->filesList, &FilesList::init);

    this->loadFolders();


    // this->showFullScreen();
    // this->showMaximized();

    this->updateClock();
    QTimer *clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &Flow::updateClock);
    clockTimer->start(1000);

    // this->ui->selectProfile->addItem("Manhã Show");
    // this->ui->selectProfile->addItem("Band Bom Dia");
    // this->ui->selectProfile->addItem("Clube da Band");
    this->ui->selectProfile->hide();

    ProgramBlock *blocoteste = new ProgramBlock(this);
    blocoteste->setHour("21:30");
    blocoteste->setTitle("Musical");
    this->ui->PlaylistContent->layout()->addWidget(blocoteste);
    this->blocks.append(blocoteste);

    // ProgramBlock *blocoteste2 = new ProgramBlock(this);
    // blocoteste2->setHour("22:00");
    // blocoteste2->setTitle("Comercial");
    // this->ui->PlaylistContent->layout()->addWidget(blocoteste2);
    // this->blocks.append(blocoteste2);

    // ProgramBlock *blocoteste3 = new ProgramBlock(this);
    // blocoteste3->setHour("22:05");
    // blocoteste3->setTitle("Musical");
    // this->ui->PlaylistContent->layout()->addWidget(blocoteste3);
    // this->blocks.append(blocoteste3);

    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    this->ui->PlaylistContent->layout()->addItem(spacer);


    restoreLayout();
}

Flow::~Flow(){
    saveLayout();
    delete ui;
}

void Flow::loadFolders(){
    QJsonArray folders = getFolders(this->settings->value("db").toString());

    foreach (QJsonValue jsonValue, folders) {
        QJsonObject jsonObject = jsonValue.toObject();

        QString title = jsonObject.value("title").toString();
        int type = jsonObject.value("type").toInt();

        QPushButton *item = new QPushButton(this->ui->FoldersListContent);
        item->setText(title);
        item->setToolTip(title);
        item->setProperty("id", jsonObject.value("id").toString());

        item->setIconSize(QSize(20, 20));
        item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        item->setCursor(Qt::PointingHandCursor);

        connect(item, &QPushButton::clicked, this, [this, jsonObject](){
            emit this->getFiles(jsonObject.value("path").toString(), "");
        });

        if(type == 0){
            item->setProperty("type", "jingle");
            item->setIcon(QIcon(":/images/catalog/jingle.svg"));
            this->ui->JinglesFolders->layout()->addWidget(item);
        }else if(type == 1){
            item->setProperty("type", "music");
            item->setIcon(QIcon(":/images/catalog/music.svg"));
            this->ui->MusicFolders->layout()->addWidget(item);
        }else if(type == 2){
            item->setProperty("type", "commercial");
            item->setIcon(QIcon(":/images/catalog/commercial.svg"));
            this->ui->CommercialFolders->layout()->addWidget(item);
        }else{
            item->setProperty("type", "other");
            item->setIcon(QIcon(":/images/catalog/other.svg"));
            this->ui->OtherFolders->layout()->addWidget(item);
        }
    }
}

void Flow::loadFiles(QJsonArray list, QString pathFolder){
    this->openFolder = pathFolder;

    // Deleting all widgets from the file list
    QLayoutItem *item;
    while ((item = this->ui->FilesListContent->layout()->takeAt(0)) != nullptr) {
        delete item->widget();
    }

    foreach (QJsonValue item, list) {
        QJsonArray itemArray = item.toArray();
        FileWidget *itemList = new FileWidget(this->ui->FilesListContent);
        itemList->setInfo(itemArray[0].toString(), itemArray[1].toString());

        this->ui->FilesListContent->layout()->addWidget(itemList);
    }

    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    this->ui->FilesListContent->layout()->addItem(spacer);
}





void Flow::saveLayout(){
    this->settings->setValue("layout", saveState());
}

void Flow::restoreLayout(){
    QByteArray layoutData = this->settings->value("layout").toByteArray();
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
    // QMessageBox msgBox;
    // msgBox.setWindowTitle("Fechar o NaxiStudio Flow");
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



void Flow::on_SearchLocal_clicked()
{
    this->filesList->init(this->openFolder, this->ui->SearchLine->text());
}


void Flow::on_SearchClean_clicked()
{
    this->ui->SearchLine->setText("");
    if(this->openFolder != ""){
        this->filesList->init(this->openFolder, "");
    }
}

void Flow::on_SearchLine_returnPressed()
{
    this->on_SearchLocal_clicked();
}


void Flow::on_btnPlay_clicked(){
    this->blocks[0]->controller->toNext();
    this->blocks[0]->controller->play(0);
}

