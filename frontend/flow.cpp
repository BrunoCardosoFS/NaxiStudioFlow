#include "flow.h"
#include "./ui_flow.h"

#include <QSettings>

Flow::Flow(QWidget *parent):QMainWindow(parent), ui(new Ui::Flow){
    ui->setupUi(this);

    // this->showFullScreen();

    restoreLayout();

    this->ui->DockTop->setTitleBarWidget(new QWidget());
    // this->ui->DockTop->setTitleBarWidget(nullptr);

    this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::TabPosition::North);
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
        restoreState(layoutData);  // Restaura estado salvo
    }
}


void Flow::on_pushButton_clicked()
{
    if(this->isFullScreen()){
        this->showNormal();
        // this->showMaximized();
    }else{
        this->showFullScreen();
    }
}

